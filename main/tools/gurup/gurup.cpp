
#include "gurup.h"
#include "cJSON.h"
#include "jsontool.h"

bool Gurup::file_emty(void)
{
    gurup_t kk={};
    kk.ico= 0;
    kk.id = 0xFF;
    char *mm;
    for (int i=0;i<MAX_GURUP;i++)
    {   
        asprintf(&mm,"%s %d",name,i);
        strcpy((char *)kk.name,mm);  
        kk.id = i;      
        disk->write_file(file_name,&kk,sizeof(gurup_t),i);            
        vTaskDelay(10 / portTICK_PERIOD_MS);
    } 
    return true;
}

bool Gurup::refresh_names(void)
{
    // Önceki taramadan kalan heap string'lerini serbest bırakmadan clear() yaparsak sızıntı olur.
    for (find_param_t &fp : names) {
        if (fp.name) free(fp.name);
    }
    names.clear();

    for (int i=0;i<MAX_GURUP;i++) {
        gurup_t ff={};
        disk->read_file(file_name,&ff,sizeof(gurup_t),i);
        if (ff.id!=0xFF) {
            find_param_t aa = {};
            char *nm;
            asprintf(&nm,"%s",ff.name);
            aa.name = nm;
            aa.index = i;
            names.push_back(aa);
        }
    }

    return true;
}

bool Gurup::file_format(void)
{
    if (!disk->file_control(file_name))
      {
        file_emty();
        file_ready=true;
      } else file_ready=true;

    refresh_names();

    return true;
}

void Gurup::set_group_name(uint8_t adr, const char *nm)
{
    if (adr>=MAX_GURUP) return;
    gurup_t kk={};
    disk->read_file(file_name,&kk,sizeof(gurup_t),adr);
    if (kk.id==0xFF) return;
    strcpy((char*)kk.name,nm);
    disk->write_file(file_name,&kk,sizeof(gurup_t),adr);
    refresh_names(); // yeni isim anında aranabilir olsun diye önbelleği tazele
}

cJSON *Gurup::gurup_intro(void)
{
    cJSON *Lm;
    cJSON *grp = cJSON_CreateArray();
    for (int i=0;i<MAX_GURUP;i++)
    {
        gurup_t ff={};
        disk->read_file(file_name,&ff,sizeof(gurup_t),i); 
        if (ff.id!=0xFF) {
            cJSON_AddItemToArray(grp, Lm = cJSON_CreateObject());
            cJSON_AddItemToObject(Lm, "id", cJSON_CreateNumber(i));
            cJSON_AddItemToObject(Lm, "nm", cJSON_CreateString((char*)ff.name)); 
            cJSON_AddItemToObject(Lm, "ico", cJSON_CreateNumber(ff.ico));
        }
    }
    return grp;
}

void Gurup::set_time(cJSON *pay)
{
    uint8_t adr = 0;
    JSON_getint(pay,"adres", &adr);
    gurup_t kk={};
    disk->read_file(file_name,&kk,sizeof(gurup_t),adr); 

    cJSON *t1;
    t1 = cJSON_GetObjectItem(pay, "time1");
    cJSON *t2;
    t2 = cJSON_GetObjectItem(pay, "time2");

    JSON_getint(t1,"onactive", &kk.timing1.onactive);
    JSON_getint(t1,"offactive", &kk.timing1.offactive);
    JSON_getint(t1,"power", &kk.timing1.power);
    cJSON *ot = cJSON_GetObjectItem(t1, "ontime");
    JSON_getint(ot,"saat", &kk.timing1.ontime.saat);
    JSON_getint(ot,"dakika", &kk.timing1.ontime.dakika);
    cJSON *ot1 = cJSON_GetObjectItem(t1, "offtime");
    JSON_getint(ot1,"saat", &kk.timing1.offtime.saat);
    JSON_getint(ot1,"dakika", &kk.timing1.offtime.dakika);

    JSON_getint(t2,"onactive", &kk.timing2.onactive);
    JSON_getint(t2,"offactive", &kk.timing2.offactive);
    JSON_getint(t2,"power", &kk.timing2.power);
    cJSON *gt = cJSON_GetObjectItem(t2, "ontime");
    JSON_getint(gt,"saat", &kk.timing2.ontime.saat);
    JSON_getint(gt,"dakika", &kk.timing2.ontime.dakika);
    cJSON *gt1 = cJSON_GetObjectItem(t2, "offtime");
    JSON_getint(gt1,"saat", &kk.timing2.offtime.saat);
    JSON_getint(gt1,"dakika", &kk.timing2.offtime.dakika);

    disk->write_file(file_name,&kk,sizeof(gurup_t),adr); 
}

void Gurup::get_time(uint8_t adr, zaman_t *z1, zaman_t *z2)
{
    gurup_t kk={};
    disk->read_file(file_name,&kk,sizeof(gurup_t),adr); 
    *z1 = kk.timing1;
    *z2 = kk.timing2;
}