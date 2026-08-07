
#pragma once

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

typedef void (*wifi_event_cb_t)(void*, esp_event_base_t, int32_t, void*);

#define BTT0 0x001
#define BTT1 0x002

#define WIFI_CONNECTED_BIT BTT0
#define WIFI_FAIL_BIT      BTT1


typedef struct {
    bool dhcp; //True ise ipler dhcp den gelir
    wifi_event_cb_t wifi_handler;
    wifi_event_cb_t ip_handler;
     
    uint8_t ip[15];
    uint8_t netmask[15];
    uint8_t gw[15];
    uint8_t dns[15];
    uint8_t backup_dns[15];

    uint8_t ssid[30];
    uint8_t pass[30];
} wifista_config_t;


#ifdef __cplusplus
extern "C" {
#endif

esp_err_t wifi_start(wifista_config_t *cfg);
void wifi_set_connection_bit(void);
void wifi_set_fail_bit(void);


#ifdef __cplusplus
}
#endif