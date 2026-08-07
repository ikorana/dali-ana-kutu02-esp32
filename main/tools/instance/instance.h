#ifndef _INSTANCE_H
#define _INSTANCE_H

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include <errno.h>
#include <storage_little.h>
#include <cJSON.h>

#define INSTANCE_FILE (char*)"/config/instance.bin"
#define MAX_INSTANCE 64

typedef enum 
{
    CM_LAMP = 0,
    CM_GROUP,
    CM_SCENE,
    CM_SWITCH,
    CM_UNKNOWN,
} instance_command_t;

typedef enum 
{   
    PR_ON = 0,
    PR_OFF,
    PR_TOGGLE,  
    PR_UPDIM,
    PR_DOWNDIM, 
    PR_ARCPOWER,
    PR_MAXLEVEL,
    PR_MINLEVEL,
    PR_ONOFF,
    PR_TOGMAX,
    PR_MXMN,
    PR_UNKNOWN=15,
} instance_process_type_t;



typedef struct {
    uint8_t dev_addr;  //Device adresi
    uint8_t ins_addr;  //Instance adresi
    uint8_t channel;   //Device kanalı
    uint8_t type;      //Instance tipi
    uint8_t ins_active;  //instans aktif mi (Cihazda var)
    uint8_t filter;    //Filtre (cihazda var)
    uint8_t timers[3]; //Zamanlamalar (cihazda var)
    
    instance_command_t com; //Komut
    uint8_t com_addr;    //Komutun uygulanacagı Adresi (röle lamba vs)
    instance_process_type_t process; //İşlem 
    uint8_t status;      //çıkışın son durumunu gösterir
    uint8_t lamp_channel; //çıkışın kanalı nedir
    //--------
    uint8_t temp_set;
    uint8_t temp; 
    uint8_t temp_type;  //0 ısıtma 1 sogutma 2 manuel on 3 manuel off    
} instance_t;

class Instance
{
    public:
      Instance() {};
      ~Instance(){};

      bool file_init(StorageLittle *dsk) {
        disk = dsk;
        return file_format();
      };

      void bosalt(instance_t *kk);
      void add(instance_t *kk);
      void del(uint8_t adr);

      esp_err_t get_instance(uint8_t adr, uint8_t ins, instance_t *kk);
      void set_instance(uint8_t adr, uint8_t ins, instance_t *kk);
      
      cJSON *instance_intro(void);
      void list_instance(void);
      void clear_file(void) {
        file_emty();
      }

      
    private:
      StorageLittle *disk;


      bool file_ready=false;
      bool file_format(void);
      bool file_emty(void);
};

#endif 