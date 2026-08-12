
#include "switch.h"
#include "freertos/idf_additions.h"
#include "tools/gear/gear.h"

bool Anahtar::file_emty(void)
{
    switch_t kk={};
    kk.ico= 0;
    kk.id = 0xFF;
    kk.level = 0x00;
    kk.type = 0x00;
    kk.sensor_id = 0xFF;
    kk.sensor_instance = 0xFF;
    kk.sensor_channel = 0xFF;
    kk.relay_id = 0xFF;
    kk.relay_channel = 0xFF;
    kk.room = 0xFF;
    kk.set_level = 16;
    strcpy((char *)kk.name,"");

    uint16_t sz = sizeof(switch_t) * MAX_SWITCH;
    switch_t *buffer = (switch_t *)calloc(1,sz);
    if (buffer == nullptr) return false;
    for(int i = 0; i < MAX_GEAR; i++) {
        memcpy(&buffer[i], &kk, sizeof(switch_t));
    }
    disk->write_file(SWITCH_FILE,buffer,sz,0); 
    free(buffer);
    return true;
}

bool Anahtar::file_format(void)
{   
    if (!disk->file_control(SWITCH_FILE))
      {
        file_emty();
        file_ready=true;          
      } else file_ready=true;
    return true;
}

esp_err_t Anahtar::switch_create(const char *name,uint8_t type,
                              uint8_t sensor_id,uint8_t sensor_instance,
                              uint8_t relay_id,uint8_t relay_channel,
                              uint8_t sensor_chn
                            )
{
    if (!file_ready) return false;
    switch_t kk={};
    uint8_t first_emty = 0xFF;
    bool found = false;
  
    for (int i=0;i<MAX_SWITCH;i++)
    {            
        disk->read_file(SWITCH_FILE,&kk,sizeof(switch_t),i); 
        if (kk.id==0xFF && first_emty==0xFF) first_emty = i;
        if (kk.id!=0xFF)
            if (strcmp((char*)kk.name,name)==0) found = true;
    }
    if (found) return ESP_ERR_NOT_SUPPORTED;
    if (first_emty!=0xFF) {
        kk.id=first_emty;
        strcpy((char*)kk.name,name);
        kk.type = type;
        kk.sensor_id = sensor_id;
        kk.sensor_instance = sensor_instance;
        kk.relay_id = relay_id;
        kk.relay_channel = relay_channel;
        kk.sensor_channel=sensor_chn;
        kk.ico = 0;
        kk.level = 0;
        kk.room = 0xFF;
        kk.set_level = 16;
        disk->write_file(SWITCH_FILE,&kk,sizeof(switch_t),first_emty); 
        return ESP_OK;
    } 
    return ESP_ERR_NO_MEM;
}
      
esp_err_t Anahtar::switch_delete(uint8_t id)
{
    if (!file_ready) return false;
    if (id==99) file_emty();
    if (id<MAX_SWITCH) {
        switch_t kk={};
        kk.id=0xFF;
        kk.ico=0;
        kk.level=0;
        strcpy((char *)kk.name,"");
        disk->write_file(SWITCH_FILE,&kk,sizeof(switch_t),id);
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
    return ESP_OK;
}

cJSON *Anahtar::switch_intro(void)
{
    cJSON *Lm;
    cJSON *grp = cJSON_CreateArray();
    for (int i=0;i<MAX_SWITCH;i++)
    {
        switch_t ff={};
        disk->read_file(SWITCH_FILE,&ff,sizeof(switch_t),i); 
        if (ff.id!=0xFF) {
            cJSON_AddItemToArray(grp, Lm = cJSON_CreateObject());
            cJSON_AddItemToObject(Lm, "tp", cJSON_CreateNumber(ff.type));
            cJSON_AddItemToObject(Lm, "id", cJSON_CreateNumber(i));
            cJSON_AddItemToObject(Lm, "nm", cJSON_CreateString((char*)ff.name)); 
            cJSON_AddItemToObject(Lm, "ico", cJSON_CreateNumber(ff.ico));
            cJSON_AddItemToObject(Lm, "level", cJSON_CreateNumber(ff.level));
            cJSON_AddItemToObject(Lm, "sens_id", cJSON_CreateNumber(ff.sensor_id));
            cJSON_AddItemToObject(Lm, "sens_ins", cJSON_CreateNumber(ff.sensor_instance));
            cJSON_AddItemToObject(Lm, "senschn", cJSON_CreateNumber(ff.sensor_channel));
            cJSON_AddItemToObject(Lm, "rel_id", cJSON_CreateNumber(ff.relay_id));
            cJSON_AddItemToObject(Lm, "rel_chn", cJSON_CreateNumber(ff.relay_channel));
            cJSON_AddItemToObject(Lm, "room", cJSON_CreateNumber(ff.room));
            cJSON_AddItemToObject(Lm, "set", cJSON_CreateNumber(ff.set_level));
        }
    }
    return grp;
}

void Anahtar::small_intro(uint8_t room, cJSON *py)
{
    cJSON *Lm;
    for (int i=0;i<MAX_SWITCH;i++)
    {
        switch_t ff={};
        disk->read_file(SWITCH_FILE,&ff,sizeof(switch_t),i); 
        if (ff.id!=0xFF && ff.room==room) {
            cJSON_AddItemToArray(py, Lm = cJSON_CreateObject());
            cJSON_AddItemToObject(Lm, "tp", cJSON_CreateNumber(ff.type));
            cJSON_AddItemToObject(Lm, "id", cJSON_CreateNumber(i));
            cJSON_AddItemToObject(Lm, "nm", cJSON_CreateString((char*)ff.name)); 
            cJSON_AddItemToObject(Lm, "ico", cJSON_CreateNumber(ff.ico));
            cJSON_AddItemToObject(Lm, "level", cJSON_CreateNumber(ff.level));
            cJSON_AddItemToObject(Lm, "sens_id", cJSON_CreateNumber(ff.sensor_id));
            cJSON_AddItemToObject(Lm, "sens_ins", cJSON_CreateNumber(ff.sensor_instance));
            cJSON_AddItemToObject(Lm, "senschn", cJSON_CreateNumber(ff.sensor_channel));
            cJSON_AddItemToObject(Lm, "rel_id", cJSON_CreateNumber(ff.relay_id));
            cJSON_AddItemToObject(Lm, "rel_chn", cJSON_CreateNumber(ff.relay_channel));
            cJSON_AddItemToObject(Lm, "room", cJSON_CreateNumber(ff.room));
            cJSON_AddItemToObject(Lm, "set", cJSON_CreateNumber(ff.set_level));
        }
    }
}


void Anahtar::list_switch(void) {
    for (int i=0;i<MAX_SWITCH;i++)
    {
        switch_t ff={};
        disk->read_file(SWITCH_FILE,&ff,sizeof(switch_t),i); 
        if (ff.id!=0xFF) {
            printf("Id : %02d Type: %02d SensId:%02d SendINS:%d RelId:%02d RelChn:%d Set:%02d Name : %s\n",
                ff.id,ff.type,
                ff.sensor_id, ff.sensor_instance,
                ff.relay_id,ff.relay_channel,
                ff.set_level,
                ff.name
            ); 
            }
    }
}



