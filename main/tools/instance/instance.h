#ifndef _INSTANCE_H
#define _INSTANCE_H

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include <errno.h>
#include <storage_little.h>
#include <cJSON.h>

#define MAX_INSTANCE 64

typedef enum : uint8_t
{
    CM_LAMP = 0,
    CM_GROUP,
    CM_SCENE,
    CM_SWITCH,
    CM_UNKNOWN,
} instance_command_t;

typedef enum : uint8_t
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
    PR_TOGDIM, // Basılı tutma başına yön değişir: 1.basış YUKARI, 2.basış AŞAĞI... (instance_t.status'ta saklanır)
    PR_UNKNOWN=15,
} instance_process_type_t;

// Instance'ın (giriş kaynağının) türü — hangi payload'ın/mantığın geçerli olacağını belirler.
// Değerler DALI/STM32 protokolündeki gerçek tip kodlarıyla eşleşiyor (main.cpp'deki
// mevcut ins.type==0x01/0x06 kontrolleriyle uyumlu — keyfi seçilmedi).
typedef enum
{
    INSTANCE_TYPE_BUTTON      = 0x01, // Anahtar / push-button
    INSTANCE_TYPE_TEMPERATURE = 0x06, // Isı sensörü / termostat girişi
    INSTANCE_TYPE_MOTION      = 0x03, // Hareket sensörü (henüz firmware'de karşılığı yok, yer tutucu)
} instance_type_t;

// instance_t: hem envanter kaydı ("bu adreste böyle bir giriş var, tipi bu") hem de
// hedef/davranış bilgisi ("bu giriş neyi, nasıl tetikler") — tüm instance tiplerinde
// (buton, hareket, termostat...) aynı şema kullanılıyor. "Anahtarı aç, üzerine hedefi
// (lamba/grup/senaryo/anahtar) ata" prensibiyle: her instance kendi tek hedefini taşır.
typedef struct {
    uint8_t channel;     // DALI: 1-4 | Yerel: 10
    uint8_t dev_addr;    // DALI: kısa adres (0-63) | Yerel: pin numarası
    uint8_t ins_addr;    // DALI: instance no (0-31) | Yerel: 0 (kullanılmıyor)
    uint8_t type;        // instance_type_t
    uint8_t ins_active;  // Aktif mi (DALI'de fiziksel cihazda da ayarlanır)
    uint8_t filter;      // DALI instance event filtresi (yerelde kullanılmaz, 0 kalır)

    instance_command_t com;         // Hedefin türü: lamba/grup/senaryo/anahtar
    instance_process_type_t process; // Hedefe uygulanacak eylem (aç/kapat/toggle...)
    uint8_t com_addr;    // Hedefin adresi (cihaz/grup/senaryo/anahtar id'si)
    uint8_t lamp_channel; // Hedefin kanalı

    // Termostat'a özel çalışma zamanı durumu — olay-tetiklemeli değil, kendi kendine
    // sürekli değerlendirip com_addr/lamp_channel'daki hedefi kontrol eder.
    uint8_t status;       // Termostat: röle/kontaktör son durumu
    uint8_t temp;         // Termostat: son okunan sıcaklık
    uint8_t temp_set;     // Termostat: hedef sıcaklık
    uint8_t temp_type;    // Termostat: 0 ısıtma, 1 soğutma, 2 manuel açık, 3 manuel kapalı
} instance_t;

class Instance
{
    public:
      Instance() {};
      ~Instance(){};

      bool file_init(StorageLittle *dsk, const char *fn) {
        disk = dsk;
        file_name = fn;
        return file_format();
      };

      void bosalt(instance_t *kk);
      void add(instance_t *kk);
      void del(uint8_t channel, uint8_t adr);

      esp_err_t get_instance(uint8_t channel, uint8_t adr, uint8_t ins, instance_t *kk);
      void set_instance(uint8_t channel, uint8_t adr, uint8_t ins, instance_t *kk);
      bool get_instance_at(uint8_t index, instance_t *kk); // ham slot okuma (senkronizasyon/temizlik taramaları için)

      void instance_intro(cJSON *arr); // mevcut diziye kendi kayıtlarını ekler (DALI + yerel katalog birleştirilebilsin diye)
      void list_instance(void);
      void clear_file(void) {
        file_emty();
      }

      
    private:
      StorageLittle *disk;
      const char *file_name;

      bool file_ready=false;
      bool file_format(void);
      bool file_emty(void);
};

#endif 