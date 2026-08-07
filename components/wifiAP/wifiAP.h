
#pragma once

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

typedef void (*wifiap_event_cb_t)(void*, esp_event_base_t, int32_t, void*);

#define BTTAP0 0x001
#define BTTAP1 0x002

#define WIFIAP_CONNECTED_BIT BTTAP0
#define WIFIAP_FAIL_BIT      BTTAP1


typedef struct {
    bool dhcp; //True ise ipler dhcp den gelir
    wifiap_event_cb_t wifi_handler;
    wifiap_event_cb_t ip_handler;
     
    uint8_t ip[15];
    uint8_t netmask[15];
    uint8_t gw[15];
    uint8_t dns[15];

    uint8_t ssid[30];
    uint8_t pass[30];
} wifiap_config_t;


#ifdef __cplusplus
extern "C" {
#endif

esp_err_t wifiap_start(void);
void wifiap_set_connection_bit(void);
void wifiap_set_fail_bit(void);


#ifdef __cplusplus
}
#endif