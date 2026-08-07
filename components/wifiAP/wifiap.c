
#include "wifiap.h"

wifiap_config_t *wconfig0;
EventGroupHandle_t WEvent0;

/* void configure_ip(esp_netif_t *netif) {
   
    if (wconfig->dhcp) {
        // DHCP modu
        esp_netif_dhcpc_start(netif);
        
    } else {
        // Statik IP modu
        esp_netif_dhcpc_stop(netif);
        esp_netif_ip_info_t ip_info={};
        ip_info.ip.addr = esp_ip4addr_aton((const char *)wconfig->ip);
        ip_info.netmask.addr = esp_ip4addr_aton((const char *)wconfig->netmask);
        ip_info.gw.addr = esp_ip4addr_aton((const char *)wconfig->gw);
        esp_netif_set_ip_info(netif, &ip_info);
        
        esp_netif_dns_info_t dns;
        dns.ip.u_addr.ip4.addr = esp_ip4addr_aton((const char *)wconfig->dns);
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns));
    }
} */

static void ap_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGW("AP", "client connected, AID=%d", event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGW("AP", "client disconnected, AID=%d", event->aid);
    }
}
esp_err_t wifi_init_ap()
{

    esp_netif_t* wifiAP = esp_netif_create_default_wifi_ap(); 
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    esp_err_t kk  = esp_wifi_init(&cfg);
    if(kk!=ESP_OK && kk != ESP_ERR_INVALID_STATE) { ESP_LOGE("WIFI_AP", "esp_wifi_init failed! %s", esp_err_to_name(kk)); return ESP_FAIL; }
    ESP_LOGI("WIFI_AP", "ESP WiFi Initialized.");
    if(esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &ap_event_handler,
                                        NULL,
                                        NULL)!=ESP_OK) return ESP_FAIL; 
    wifi_ap_config_t aa = {};
    strcpy((char*)aa.ssid, "SMQ_MAIN");
    strcpy((char*)aa.password, "12345678");
    aa.ssid_len = strlen("SMQ_MAIN");

    aa.channel = 11;
    aa.max_connection = 5;
    aa.authmode = WIFI_AUTH_WPA_WPA2_PSK;
   
    wifi_config_t wifi_config = {
        .ap = aa,
    };

    esp_netif_dhcps_stop(wifiAP);
    esp_netif_ip_info_t info_t;
    memset(&info_t, 0, sizeof(esp_netif_ip_info_t));
        info_t.ip.addr = esp_ip4addr_aton((const char *)"192.168.7.1");
        info_t.netmask.addr = esp_ip4addr_aton((const char *) "255.255.255.0");
        info_t.gw.addr = esp_ip4addr_aton((const char *) "192.168.7.1"); 

    esp_netif_set_ip_info(wifiAP, &info_t);
    esp_netif_dhcps_start(wifiAP);

    if(esp_wifi_set_mode(WIFI_MODE_AP)!=ESP_OK) { ESP_LOGE("WIFI_AP", "esp_wifi_set_mode failed!"); return ESP_FAIL; }
    if(esp_wifi_set_config(WIFI_IF_AP, &wifi_config)!=ESP_OK) { ESP_LOGE("WIFI_AP", "esp_wifi_set_config failed!"); return ESP_FAIL; }
    kk = esp_wifi_start();
    if(kk!=ESP_OK) { ESP_LOGE("WIFI_AP", "esp_wifi_start failed! %s", esp_err_to_name(kk)); return ESP_FAIL; }

    return ESP_OK;
}

esp_err_t wifiap_start(void)
{
    WEvent0 = xEventGroupCreate();
    wconfig0 = NULL;
    esp_err_t ret = wifi_init_ap();
    /* EventBits_t bits = xEventGroupWaitBits(WEvent0,
	            WIFIAP_CONNECTED_BIT | WIFIAP_FAIL_BIT,
	            pdTRUE,
	            pdFALSE,
	            portMAX_DELAY);
    if (bits & WIFIAP_FAIL_BIT) return ESP_FAIL; */
    if (ret==ESP_OK) esp_wifi_set_ps(WIFI_PS_NONE);
    return ret; 
}

void wifiap_set_connection_bit(void) 
{
    xEventGroupSetBits(WEvent0, WIFIAP_CONNECTED_BIT);
}
void wifiap_set_fail_bit(void) 
{
    xEventGroupSetBits(WEvent0, WIFIAP_FAIL_BIT);
}
