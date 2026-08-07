#include "ESP32Time.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"

#include <math.h>
#include <time.h>

#define PI 3.14159265358979323846

// Güneşin doğuş/batış hesaplama fonksiyonu (Basitleştirilmiş NOAA algoritması)
// gun_sayisi: Yılın kaçıncı günü (1-365)
// zenit: Resmi doğuş/batış için genellikle 90.833 derecedir
void hesapla_gunes(int gun_sayisi, double enlem, double boylam, double utc_ofset, int *dogus_dk, int *batis_dk) {
    double latRad = enlem * PI / 180.0;
    
    // Güneş dik açıklığı (Declination) hesaplama
    double decl = 0.409 * sin(2.0 * PI * (gun_sayisi - 81) / 365.0);
    
    // Saat açısı (Hour Angle) hesaplama
    double zenithRad = 90.833 * PI / 180.0; // Gün ışığı kırılması dahil standart değer
    double cosH = (cos(zenithRad) - sin(latRad) * sin(decl)) / (cos(latRad) * cos(decl));
    
    if (cosH > 1.0 || cosH < -1.0) {
        // Kutup dairesi istisnası (Güneş hiç batmıyor veya doğmuyor)
        *dogus_dk = -1;
        *batis_dk = -1;
        return;
    }
    
    double H = acos(cosH) * 180.0 / PI;
    
    // Güneş zamanı düzeltmeleri (Equation of Time basitleştirilmiş)
    double eqTime = 9.87 * sin(4.0 * PI * (gun_sayisi - 81) / 365.0) - 7.53 * cos(2.0 * PI * (gun_sayisi - 81) / 365.0);
    
    // Doğuş ve batış zamanları (Öğle vaktine göre dakika cinsinden)
    double ogle_vakti = 720.0 - (4.0 * boylam) - eqTime + (utc_ofset * 60.0);
    
    *dogus_dk = (int)(ogle_vakti - (H * 4.0));
    *batis_dk = (int)(ogle_vakti + (H * 4.0));
}


enum Bolge { BATI = 1, ORTA, GUNEY, DOGU };

const char *gun_isimleri[] = {
        "Pazar", 
        "Pazartesi", 
        "Salı", 
        "Çarşamba", 
        "Perşembe", 
        "Cuma", 
        "Cumartesi"
    };

void bolgeye_gore_koordinat_sec(enum Bolge secilen_bolge, double *enlem, double *boylam, double *utc) {
    *utc = 3.0; // Türkiye için her zaman sabit
    switch(secilen_bolge) {
        case BATI:  *enlem = 40.5; *boylam = 28.5; break;
        case ORTA:  *enlem = 39.9; *boylam = 32.8; break;
        case GUNEY: *enlem = 36.9; *boylam = 33.0; break;
        case DOGU:  *enlem = 39.0; *boylam = 41.5; break;
    }
}


void zaman_ve_hafta_bilgisi_yazdir() {
    time_t now;
    
    // Mevcut zamanı al
    time(&now);
    // Yerel zamana (Timezone ayarınıza göre) çevir
    localtime_r(&now, &tarih_saat.timeinfo);
	setenv("TZ", "UTC-03:00", 1);
	tzset();
	localtime_r(&now, &tarih_saat.timeinfo);

    // 1. Haftanın Kaçıncı Günü? (Pazartesi=1, Salı=2 ... Pazar=7 yapmak için küçük bir düzeltme)
    tarih_saat.haftanin_gunu = tarih_saat.timeinfo.tm_wday; 
    if (tarih_saat.haftanin_gunu == 0) {
        tarih_saat.haftanin_gunu = 7; // Pazar gününü 0 yerine 7 yapıyoruz (isteğe bağlı)
    }

    // 2. Yılın Kaçıncı Haftası? (ISO 8601 Standardı %V ile)
    char hafta_string[4];
    strftime(hafta_string, sizeof(hafta_string), "%V", &tarih_saat.timeinfo);
    tarih_saat.yilin_haftasi = atoi(hafta_string);

    // 3. Yılın Kaçıncı Günü? (0-364 arası döndüğü için +1 ekliyoruz)
    tarih_saat.yilin_gunu = tarih_saat.timeinfo.tm_yday + 1;

     double enlem, boylam, utc;
    bolgeye_gore_koordinat_sec(Bolge(GlobalConfig.bolge), &enlem, &boylam, &utc);

    int dogus_toplam_dakika, batis_toplam_dakika;
    
    // timeinfo.tm_yday: Yılın kaçıncı günü olduğunu verir (0-364)
    hesapla_gunes(tarih_saat.timeinfo.tm_yday + 1, enlem, boylam, utc, &dogus_toplam_dakika, &batis_toplam_dakika);

    // Dakikayı Saat:Dakika formatına çevirme
    int dogus_saat = dogus_toplam_dakika / 60;
    int dogus_dakika = dogus_toplam_dakika % 60;
    
    int batis_saat = batis_toplam_dakika / 60;
    int batis_dakika = batis_toplam_dakika % 60;

    localtime_r(&now, &tarih_saat.sunrise);
    localtime_r(&now, &tarih_saat.sunset);
    tarih_saat.sunrise.tm_hour = dogus_saat;
    tarih_saat.sunrise.tm_min = dogus_dakika;
    tarih_saat.sunrise.tm_sec = 0;

    tarih_saat.sunset.tm_hour = batis_saat;
    tarih_saat.sunset.tm_min = batis_dakika;
    tarih_saat.sunset.tm_sec = 0;


    mktime(&tarih_saat.sunrise);
    mktime(&tarih_saat.sunset);


    char *rr = (char *)calloc(1,50);
    char *rr0 = (char *)calloc(1,50);
    char *rr1 = (char *)calloc(1,50);
    strftime(rr, 50, "%d.%m.%Y %H:%M:%S", &tarih_saat.timeinfo);
    strftime(rr0, 50, "%H:%M", &tarih_saat.sunrise);
    strftime(rr1, 50, "%H:%M", &tarih_saat.sunset);

    // Log çıktıları
    ESP_LOGW("TIME","                    =========================== Zaman Analizi ===========================");   
    ESP_LOGI("TIME","                    Bugün: %s %s", rr,gun_isimleri[tarih_saat.timeinfo.tm_wday]);
    ESP_LOGI("TIME","                    Bugün yılın %d. günü.", tarih_saat.yilin_gunu);
    ESP_LOGI("TIME","                    Yılın %d. haftasındayız.", tarih_saat.yilin_haftasi);
    ESP_LOGI("TIME","                    Haftanın %d. günündeyiz (1:Pazartesi, 7:Pazar).", tarih_saat.haftanin_gunu);
    ESP_LOGI("TIME","                    Güneş Doğuş Saati %s", rr0);
    ESP_LOGI("TIME","                    Güneş Batış Saati %s", rr1);
    ESP_LOGI("TIME","                    ====================================================================="); 

    free(rr1);
    free(rr0);
    free(rr);
    
}


uint8_t wifi_retry=0; 
void time_sync(struct timeval *tv)
{   
    ESP_LOGI(TAG,"Time Sync");
    time_t now;
	struct tm tt1;
	time(&now);
	localtime_r(&now, &tt1);
	setenv("TZ", "UTC-03:00", 1);
	tzset();
	localtime_r(&now, &tt1);
  //  saat.esp_to_device();

  //  char *rr = (char *)malloc(50);
  //  Get_current_date_time(rr);
  //  ESP_LOGW(TAG, "Tarih/Saat %s", rr);

    zaman_ve_hafta_bilgisi_yazdir();

   // free(rr);
}

static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        
        // 1. Bağlantı hızını sorgula
        eth_speed_t speed;
        esp_eth_ioctl(eth_handle, ETH_CMD_G_SPEED, &speed);
        
        // 2. Duplex modunu yeni yöntemle sorgula
        eth_duplex_t duplex;
        esp_eth_ioctl(eth_handle, ETH_CMD_G_DUPLEX_MODE, &duplex);  // Yeni komut
        
        ESP_LOGI("ETH", "Bağlantı: %s, %s",
                speed == ETH_SPEED_100M ? "100 Mbps" : "10 Mbps",
                duplex == ETH_DUPLEX_FULL ? "Full Duplex" : "Half Duplex");
        
        break;

    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        Network_Status = ESP_FAIL; 
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{

    if ((event_id==IP_EVENT_STA_GOT_IP || event_id==IP_EVENT_ETH_GOT_IP || event_id==ESP_NETIF_IP_EVENT_GOT_IP))
              {
                // Net.wifi_update_clients();
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                strcpy((char *)NetworkConfig.ip,  Addr.to_string(event->ip_info.ip.addr));
                strcpy((char *)NetworkConfig.netmask,  Addr.to_string(event->ip_info.netmask.addr));
                strcpy((char *)NetworkConfig.gateway,  Addr.to_string(event->ip_info.gw.addr));
                NetworkConfig.broadcast = (uint32_t)(event->ip_info.ip.addr) | (uint32_t)0xFF000000UL;
                //tcpclient.wait = false;
                
                    if (event_id==IP_EVENT_ETH_GOT_IP) {
                        set_ethernet_connect_bit();
                        //strcpy((char *)wan_durumu,"E");
                    }

                    if (event_id==IP_EVENT_STA_GOT_IP) {
                        wifi_set_connection_bit();
                        //strcpy((char *)wan_durumu,"W");
                    }

                ESP_LOGI(TAG, "IP Received");
                 if (GlobalConfig.time_sync==1)
                {
                    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
                    esp_sntp_setservername(0, "pool.ntp.org");  // Birincil NTP
                    esp_sntp_setservername(1, "time.nist.gov"); 
                    esp_sntp_set_time_sync_notification_cb(time_sync);
                    esp_sntp_init();
                    ESP_LOGI("INIT","Sntp Başlatıldı");
                } 
                Network_Status = ESP_OK; 
              }
}

void wifi_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{

    //printf("wifi handler %ld %s\n",id,base);

    if (id==WIFI_EVENT_STA_DISCONNECTED)
              {
                 // tcpclient.wait = true;
                if (wifi_retry < NetworkConfig.WIFI_MAXIMUM_RETRY) {
                	  esp_wifi_connect();
                      wifi_retry++;
                      ESP_LOGW(TAG, "Tekrar Baglanıyor %d",NetworkConfig.WIFI_MAXIMUM_RETRY-wifi_retry);
                                                      } else {
                      wifi_set_fail_bit();                                  
                                                      }
              }

    if (id==WIFI_EVENT_STA_START)
        {
        wifi_retry=0;
        ESP_LOGW(TAG, "%s Wifi Network Connecting..",NetworkConfig.ssid);
        esp_wifi_connect();
        }

}