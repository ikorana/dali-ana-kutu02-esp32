#pragma once

#include "esp_eth_driver.h"



typedef void (*event_cb_t)(void*, esp_event_base_t, int32_t, void*);

typedef struct {
    bool mac_change;
    bool dhcp; //True ise ipler dhcp den gelir
    event_cb_t eth_handler;
    event_cb_t ip_handler;
     
    uint8_t ip[15];
    uint8_t netmask[15];
    uint8_t gw[15];
    uint8_t dns[15];
    uint8_t backup_dns[15];
} ethernet_config_t;

#define ETH_CONNECTED_BIT BIT0
#define ETH_FAIL_BIT      BIT1

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t eth_init(esp_eth_handle_t *eth_handles_out[], uint8_t *eth_cnt_out, bool change_mac);
esp_err_t eth_deinit(esp_eth_handle_t *eth_handles, uint8_t eth_cnt);

esp_err_t ethernet_init(ethernet_config_t *conf);
void set_ethernet_connect_bit(void);
void set_ethernet_fail_bit(void);

#ifdef __cplusplus
}
#endif