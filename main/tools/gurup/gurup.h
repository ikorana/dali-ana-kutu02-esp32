#ifndef _GURUP_H
#define _GURUP_H

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include <errno.h>
#include <storage_little.h>
#include <cJSON.h>
#include <core.h>
#include <vector>
#include "tools/gear/gear.h" // find_param_t için


#define MAX_GURUP 16

typedef struct {
    uint8_t name[20];
    uint8_t ico;
    uint8_t id;
    zaman_t timing1;
    zaman_t timing2;
} gurup_t;

class Gurup
{
    public:
      Gurup() {};
      ~Gurup(){};

      bool file_init(StorageLittle *dsk, const char *fn, const char *nm) {
        disk = dsk;
        file_name = fn;
        name = nm;
        return file_format();
      };
      cJSON *gurup_intro(void);
      void set_time(cJSON *pay);
      void get_time(uint8_t adr, zaman_t *z1, zaman_t *z2);
      void set_group_name(uint8_t adr, const char *nm);
      std::vector<find_param_t> get_name(void) {
        return names;
      }

    private:
      StorageLittle *disk;
      const char *file_name;
      const char *name;
      bool file_ready=false;
      bool file_format(void);
      bool file_emty(void);
      bool refresh_names(void);
      std::vector<find_param_t> names;

};

#endif 