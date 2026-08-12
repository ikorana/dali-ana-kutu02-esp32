
#include "esp_log_level.h"

#include "nvs_flash.h"
#include <cstdio>
void default_config(void);
void network_default_config(void);

void global_define()
{
    ESP_LOGD("DISK","Read Global Variable");
    
    disk.read_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig), 0);
	if (GlobalConfig.open_default==0 ) {
		//Global ayarlar diskte kayıtlı değil. Kaydet.
		 default_config();
		 disk.read_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
		 FATAL_MSG(GlobalConfig.open_default,"Global Initilalize File ERROR !...");
	}
    if (GlobalConfig.disk_format==1)
    {
        ESP_LOGD("DISK","Disk Formatlanıyor. Bekleyiniz");
        disk.format();
        vTaskDelay(1000/portTICK_PERIOD_MS);
        esp_restart();
    }
}

void network_define()
{
    ESP_LOGD(TAG,"NETWORK DEFAULT CONFIG");
    disk.read_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig), 0);
	if (NetworkConfig.home_default==0 ) {
		//Network ayarları diskte kayıtlı değil. Kaydet.
		 network_default_config();
		 disk.read_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig),0);
		 FATAL_MSG(NetworkConfig.home_default, "Network Initilalize File ERROR !...");
	}   
}

void default_config(void)
{
     ESP_LOGD("DISK","      Write Default Global Variable");
     GlobalConfig.open_default = 1;
     GlobalConfig.device_id = 1;
     GlobalConfig.project_number=1;
     
     GlobalConfig.http_start=1;
     GlobalConfig.time_sync=1;
     GlobalConfig.disk_format = 0;
     GlobalConfig.mod_format = 0;
     strcpy((char*)GlobalConfig.admin,"default");
     GlobalConfig.log_level = 3;
     GlobalConfig.mode = 0;
     GlobalConfig.alarm = 0;
     GlobalConfig.alarm_aktif = 0;
     strcpy((char*)GlobalConfig.license,"SMQLC02");
     strcpy((char*)GlobalConfig.mqtt,"broker.emqx.io");

     GlobalConfig.kanal1 = 1;
     GlobalConfig.kanal2 = 1;
     GlobalConfig.kanal3 = 1;
     GlobalConfig.kanal4 = 1;

     GlobalConfig.binaNo = 1;
     GlobalConfig.katNo = 1;
     GlobalConfig.daireNo = 2;
     GlobalConfig.Ai = 1;
     GlobalConfig.ping_active = 0;
     GlobalConfig.bolge = 1;
     GlobalConfig.sulama = 0;
     GlobalConfig.aydinlatma=0;
     GlobalConfig.lon =0.0;
     GlobalConfig.lat =0.0;
     GlobalConfig.temp =0.0;
     GlobalConfig.rain =0.0;

     GlobalConfig.Aproject_number=GlobalConfig.project_number;
     GlobalConfig.AbinaNo=GlobalConfig.binaNo;
     GlobalConfig.AkatNo=GlobalConfig.katNo;
     GlobalConfig.AdaireNo=GlobalConfig.daireNo;
     strcpy((char*)GlobalConfig.Alicense,(char *)GlobalConfig.license);
     strcpy((char*)GlobalConfig.Aip,(char *)NetworkConfig.ip);
     GlobalConfig.Alon=GlobalConfig.lon;
     GlobalConfig.Alat=GlobalConfig.lat;

     disk.file_control(GLOBAL_FILE);
     disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
}

void network_default_config(void)
{
     ESP_LOGD("DISK","      Write Default Network Variable");
     NetworkConfig.home_default = 1;
     NetworkConfig.wan_type = WAN_WIFI; //WAN_WIFI; //WAN_ETHERNET   
     NetworkConfig.ipstat = STATIC_IP; //STATIC_IP; //DYNAMIC_IP
     
     strcpy((char*)NetworkConfig.ip,"192.168.1.80");
     strcpy((char*)NetworkConfig.netmask,"255.255.255.0");
     strcpy((char*)NetworkConfig.gateway,"192.168.1.1");
     strcpy((char*)NetworkConfig.dns,"8.8.8.8");
     strcpy((char*)NetworkConfig.backup_dns,"4.4.4.4");

     strcpy((char*)NetworkConfig.ssid,"Akdogan_2.4G");
     strcpy((char*)NetworkConfig.pass,"651434_2.4"); 

    // strcpy((char*)NetworkConfig.ssid,"IMS_YAZILIM");
    // strcpy((char*)NetworkConfig.pass,"mer6514a4c"); 

    //strcpy((char*)NetworkConfig.ssid,"SMQ_NETWORK");
    //strcpy((char*)NetworkConfig.pass,"12345678"); 


     NetworkConfig.channel = 11;    
     NetworkConfig.WIFI_MAXIMUM_RETRY=5;
    
    disk.file_control(NETWORK_FILE);
    disk.write_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig),0);
}

zaman_job_t *create_zaman_data(zaman_tip_t cmd, uint8_t inx, uint8_t pow, uint8_t adr)
{
    zaman_job_t *jb = (zaman_job_t *) calloc(1,sizeof(zaman_job_t));
    jb->command = cmd;
    jb->index = inx;
    jb->power = pow;
    jb->addr = adr;
    return jb;
}

/* void add_job_record(group_t grp)
{
    zaman_job_t *jb = create_zaman_data(COM_ON,0,kk.power, adr);

} */

void add_all_group_cron(void)
{
  for (int i=0;i<16;i++)
    {
      gurup_t kk={};
      disk.read_file(GURUP_FILE,&kk,sizeof(gurup_t),i);
      if (kk.timing1.onactive==1) {
         // add_job_record(kk);
      }
      if (kk.timing2.offactive==1) {
         // add_job_record(kk);
      }
    }
}

TaskHandle_t flash_task_handle = NULL;
void led_flash_task(void *pvParameter) {
    uint8_t state = 0;
    while (1) {
        state = !state;
        gpio_set_level(LED, state);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
// Flash işlemini başlatan fonksiyon
void start_flash() {
    if (flash_task_handle == NULL) {
        xTaskCreate(led_flash_task, "led_flash_task", 2048, NULL, 5, &flash_task_handle);
    }
}

// Flash işlemini durduran fonksiyon
void stop_flash() {
    if (flash_task_handle != NULL) {
        vTaskDelete(flash_task_handle);
        flash_task_handle = NULL;
        gpio_set_level(LED, 0); // Durunca LED'i söndür
    }
}

void file_initialize(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED) | (1ULL << STM_RESET) | (1ULL << STM_PING);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(LED, 0);
    gpio_set_level(STM_RESET, 0);
    gpio_set_level(STM_PING, 1);

    gpio_config_t io_conf1;
    io_conf1.intr_type = GPIO_INTR_DISABLE;
    io_conf1.mode = GPIO_MODE_INPUT;
    io_conf1.pin_bit_mask = (1ULL << STM_PONG);
    io_conf1.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf1.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf1);

    start_flash();
   
    disk.init();
    global_define();
    ESP_LOGI(TAG,"Log Level :%d ",GlobalConfig.log_level);

    esp_log_level_set("*",esp_log_level_t(GlobalConfig.log_level));
    esp_log_level_set("wifi", ESP_LOG_NONE);
    esp_log_level_set("wifi_init", ESP_LOG_NONE);
    esp_log_level_set("phy_init", ESP_LOG_NONE);
    esp_log_level_set("gpio", ESP_LOG_NONE);
    esp_log_level_set("httpd_txrx", ESP_LOG_NONE);

    network_define();

    instance.file_init(&disk);
   
       gear01.file_init(&disk,GEAR01_FILE, &instance);
       gear02.file_init(&disk,GEAR02_FILE, &instance);
       gear03.file_init(&disk,GEAR03_FILE, &instance);
       gear04.file_init(&disk,GEAR04_FILE, &instance);
       gear10.file_init(&disk,GEAR10_FILE, &instance);

    if (GlobalConfig.mod_format==1) {
        gear10.clear_file();
        GlobalConfig.mod_format=0;
        disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
    }   
   
    gurup.file_init(&disk,GURUP_FILE,"Gurup");
    scene.file_init(&disk,SCENE_FILE,"Senaryo");
    room.file_init(&disk);
    anahtar.file_init(&disk);
    fcm.file_init(&disk);
    

    add_all_group_cron();

    disk.list("/config","*.*");

}

#include "ArduinoJson.h"



void Local_Device_Read(void) {
    const char *name1="/config/device.json";
    if (disk.file_search(name1))
      {
        int fsize = disk.file_size(name1); 
        char *buf = (char *) malloc(fsize+5);
        if (buf==NULL) {ESP_LOGE("TEST", "memory not allogate"); return ;}
        FILE *fd = fopen(name1, "r");
        if (fd == NULL) {ESP_LOGE("TEST", "%s not open",name1); return ;}
        fread(buf, fsize, 1, fd);
        fclose(fd);
        
        DynamicJsonDocument doc(fsize+5);
        DeserializationError error = deserializeJson(doc, buf);
        if (error) {
          ESP_LOGE("TEST","deserializeJson() failed: %s",error.c_str());
          return;
        } 

        uint8_t event_id=0;
        for (JsonObject function : doc["events"].as<JsonArray>()) {
            const char* a_ev = function["type"];
            const char* a_cat = function["category"];
            Base_Event *new_event = new Base_Event(a_ev, a_cat, event_id);
            event_id++;
            event_list.push_back(new_event);

            //printf("Event : %s %s %d\n",a_ev,a_cat,event_id);
        }

        JsonArray AArray = doc["actions"].as<JsonArray>();
        size_t elemanSayisi = AArray.size();
        uint8_t gcount = gear10.get_gear_count();
        if (elemanSayisi!=gcount) {
            ESP_LOGW("TEST","Action Count Mismatch : %d != %d. Action file Formating..",elemanSayisi,gcount);
            gear10.clear_file();
        }

                   
        for (JsonObject function : AArray) {
          
          const char* a_type = function["action_type"];  
          int a_id = function["action_id"];
          int a_channel = function["channel"];
          int a_tim = function["time"];
          uint8_t Ltype=0x06, LExt=0x06;
          const char* a_name = function["action_name"];
          int a_room = function["action_room"];

          //printf("Action ROOM : %d\n",a_room);
          
          
          if (strcmp(a_type,"relay")==0) {Ltype=0x07;LExt=0x07;}
          if (strcmp(a_type,"relaylamp")==0) {Ltype=0x07;LExt=0x76;}
          if (strcmp(a_type,"energy")==0) {Ltype=0x07;LExt=0x78;}
          if (strcmp(a_type,"water")==0) {Ltype=0x07;LExt=0x79;}
          if (strcmp(a_type,"gas")==0) {Ltype=0x07;LExt=0x7A;}
          if (strcmp(a_type,"elevator")==0) {Ltype=0x07;LExt=0x7B;}
          if (strcmp(a_type,"blind")==0) {Ltype=0x07;LExt=0x77;}
          if (strcmp(a_type,"socket")==0) {Ltype=0x07;LExt=0x7C;}
          if (strcmp(a_type,"mwater")==0) {Ltype=0x07;LExt=0x7D;}
          if (strcmp(a_type,"door")==0) {Ltype=0x07;LExt=0x7E;}
          if (strcmp(a_type,"garage")==0) {Ltype=0x07;LExt=0x7F;}
          if (strcmp(a_type,"movement")==0) {Ltype=0x07;LExt=0x80;}


            Base_Device* new_relay = nullptr;
            if (strcmp(a_type,"relay")==0) new_relay = new Relay(a_id, a_name, a_channel);
            if (strcmp(a_type,"relaylamp")==0) new_relay = new RelayLamp(a_id, a_name, a_channel);
            if (strcmp(a_type,"energy")==0) new_relay = new Energy(a_id,  a_name, a_channel);
            if (strcmp(a_type,"elevator")==0) new_relay = new Elevator(a_id,  a_name, a_channel);
            if (strcmp(a_type,"gas")==0) new_relay = new Gas(a_id,  a_name, a_channel);
            if (strcmp(a_type,"blind")==0) new_relay = new Blind(a_id,  a_name, a_channel); 
            if (strcmp(a_type,"water")==0) new_relay = new Water(a_id,  a_name, a_channel); 
            if (strcmp(a_type,"socket")==0) new_relay = new Socket(a_id,  a_name, a_channel); 
            if (strcmp(a_type,"mwater")==0) new_relay = new MWater(a_id,  a_name, a_channel);
            if (strcmp(a_type,"door")==0) new_relay = new Door(a_id,  a_name, a_channel);
            if (strcmp(a_type,"garage")==0) new_relay = new Garage(a_id,  a_name, a_channel);
            if (strcmp(a_type,"movement")==0) {
                new_relay = new Movement(a_id,  a_name, a_channel);
                ((Movement *)new_relay)->set_event_proc(&send_events);
                ((Movement *)new_relay)->set_event_id(find_event_id("MOVEMENT_EVENT"));
            }


            if (new_relay!=nullptr) {
                new_relay->set_callback(base_function_callback);
                new_relay->set_saycallback(base_function_saycallback);
                new_relay->set_room(a_room);
                new_relay->set_time(a_tim);
               
                for (JsonObject op : function["mapped_outputs"].as<JsonArray>()) {
                    const char* op_type = op["type"];
                    const char* op_desc = op["desc"];  
                    int op_pin = op["id"];
                    int op_active = op["active"]; op_active=1;
                    Base_Port* new_port = new Base_Port(op_pin, op_active, (char*)op_type, myUart, &reliableUartManager, (char*)op_desc);
                    new_relay->add_out_port(new_port);
                }

                for (JsonObject ip : function["mapped_inputs"].as<JsonArray>()) {
                    const char* ip_type = ip["type"]; 
                    const char* ip_desc = ip["desc"];  
                    int ip_pin = ip["id"];
                    int ip_active = ip["active"];
                    In_Base_Port* i_port = new In_Base_Port(ip_pin, ip_active, (char*)ip_type, (char*)ip_desc);
                    new_relay->add_in_port(i_port);
                }   
                
                for (JsonObject ev : function["event_links"].as<JsonArray>()) {
                    const char* ev_type = ev["type"]; 
                    const char* ev_state = ev["event_state"]; 
                    const char* ac_state = ev["action_state"]; 
                    event_state_t *i_event = new event_state_t();  
                    i_event->event_id = find_event_id(ev_type);
                    if (strcmp(ev_state,"on")==0) i_event->event_state = 1;
                    if (strcmp(ev_state,"off")==0) i_event->event_state = 0;
                    if (strcmp(ac_state,"on")==0) i_event->action_state = 1;
                    if (strcmp(ac_state,"off")==0) i_event->action_state = 0;
                    new_relay->add_events(i_event);
                   // printf("%s %d %d %d\n",ev_type,i_event->event_id,i_event->event_state,i_event->action_state);
                }       
                

                // printf("%s %d %d %s %d\n",a_type,a_id,a_channel,a_name, a_pin);
                
                    gear_t gr = {};
                    gr.aktif = 1;
                    gr.short_addr = a_id;
                    gr.type = Ltype;
                    gr.ext_type = LExt;
                    gr.kanal = (dev_sourge_t)10;
                    gr.instance = 0;
                    gr.ico = 0;
                    gr.room = a_room;   
                    gr.anahtar=0xff;        
                    gr.spec.group[0] = 0;
                    gr.spec.group[1] = 0; 
                    strcpy((char*)gr.name,a_name);
                    gear10.update_gear(a_id,&gr);   
                    

                    //new_relay->add_port(new_port);
                    new_relay->set_gear(&gear10);
                    new_relay->time = a_tim;
                    cihaz_listesi.push_back(new_relay); 
                    //new_relay->init();
            }
                   
        }

        /* for (Base_Device* cihaz : cihaz_listesi) {
            printf("Liste %s\n",cihaz->get_name());
             //delete cihaz; // virtual ~Relay() çağrılır, hem cihaz hem asprintf ismi silinir.
        } */

        gear10.list_gear();

        doc.clear();                       
        free(buf);
      }
}


#include "firebase_tool.cpp"

#include "jsontool.h"


void send_events(cJSON *pay) {
    if (pay==NULL) return;
    for (Base_Device* dv : cihaz_listesi) {
        dv->set_events(pay);
    }
    if (irrigation) irrigation->set_events(pay);
}


void dakika_task(void *pvParameters) {
    struct tm timeinfo;
    time_t now;

    ESP_LOGI(TAG, "Dakika takip taskı başlatıldı.");

    while (1) {
        // 1. Mevcut zamanı al
        time(&now);
        localtime_r(&now, &timeinfo);

        // 2. Bir sonraki dakikanın başına kalan saniyeyi hesapla
        // Örneğin şu an saniye 24 ise: 60 - 24 = 36 saniye kaldı.
        int kalan_saniye = 60 - timeinfo.tm_sec;

        // 3. Hesaplanan süre kadar taskı blokla (uyut)
        // Milisaniye cinsinden hesaplayıp Tick değerine çeviriyoruz
        vTaskDelay(pdMS_TO_TICKS(kalan_saniye * 1000));

        // ---------------------------------------------------------
        // TAM DAKİKA DEĞİŞTİĞİ ANDA ÇALIŞACAK KODLAR BURAYA GELECEK
        // ---------------------------------------------------------

        time(&now);
        localtime_r(&now, &timeinfo);

        int su_an = (timeinfo.tm_hour * 60) + timeinfo.tm_min;
        int sunrise = (tarih_saat.sunrise.tm_hour * 60) + tarih_saat.sunrise.tm_min;
        int sunset = (tarih_saat.sunset.tm_hour * 60) + tarih_saat.sunset.tm_min;

        if (su_an == sunrise) {
            ESP_LOGI(TAG, "Güneş doğdu! SUNRISE_EVENT tetiklenebilir.");
            uint8_t evid = find_event_id("SUNRISE_EVENT");
            cJSON *pay = cJSON_CreateObject();
            cJSON_AddStringToObject(pay, "com", "EVENTS");
            cJSON_AddStringToObject(pay, "event", "SUNRISE_EVENT");
            cJSON_AddNumberToObject(pay, "event_id", evid);
            cJSON_AddNumberToObject(pay, "state", 1);
            send_events(pay);
            cJSON_Delete(pay);
            // Burada röle tetikleme kodunu ekleyebilirsiniz.
        } else if (su_an == sunset) {
            ESP_LOGI(TAG, "Güneş battı! SUNSET_EVENT tetiklenebilir.");
            // Burada röle tetikleme kodunu ekleyebilirsiniz.
            uint8_t evid = find_event_id("SUNSET_EVENT");
            cJSON *pay = cJSON_CreateObject();
            cJSON_AddStringToObject(pay, "com", "EVENTS");
            cJSON_AddStringToObject(pay, "event", "SUNSET_EVENT");
            cJSON_AddNumberToObject(pay, "event_id", evid);
            cJSON_AddNumberToObject(pay, "state", 1);
            send_events(pay);
            cJSON_Delete(pay);
        }

        //CLOCK_UPDATE_EVENT tetikleme

        int c_wday = timeinfo.tm_wday; 
        int my_wday = (c_wday == 0) ? 7 : c_wday;
        
        uint8_t evid = find_event_id("CLOCK_UPDATE_EVENT");
        cJSON *pay = cJSON_CreateObject();
        cJSON_AddStringToObject(pay, "com", "EVENTS");
        cJSON_AddStringToObject(pay, "event", "CLOCK_UPDATE_EVENT");
        cJSON_AddNumberToObject(pay, "event_id", evid);
        cJSON_AddNumberToObject(pay, "hour", timeinfo.tm_hour);
        cJSON_AddNumberToObject(pay, "minute", timeinfo.tm_min);
        cJSON_AddNumberToObject(pay, "daynum", my_wday);
        send_events(pay);
        cJSON_Delete(pay);
  
       // ESP_LOGI(TAG, "=> Dakika değişti! Şu anki saat: %02d:%02d:%02d", 
       //          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

        
        // Küçük bir güvenlik önlemi: Task uyandıktan sonra mikro saniyelik 
        // işletim sistemi gecikmelerinden dolayı saniye hala 59 görünüyor veya 
        // üst üste çift tetiklenme olmasın diye döngü başına geçmeden önce 1 saniye bekletiyoruz.
        vTaskDelay(pdMS_TO_TICKS(1100)); 
    }
}


int dayToNumber(const char* day) {
    if (strcmp(day, "Mon") == 0) return 1;
    if (strcmp(day, "Tue") == 0) return 2;
    if (strcmp(day, "Wed") == 0) return 3;
    if (strcmp(day, "Thu") == 0) return 4;
    if (strcmp(day, "Fri") == 0) return 5;
    if (strcmp(day, "Sat") == 0) return 6;
    if (strcmp(day, "Sun") == 0) return 7;
    return 0; // Hatalı veya geçersiz gün gelirse
}


void Local_irrigation_Read(void)
{
    const char *name1="/config/irrigation.json";
    if (disk.file_search(name1))
    {
        int fsize = disk.file_size(name1); 
        char *buf = (char *) malloc(fsize+5);
        if (buf==NULL) {ESP_LOGE("TEST", "memory not allogate"); return ;}
        FILE *fd = fopen(name1, "r");
        if (fd == NULL) {ESP_LOGE("TEST", "%s not open",name1); return ;}
        fread(buf, fsize, 1, fd);
        fclose(fd);
        
        DynamicJsonDocument doc(fsize+5);
        DeserializationError error = deserializeJson(doc, buf);
        if (error) {
          ESP_LOGE("TEST","deserializeJson() failed: %s",error.c_str());
          return;
        } 

        irrigation = new Irrigation();
        irrigation->set_udp_server(&udp_server);

        const char* a_stat = doc["status"];
        if (strcmp(a_stat,"enabled")==0) irrigation->set_active(true); else irrigation->set_active(false);
        int a_smart = doc["smart"];
        if (a_smart==1) irrigation->set_smart(true); else irrigation->set_smart(false);

        JsonObject hw =  doc["hardware"];
        JsonObject motor = hw["motor"];

        int motorId = motor["id"];                 // 16
        const char* typ = motor["type"];    
        int startDelay = motor["start_delay_ms"];  

        if (strcmp(typ,"relay")==0) {
            Base_Port *prt = new Base_Port(motorId,1,(char*)"relay",myUart,&reliableUartManager,(char*)"motor");
           irrigation->set_motor(motorId,startDelay,prt); 

           //printf("MOTOR %d %d\n",motorId,startDelay);
         

           JsonArray schs = doc["schedules"];
           for (JsonObject sch : schs) {
                const char* tm = sch["start_time"];

                Schedule *sc = new Schedule(); 
                sc->set_time(tm);


                JsonArray adays = sch["active_days"];
                for (const char* day : adays) {
                    //printf("DAY %s %d\n",day,dayToNumber(day));
                    sc->add_day(dayToNumber(day));
                }

                JsonArray durs = sch["durations"];
                for (JsonObject dur : durs) {
                    int segId = dur["id"];
                    int relId = dur["relay_id"];
                    int tm = dur["time"];
                    //printf("        SEGMENT %d Relay:%d time:%d",segId,relId,tm);
                    Base_Port *prt = new Base_Port(relId,1,(char*)"relay",myUart,&reliableUartManager,nullptr);
                    sc->add_adim(segId,relId,tm,prt);
                    irrigation->add_port(prt);
                }
            
                irrigation->add_schedule(sc);
           }

           JsonArray evvs = doc["event_links"];
           for (JsonObject evv : evvs) {
              event_state_t *sv = new event_state_t();  
              sv->event_id = find_event_id(evv["type"]);
              sv->event_state = (evv["event_state"]=="on")?1:0;
              sv->action_state = (evv["action_state"]=="on")?1:0;
              irrigation->add_events(sv);
           }

            event_state_t *sv = new event_state_t(); 
            sv->event_id = find_event_id("CLOCK_UPDATE_EVENT");
            sv->event_state = 0;
            sv->action_state = 0;
            irrigation->add_events(sv);

        }


        irrigation->list();
        
        doc.clear();                       
        free(buf);
        }
        
}

void Local_lighting_Read(void) 
{
    const char *name1="/config/lighting.json";
    if (disk.file_search(name1))
    {
        int fsize = disk.file_size(name1); 
        char *buf = (char *) malloc(fsize+5);
        if (buf==NULL) {ESP_LOGE("TEST", "memory not allogate"); return ;}
        FILE *fd = fopen(name1, "r");
        if (fd == NULL) {ESP_LOGE("TEST", "%s not open",name1); return ;}
        fread(buf, fsize, 1, fd);
        fclose(fd);
        
        DynamicJsonDocument doc(fsize+5);
        DeserializationError error = deserializeJson(doc, buf);
        if (error) {
          ESP_LOGE("TEST","deserializeJson() failed: %s",error.c_str());
          return;
        } 

        doc.clear();                       
        free(buf);
    } 
}


/*

curl -X POST https://us-central1-telefon05.cloudfunctions.net/sendAlarmNotification \
-H "Content-Type: application/json" \
-d '{                                                 
  "data": {
    "type": "license",
    "target":"SMQLC01",           
    "title": "🚨 UYAAAANnnnnnn",
    "body": "Ses modülü tetikleniyor...",
    "text": "Naber çağlayan"
  }              
}'                     
      

curl -X POST https://us-central1-telefon05.cloudfunctions.net/sendAlarmNotification \
-H "Content-Type: application/json" \
-d '{
  "data": {
    "type": "license",
    "target":"SMQLC01", 
    "title": "🚨 UYAAAANnnnnnn",
    "body": "Ses modülü tetikleniyor...",
    "text": "Naber çağlayan"
  }
}'


*/