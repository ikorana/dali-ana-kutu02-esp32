
#include "gear.h"
#include "tools/instance/instance.h"

bool Gear::file_emty(void)
{
    gear_t kk={};
    memset(&kk,0,sizeof(kk));
    kk.room=0xff;
    kk.anahtar=0xff;
    kk.short_addr=0xff;  

    /* gear_t ab={};
    for(int i = 0; i < MAX_GEAR; i++) {
       disk->read_file(file_name,&ab,sizeof(gear_t),i); 
       if (ab.short_addr!=0xff && ab.instance!=0xff) { 
            Ins->del(ab.short_addr);
       }
    } */

    uint16_t sz = sizeof(gear_t) * MAX_GEAR;
    gear_t *buffer = (gear_t *)calloc(1,sz);
    if (buffer == nullptr) return false;
    for(int i = 0; i < MAX_GEAR; i++) {
        memcpy(&buffer[i], &kk, sizeof(gear_t));
    }
    disk->write_file(file_name,buffer,sz,0); 
    free(buffer);
    return true;
}

void Gear::set_gurup(uint8_t adr, uint8_t glow, uint8_t ghigh)
{
   gear_t kk={};
   disk->read_file(file_name,&kk,sizeof(gear_t),adr);
   kk.spec.group[0]=glow;
   kk.spec.group[1]=ghigh;
   disk->write_file(file_name,&kk,sizeof(gear_t),adr);
}

void Gear::get_gurup(uint8_t adr, uint8_t *glow, uint8_t *ghigh)
{
   gear_t kk={};
   disk->read_file(file_name,&kk,sizeof(gear_t),adr);
   *glow  = kk.spec.group[0];
   *ghigh = kk.spec.group[1];
}

void Gear::set_lamp_override(uint8_t adr, uint8_t val)
{
   gear_t kk={};
   disk->read_file(file_name,&kk,sizeof(gear_t),adr);
   kk.spec.reserved[0] = val;
   disk->write_file(file_name,&kk,sizeof(gear_t),adr);
   refresh_names(); // room_entries önbelleğindeki lamp_override anında güncellensin
}

bool Gear::refresh_names(void)
{
    // Önceki taramadan kalan heap string'lerini serbest bırakmadan clear() yaparsak sızıntı olur.
    for (find_param_t &fp : names) {
        if (fp.name) free(fp.name);
    }
    names.clear();
    room_entries.clear();

    for (int i=0;i<MAX_GEAR;i++) {
        gear_t ff={};
        disk->read_file(file_name,&ff,sizeof(gear_t),i);
        if (ff.short_addr!=0xFF) {
            find_param_t aa = {};
            char *nm;
            asprintf(&nm,"%s",ff.name);
            aa.name = nm;
            aa.index = i;
            names.push_back(aa);

            gear_room_entry_t re = {};
            re.index = i;
            re.room = ff.room;
            re.ext_type = ff.ext_type;
            re.short_addr = ff.short_addr;
            re.lamp_override = ff.spec.reserved[0];
            room_entries.push_back(re);
        }
    }

    return true;
}

bool Gear::file_format(void)
{
    if (!disk->file_control(file_name))
      {
        file_emty();
      }

      refresh_names();

      file_ready=true;
    return true;
}

cJSON *Gear::gear_intro(void)
{
    cJSON *Lm;
    cJSON *grp = cJSON_CreateArray();
    for (int i=0;i<MAX_GEAR;i++)
    {
        gear_t ff={};
        disk->read_file(file_name,&ff,sizeof(gear_t),i); 
        if (ff.short_addr!=0xFF) {
            cJSON_AddItemToArray(grp, Lm = cJSON_CreateObject());
            cJSON_AddItemToObject(Lm, "gr", cJSON_CreateNumber(ff.short_addr));
            cJSON_AddItemToObject(Lm, "nm", cJSON_CreateString((char*)ff.name)); 
            cJSON_AddItemToObject(Lm, "tp", cJSON_CreateNumber(ff.type)); 
            cJSON_AddItemToObject(Lm, "ex", cJSON_CreateNumber(ff.ext_type));   
            cJSON_AddItemToObject(Lm, "pr", cJSON_CreateNumber(ff.kanal));
            cJSON_AddItemToObject(Lm, "ico", cJSON_CreateNumber(ff.ico));  
            cJSON_AddItemToObject(Lm, "vr", cJSON_CreateNumber(ff.room));
            cJSON_AddItemToObject(Lm, "in", cJSON_CreateNumber(ff.instance));
            cJSON_AddItemToObject(Lm, "an", cJSON_CreateNumber(ff.anahtar));
        }
    }
    return grp;
}

void Gear::gear_small_intro(uint8_t room, cJSON *py)
{
    cJSON *Lm;
    for (int i=0;i<MAX_GEAR;i++)
    {
        gear_t ff={};
        disk->read_file(file_name,&ff,sizeof(gear_t),i); 
        if (ff.short_addr!=0xFF && ff.room==room) {
            cJSON_AddItemToArray(py, Lm = cJSON_CreateObject());
            cJSON_AddItemToObject(Lm, "gr", cJSON_CreateNumber(ff.short_addr));
            cJSON_AddItemToObject(Lm, "nm", cJSON_CreateString((char*)ff.name)); 
            cJSON_AddItemToObject(Lm, "tp", cJSON_CreateNumber(ff.type)); 
            cJSON_AddItemToObject(Lm, "ex", cJSON_CreateNumber(ff.ext_type));   
            cJSON_AddItemToObject(Lm, "pr", cJSON_CreateNumber(ff.kanal));
            cJSON_AddItemToObject(Lm, "ico", cJSON_CreateNumber(ff.ico));  
            cJSON_AddItemToObject(Lm, "vr", cJSON_CreateNumber(ff.room));
            cJSON_AddItemToObject(Lm, "in", cJSON_CreateNumber(ff.instance));
            cJSON_AddItemToObject(Lm, "an", cJSON_CreateNumber(ff.anahtar));
        }
    }
}

void Gear::refresh(uint8_t *count, pck_t *pck, send_AK_t sendAK, send_Status_t sendStatus, bool is_mqtt)
{
    for (int i=0;i<MAX_GEAR;i++)
    {
        gear_t ff={};
        disk->read_file(file_name,&ff,sizeof(gear_t),i); 
        if (ff.short_addr!=0xFF) {
            *count = *count + 1;
            
            sendStatus(ff.short_addr, ff.kanal, pck, is_mqtt);

            cJSON *pay = cJSON_CreateObject();  
            cJSON_AddStringToObject(pay, "com", "search");
            cJSON_AddNumberToObject(pay, "count", *count);
      
            sendAK(pay,pck,is_mqtt);
            vTaskDelay(50/portTICK_PERIOD_MS);
        }
    }
}

void Gear::set_status(uint8_t adr, uint8_t glow, uint8_t ghigh, uint8_t level)
{
    gear_t ff={};
    disk->read_file(file_name,&ff,sizeof(gear_t),adr); 
    ff.spec.group[0]=glow;
    ff.spec.group[1]=ghigh;
    ff.spec.status=level;
    disk->write_file(file_name,&ff,sizeof(gear_t),adr);
}

void Gear::read_gear(uint8_t adr, gear_t *ff) {
    if (!disk->read_file(file_name,ff,sizeof(gear_t),adr))  printf("File Read ERROR %s %d\n",file_name,adr); 
}
void Gear::write_gear(uint8_t adr, gear_t *ff) {
    disk->write_file(file_name,ff,sizeof(gear_t),adr); 
}
void Gear::update_gear(uint8_t adr, gear_t *ff) {
    gear_t rr = {};
    disk->read_file(file_name,&rr,sizeof(gear_t),adr);
    if (rr.short_addr==0xFF) {
        disk->write_file(file_name,ff,sizeof(gear_t),adr);
    } else {
        rr.random_addr = ff->random_addr;
        rr.type = ff->type;
        rr.ext_type = ff->ext_type;
        rr.kanal = ff->kanal;
        rr.instance = ff->instance;
        if (ff->room!=0xFF) rr.room = ff->room;
        if (ff->anahtar!=0xFF) rr.anahtar = ff->anahtar;

       // printf("Update Gear ROOM:%d\n",rr.room);

        //strcpy((char*)rr.name,(char *)ff->name);

        disk->write_file(file_name,&rr,sizeof(gear_t),adr);
    }    
}

uint8_t Gear::get_gear_count(void) {
    uint8_t count=0;
    for (int i=0;i<MAX_GEAR;i++) {
        gear_t ff={};
        disk->read_file(file_name,&ff,sizeof(gear_t),i);
        if (ff.short_addr!=0xFF) {
            count++;
        }
    }
    return count;
}


void Gear::list_gear(void) {
    for (int i=0;i<MAX_GEAR;i++) {
        gear_t ff={};
        disk->read_file(file_name,&ff,sizeof(gear_t),i);
        if (ff.short_addr!=0xFF) {
            printf("Addr : %02d Rand: %06lX Name : %-30s Type : %02d Ext : %02d room:%03d anahtar:%3d Ins:%2d\n",
                    ff.short_addr,
                    ff.random_addr,
                    ff.name,
                    ff.type,
                    ff.ext_type,
                    ff.room,
                    ff.anahtar,
                    ff.instance
            );
        }
    }
}

