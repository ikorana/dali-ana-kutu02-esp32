
#include "room.h"

bool Room::file_emty(void)
{
    room_t kk={};
    kk.ico= 0;
    kk.id = 0xFF;
    strcpy((char *)kk.name,"");

    uint16_t sz = sizeof(room_t) * MAX_ROOM;
    room_t *buffer = (room_t *)calloc(1,sz);
    if (buffer == nullptr) return false;
    for(int i = 0; i < MAX_ROOM; i++) {
        memcpy(&buffer[i], &kk, sizeof(room_t));
    }
    disk->write_file(ROOM_FILE,buffer,sz,0); 
    free(buffer);
    return true;
}

bool Room::refresh_names(void)
{
    // Önceki taramadan kalan heap string'lerini serbest bırakmadan clear() yaparsak sızıntı olur.
    for (find_param_t &fp : names) {
        if (fp.name) free(fp.name);
    }
    names.clear();

    for (int i=0;i<MAX_ROOM;i++) {
        room_t ff={};
        disk->read_file(ROOM_FILE,&ff,sizeof(room_t),i);
        
        if (ff.id!=0xFF) {
            find_param_t aa = {};
            char *nm;
            asprintf(&nm,"%s",ff.name);
            aa.name = nm;
            aa.index = ff.id;
            //printf("ODA %s %d %d eklendi\n",ff.name,ff.id, ff.ico);
            names.push_back(aa);
            //free(nm);
        }
    }

    return true;
}

bool Room::file_format(void)
{
    if (!disk->file_control(ROOM_FILE))
      {
        file_emty();
        file_ready=true;
      } else file_ready=true;

    refresh_names();

    return true;
}

esp_err_t Room::room_create(const char *name)
{
    if (!file_ready) return false;
    room_t kk={};
    uint8_t first_emty = 0xFF;
    bool found = false;

    for (int i=0;i<MAX_ROOM;i++)
    {
        disk->read_file(ROOM_FILE,&kk,sizeof(room_t),i);
        if (kk.id==0xFF && first_emty==0xFF) first_emty = i;
        if (kk.id!=0xFF)
            if (strcmp((char*)kk.name,name)==0) found = true;
    }
    if (found) return ESP_ERR_NOT_SUPPORTED;
    if (first_emty!=0xFF) {
        kk.id=first_emty;
        strcpy((char*)kk.name,name);
        disk->write_file(ROOM_FILE,&kk,sizeof(room_t),first_emty);
        refresh_names(); // yeni oda anında aranabilir olsun diye önbelleği tazele
        return ESP_OK;
    }
    return ESP_ERR_NO_MEM;
}

esp_err_t Room::room_delete(uint8_t id)
{
    if (!file_ready) return false;
    if (id==99) file_emty();
    if (id<MAX_ROOM) {
        room_t kk={};
        kk.id=0xFF;
        kk.ico=0;
        strcpy((char *)kk.name,"");
        disk->write_file(ROOM_FILE,&kk,sizeof(room_t),id);
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
    refresh_names(); // silinen/sıfırlanan odalar önbellekten de düşsün
    return ESP_OK;
}

cJSON *Room::room_intro(void)
{
    cJSON *Lm;
    cJSON *grp = cJSON_CreateArray();
    for (int i=0;i<MAX_ROOM;i++)
    {
        room_t ff={};
        disk->read_file(ROOM_FILE,&ff,sizeof(room_t),i); 
        if (ff.id!=0xFF) {
            cJSON_AddItemToArray(grp, Lm = cJSON_CreateObject());
            cJSON_AddItemToObject(Lm, "id", cJSON_CreateNumber(ff.id));
            cJSON_AddItemToObject(Lm, "nm", cJSON_CreateString((char*)ff.name)); 
            cJSON_AddItemToObject(Lm, "ico", cJSON_CreateNumber(ff.ico));
        }
    }
    return grp;
}

void Room::list_room(void) {
    for (int i=0;i<MAX_ROOM;i++)
    {
        room_t ff={};
        disk->read_file(ROOM_FILE,&ff,sizeof(room_t),i); 
        printf("Id : %02d Icon: %02d Name : %s\n",ff.id,ff.ico,ff.name); 
    }
}