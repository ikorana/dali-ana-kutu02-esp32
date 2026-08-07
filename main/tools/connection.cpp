#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "core.h"

// Mevcut projedeki header'lar
#include "eth5500.h"
#include "wifista.h"
#include "wifiAP.h"

static const char *TAG_CONN = "NET_FALLBACK";

// Forward declaration
void start_app_services();
extern bool services_started;
extern uint8_t Active_Network_connection;

/**
 * @brief Ag durumunu izleyen task. 
 * Baglanti koptugunda veya AP modundayken kablo takildiginda servisleri baslatir.
 */
void network_monitor_task(void *pvParameters) {
    while (1) {
        // Eger servisler henuz baslamamissa ve IP alinmissa servisleri baslat
        if (!services_started && Network_Status == ESP_OK) {
            ESP_LOGI(TAG_CONN, "Gecikmeli baglanti tespit edildi. Servisler calistiriliyor...");
            start_app_services();
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // 10 saniyede bir kontrol et
    }
}

/**
 * @brief Kademeli ağ başlatma algoritması
 * Sıralama: Ethernet -> WiFi Client (STA) -> WiFi Access Point (AP)
 */
esp_err_t start_network_manager() {
    ESP_LOGI(TAG_CONN, "Ag hiyerarsisi baslatiliyor...");

    // Monitor task'ı her durumda başlatıyoruz
    xTaskCreate(network_monitor_task, "net_monitor", 3072, NULL, 3, NULL);

    if (NetworkConfig.wan_type==WAN_ETHERNET) 
    {
        // --- 1. ADIM: ETHERNET DENEMESİ ---
        ESP_LOGI(TAG_CONN, "Ethernet deneniyor...");
        ethernet_config_t et = {};
        et.mac_change = (NetworkConfig.mac_chg == 1) ? 1 : 0;
        et.dhcp = (NetworkConfig.ipstat == 1) ? 0 : 1;
        et.eth_handler = eth_event_handler;
        et.ip_handler = got_ip_event_handler;

        // IP Ayarlarını Kopyala
        strcpy((char*)et.ip, (char *)NetworkConfig.ip);
        strcpy((char*)et.netmask, (char *)NetworkConfig.netmask);
        strcpy((char*)et.gw, (char *)NetworkConfig.gateway);
        strcpy((char*)et.dns, (char *)NetworkConfig.dns);
        strcpy((char*)et.backup_dns, (char *)NetworkConfig.backup_dns);

        // Ethernet Init: eth5500.c içinde zaten 10sn IP bekliyor.
        if (ethernet_init(&et) == ESP_OK) {
            ESP_LOGI(TAG_CONN, "Ethernet Baglantisi Basarili!");
            Active_Network_connection = 1;
            return ESP_OK; 
        } else {
            ESP_LOGW(TAG_CONN, "Ethernet baglanamadi, WiFi deneniyor...");
        }
    }
    // --- 2. ADIM: WIFI STA (CLIENT) DENEMESİ ---
    ESP_LOGI(TAG_CONN, "WiFi STA deneniyor...");
    wifista_config_t ws = {};
    ws.dhcp = (NetworkConfig.ipstat == 1) ? 0 : 1;
    ws.wifi_handler = wifi_event_handler;
    ws.ip_handler = got_ip_event_handler;

    strcpy((char*)ws.ip, (char *)NetworkConfig.ip);
    strcpy((char*)ws.netmask, (char *)NetworkConfig.netmask);
    strcpy((char*)ws.gw, (char *)NetworkConfig.gateway);
    strcpy((char*)ws.dns, (char *)NetworkConfig.dns);
    strcpy((char*)ws.backup_dns, (char *)NetworkConfig.backup_dns);
    strcpy((char*)ws.ssid, (char *)NetworkConfig.ssid);
    strcpy((char*)ws.pass, (char *)NetworkConfig.pass);

    // wifi_start: wifista.c içinde xEventGroupWaitBits ile (WIFI_MAXIMUM_RETRY kadar) 
    // sonucu bekler ve baglanamazsa ESP_FAIL doner.
    if (wifi_start(&ws) == ESP_OK) {
        ESP_LOGI(TAG_CONN, "WiFi STA Baglantisi Basarili!");
        Active_Network_connection = 2;
        return ESP_OK;
        if (NetworkConfig.wan_type==WAN_WIFI) {
            NetworkConfig.wan_type = WAN_ETHERNET;
            disk.write_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig),0);
        }
    }

    // --- 3. ADIM: WIFI AP (ACCESS POINT) MODU ---
    // Eğer buraya ulaşıldıysa Ethernet ve WiFi Client başarısız olmuştur.
    ESP_LOGE(TAG_CONN, "Tum baglantilar basarisiz. AP Moduna geciliyor...");
    
    // AP modunda cihazın kendi servislerini (Web Server vb.) açabilmesi için status OK olmalı
    // Ancak bağlantı tipi olarak AP olduğunu belirten bir bayrak da tutulabilir.
    if (wifiap_start() == ESP_OK) {
        Network_Status = ESP_OK; 
        Active_Network_connection = 3;
        ESP_LOGI(TAG_CONN, "WiFi AP Baglantisi Basarili!");
        return ESP_OK;
    }
    
    return ESP_FAIL;
}
