#ifndef _CORE_H
#define _CORE_H

#include "cJSON.h"
#include "udp_server_gem.h"

typedef enum {
    IP_DEFAULT = 0,
    STATIC_IP, 
    DYNAMIC_IP, 
} home_ipstat_type_t;

typedef enum {
    WAN_DEFAULT = 0,
    WAN_ETHERNET,
    WAN_WIFI,
} home_wan_type_t;

typedef struct {
    uint8_t open_default;
    uint8_t device_id;
    uint8_t http_start;
    uint8_t time_sync;
    uint8_t version[30];
    uint8_t disk_format;
    uint8_t admin[30]; 
    uint8_t log_level;
    uint8_t mode;
    uint8_t alarm;
    uint8_t alarm_aktif;
    uint8_t mqtt[64]; 
    uint8_t Ai;
    uint8_t kanal1;
    uint8_t kanal2;
    uint8_t kanal3;
    uint8_t kanal4;
    uint8_t mod_format;
    uint8_t ping_active;

    uint8_t project_number;
    uint8_t binaNo;
    uint8_t katNo;
    uint8_t daireNo;
    uint8_t license[29]; 
    uint8_t bolge;

    uint8_t sulama;
    uint8_t aydinlatma;

    uint8_t Aproject_number;
    uint8_t AbinaNo;
    uint8_t AkatNo;
    uint8_t AdaireNo;
    uint8_t Alicense[29];
    uint8_t Aip[17];
    float Alon;
    float Alat;

    float lon;
    float lat;
    float temp;
    float rain;
    uint8_t yer[60];
} config_t;

typedef struct {
    uint8_t home_default;
    home_ipstat_type_t ipstat;
    home_wan_type_t wan_type;
    uint8_t ip[17];               //Static ip 
    uint8_t netmask[17];          //Static netmask
    uint8_t gateway[17];          //Static gateway
    uint8_t dns[17]; 
    uint8_t backup_dns[17]; 
    uint32_t broadcast;
    uint8_t WIFI_MAXIMUM_RETRY;
    uint8_t ssid[30];
    uint8_t pass[30]; 
    uint8_t channel;
    uint8_t mac_chg;
} network_config_t;

typedef enum {
    DALI_WIRELESS = 0x00,
    DALI_KANAL1,
    DALI_KANAL2,
    DALI_KANAL3,
    DALI_ALL,
    DALI_LOCAL = 10
} __attribute__((packed)) dev_sourge_t;

typedef struct {
    uint8_t saat;
    uint8_t dakika;
} saat_t;

typedef struct {
    uint8_t onactive;
    saat_t ontime;
    uint8_t offactive;
    saat_t offtime;
    uint8_t power;
} zaman_t;

typedef enum 
{
    TYPE_GURUP=0,
    TYPE_SCENE,
    TYPE_SPECIAL,
    COM_ON,
    COM_OFF,
    COM_SCENE,  
    COM_NONE,     
} zaman_tip_t;

typedef struct {
    zaman_tip_t command; //>3 
    uint8_t index;
    uint8_t addr;
    uint8_t power;
} zaman_job_t;



typedef struct {
    cJSON *pck;
    remote_t *rem;
} pck_t;

typedef struct {
    pck_t pck;
    cJSON *payload;
    bool is_mqtt;
} search_task_param_t;

typedef void (*send_AK_t)(cJSON *pay, pck_t *pck, bool is_mqtt);
typedef void (*send_Status_t)(uint8_t adr, uint8_t kanal, pck_t *pck, bool is_mqtt);



typedef struct
{
    pck_t pck;
    char *payload;
    bool is_mqtt;

} voice_task_param_t;


typedef struct {
    uint8_t event_id;
    uint8_t event_state;
    uint8_t action_state;
} event_state_t;


typedef enum {BOS,GRUP,SENARYO,LAMBA, PERDE, KAPI, TAMAM, VAZGEC, RADYO,ROOM,PRIZ} komuttipi_t;
typedef enum {ABOS,AC,KAPAT,UYGULA,ENDUSUK,ENYUKSEK,ORTALA,GUC,SONRA,ONCE,SESARTI,SESEKSI,SESSUS,SESDEVAM} altkomuttipi_t;



#endif