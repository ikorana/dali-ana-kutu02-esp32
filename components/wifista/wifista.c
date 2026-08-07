
#include "wifista.h"

wifista_config_t *wconfig;
EventGroupHandle_t WEvent;

void configure_ip(esp_netif_t *netif) {
   
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
        //ip4addr_aton((const char *)wconfig->dns, &dns.ip.u_addr.ip4);
        dns.ip.type = ESP_IPADDR_TYPE_V4;
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns));

        esp_netif_dns_info_t dns_backup;
        dns_backup.ip.u_addr.ip4.addr = esp_ip4addr_aton((const char *)wconfig->backup_dns);
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_backup));
    }
}


void wifi_init_sta()
{
    // 1. NVS flash'ı başlat
    /* esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS bozulmuş veya yer kalmamış, partisyonu sil ve tekrar dene
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    // 2. Ağ arabirimini ve event loop'u oluştur
    ESP_ERROR_CHECK(esp_netif_init()); */
   // ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();

    // 3. Statik IP ayarla (DHCP'den önce)
    configure_ip(netif);

    // 4. WiFi yapılandırması
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }

    // 5. Event handler'ları kaydet
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wconfig->wifi_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wconfig->ip_handler, NULL, NULL));

    // 6. WiFi istemci modunu ayarla
    wifi_sta_config_t aa = {};
    strcpy((char*)aa.ssid, (char*)wconfig->ssid);
    strcpy((char*)aa.password,  (char*)wconfig->pass);
    wifi_config_t wi_config = {
        .sta = aa,
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wi_config));

    // 7. WiFi'yi başlat
    ESP_ERROR_CHECK(esp_wifi_start());
}

esp_err_t wifi_start(wifista_config_t *cfg)
{
    WEvent = xEventGroupCreate();
    wconfig = cfg;
    wifi_init_sta();

    // Bağlantı veya hata bitini maksimum 240 saniye bekle
    EventBits_t bits = xEventGroupWaitBits(WEvent,
	            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
	            pdTRUE,
	            pdFALSE,
	            pdMS_TO_TICKS(240000));

    // Eğer bağlandık biti gelmediyse (zaman aşımı veya fail bit) başarısız dön
    if (!(bits & WIFI_CONNECTED_BIT)) return ESP_FAIL;

    esp_wifi_set_ps(WIFI_PS_NONE);
    return ESP_OK; 
}

void wifi_set_connection_bit(void) 
{
    xEventGroupSetBits(WEvent, WIFI_CONNECTED_BIT);
}
void wifi_set_fail_bit(void) 
{
    xEventGroupSetBits(WEvent, WIFI_FAIL_BIT);
}
