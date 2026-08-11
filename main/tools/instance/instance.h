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

// NOT: com/process artık instance_t'de değil, trigger_t içinde (gear.h) —
// bir switch/sensor'un NEYİ tetikleyeceği artık lambanın kendi kaydında tutuluyor.
// Bu iki enum burada kalıyor çünkü trigger_t bunları kullanıyor (gear.h instance.h'ı include eder).
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

// instance_t artık sadece bir ENVANTER kaydı: "bu adreste böyle bir giriş var, tipi bu".
// Hedef/davranış bilgisi (kim tetiklenecek, nasıl) artık burada değil — ilgili lambanın
// kendi gear_t kaydındaki trigger_t[] dizisinde tutuluyor (bkz. gear.h).
typedef struct {
    uint8_t channel;     // DALI: 1-4 | Yerel: 10
    uint8_t dev_addr;    // DALI: kısa adres (0-63) | Yerel: pin numarası
    uint8_t ins_addr;    // DALI: instance no (0-31) | Yerel: 0 (kullanılmıyor)
    uint8_t type;        // instance_type_t
    uint8_t ins_active;  // Aktif mi (DALI'de fiziksel cihazda da ayarlanır)
    uint8_t filter;      // DALI instance event filtresi (yerelde kullanılmaz, 0 kalır)

    // Tipe özel çalışma zamanı durumu/hedefi — buton (INSTANCE_TYPE_BUTTON) tipinde
    // kullanılmaz (butonun hedefi lambanın kendi trigger_t'sinde). Termostat ise
    // olay-tetiklemeli değil, kendi kendine sürekli değerlendirip kendi hedefini
    // kontrol eden bir yapı — bu yüzden hedefi burada, instance'ın kendi kaydında tutuluyor.
    uint8_t status;       // Termostat: röle/kontaktör son durumu
    uint8_t temp;         // Termostat: son okunan sıcaklık
    uint8_t temp_set;     // Termostat: hedef sıcaklık
    uint8_t temp_type;    // Termostat: 0 ısıtma, 1 soğutma, 2 manuel açık, 3 manuel kapalı
    uint8_t com_addr;     // Termostat: kontrol edilecek röle/kontaktör adresi
    uint8_t lamp_channel; // Termostat: hedefin kanalı
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
      void del(uint8_t channel, uint8_t adr);

      esp_err_t get_instance(uint8_t channel, uint8_t adr, uint8_t ins, instance_t *kk);
      void set_instance(uint8_t channel, uint8_t adr, uint8_t ins, instance_t *kk);

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