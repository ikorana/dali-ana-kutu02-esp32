#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_event.h"
#include "time.h"

#include <vector>


#define STM_TX GPIO_NUM_33 //STM32ye baglanan UART
#define STM_RX GPIO_NUM_32 //STM32ye baglanan UART 
#define STM_RESET GPIO_NUM_15 //STM32 Resetleme Ucu

#define STM_PING GPIO_NUM_25 //BU HAT NORMALDE 1 DE DURACAK
#define STM_PONG GPIO_NUM_26 //BU HAT NORMALDE 1 DE DURACAK

//STMDE PB12 İNPUT OLACAK PİNG BURADAN GELECEK
//PB11 DEN CEVAP VERECEK 


#define CAN_D GPIO_NUM_17 //Can Bus  
#define CAN_R GPIO_NUM_13 //Can Bus

#define LED GPIO_NUM_0


#define BIT_START    (1 << 0)
#define BIT_CONTINUE (1 << 1)
#define BIT_STOP     (1 << 2)

EventGroupHandle_t searchGROUP = xEventGroupCreate();
class Base_Event;
std::vector<Base_Event *> event_list; 
typedef struct {
    struct tm timeinfo;
    struct tm sunrise;
    struct tm sunset; 
    uint8_t haftanin_gunu;
    uint8_t yilin_haftasi;
    uint16_t yilin_gunu;
} tarih_saat_t;

tarih_saat_t tarih_saat = {};

typedef struct {
    uint8_t komut;
    uint8_t altkomut;
    uint8_t index;
    uint8_t location;
    uint8_t room;
    // "refresh" gibi kendi taraması arka planda süren, henüz bitmemiş
    // komutlar için true — firebase_voice_task "Komutunuz uygulandı"
    // yerine kısa bir "bekleyiniz" mesajı söyler, sonucun (say ile) erken
    // gelen asıl özetle çakışmaması için.
    bool devam_ediyor;
} voice_sonuc_t;

QueueHandle_t xvoiceQueue = NULL;

#include "tools/dali_command.h"

#include "core.h"
#include "storage_little.h"
#include "eth5500.h"
#include "wifista.h"
#include "wifiAP.h"
#include "iptool.h"
#include "esp_sntp.h"
#include "udp_server_gem.h"
#include "esp32Time.h"

#include "http.h"
#include "esp_ota_ops.h"
#include "tftp_ota_server.h"
#include "tftp_disk_server.h"
#include "mqtt_client.h"
#include "mdns.h"
#include "esp_http_client.h"
#include "esp_tls.h"

#include "tools/Uart.hpp"

void to_lowercase(char *str) {
    while (*str) {
        // Her karakteri sırayla küçük harfe çeviriyoruz
        *str = tolower((unsigned char)*str);
        str++;
    }
}

void ing_yap(const char *src, char *dest) {
    while (*src) {
        // UTF-8'de Türkçe karakterler iki bayt kaplar ve ilk baytları genellikle 0xC3 veya 0xC4'tür.
        if ((unsigned char)*src == 0xC3) {
            src++; // İkinci bayta geç
            switch ((unsigned char)*src) {
                case 0x87: *dest++ = 'C'; break;  // Ç
                case 0xA7: *dest++ = 'c'; break;  // ç
                case 0x96: *dest++ = 'O'; break;  // Ö
                case 0xB6: *dest++ = 'o'; break;  // ö
                case 0x9C: *dest++ = 'U'; break;  // Ü
                case 0xBC: *dest++ = 'u'; break;  // ü
                default:
                    // Eğer eşleşmediyse bozmamak için ilk baytı ve bu baytı aynen yaz
                    *dest++ = 0xC3; 
                    *dest++ = *src; 
                    break;
            }
        } 
        else if ((unsigned char)*src == 0xC4) {
            src++; // İkinci bayta geç
            switch ((unsigned char)*src) {
                case 0x9E: *dest++ = 'G'; break;  // Ğ
                case 0x9F: *dest++ = 'g'; break;  // ğ
                case 0xB0: *dest++ = 'I'; break;  // İ (UTF-8'de büyük İ harfi C4 B0'dır)
                case 0xB1: *dest++ = 'i'; break;  // ı
                default: 
                    *dest++ = 0xC4; 
                    *dest++ = *src; 
                    break;
            }
        } 
        else if ((unsigned char)*src == 0xC5) {
            src++; // İkinci bayta geç
            switch ((unsigned char)*src) {
                case 0x9E: *dest++ = 'S'; break;  // Ş
                case 0x9F: *dest++ = 's'; break;  // ş
                default: 
                    *dest++ = 0xC5; 
                    *dest++ = *src; 
                    break;
            }
        } 
        else {
            // Standart ASCII karakteri (İngilizce harfler, sayılar vb. direkt kopyala)
            *dest++ = *src;
        }
        src++;
    }
    *dest = '\0'; // String sonlandırıcıyı ekle
}


void mqtt_send(const char *data);
QueueHandle_t IntQueue = xQueueCreate(5, sizeof(uint32_t));

typedef void (*base_callback_t)(void *fnc, uint8_t id, const char* txt);
typedef void (*base_say_callback_t)(void *fnc, uint8_t id, const char* txt);
typedef void (*send_events_t)(cJSON *par);
void send_events(cJSON *pay); 


#include "tools/room/room.h"
#include "tools/switch/switch.h"
#include "tools/gurup/gurup.h"
#include "tools/instance/instance.h"
#include "tools/gear/gear.h"
#include "tools/fcm/fcm.h"
#include "tools/base/base.h"
#include "tools/base/relay.h"
#include "tools/base/energy.h"
#include "tools/base/elevator.h"
#include "tools/base/gas.h"
#include "tools/base/blind.h"
#include "tools/base/water.h"
#include "tools/base/priz.h"
#include "tools/base/mwater.h"
#include "tools/base/door.h"
#include "tools/base/garage.h"
#include "tools/base/relaylamp.h"
#include "tools/base/movement.h"

#include "tools/base/irrigation.h"

std::vector<Base_Device*> cihaz_listesi;

// Global ReliableUartManager instance
ReliableUartManager reliableUartManager;

Irrigation *irrigation = nullptr;


void temp_role_degerlendir(instance_t *ins);
void send_wifi(cJSON *pay);
void base_function_callback(void *fnc, uint8_t id, const char *txt);
void base_function_saycallback(void *fnc, uint8_t id, const char *txt);
void send_AK(cJSON *pay, pck_t *pck, bool is_mqtt=false);
static void voice_parse_task(void *args);
static void new_voice_parse_task(void *args);
uint8_t find_devicename(std::vector<find_param_t> lst, char *txt);
static int apply_category_fanout(uint8_t ext_type_beklenen, uint8_t oda_index,
                                  altkomuttipi_t altkomut, uint8_t guc_yuzde, uint8_t guc_seviyesi,
                                  pck_t *pck, bool is_mqtt);
static bool apply_named_device_command(const char *search_text, altkomuttipi_t altkomut,
                                        uint8_t guc_yuzde, uint8_t guc_seviyesi,
                                        pck_t *pck, bool is_mqtt);

typedef struct {
    uint8_t addr;
    uint8_t type;
    uint8_t exttype;
    uint8_t name[32];
} searchMessage_t;

// Kuyruk tutucu (handle)
QueueHandle_t searchQueue = xQueueCreate(5, sizeof(searchMessage_t));
QueueHandle_t wsearchQueue = xQueueCreate(5, sizeof(searchMessage_t));

// searchQueue/wsearchQueue düzinelerce yerden paylaşılıyor; reset->send->receive
// üçlüsünü bu mutex'lerle seri hale getiriyoruz ki eşzamanlı iki istek birbirinin
// cevabını çalmasın.
SemaphoreHandle_t searchQueueMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t wsearchQueueMutex = xSemaphoreCreateMutex();

static uint8_t willTopic[64];
static uint8_t inTopic[64];
static uint8_t outTopic[64];


#include "tools/sound_command.cpp"
#include "tools/define.cpp"
#include "tools/handler.cpp"
#include "tools/udp_events.cpp"
#include "tools/global_tools.cpp"
#include "tools/httpd_events.cpp"
#include "tools/mqtt_tool.cpp"
#include "tools/connection.cpp"
#include "tools/voice.cpp"

typedef struct {
    uint32_t addr     : 8;
    uint32_t instance : 6;
    uint32_t event    : 10;
    uint32_t reserved : 8;
} EventData_t;

typedef union {
    EventData_t alan;        // Bit alanları olarak
    uint32_t  value;         // 32-bit değer olarak
    uint8_t   by[4];      // 4 byte olarak
} EventDataUnion_t;

typedef struct {
   uint8_t channel;
   uint8_t adr;
   uint8_t ins;
   uint8_t ev;
} event_proc_par_t;

static bool stm_send_status = false;


void base_function_saycallback(void *fnc, uint8_t id, const char *txt) {

   // Base_Device *dev = (Base_Device *)fnc;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com","say");
    cJSON_AddStringToObject(root, "txt", txt);
    char *dat = cJSON_PrintUnformatted(root);
    udp_server.send_unicast_all((uint8_t *)dat,strlen(dat));
   // printf("%s\n",dat);
    cJSON_free(dat);
    cJSON_Delete(root);
    printf("SAY : %s\n",txt);
}

void base_function_callback(void *fnc, uint8_t id, const char *txt) {
    Base_Device *dev = (Base_Device *)fnc;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com","get_level");
    cJSON_AddNumberToObject(root, "adres",dev->device_id);
    cJSON_AddNumberToObject(root, "kanal",dev->device_channel);
    cJSON_AddNumberToObject(root, "power",dev->state);
    if (txt!=nullptr) cJSON_AddStringToObject(root, "txt", txt);
    char *dat = cJSON_PrintUnformatted(root);
    udp_server.send_unicast_all((uint8_t *)dat,strlen(dat));
//    printf("%s\n",dat);

    cJSON_free(dat);
    cJSON_Delete(root);
    //printf("Mesaj Geldi %s State:%d ID:%d\n",dev->name,dev->state,id);
}

void lamp_action(uint8_t adr, uint8_t grp, uint8_t cmd, uint8_t chn, bool say=true)
{
        if (chn==10) {
            // Yerel cihaz — DALI/STM'e hiç gitmez, cihaz_listesi üzerinden
            // doğrudan (senkron) uygulanır.
            for (Base_Device* cihaz : cihaz_listesi) {
                if (cihaz->device_id==adr || adr==0xff) {
                    if (cmd==0x00) cihaz->off(say);
                    else cihaz->set_power(254, say); // MAX LEVEL (0x05) ve benzeri "aç" komutları
                }
            }
            return;
        }

        uint32_t cmm = create_command(adr,(grp==0)?false:true,false,cmd);
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "send");
        cJSON_AddNumberToObject(root, "hexcom", cmm);
        cJSON_AddNumberToObject(root, "kanal",chn);
        cJSON_AddNumberToObject(root, "bit",16);
        send_STM(root);
        cJSON_Delete(root);
}

// instance_t.process -> DALI hexcom eşlemesi. OFF/TOGGLE/sensör-kaynaklı tipler burada değil,
// trigger_instance() içinde ayrı ele alınıyor (özel STM komutu ya da canlı durum gerektiriyorlar).
static uint8_t process_to_hexcom(instance_process_type_t proc)
{
    switch (proc) {
        case PR_ON:       return 0x0A; // GOTO LAST ACTIVE LEVEL
        case PR_MAXLEVEL: return 0x05; // RECALL MAX LEVEL
        case PR_MINLEVEL: return 0x06; // RECALL MIN LEVEL
        case PR_UPDIM:    return 0x01; // UP
        case PR_DOWNDIM:  return 0x02; // DOWN
        default:          return 0x00;
    }
}

typedef struct {
    uint8_t adr;
    uint8_t kanal;
    uint8_t grp;
    bool say;
} toggle_query_par_t;

// DALI'de gerçek toggle: mevcut durumu sorgulayıp tam tersini gönderir. STM'nin kendi
// donanımsal toggle komutuna (0xEE) güvenmek yerine biz garanti ediyoruz — bu yüzden ayrı
// bir task olarak çalışır (query bloklayıcı, tus/UART işleme hattını beklemeye almamalı).
static void dali_toggle_query_task(void *pvParameters)
{
    toggle_query_par_t param = {};
    memcpy(&param, pvParameters, sizeof(toggle_query_par_t));

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", create_command(param.adr, param.grp==1, false, QUERY_STATUS));
    cJSON_AddNumberToObject(root, "kanal", param.kanal);
    cJSON_AddNumberToObject(root, "bit", 16);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    send_STM(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(3000));
    xSemaphoreGive(searchQueueMutex);

    bool isOn = ((msg.name[0] & 0x04) == 0x04);
    lamp_action(param.adr, param.grp, isOn ? 0x00 : 0x05, param.kanal, param.say);

    vTaskDelete(NULL);
}

// Bir instance'ın (DALI ya da yerel, fark etmez) kendi kayıtlı hedefini (com/com_addr/
// lamp_channel) process'ine göre tetikler. event_stat, sensör-kaynaklı process tiplerinde
// (Max On/Off, MxOn/MnOff) ham olay durumunu (0/1) taşır; buton tipi tetiklemelerde 1 gönderilir.
// followState=true: process'ten bağımsız, doğrudan event_stat'ı takip eder (ANAHTAR/momentary
// tuş — basılıyken açık, bırakılınca kapalı).
// toggleState=true: process'ten bağımsız, her çağrıda hedefin durumunu tersine çevirir (SWITCH
// — her stat==1'de lamba durumu değişir).
void trigger_instance(instance_t &ins, uint8_t event_stat = 1, bool say = true, bool followState = false, bool toggleState = false)
{
    if (ins.com == CM_UNKNOWN || ins.com_addr == 0xff) return; // hedef atanmamış

    if (followState) {
        uint8_t grp = (ins.com == CM_GROUP) ? 1 : 0;
        uint8_t hexcom = (event_stat != 0) ? 0x05 : 0x00; // Max Level : Off
        switch (ins.com) {
            case CM_LAMP:
            case CM_GROUP:
                lamp_action(ins.com_addr, grp, hexcom, ins.lamp_channel, say);
                break;
            case CM_SWITCH: {
                static task_par_t aktar = {};
                aktar.command = (event_stat != 0) ? 1 : 2; // 1: Aç/Max, 2: Kapat
                aktar.id = ins.com_addr;
                aktar.level = 200;
                aktar.task = NULL;
                xTaskCreatePinnedToCore(anahtar_on_task, "instrig", 4096, &aktar, 2, NULL, 1);
            } break;
            case CM_SCENE:
                // Senaryonun "kapalı" karşılığı yok — sadece basma anında (stat==1) tetiklenir.
                if (event_stat != 0) {
                    uint8_t sceneHex = 0x10 + ins.com_addr;
                    for (uint8_t kanal = 1; kanal <= 3; kanal++) lamp_action(0xFF, 0, sceneHex, kanal, say);
                }
                break;
            default: break;
        }
        return;
    }

    if (toggleState) {
        uint8_t grp = (ins.com == CM_GROUP) ? 1 : 0;
        switch (ins.com) {
            case CM_LAMP:
            case CM_GROUP:
                if (ins.lamp_channel == 10) {
                    for (Base_Device* cihaz : cihaz_listesi) {
                        if (cihaz->device_id == ins.com_addr || ins.com_addr == 0xff) {
                            if (cihaz->get_status() != 0) cihaz->off(say); else cihaz->set_power(254, say);
                        }
                    }
                } else {
                    static toggle_query_par_t tparam = {};
                    tparam.adr = ins.com_addr;
                    tparam.kanal = ins.lamp_channel;
                    tparam.grp = grp;
                    tparam.say = say;
                    xTaskCreatePinnedToCore(dali_toggle_query_task, "togglequery", 4096, &tparam, 1, NULL, 1);
                }
                break;
            case CM_SWITCH: {
                static task_par_t aktar = {};
                aktar.command = 4; // Toggle (anahtar_onaction cmd tablosu)
                aktar.id = ins.com_addr;
                aktar.level = 200;
                aktar.task = NULL;
                xTaskCreatePinnedToCore(anahtar_on_task, "instrig", 4096, &aktar, 2, NULL, 1);
            } break;
            case CM_SCENE: {
                uint8_t sceneHex = 0x10 + ins.com_addr;
                for (uint8_t kanal = 1; kanal <= 3; kanal++) lamp_action(0xFF, 0, sceneHex, kanal, say);
            } break;
            default: break;
        }
        return;
    }

    bool isToggle = (ins.process == PR_TOGGLE || ins.process == PR_TOGMAX);
    bool isSensorDriven = (ins.process == PR_ONOFF || ins.process == PR_MXMN);

    switch (ins.com) {
        case CM_LAMP:
        case CM_GROUP: {
            uint8_t grp = (ins.com == CM_GROUP) ? 1 : 0;

            if (isToggle) {
                if (ins.lamp_channel == 10) {
                    for (Base_Device* cihaz : cihaz_listesi) {
                        if (cihaz->device_id == ins.com_addr || ins.com_addr == 0xff) {
                            if (cihaz->get_status() != 0) cihaz->off(say); else cihaz->set_power(254, say);
                        }
                    }
                } else {
                    // DALI'de gerçek toggle: STM'nin 0xEE özel komutu DALI-2 standardının bir
                    // parçası değil ve güvenilirliği doğrulanamadı — bunun yerine SWITCH'te
                    // kullandığımız aynı sorgula+tersine-çevir mekanizması (standart
                    // QUERY_STATUS) kullanılıyor.
                    static toggle_query_par_t tparam = {};
                    tparam.adr = ins.com_addr;
                    tparam.kanal = ins.lamp_channel;
                    tparam.grp = grp;
                    tparam.say = say;
                    xTaskCreatePinnedToCore(dali_toggle_query_task, "togglequery", 4096, &tparam, 1, NULL, 1);
                }
            } else if (isSensorDriven) {
                uint8_t offHex = (ins.process == PR_MXMN) ? 0x06 : 0x00; // MIN ya da OFF
                uint8_t hexcom = (event_stat != 0) ? 0x05 : offHex; // MAX ya da yukarıdaki
                lamp_action(ins.com_addr, grp, hexcom, ins.lamp_channel, say);
            } else if (ins.process == PR_OFF) {
                lamp_action(ins.com_addr, grp, 0x00, ins.lamp_channel, say);
            } else if (ins.process == PR_ARCPOWER) {
                // Doğrudan seviye değeri instance_t'de tutulmuyor — henüz desteklenmiyor.
            } else {
                lamp_action(ins.com_addr, grp, process_to_hexcom(ins.process), ins.lamp_channel, say);
            }
        } break;

        case CM_SCENE: {
            // SENARYO: DALI kanal 1-3'e "goto scene" broadcast — voice.cpp'deki senaryo
            // tetiklemesiyle aynı yol (adr=0xFF, grp=0, hexcom=GOTO_SCENE+senaryo_no).
            uint8_t hexcom = 0x10 + ins.com_addr;
            for (uint8_t kanal = 1; kanal <= 3; kanal++) {
                lamp_action(0xFF, 0, hexcom, kanal, say);
            }
        } break;

        case CM_SWITCH: {
            // Virtual anahtar fan-out — anahtar_tool.cpp'deki an_on/an_off mekanizmasının
            // ta kendisi (ağ cevabı beklemeden, doğrudan gear taramasını tetikliyoruz).
            uint8_t cmd;
            if (isToggle) cmd = 4;
            else if (ins.process == PR_OFF) cmd = 2;
            else cmd = 1;

            static task_par_t aktar = {};
            aktar.command = cmd;
            aktar.id = ins.com_addr;
            aktar.level = 200;
            aktar.task = NULL;
            xTaskCreatePinnedToCore(anahtar_on_task, "instrig", 4096, &aktar, 2, NULL, 1);
        } break;

        default: break;
    }
}

// Hareket sensörü "retrigger" timer'ı: DALI ya da yerel fark etmeksizin, stat==1 geldiğinde
// hedef hemen açılır ve bir timer başlar; tekrar stat==1 gelirse timer sıfırlanır (ışık açık
// kalmaya devam eder); timer süresi dolunca hedef kapatılır. Süre şimdilik sabit — kullanıcı
// bazında ayarlanabilir hale getirmek ayrı bir adım.
#define SENSOR_TIMER_SEC 10

typedef struct {
    uint32_t key; // (channel<<16)|(dev_addr<<8)|ins_addr — sensörün kimliği
    instance_t ins; // tetiklendiği andaki hedef bilgisiyle (com/com_addr/lamp_channel/process)
    esp_timer_handle_t timer;
} sensor_timer_t;

static std::vector<sensor_timer_t*> sensor_timers;

static void sensor_timer_off_cb(void *arg)
{
    sensor_timer_t *st = (sensor_timer_t*)arg;
    printf("SENSOR_TIMER Kanal:%d Adr:%d:%d SURE DOLDU -> KAPATILIYOR\n",
           st->ins.channel, st->ins.dev_addr, st->ins.ins_addr);
    trigger_instance(st->ins, 0, true, true); // followState: hedefi kapat
}

void sensor_timer_trigger(instance_t &ins)
{
    uint32_t key = ((uint32_t)ins.channel << 16) | ((uint32_t)ins.dev_addr << 8) | ins.ins_addr;

    sensor_timer_t *st = nullptr;
    for (sensor_timer_t *t : sensor_timers) { if (t->key == key) { st = t; break; } }

    if (st == nullptr) {
        st = new sensor_timer_t();
        st->key = key;
        st->timer = nullptr;
        esp_timer_create_args_t args = {};
        args.callback = &sensor_timer_off_cb;
        args.arg = st;
        args.name = "sensortmr";
        esp_timer_create(&args, &st->timer);
        sensor_timers.push_back(st);
    }
    st->ins = ins; // güncel hedef bilgisiyle (uygulamadan yeni set edilmiş olabilir) tazele

    // Süre app'ten (tset, DAKİKA) geliyor; hiç ayarlanmamışsa (0) sabit varsayılana (saniye) düş.
    uint32_t seconds = (ins.temp_set > 0) ? (uint32_t)ins.temp_set * 60 : SENSOR_TIMER_SEC;

    bool wasActive = esp_timer_is_active(st->timer);
    if (wasActive) esp_timer_stop(st->timer);
    esp_timer_start_once(st->timer, (uint64_t)seconds * 1000000ULL);
    printf("SENSOR_TIMER Kanal:%d Adr:%d:%d %s (%lu sn)\n",
           ins.channel, ins.dev_addr, ins.ins_addr,
           wasActive ? "SIFIRLANDI" : "BASLADI", (unsigned long)seconds);

    trigger_instance(ins, 1, true, true); // followState: hedefi hemen aç
}

// DALI "event" handler'ından (ix==1 tus bayrağı && ins.type==0x01 BUTTON) tetiklenir.
// param->ev DALI2 standardı: 0=bırakıldı, 1=basıldı, 9=Long Press Start, 11=Long Press Repeat
// (basılı tutulduğu sürece tekrarlanır), 12=Long Press Stop. Kullanıcının app'ten (Aksiyon
// dropdown, ins_anahtar.dart) atadığı process'e göre davranır — yerelde kurulan ANAHTAR/SWITCH
// ayrımı (filter alanının yeniden kullanımı) burada YOK, DALI'nin filter'ı kendi gerçek işine
// (instance event filtresi) devam ediyor.
void event_process_task(void *pvParameters)
{
   event_proc_par_t param = {};
   memcpy(&param, pvParameters, sizeof(event_proc_par_t));

   instance_t ins = {};
   esp_err_t kk = pick_instance(param.channel).get_instance(param.channel, param.adr, param.ins, &ins);
   if (kk == ESP_OK) {
       bool isSensorDriven = (ins.process == PR_ONOFF || ins.process == PR_MXMN);
       bool isDim = (ins.process == PR_UPDIM || ins.process == PR_DOWNDIM);
       if (ins.process == PR_TOGDIM && (ins.com == CM_LAMP || ins.com == CM_GROUP)) {
           // Yön (ins.status: 1=AŞAĞI, başka her değer -taze kayıtta 0xFF dahil- YUKARI) basış
           // boyunca sabit kalır, sadece Start/Repeat'te okunur; Stop'ta bir sonraki basış için
           // ters çevrilip kalıcı olarak saklanır.
           if (param.ev == 9 || param.ev == 11) {
               uint8_t grp = (ins.com == CM_GROUP) ? 1 : 0;
               uint8_t hexcom = (ins.status == 1) ? 0x02 : 0x01; // AŞAĞI : YUKARI
               lamp_action(ins.com_addr, grp, hexcom, ins.lamp_channel, true);
           } else if (param.ev == 12) {
               ins.status = (ins.status == 1) ? 0 : 1;
               pick_instance(param.channel).set_instance(param.channel, param.adr, param.ins, &ins);
           }
       } else if (isDim) {
           // Basılı tutma boyunca (Start=9, Repeat=11) kademeli tetiklenir; kısa basış (0/1)
           // ve Long Press Stop (12) için bir şey yapılmaz — DALI ballast'ı UP/DOWN komutunu
           // aldıkça kendi kendine rampalıyor.
           if (param.ev == 9 || param.ev == 11) {
               trigger_instance(ins, 1);
           }
       } else if (isSensorDriven) {
           trigger_instance(ins, param.ev); // her iki kenarda da (Event 0 ve 1), yön event'e göre
       } else if (param.ev == 1) {
           trigger_instance(ins, 1); // sadece basmada (Event=1)
       }
   }

   vTaskDelete(NULL);
}



void process_and_control_task(void *pvParameters)
{
   instance_t *param = (instance_t *)pvParameters;
   static instance_t cpins = {};
   memcpy(&cpins, param, sizeof(instance_t));

   if (cpins.lamp_channel==10) {
       // Yerel hedef: cihaz_listesi çağrısı senkron ve kesin, DALI tarzı
       // sorgu+retry ile durum doğrulamaya hiç gerek yok. say=false: termostatın
       // otomatik aç/kapa döngüsü her seferinde sesli anons tetiklemesin.
       lamp_action(cpins.com_addr, 0, (cpins.status==1) ? 0x05 : 0x00, cpins.lamp_channel, false);
   } else {
       bool don = true;
       uint8_t count=0, stat=1;

       // Emir+doğrulama döngüsünün tamamı boyunca (birkaç send/receive turu)
       // searchQueue'yu tek başımıza kullanıyoruz.
       xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
       do {
            count++;
            if (cpins.status==1) {
                    //En yüksekte AÇ (Max Level)
                    lamp_action(cpins.com_addr,0,0x05,cpins.lamp_channel);
                } else {
                    //Kapat
                    lamp_action(cpins.com_addr,0,0x00,cpins.lamp_channel);
                }

            vTaskDelay(500/portTICK_PERIOD_MS);
            //process gönderildi durum sorgulanacak

            cJSON *pay = cJSON_CreateObject();
            cJSON_AddStringToObject(pay, "com", "query");
            cJSON_AddNumberToObject(pay, "hexcom", create_command(cpins.com_addr,false,false,QUERY_STATUS));
            cJSON_AddNumberToObject(pay, "kanal",cpins.lamp_channel);
            cJSON_AddNumberToObject(pay, "bit",16);
            send_STM(pay);
            cJSON_Delete(pay);

            //Açma veya kapatma emrinden sonra cihazın statusunda lampon parametresi 1 veya 0 olmalı
            searchMessage_t msg = {};
            xQueueReset(searchQueue);
            xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(3000));
            if (count>3) {don=false;stat=0;}
            //Eger
            if (cpins.status==1) {if ((msg.name[0]&0x04)==0x04) don=false;}
            if (cpins.status==0) {if ((msg.name[0]&0x04)!=0x04) don=false;}
       } while (don);
       xSemaphoreGive(searchQueueMutex);

       if (stat==0) {
          cpins.status=0;
          pick_instance(cpins.channel).set_instance(cpins.channel,cpins.dev_addr,cpins.ins_addr,&cpins);
       }
   }

    cJSON *root = cJSON_CreateObject();                      
    cJSON_AddStringToObject(root, "com", "push_temp");
    cJSON_AddNumberToObject(root, "adres", (cpins.dev_addr)); 
    cJSON_AddNumberToObject(root, "ins", (cpins.ins_addr)); 
    cJSON_AddNumberToObject(root, "temp", cpins.temp);
    cJSON_AddNumberToObject(root, "relay", cpins.status);
    cJSON_AddNumberToObject(root, "set", cpins.temp_set);
    cJSON_AddNumberToObject(root, "error", 0);
    cJSON_AddNumberToObject(root, "type", cpins.temp_type);
    char *dat = cJSON_PrintUnformatted(root);
    if (cpins.temp>0) {
        udp_server.send_broadcast((uint8_t *)dat,strlen(dat));
        udp_server.send_unicast_all((uint8_t *)dat,strlen(dat));
        mqtt_send(dat);
    }
    cJSON_free(dat);
    cJSON_Delete(root);

   vTaskDelete(NULL);
}   

void temp_role_degerlendir(instance_t *ins) {

    if (ins->temp_type==0) {
        //ISITMA
       if (ins->temp>=ins->temp_set) ins->status=0; else ins->status=1;
    } else if (ins->temp_type==1) {
        //SOGUTMA
       if (ins->temp<=ins->temp_set) ins->status=1; else ins->status=0; 
    } else if (ins->temp_type==2) {
       //MANUEL ON 
       //33 dereceye kadar durmadan ısıtır
       if (ins->temp>=33) ins->status=0; else ins->status=1;
    } else {
       //MANUEL OFF
       ins->status=0;
    }

    xTaskCreatePinnedToCore(
                        process_and_control_task,           // Task fonksiyonunun adı
                        "pevptask",          // Task'ın ismi (debug için)
                        4096,               // Stack büyüklüğü artırıldı
                        ins,               // Task'a kopyalanan parametre gönderiliyor
                        1,                  // Öncelik sırası (0 en düşük)
                        NULL,               // Task handle (referans gerekmiyorsa NULL)
                        1                   // Çalışacağı çekirdek (0 veya 1)
                    );
  
   //Burada kesin rölenin çekmesi veya bırakması saglanacak
}

void Alarm_Activeted(const char *txt, uint8_t source)
{
    GlobalConfig.alarm_aktif=1;
    disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
    cJSON *root = cJSON_CreateObject();                      
    cJSON_AddStringToObject(root, "com", "alarm");
    cJSON_AddNumberToObject(root, "status", 1);
    cJSON_AddStringToObject(root, "txt", txt);
    char *dat = cJSON_PrintUnformatted(root);
    udp_server.send_broadcast((uint8_t *)dat,strlen(dat));
    udp_server.send_unicast_all((uint8_t *)dat,strlen(dat));
    mqtt_send(dat);
    cJSON_free(dat);
    cJSON_Delete(root);

    xTaskCreatePinnedToCore(
        firebase_notification_task,           // Task fonksiyonunun adı
        "fntask",          // Task'ın ismi (debug için)
        8192,               // Stack büyüklüğü artırıldı
        (void *)txt,               // Task'a kopyalanan parametre gönderiliyor
        1,                  // Öncelik sırası (0 en düşük)
        NULL,               // Task handle (referans gerekmiyorsa NULL)
        1                   // Çalışacağı çekirdek (0 veya 1)
    );   

    cJSON *pay2 = cJSON_CreateObject(); 
    uint8_t evid = find_event_id("ALARM_STATE_EVENT"); 
    cJSON_AddStringToObject(pay2, "com", "EVENTS");
    cJSON_AddStringToObject(pay2, "event", "ALARM_STATE_EVENT");
    cJSON_AddNumberToObject(pay2, "event_id", evid);
    cJSON_AddNumberToObject(pay2, "source", source);
    cJSON_AddNumberToObject(pay2, "state", 1);
    send_events(pay2);
    cJSON_Delete(pay2);

}


void uartCallback(const uint8_t *data, size_t len)
{
    uint8_t *buf = (uint8_t *)calloc(1,len+1);
    memcpy(buf,data,len);
    buf[len] = '\0';

    buf[strcspn((char*)buf, "\r\n")] = '\0';

    printf("UART GELEN << %s\n",buf);
    
    cJSON *rcv_json = cJSON_Parse((const char *)buf);
    if (rcv_json!=nullptr) {
        char *command = (char *)calloc(1,25);    
        JSON_getstring(rcv_json,"com", command,25);
        //printf("Command : %s\n",command);

        if (strcmp(command, "stm_status") == 0) {
            stm_send_status = true;
        }

        if (strcmp(command, "stm_boot") == 0) {
            // STM32 (yeniden) başladı: kanal durumlarını tekrar bildirmek için
            // led_task'ın bir sonraki turunda stm_status'u yeniden göndert.
            stm_send_status = false;
        }

        if (strcmp(command, "tus") == 0) {
            int pin = -1, stat = -1;
            if (cJSON_HasObjectItem(rcv_json, "pin")) {
                pin = cJSON_GetObjectItem(rcv_json, "pin")->valueint;
            }
            if (cJSON_HasObjectItem(rcv_json, "stat")) {
                stat = cJSON_GetObjectItem(rcv_json, "stat")->valueint;
            }
            if (pin != -1 && stat != -1) {
                if (GlobalConfig.alarm==2 && GlobalConfig.alarm_aktif==0) Alarm_Activeted("ELEKTRIKSEL FAALİYET ALGILANDI",4);

                instance_t ins = {};
                bool isManaged = (instanceL.get_instance(10, (uint8_t)pin, 0, &ins) == ESP_OK && ins.ins_active == 1);

                if (isManaged) {
                    // Bu pin boşta (device.json'da bir cihaza doğrudan bağlı değil) — hedefini
                    // instanceL'deki kayda göre tetikle, eski button_process yoluna hiç girme.
                    // Tip (ve BUTTON içinde filter alt-tipi) tek tek ele alınıyor:
                    // - ANAHTAR (BUTTON, filter=0, momentary tuş): doğrudan durumu takip eder —
                    //   basılıyken (stat=1) yanar, bırakılınca (stat=0) söner.
                    // - SWITCH (BUTTON, filter=1, maintained anahtar): her stat==1'de hedefin
                    //   durumu tersine çevrilir (gerçek toggle, stat==0'da hiçbir şey olmaz).
                    // - Hareket (MOTION): stat 0 ya da 1 fark etmeksizin (bu pin her basışta
                    //   tek bir tus mesajı gönderiyor, stat değeri basma/bırakma anlamına
                    //   gelmiyor) hedef açılır ve bir "retrigger" timer'ı başlar/sıfırlanır;
                    //   kapatma kararını stat değil, timer verir.
                    // - Diğerleri: sadece basma anında (stat==1) tetiklenir.
                    if (ins.type == INSTANCE_TYPE_BUTTON && ins.filter == 1) {
                        if (stat == 1) trigger_instance(ins, 1, true, false, true);
                    } else if (ins.type == INSTANCE_TYPE_BUTTON) {
                        trigger_instance(ins, (uint8_t)stat, true, true);
                    } else if (ins.type == INSTANCE_TYPE_MOTION) {
                        sensor_timer_trigger(ins);
                    } else if (stat == 1) {
                        trigger_instance(ins, 1);
                    }
                } else {
                    // Pasif (bir cihaza zaten doğrudan bağlı) ya da hiç kayıtlı değil — eski akış.
                    for (Base_Device* cihaz : cihaz_listesi) {
                        cihaz->button_process(pin,stat);
                    }
                }
            }

        }

        // ReliableUartManager için ACK işleme
        if (strcmp(command, "role") == 0) { // STM32'den gelen onay mesajının "role_resp" olduğu varsayılıyor
            int pin = -1, stat = -1;
            if (cJSON_HasObjectItem(rcv_json, "pin")) {
                pin = cJSON_GetObjectItem(rcv_json, "pin")->valueint;
            }
            if (cJSON_HasObjectItem(rcv_json, "stat")) {
                stat = cJSON_GetObjectItem(rcv_json, "stat")->valueint;
            }
            if (pin != -1 && stat != -1) {
                if (GlobalConfig.alarm==2 && GlobalConfig.alarm_aktif==0) Alarm_Activeted("ELEKTRIKSEL FAALİYET ALGILANDI", 4);
                reliableUartManager.process_response(pin, stat);
            }
        }

        if (strcmp(command,"alarm")==0) {
            uint8_t status=0, sourge=0, aktif = 0;
            char *txt;
            JSON_getint(rcv_json,"status", &status);
            JSON_getint(rcv_json,"sourge",&sourge);
            if (status==1) {
                if (sourge==1) {asprintf(&txt,"YANGIN ALARMI"); aktif=1;}
                if (sourge==2) {asprintf(&txt,"SU BASKINI ALARMI"); aktif=1;}
                if (sourge==3) {asprintf(&txt,"GAZ ALARMI"); aktif=1;}

                if (GlobalConfig.alarm==2) {
                   if (sourge==4) {asprintf(&txt,"ÇEVRESEL HAREKET ALGILANDI"); aktif=1;}
                }
            if (aktif==1) {
                Alarm_Activeted(txt, sourge);
                vTaskDelay(200/portTICK_PERIOD_MS);
                free(txt);                
              }
            }
        }

        if (strcmp(command,"log")==0) {
            //udp_server.send_broadcast((uint8_t *)buf,strlen((char*)buf));
            udp_server.send_unicast_all((uint8_t *)buf,strlen((char*)buf));
            //mqtt_send((char*)buf);
        }

        if (strcmp(command,"hat_error")==0) {
            // STM32'den gelen DALI hat arızası (kısa devre/kopukluk) bildirimi.
            // Yaşamsal alarm değil, donanım arıza telemetrisi: ham mesajı olduğu gibi
            // yerel ağdaki uygulamaya ve buluta ilet.
            udp_server.send_unicast_all((uint8_t *)buf,strlen((char*)buf));
            mqtt_send((char*)buf);

            // data: bit0=DALI kanal1, bit1=kanal2, bit2=kanal3 (main.c: hat_error |= 1<<(chn-1))
            uint8_t hat_mask = 0;
            JSON_getint(rcv_json,"data",&hat_mask);
            for (uint8_t ch = 0; ch < 3; ch++) {
                if (hat_mask & (1 << ch)) {
                    ESP_LOGE(TAG, "DALI hat arizasi: kanal %d", ch + 1);
                }
            }
        }

        if (strcmp(command,"event")==0) {
            uint32_t data=0;
            JSON_getlong(rcv_json,"data", &data);
            //printf("EVENT : %6lX\n",data);
            EventDataUnion_t ev;
            ev.value = data;

            uint8_t ix = (ev.alan.instance & 0b00100000)>>5; //1 ise tus eventtir
            if (ix==1) ev.alan.instance = ev.alan.instance & 0b00011111;

            
            uint8_t event_kanal = 2; // fallback: eski STM firmware "kanal" göndermeyebilir
            JSON_getint(rcv_json,"kanal", &event_kanal);
            instance_t ins = {};
            esp_err_t kk= instance.get_instance(event_kanal, ev.alan.addr>>1,ev.alan.instance, &ins);
            printf("Event Adr:%d Ins:%d Event:%d Tus:%d Type:%d\n",ev.alan.addr>>1,ev.alan.instance,ev.alan.event,ix,ins.type);

            //printf("%d:%d %d %s\n",ev.alan.addr>>1,ev.alan.instance,kk,esp_err_to_name(kk));

            if (kk==ESP_OK)
            {
                if (ix==1 && ins.type==0x01) {
                   //Anahtar instance Buna göre işlem yapılacak

                   if (GlobalConfig.alarm==2) {
                        GlobalConfig.alarm_aktif=1;
                        disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
                        cJSON *root = cJSON_CreateObject();                      
                        cJSON_AddStringToObject(root, "com", "alarm");
                        cJSON_AddNumberToObject(root, "status", 1);
                        cJSON_AddStringToObject(root, "txt", "Hırsızlık Alarmı");
                        char *dat = cJSON_PrintUnformatted(root);
                        udp_server.send_broadcast((uint8_t *)dat,strlen(dat));
                        udp_server.send_unicast_all((uint8_t *)dat,strlen(dat));
                        mqtt_send(dat);
                        cJSON_free(dat);
                        cJSON_Delete(root);
                   }

                   static event_proc_par_t param = {};
                   param.channel = event_kanal;
                   param.adr = ins.dev_addr;
                   param.ins = ins.ins_addr;
                   param.ev = ev.alan.event;
                   
                   xTaskCreatePinnedToCore(
                        event_process_task,           // Task fonksiyonunun adı
                        "evptask",          // Task'ın ismi (debug için)
                        4096,               // Stack büyüklüğü artırıldı
                        &param,               // Task'a kopyalanan parametre gönderiliyor
                        1,                  // Öncelik sırası (0 en düşük)
                        NULL,               // Task handle (referans gerekmiyorsa NULL)
                        1                   // Çalışacağı çekirdek (0 veya 1)
                    );
                }
                if (ins.type==0x06) {
                    if (ev.alan.event<0x200) {
                        //Isı Sensörü Gelen data ısı datasıdır. 
                        //Sete bakarak röleyi hareket ettir. 
                        //Broadcast olarak Isı datası gönder
                        if (ev.alan.event<2) return;
                        ins.temp = ev.alan.event;
                        temp_role_degerlendir(&ins);
                        instance.set_instance(event_kanal, ev.alan.addr>>1,ev.alan.instance,&ins);
                        
                        cJSON *root = cJSON_CreateObject();                      
                        cJSON_AddStringToObject(root, "com", "push_temp");
                        cJSON_AddNumberToObject(root, "adres", (ev.alan.addr>>1)); 
                        cJSON_AddNumberToObject(root, "ins", (ev.alan.instance)); 
                        cJSON_AddNumberToObject(root, "temp", ins.temp);
                        cJSON_AddNumberToObject(root, "relay", ins.status);
                        cJSON_AddNumberToObject(root, "set", ins.temp_set);
                        cJSON_AddNumberToObject(root, "error", 0);
                        cJSON_AddNumberToObject(root, "type", ins.temp_type);
                        char *dat = cJSON_PrintUnformatted(root);
                        //ESP_LOGI("SEND_BR","UDP GIDEN >> %s",dat);
                        if (ins.temp>0) {
                            udp_server.send_broadcast((uint8_t *)dat,strlen(dat));
                            udp_server.send_unicast_all((uint8_t *)dat,strlen(dat));
                            mqtt_send(dat);
                        }
                        cJSON_free(dat);
                        cJSON_Delete(root);
                        return ;
                    }
                    if (ev.alan.event<0x204) {
                        //Sensör arızası
                        cJSON *root = cJSON_CreateObject();                      
                        cJSON_AddStringToObject(root, "com", "push_temp");
                        cJSON_AddNumberToObject(root, "adres", (ev.alan.addr>>1)); 
                        cJSON_AddNumberToObject(root, "ins", (ev.alan.instance)); 
                        cJSON_AddNumberToObject(root, "temp", 0);
                        cJSON_AddNumberToObject(root, "relay", 0);
                        cJSON_AddNumberToObject(root, "set", 0);
                        cJSON_AddNumberToObject(root, "error", 1);
                        char *dat = cJSON_PrintUnformatted(root);
                        ESP_LOGI("SEND_BR","UDP GIDEN >> %s",dat);
                        udp_server.send_broadcast((uint8_t *)dat,strlen(dat));
                        udp_server.send_unicast_all((uint8_t *)dat,strlen(dat));
                        mqtt_send(dat);
                        cJSON_free(dat);
                        cJSON_Delete(root);
                    }                   
                    if (ev.alan.event<0x208) {
                        //sensör resetlenmiş. 
                        cJSON *root = cJSON_CreateObject();                      
                        cJSON_AddStringToObject(root, "com", "push_temp");
                        cJSON_AddNumberToObject(root, "adres", (ev.alan.addr>>1)); 
                        cJSON_AddNumberToObject(root, "ins", (ev.alan.instance)); 
                        cJSON_AddNumberToObject(root, "temp", ev.alan.event);
                        cJSON_AddNumberToObject(root, "relay", ins.status);
                        cJSON_AddNumberToObject(root, "set", ins.temp_set);
                        cJSON_AddNumberToObject(root, "error", 2);
                        char *dat = cJSON_PrintUnformatted(root);
                        ESP_LOGI("SEND_BR","UDP GIDEN >> %s",dat);
                        udp_server.send_broadcast((uint8_t *)dat,strlen(dat));
                        udp_server.send_unicast_all((uint8_t *)dat,strlen(dat));
                        mqtt_send(dat);
                        cJSON_free(dat);
                        cJSON_Delete(root);
                    }
                }
            }

        }

        if (strcmp(command,"response")==0) {
            ESP_LOGI("UART","%s",buf);
            searchMessage_t msg = {};
            uint8_t a0=0;
            if (JSON_getint(rcv_json,"A0", &a0)) msg.name[0] = a0;
            if (JSON_getint(rcv_json,"A1", &a0)) msg.name[1] = a0;
            if (JSON_getint(rcv_json,"A2", &a0)) msg.name[2] = a0;
            if (JSON_getint(rcv_json,"A3", &a0)) msg.name[3] = a0;
            if (JSON_getint(rcv_json,"A4", &a0)) msg.name[4] = a0;
            if (JSON_getint(rcv_json,"A5", &a0)) msg.name[5] = a0;
            if (JSON_getint(rcv_json,"A6", &a0)) msg.name[6] = a0;  
            xQueueSend(searchQueue, (void *)&msg, pdMS_TO_TICKS(10));
        }

        if (strcmp(command,"search")==0) {
           char *stat = (char *)calloc(1,25);    
           JSON_getstring(rcv_json,"stat", stat,25);
           if (strcmp(stat,"start")==0) {
               xEventGroupSetBits(searchGROUP, BIT_START);
           }
           if (strcmp(stat,"stop")==0) {
               printf("stop serbest bırakılıyor\n");
               xEventGroupSetBits(searchGROUP, BIT_STOP);
           }
           free(stat);
        }
        if (strcmp(command,"instance")==0) {
            ESP_LOGI("UART","%s",buf);
            uint8_t adr =255, id=255, type=255, chn=255, act=0x00;
            JSON_getint(rcv_json,"adr",&adr);
            JSON_getint(rcv_json,"id",&id);
            JSON_getint(rcv_json,"type",&type);
            JSON_getint(rcv_json,"chn",&chn);
            JSON_getint(rcv_json,"act",&act);
            //Bilgileri diske kaydet
            instance_t ins = {};
            instance.bosalt(&ins);
            ins.dev_addr = adr;
            ins.ins_addr = id;
            ins.channel = chn;
            ins.type = type;
            ins.ins_active = act;
            ins.status = 0;
            instance.add(&ins);
           // printf("   INS ADR : %d ID : %d TYPE : %d CHN:%d\n",adr, id,type,chn);
        }

        if (strcmp(command,"device")==0) {
            ESP_LOGI("UART","%s",buf);

            uint32_t rnd = 0;
            uint8_t adr =255, type=255, exttype=255, ins=255, chn=255;
            JSON_getlong(rcv_json,"rnd",&rnd);
            JSON_getint(rcv_json,"adr",&adr);
            JSON_getint(rcv_json,"type",&type);
            JSON_getint(rcv_json,"ext",&exttype);
            JSON_getint(rcv_json,"ins",&ins);
            JSON_getint(rcv_json,"chn",&chn);
            //Bilgileri diske kaydet
            searchMessage_t msg = {};
            gear_t gr = {};
            gr.aktif = 1;
            gr.short_addr = adr;
            gr.random_addr = rnd;
            gr.type = type;
            gr.ext_type = exttype;
            gr.kanal = (dev_sourge_t)chn;
            gr.instance = ins;
            gr.ico = 0;
            gr.room = 0xFF;   
            gr.anahtar=0xff;        
            gr.spec.status = 0;
            gr.spec.group[0] = 0;
            gr.spec.group[1] = 0;

            printf("ADR : %d TYPE : %d EXT : %d\n",adr, type,exttype);

            char *txt;
            char *harf;
            if (chn==1) asprintf(&harf,"A%02d",gr.short_addr);
            if (chn==2) asprintf(&harf,"B%02d",gr.short_addr);
            if (chn==3) asprintf(&harf,"C%02d",gr.short_addr);
            uint8_t fnd = 0;
            if (type==0x06) {asprintf(&txt,"Lamba%s",harf);fnd=1;} 
            if (type==0x08 ) {asprintf(&txt,"Rgb%s",harf); fnd=1;}
            if (type==0x07 && exttype==0x07) {asprintf(&txt,"Role%s",harf);fnd=1;} 
            if (type==0x07 && exttype==0x0B) {asprintf(&txt,"Anahtar%s",harf); fnd=1;}
            if (type==0x07 && exttype==0x77) {asprintf(&txt,"Perde%s",harf); fnd=1;}
            //else asprintf(&txt,"Btn%s",harf); 

            if (fnd==0) asprintf(&txt,"Bilinmeyen%s",harf);

            strcpy((char*)gr.name,txt);
            strcpy((char*)msg.name,txt);
            free(harf);
            free(txt);

            if (chn==1) gear01.update_gear(adr,&gr);
            if (chn==2) gear02.update_gear(adr,&gr);
            if (chn==3) gear03.update_gear(adr,&gr);

            
            msg.addr = adr;
            msg.type = type;
            msg.exttype = exttype;            
            xEventGroupSetBits(searchGROUP, BIT_CONTINUE);
            xQueueSend(searchQueue, (void *)&msg, pdMS_TO_TICKS(10));
        }
        free(command);
        cJSON_Delete(rcv_json);
    }
    
    
   // ESP_LOGI("UART","%s",buf);
    free(buf);
}

// Yeni görev: ReliableUartManager'ın zaman aşımı kontrollerini periyodik olarak çalıştırır
void reliable_uart_monitor_task(void *pvParameters) {
    UartDma* uart_instance = static_cast<UartDma*>(pvParameters);
    for (;;) {
        reliableUartManager.check_timeouts(uart_instance);
        vTaskDelay(pdMS_TO_TICKS(100)); // Her 100ms'de bir kontrol et
    }
}



void term_tara_task(void *pvParameters) 
{
    //Termostatları TARA
    vTaskDelay(10000/portTICK_PERIOD_MS);

    instance_t ff={};
    Instance* stores[2] = {&instance, &instanceL}; // DALI + yerel termostat kayıtları
    for (Instance* store : stores)
    for (int i=0;i<MAX_INSTANCE;i++)
    {
        store->get_instance_at(i,&ff);
        if (ff.dev_addr!=0xFF) {
            if (ff.type==0x06 && ff.ins_active>0) {
                //Bu Aktif bir termostattır
                //Once ısı okumasını isteyecegiz sonra da degerlendirmesi yapılacak
                printf("Term %d:%d\n",ff.dev_addr,ff.ins_addr);
             
                //Isısını istedik
                uint32_t cmm = (((ff.dev_addr<<1)|1) << 16)| (ff.ins_addr<<8) | (0xBC) ; 
                char *mm;
                asprintf(&mm,"%06lX:18:01:%02X#",cmm,ff.channel);
                xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
                myUart->send(mm);
                free(mm);

                searchMessage_t msg = {};
                xQueueReset(searchQueue);
                xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(5000));
                xSemaphoreGive(searchQueueMutex);
                ff.temp = msg.name[0];

                xTaskCreatePinnedToCore(
                        process_and_control_task,           // Task fonksiyonunun adı
                        "pevptask",          // Task'ın ismi (debug için)
                        4096,               // Stack büyüklüğü artırıldı
                        &ff,               // Task'a kopyalanan parametre gönderiliyor
                        1,                  // Öncelik sırası (0 en düşük)
                        NULL,               // Task handle (referans gerekmiyorsa NULL)
                        1                   // Çalışacağı çekirdek (0 veya 1)
                    );

               
            }
        }
    }
    vTaskDelete(NULL);
}

void led_task(void *pvParameters) 
{
   bool cnt=0;
   if (Network_Status!=ESP_OK || Active_Network_connection==3) {
       //Eğer ağ kurulamadıysa LED hızlı yanıp sönsün
       while(1) {
          gpio_set_level(LED, cnt);
          cnt=!cnt;
          vTaskDelay(100/portTICK_PERIOD_MS);
       }
   }

    //Ağ kurulduysa LED yavaş yanıp sönecek
   
        uint8_t ping_counter=0; 
        while(1) {
            gpio_set_level(LED,0);
            

            if (GlobalConfig.ping_active==1)
            {
                    uint8_t wcounter=0;
                    while(1) {
                        gpio_set_level(STM_PING, 0);
                        vTaskDelay(2/portTICK_PERIOD_MS);
                        if (gpio_get_level(STM_PONG)==0) {
                            ping_counter=0;
                            break;
                        }
                        gpio_set_level(STM_PING, 1);
                        vTaskDelay(2/portTICK_PERIOD_MS);
                        if (++wcounter>2) {
                            ping_counter++;
                            break;
                        }
                    }
                    if (ping_counter>4) {
                                //STM RESET
                                //Resetlemek için STM_RESET pini 1 yapılacak
                                ping_counter=0;
                                gpio_set_level(STM_RESET, 1);
                                vTaskDelay(10/portTICK_PERIOD_MS);
                                gpio_set_level(STM_RESET, 0);
                                ESP_LOGE(TAG,"STM Cevap vermiyor RESETLENDI");
                    }
      }

      if (stm_send_status==false) {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "com", "stm_status");
            cJSON_AddNumberToObject(root, "ping", GlobalConfig.ping_active);
            cJSON_AddNumberToObject(root, "kanal1", GlobalConfig.kanal1);
            cJSON_AddNumberToObject(root, "kanal2", GlobalConfig.kanal2);
            cJSON_AddNumberToObject(root, "kanal3", GlobalConfig.kanal3);

            send_STM(root);
            ESP_LOGI(TAG,"STM ye durum bildirildi");
        }

      vTaskDelay(2800/portTICK_PERIOD_MS);
      gpio_set_level(LED,1);
      vTaskDelay(100/portTICK_PERIOD_MS);
    }
   
}


void initialise_mdns(void)
{
    // 1. mDNS Servisini Başlat
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mDNS başlatılamadı: %s", esp_err_to_name(err));
        return;
    }

    // 2. Cihazın Hostname'ini ayarla (Örn: akillihome.local)
    ESP_ERROR_CHECK(mdns_hostname_set("SMQMAIN"));
    ESP_LOGI(TAG, "mdns hostname ayarlandı: SMQMAIN.local");

    // 3. Cihazın varsayılan instance adını ayarla (İsteğe bağlı)
    ESP_ERROR_CHECK(mdns_instance_name_set("SmartQ Akilli Ev Kontrolcü"));

    // 4. Bir servis tanımla ve ağa duyur (Örn: Port 80 üzerinde çalışan bir HTTP web sunucusu)
    // Bu sayede Flutter uygulamaları veya mDNS tarayıcılar bu cihazı otomatik bulabilir.
    ESP_ERROR_CHECK(mdns_service_add("SMQMAIN_Web_Server", "_http", "_tcp", 80, NULL, 0));
    
    // İsteğe bağlı: Servise TXT kayıtları (metadata) ekleme
    mdns_txt_item_t serviceTxtData[2] = {
        {"board", "SMQMAIN"},
        {"version", "1.0.0"}
    };
    ESP_ERROR_CHECK(mdns_service_txt_set("_http", "_tcp", serviceTxtData, 2));
}

bool services_started = false;
void start_app_services() {
    if (services_started) return;
    if (Network_Status != ESP_OK) return;

    ESP_LOGI(TAG,"UDP Worker Start");
    // UDP İşleme Kuyruğu ve Taskı Oluşturuluyor
    udp_processing_queue = xQueueCreate(10, sizeof(udp_msg_t*));
    xTaskCreate(udp_worker_task, "udp_worker", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "UDP Server (%d) Start Ediliyor",0xD002);
    udp_server.start(0xD002, &on_udp_message_received);

    ESP_LOGI(TAG, "Network VAR..");
    ESP_LOGI(TAG, "OTA server (79) ve TFTP server (69) start ediliyor");
    ota_server.ota_start(false);
    disk_server.disk_start(false);

    initialise_mdns();

    strcpy((char*)GlobalConfig.version,desc->version);    

    if (GlobalConfig.http_start==1)
    {
        ESP_LOGI(TAG, "Web server (80) start ediliyor");
        httpd.init("/config", httpd_replace_func,httpd_handler,true,false);
        httpd.start();
    } 

    if (Active_Network_connection>0 && Active_Network_connection<3) mqtt_start();
    
    stop_flash(); // LED animasyonunu durdur
    services_started = true;
    ESP_LOGI(TAG, "Tum ag servisleri baslatildi.");
    if (GlobalConfig.alarm==0) ESP_LOGW(TAG,"ALARM Kapalı");
    if (GlobalConfig.alarm==2) ESP_LOGE(TAG,"ALARM KURULU...");

    if (Active_Network_connection>0 && Active_Network_connection<3) {
        query_dns("smartq.com.tr");
        xTaskCreatePinnedToCore(
                        firebase_register_task,           // Task fonksiyonunun adı
                        "fbtask",          // Task'ın ismi (debug için)
                         8192,               // Stack büyüklüğü artırıldı
                        NULL,               // Task'a kopyalanan parametre gönderiliyor
                        1,                  // Öncelik sırası (0 en düşük)
                        NULL,               // Task handle (referans gerekmiyorsa NULL)
                        1                   // Çalışacağı çekirdek (0 veya 1)
                    );       
    } 

    //Cihazlar ilklendiriliyor
    for (Base_Device* cihaz : cihaz_listesi) {
            cihaz->init();
    } 


}

extern "C" void app_main()
{
    ESP_LOGI(TAG,"Program Initializing...");
    esp_event_loop_create_default();
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGI(TAG, "NVS Flash Initialized.");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_LOGI(TAG, "ESP Netif Initialized.");

    file_initialize();
    if (NetworkConfig.mac_chg==1)
    {
        NetworkConfig.mac_chg = 0;
        disk.write_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig),0);
    }
    if (GlobalConfig.disk_format==1)
    {
        GlobalConfig.disk_format = 0;
        disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
    }


    start_network_manager(); 

    if (myUart->begin(STM_RX, STM_TX, 460800) == ESP_OK) {
        myUart->set_Callback(uartCallback);
        ESP_LOGI(TAG, "UART DMA Aktif");       
        // Dinlemeyi başlat (Arka plan task'ı oluşturulur)
        myUart->startListening();
    } 
    
    // Reliable UART izleme görevini başlat
    xTaskCreatePinnedToCore(
        reliable_uart_monitor_task,
        "reliable_uart_mon",
        4096, // Stack boyutu
        myUart, // myUart örneğini göreve parametre olarak geçir
        10,   // Öncelik
        NULL,
        1     // Çekirdek
    );

    ESP_LOGI(TAG, "\nSTARTED SYSTEM...");

    Local_Device_Read();

    if (GlobalConfig.sulama) {
        Local_irrigation_Read();
    }

    if (GlobalConfig.sulama) {
        Local_lighting_Read();
    }



    xTaskCreatePinnedToCore(
                        term_tara_task,           // Task fonksiyonunun adı
                        "evptask",          // Task'ın ismi (debug için)
                        4096,               // Stack büyüklüğü artırıldı
                        NULL,               // Task'a kopyalanan parametre gönderiliyor
                        1,                  // Öncelik sırası (0 en düşük)
                        NULL,               // Task handle (referans gerekmiyorsa NULL)
                        1                   // Çalışacağı çekirdek (0 veya 1)
                    );
    
    strcpy((char*)GlobalConfig.version,desc->version);

    ESP_LOGI(TAG, "      SMARTQ DAli Home Controller");
    ESP_LOGI(TAG, "      ======================================================");
    ESP_LOGI(TAG, "             Version %s", desc->version);
    ESP_LOGI(TAG, "        Dali Version %s", "STM_V1.00");
    ESP_LOGI(TAG, "        Project Name %s", desc->project_name);
    ESP_LOGI(TAG, "             Compile %s %s", desc->date, desc->time);
    ESP_LOGI(TAG, "                 IDF %s", desc->idf_ver);    
    ESP_LOGI(TAG, "             Wifi is %s", (NetworkConfig.wan_type==WAN_WIFI)?"ON":"OFF");
    ESP_LOGI(TAG, "         Ethernet is %s", (NetworkConfig.wan_type==WAN_ETHERNET)?"ON":"OFF");    
    ESP_LOGI(TAG, "                  IP %s", (char*)NetworkConfig.ip);
    ESP_LOGI(TAG, "             Netmask %s", (char*)NetworkConfig.netmask);
    ESP_LOGI(TAG, "           MDns Host %s", (char*)NetworkConfig.dns);
    ESP_LOGI(TAG, "        Mqtt Broaker %s", (char*)GlobalConfig.mqtt);
    ESP_LOGI(TAG, "             License %s", (char*)GlobalConfig.license);
    ESP_LOGI(TAG, "               Admin %s", (char*)GlobalConfig.admin);
    ESP_LOGI(TAG, "             Kanal 1 %s", (GlobalConfig.kanal1)?"ON":"OFF");
    ESP_LOGI(TAG, "             Kanal 2 %s", (GlobalConfig.kanal2)?"ON":"OFF");
    ESP_LOGI(TAG, "             Kanal 3 %s", (GlobalConfig.kanal3)?"ON":"OFF");
    ESP_LOGI(TAG, "     Dali Wifi Kanal %s", (GlobalConfig.kanal4)?"ON":"OFF");
    ESP_LOGI(TAG, "          Active WAN %d", Active_Network_connection);
    ESP_LOGI(TAG, "             Bina NO %d", GlobalConfig.binaNo);
    ESP_LOGI(TAG, "              Kat NO %d", GlobalConfig.katNo);
    ESP_LOGI(TAG, "            Daire NO %d", GlobalConfig.daireNo);

    ESP_LOGI(TAG, "      Sulama Sistemi %s", (GlobalConfig.sulama)?"AÇIK":"KAPALI");
    ESP_LOGI(TAG, "    Çevre Aydınlatma %s", (GlobalConfig.aydinlatma)?"AÇIK":"KAPALI");
    ESP_LOGI(TAG, "      ======================================================");


    if (Network_Status!=ESP_OK) {
        ESP_LOGE(TAG,"NETWORK YOK...");

    }

    if (GlobalConfig.alarm_aktif==1) ESP_LOGE(TAG,"AKTIF ALARM VAR");

    


    xTaskCreatePinnedToCore(
                        led_task,           // Task fonksiyonunun adı
                        "etask",          // Task'ın ismi (debug için)
                        2048,               // Stack büyüklüğü artırıldı
                        NULL,               // Task'a kopyalanan parametre gönderiliyor
                        1,                  // Öncelik sırası (0 en düşük)
                        NULL,               // Task handle (referans gerekmiyorsa NULL)
                        1                   // Çalışacağı çekirdek (0 veya 1)
                    );


    xTaskCreatePinnedToCore(dakika_task, "dakika_tetikleme_task", 3072, NULL, 5, NULL, 1);                
             
    while(1)
    {       
        vTaskDelay(10/portTICK_PERIOD_MS);
    }


}
