
#include "fcm.h"

bool Fcm::file_emty(void)
{
    fcm_t kk={};
    strcpy((char *)kk.uuid,"");
    strcpy((char *)kk.fcm,"");
    strcpy((char *)kk.name,"");
    kk.id = 0xFF;

    uint16_t sz = sizeof(fcm_t) * MAX_FCM;
    fcm_t *buffer = (fcm_t *)calloc(1,sz);
    if (buffer == nullptr) return false;
    for(int i = 0; i < MAX_FCM; i++) {
        memcpy(&buffer[i], &kk, sizeof(fcm_t));
    }
    disk->write_file(FCM_FILE,buffer,sz,0); 
    free(buffer);
    return true;
}

bool Fcm::file_format(void)
{   
    if (!disk->file_control(FCM_FILE))
      {
        file_emty();
        file_ready=true;          
      } else file_ready=true;
    return true;
}

esp_err_t Fcm::fcm_create(const char *nm, const char *uid, const char *fc)
{
    if (!file_ready) return false;
    fcm_t kk={};
    uint8_t first_emty = 0xFF;
    bool found = false;
  
    for (int i=0;i<MAX_FCM;i++)
    {            
        disk->read_file(FCM_FILE,&kk,sizeof(fcm_t),i); 
        if (kk.id==0xFF && first_emty==0xFF) first_emty = i;
        if (kk.id!=0xFF)
            if (strcmp((char*)kk.name,nm)==0) found = true;
    }
    if (found) return ESP_ERR_NOT_SUPPORTED;
    if (first_emty!=0xFF) {
        kk.id=first_emty;
        strcpy((char*)kk.name,nm);
        strcpy((char*)kk.uuid,uid);
        strcpy((char*)kk.fcm,fc);

        disk->write_file(FCM_FILE,&kk,sizeof(fcm_t),first_emty); 
        return ESP_OK;
    } 
    return ESP_ERR_NO_MEM;
}
      
esp_err_t Fcm::fcm_delete(uint8_t id)
{
    if (!file_ready) return false;
    if (id==99) file_emty();
    if (id<MAX_FCM) {
        fcm_t kk={};
        kk.id=0xFF;
        strcpy((char *)kk.uuid,"");
        strcpy((char *)kk.fcm,"");
        strcpy((char *)kk.name,"");

        disk->write_file(FCM_FILE,&kk,sizeof(fcm_t),id);
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
    return ESP_OK;
}


void Fcm::list_fcm(void) {
    for (int i=0;i<MAX_FCM;i++)
    {
        fcm_t ff={};
        disk->read_file(FCM_FILE,&ff,sizeof(fcm_t),i); 
        if (ff.id==0xFF) continue;
        printf("Id : %02d UUID: %-32s Fcm : %s\n",ff.id,ff.uuid,ff.fcm); 
    }
}


void Fcm::update_fcm(fcm_t *gg) {
    fcm_t ff = {};
    uint8_t found=0x00;

    for (int i=0;i<MAX_FCM;i++)
    {
        disk->read_file(FCM_FILE,&ff,sizeof(fcm_t),i); 
        if (strcmp((char*)ff.name,(char*)gg->name)==0) {
            found=i;
            break;
        }
    }

    if (found>0) {
        disk->read_file(FCM_FILE,&ff,sizeof(fcm_t),found);
        strcpy((char *)ff.uuid,(char *)gg->uuid);
        strcpy((char *)ff.fcm,(char *)gg->fcm);
        strcpy((char *)ff.name,(char *)gg->name);
        ff.id=found;
        disk->write_file(FCM_FILE,&ff,sizeof(fcm_t),found);  
    } else {
        fcm_create((char *)gg->name,(char *)gg->uuid,(char *)gg->fcm);
    }

}