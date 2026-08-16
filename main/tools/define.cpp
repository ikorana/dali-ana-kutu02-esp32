

   #define GEAR01_FILE (char*)"/config/gear01.bin" //Gear Dosyası
   #define GEAR02_FILE (char*)"/config/gear02.bin" //Gear Dosyası
   #define GEAR03_FILE (char*)"/config/gear03.bin" //Gear Dosyası
   #define GEAR04_FILE (char*)"/config/gear04.bin" //Gear Dosyası
   #define GEAR10_FILE (char*)"/config/gear10.bin" //Gear Dosyası

#define GLOBAL_FILE "/config/config.bin"
#define NETWORK_FILE "/config/network.bin"  
#define GURUP_FILE (char*)"/config/gurup.bin"
#define SCENE_FILE (char*)"/config/scene.bin"


static const char *TAG = "CONT03";

#define FATAL_MSG(a, str)                          \
    if (!(a))                                                     \
    {                                                             \
        ESP_LOGE(TAG, "%s(%d): %s", __FUNCTION__, __LINE__, str); \
        abort();                                         \
    }
     
auto* myUart = new UartDma(UART_NUM_2);    
StorageLittle disk;
config_t GlobalConfig = {};
network_config_t NetworkConfig = {};
esp_err_t Network_Status=ESP_FAIL; 
uint8_t Active_Network_connection=0; //0: Yok, 1: Ethernet, 2: WiFi, 3: AP
IPAddr Addr = IPAddr();
Udp_Server_Gem udp_server = Udp_Server_Gem();
QueueHandle_t udp_processing_queue = NULL;
const esp_app_desc_t *desc = esp_app_get_description();
ESP32Time saat;
Room room = Room();
Anahtar anahtar = Anahtar();
Gurup gurup = Gurup();
Gurup scene = Gurup();
Instance instance = Instance();   // DALI (kanal 1-4) instance kataloğu — /config/instance.bin
Instance instanceL = Instance();  // Yerel (kanal 10) anahtar/sensör kataloğu — /config/instanceL.bin, device.json ile senkronize
Fcm fcm = Fcm();

   Gear gear01 = Gear();
   Gear gear02 = Gear();
   Gear gear03 = Gear();
   Gear gear04 = Gear();

   Gear gear10 = Gear();


Httpd httpd = Httpd();
TftpOtaServer ota_server(79);
TftpDiskServer disk_server(69);

esp_mqtt_client_handle_t mqtt_client = NULL;


#include "define_unit.cpp"
