#ifndef _GEAR_H
#define _GEAR_H

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include <errno.h>
#include <storage_little.h>
#include <cJSON.h>
#include <core.h>
#include <vector>

#include "tools/instance/instance.h"

#define MAX_GEAR 64

typedef struct {
  char *name;
  uint8_t index;
} find_param_t;

// Oda bazlı fan-out için (örn. "salon lambalarını aç") — file_format() sırasında
// isimlerle AYNI döngüde, ekstra disk okuması olmadan dolduruluyor.
typedef struct {
  uint8_t index;
  uint8_t room;
  uint8_t ext_type;
  uint8_t short_addr;
} gear_room_entry_t;


typedef struct {
    uint8_t group[2];
    uint8_t status;
    uint8_t reserved[5];
} __attribute__((packed)) dev_spec_t;

typedef struct {
    uint8_t aktif;
    uint8_t short_addr;
    uint32_t random_addr;
    uint8_t name[32];

    uint8_t type;
    uint8_t ext_type;
    dev_sourge_t kanal;        
    uint8_t ico;
    uint8_t room;
    uint8_t instance;
    dev_spec_t spec;
    uint8_t anahtar;
} __attribute__((packed)) gear_t;


class Gear
{
    public:
      Gear() {};
      ~Gear(){};

      bool file_init(StorageLittle *dsk, const char *fn, Instance *inst) {
        disk = dsk;
        file_name = fn;
        Ins = inst;
        return file_format();
      };
      cJSON *gear_intro(void);
      void gear_small_intro(uint8_t room, cJSON *py);
      void clear_file(void) {
        file_emty();
      }
      void set_gurup(uint8_t adr, uint8_t glow, uint8_t ghigh);
      void get_gurup(uint8_t adr, uint8_t *glow, uint8_t *ghigh);
      void refresh(uint8_t *count,pck_t *pck, send_AK_t sendAK, send_Status_t sendStatus, bool is_mqtt);
      void set_status(uint8_t adr, uint8_t glow, uint8_t ghigh, uint8_t level);

      void read_gear(uint8_t adr, gear_t *ff);
      void write_gear(uint8_t adr, gear_t *ff);
      void update_gear(uint8_t adr, gear_t *ff);
      void list_gear(void);
      uint8_t get_gear_count(void);
      bool refresh_names(void);
      std::vector< find_param_t > get_name(void) {
        return names;
      }
      std::vector< gear_room_entry_t > get_room_entries(void) {
        return room_entries;
      }




    private:
      StorageLittle *disk;
      Instance *Ins;
      const char *file_name;
      bool file_ready=false;
      bool file_format(void);
      bool file_emty(void);
      std::vector< find_param_t > names;
      std::vector< gear_room_entry_t > room_entries;
};

#endif 