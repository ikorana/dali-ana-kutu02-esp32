#ifndef _SWITCH_H
#define _SWITCH_H

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include <errno.h>
#include <storage_little.h>
#include <cJSON.h>

#define SWITCH_FILE (char*)"/config/switch.bin" 
#define MAX_SWITCH 64

typedef struct {
    uint8_t type;
    uint8_t name[16];
    uint8_t ico;
    uint8_t id;
    uint8_t room;
    uint8_t level;
    uint8_t sensor_id;
    uint8_t sensor_instance;
    uint8_t sensor_channel;
    uint8_t relay_id;
    uint8_t relay_channel;
    uint8_t set_level; //anahtar için bu alan last level iken termostatta set olarak kullanılır
} switch_t;

class Anahtar;

typedef struct {
  Anahtar *self;
  switch_t *anahtar;
} switch_param_t;

class Anahtar
{
    public:
      Anahtar() {};
      ~Anahtar(){};

      bool file_init(StorageLittle *dsk) {
        disk = dsk;
        return file_format();
      };
      esp_err_t switch_create(const char *name,uint8_t type,
                              uint8_t sensor_id,uint8_t sensor_instance,
                              uint8_t relay_id,uint8_t relay_channel,
                              uint8_t sensor_channel);
      esp_err_t switch_delete(uint8_t id);
      cJSON *switch_intro(void);
      void small_intro(uint8_t room, cJSON *py);

      uint8_t get_level(uint8_t id) {
        switch_t ff={};
        disk->read_file(SWITCH_FILE,&ff,sizeof(switch_t),id); 
        return ff.level;
      }
      
      void get_switch(uint8_t id, switch_t *ff) {
        disk->read_file(SWITCH_FILE,ff,sizeof(switch_t),id); 
      }
      void set_switch(uint8_t id, switch_t *ff) {
        disk->write_file(SWITCH_FILE,ff,sizeof(switch_t),id); 
      }

      void list_switch(void);
      
    private:
      StorageLittle *disk;

      bool file_ready=false;
      bool file_format(void);
      bool file_emty(void);
};

#endif 