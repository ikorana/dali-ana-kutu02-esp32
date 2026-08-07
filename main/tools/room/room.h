#ifndef _ROOM_H
#define _ROOM_H

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include <errno.h>
#include <storage_little.h>
#include <cJSON.h>
#include <vector>
#include "tools/gear/gear.h" // find_param_t için

#define ROOM_FILE (char*)"/config/room.bin"
#define MAX_ROOM 32

typedef struct {
    uint8_t name[16];
    uint8_t ico;
    uint8_t id;
} room_t;

class Room
{
    public:
      Room() {};
      ~Room(){};

      bool file_init(StorageLittle *dsk) {
        disk = dsk;
        return file_format();
      };
      esp_err_t room_create(const char *name);
      esp_err_t room_delete(uint8_t id);
      cJSON *room_intro(void);
      void list_room(void);
      std::vector<find_param_t> get_name(void) {
        return names;
      }

    private:
      StorageLittle *disk;
      bool file_ready=false;
      bool file_format(void);
      bool file_emty(void);
      bool refresh_names(void);
      std::vector<find_param_t> names;
};

#endif 