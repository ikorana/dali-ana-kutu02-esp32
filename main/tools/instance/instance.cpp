

#include "instance.h"

void Instance::bosalt(instance_t *kk)
{
    kk->channel = 0xFF;
    kk->dev_addr = 0xFF;
    kk->ins_addr = 0xFF;
    kk->type = 0x00;
    kk->ins_active = 0x00;
    kk->filter = 0x00;

    kk->status = 0xFF;
    kk->temp = 0x00;
    kk->temp_set = 16;
    kk->temp_type = 0x00;
    kk->com_addr = 0xFF;
    kk->lamp_channel = 0xFF;
}

bool Instance::file_emty(void)
{
    instance_t kk={};
    bosalt(&kk);

    uint16_t sz = sizeof(instance_t) * MAX_INSTANCE;
    instance_t *buffer = (instance_t *)calloc(1,sz);
    if (buffer == nullptr) return false;
    for(int i = 0; i < MAX_INSTANCE; i++) {
        memcpy(&buffer[i], &kk, sizeof(instance_t));
    }
    disk->write_file(INSTANCE_FILE,buffer,sz,0);
    free(buffer);
    return true;
}

bool Instance::file_format(void)
{
    if (!disk->file_control(INSTANCE_FILE))
      {
        file_emty();
        file_ready=true;
      } else file_ready=true;
    return true;
}

cJSON *Instance::instance_intro(void)
{
    cJSON *Lm;
    cJSON *grp = cJSON_CreateArray();
    for (int i=0;i<MAX_INSTANCE;i++)
    {
        instance_t ff={};
        disk->read_file(INSTANCE_FILE,&ff,sizeof(instance_t),i);
        if (ff.dev_addr!=0xFF) {
            cJSON_AddItemToArray(grp, Lm = cJSON_CreateObject());
            cJSON_AddItemToObject(Lm, "chn", cJSON_CreateNumber(ff.channel));
            cJSON_AddItemToObject(Lm, "adr", cJSON_CreateNumber(ff.dev_addr));
            cJSON_AddItemToObject(Lm, "iadr", cJSON_CreateNumber(ff.ins_addr));
            cJSON_AddItemToObject(Lm, "type", cJSON_CreateNumber(ff.type));
            cJSON_AddItemToObject(Lm, "act", cJSON_CreateNumber(ff.ins_active));
            if (ff.type==INSTANCE_TYPE_TEMPERATURE) {
                cJSON_AddItemToObject(Lm, "stat", cJSON_CreateNumber(ff.status));
                cJSON_AddItemToObject(Lm, "temp", cJSON_CreateNumber(ff.temp));
                cJSON_AddItemToObject(Lm, "tset", cJSON_CreateNumber(ff.temp_set));
                cJSON_AddItemToObject(Lm, "ttype", cJSON_CreateNumber(ff.temp_type));
            }
        }
    }
    return grp;
}


void Instance::add(instance_t *kk)
{
    //dosyada bu kanal+cihaz+ins adresine kayıtlı birşey var mı?
    //eger ins adr==0 ise aynı kanal+cihaz adresinde kayıtlı tüm instancelar silinecek
    instance_t bos={};
    bosalt(&bos);

    //0 nolu instance ilave edilirse o kanal+cihaza ait tüm instancelar silinir
    if (kk->ins_addr==0)
    {
        for (uint8_t  i=0;i<MAX_INSTANCE;i++)
        {
            instance_t ff={};
            disk->read_file(INSTANCE_FILE,&ff,sizeof(instance_t),i);
                if (ff.channel==kk->channel && ff.dev_addr==kk->dev_addr) {
                    //printf("%d Kayıtlı Instance(%d:%d:%d) Adresi silindi\n",i,ff.channel,ff.dev_addr,ff.ins_addr);
                    disk->write_file(INSTANCE_FILE,&bos,sizeof(instance_t),i);
                    vTaskDelay(5/portTICK_PERIOD_MS);
                                               }
        }
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
    //İlk boş adresi bul
    uint8_t bos_adres = 0;
    instance_t ff={};
    for (bos_adres=0;bos_adres<MAX_INSTANCE;bos_adres++)
    {
        disk->read_file(INSTANCE_FILE,&ff,sizeof(instance_t),bos_adres);
        if (ff.dev_addr==0xFF) break;
    }

   // printf("%d:%d:%d için bulunan Boş adres %d\n", kk->channel, kk->dev_addr, kk->ins_addr, bos_adres);

    if (bos_adres<MAX_INSTANCE) {
        disk->write_file(INSTANCE_FILE,kk,sizeof(instance_t),bos_adres);
        //printf("%d Adresine yazıldı\n", bos_adres);
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}

void Instance::del(uint8_t channel, uint8_t adr) {
    instance_t ff={};
    instance_t bos={};
    bosalt(&bos);
    for (int i=0;i<MAX_INSTANCE;i++)
    {
        disk->read_file(INSTANCE_FILE,&ff,sizeof(instance_t),i);
        if (ff.channel==channel && ff.dev_addr==adr) {
            // printf("Silinecek\n");
            disk->write_file(INSTANCE_FILE,&bos,sizeof(instance_t),i);
            vTaskDelay(5/portTICK_PERIOD_MS);
        }
    }
}

esp_err_t Instance::get_instance(uint8_t channel, uint8_t adr, uint8_t ins, instance_t *kk)
{
    instance_t ff={};
    for (int i=0;i<MAX_INSTANCE;i++)
    {
        disk->read_file(INSTANCE_FILE,&ff,sizeof(instance_t),i);
        //if(ff.dev_addr!=0xFF) printf("Aranan %d:%d:%d Bulunan %d:%d:%d\n",channel,adr,ins,ff.channel,ff.dev_addr,ff.ins_addr);
        if (ff.channel==channel && ff.dev_addr==adr && ff.ins_addr==ins) {
           // printf("Bulundu\n");
            memcpy(kk,&ff,sizeof(instance_t));
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

void Instance::set_instance(uint8_t channel, uint8_t adr, uint8_t ins, instance_t *kk)
{
    instance_t ff={};
    uint8_t index = 0;
    for (index=0;index<MAX_INSTANCE;index++)
    {
        disk->read_file(INSTANCE_FILE,&ff,sizeof(instance_t),index);
        if (ff.channel==channel && ff.dev_addr==adr && ff.ins_addr==ins) break;
    }
   // printf("Save Bulunan INDEX : %d:%d:%d %d\n",channel,adr,ins,index);
   if (index<MAX_INSTANCE)
        disk->write_file(INSTANCE_FILE,kk,sizeof(instance_t),index);
}


void Instance::list_instance(void)
{
    instance_t ff={};
    for (int i=0;i<MAX_INSTANCE;i++)
    {
        disk->read_file(INSTANCE_FILE,&ff,sizeof(instance_t),i);
        if(ff.dev_addr!=0xFF)
        printf("%d Kanal:%d Adr:%d:%d Tip:%d Aktif:%d\n",
            i,ff.channel,ff.dev_addr,ff.ins_addr,ff.type,ff.ins_active
        );
    }
}
