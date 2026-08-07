#ifndef _FCM_H
#define _FCM_H

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include <errno.h>
#include <storage_little.h>
#include <cJSON.h>

#define FCM_FILE (char*)"/config/fcm.bin" 
#define MAX_FCM 10

typedef struct {
    uint8_t name[16];
    uint8_t uuid[64];
    uint8_t fcm[256];
    uint8_t id;
} fcm_t;

class Fcm
{
    public:
      Fcm() {};
      ~Fcm(){};

      bool file_init(StorageLittle *dsk) {
        disk = dsk;
        return file_format();
      };

      void update_fcm(fcm_t *ff);

      esp_err_t fcm_create(const char *nm, const char *uid, const char *fc);
      esp_err_t fcm_delete(uint8_t id);
      void list_fcm(void);
      bool file_format(void);
      bool file_emty(void);
      
    private:
      StorageLittle *disk;
      bool file_ready=false;

      
};

#endif 