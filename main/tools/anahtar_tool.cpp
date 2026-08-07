

#include "cJSON.h"
typedef struct {
    uint8_t level;
    uint8_t id;
    uint8_t command;
    TaskHandle_t task;
    pck_t *pck;
    bool is_mqtt;
} task_par_t;

typedef struct {
    Gear *gear;
    uint8_t id;
    uint8_t task_id;
} finder_t;

typedef struct {
    uint8_t channel;
    uint8_t short_addr;
} finder_data_t;

#define TASK_1_BIT (1 << 0)
#define TASK_2_BIT (1 << 1)
#define TASK_3_BIT (1 << 2)
#define TASK_4_BIT (1 << 3)
#define TASK_5_BIT (1 << 4)
//#define ALL_TASKS_BIT (TASK_1_BIT | TASK_2_BIT | TASK_3_BIT | TASK_4_BIT | TASK_5_BIT)

EventGroupHandle_t fileEventGroup ;
QueueHandle_t xDataQueue ;

#define AN_RETRY_COUNT 3
#define DEL_TM 100

static void anahtar_onaction(uint8_t cmd, uint8_t level, gear_t *gg) 
{
    uint8_t adr = gg->short_addr;
    uint8_t ret = 0, cmd0 = 0x00;

        switch (cmd) {
            case 1 : cmd0 = 0xFF; break;
            case 2 : cmd0 = 0x00; break;
            case 3 : cmd0 = 0xFF; break;
            case 4 : cmd0 = 0xEE; break;    
            default : cmd0 = 0x00;
        }
         
        uint32_t komut = 0x00;

       // printf("type : %02X : %02X cmd:cmd0 %d:%d\n",gg->type,gg->ext_type,cmd,cmd0);

        if (cmd0 == 0xFF) {
            if (gg->type == 0x06) komut = create_command(adr, false, true, level);
            if (gg->type == 0x07 && gg->ext_type == 0x07) {komut = create_command(adr, false, false, 0x05);}// Max Level Röle
            if (gg->type == 0x07 && gg->ext_type == 0x77) {komut = create_command(adr, false, false, 0x05);} // Max Level Perde
            if (gg->type == 0x07 && gg->ext_type == 0x7C) {komut = create_command(adr, false, false, 0x05); } // Max Level Priz
        } else if (cmd0 == 0x00) {
            if (gg->type == 0x06) komut = create_command(adr, false, false, 0x00);
            if (gg->type == 0x07 && gg->ext_type == 0x07) {komut = create_command(adr, false, false, 0x00);} // Kapat
            if (gg->type == 0x07 && gg->ext_type == 0x77) {komut = create_command(adr, false, false, 0x06); } // Kapat
            if (gg->type == 0x07 && gg->ext_type == 0x7C) {komut = create_command(adr, false, false, 0x00); } // Priz
        } else if (cmd0 == 0xEE) {
            komut = create_command(adr, false, false, 0xEE); // Toggle
        }

        

        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "send");
        cJSON_AddNumberToObject(root, "hexcom", komut);
        cJSON_AddNumberToObject(root, "kanal", gg->kanal);
        cJSON_AddNumberToObject(root, "bit", 16);
        if (cmd0 == 0xEE) cJSON_AddNumberToObject(root, "special", 16);

   // printf("anahtar_onaction Komut: %02X, Level: %d, Adr: %d, Kanal: %d\n", cmd0, level, adr, gg->kanal);    

    for (int retry = 0; retry < AN_RETRY_COUNT; retry++) 
    {
        if (gg->kanal > 0 && gg->kanal < 10) send_STM(root);
        if (gg->kanal == 0) send_wifi(root);
        if (gg->kanal == 10) {
            for (Base_Device* cihaz : cihaz_listesi) {
                if (cihaz->device_id == adr) {
                   // printf("On Action Level:%d\n",level);
                    if (cmd0 == 0xFF) cihaz->set_power(0xFE);
                    if (cmd0 == 0x00) cihaz->off();
                }
            }
            cJSON_Delete(root);
            break; // Lokal cihazlarda retry gerekmez
        }
        cJSON_Delete(root);

        // Toggle komutu için ret kontrolü önemsiz kabul edildiğinden bir kez gönderip çıkıyoruz
        if (cmd0 == 0xEE) break;

        vTaskDelay(DEL_TM / portTICK_PERIOD_MS);

        // Yandı mı / Kapandı mı kontrolü için Actual Level Sorgula
        cJSON *root0 = cJSON_CreateObject();
        cJSON_AddStringToObject(root0, "com", "query");
        cJSON_AddNumberToObject(root0, "hexcom", create_command(adr, false, false, QUERY_ACTUAL_LEVEL));
        cJSON_AddNumberToObject(root, "kanal", gg->kanal);
        cJSON_AddNumberToObject(root0, "bit", 16);
        if (gg->kanal > 0) send_STM(root0);
        if (gg->kanal == 0) send_wifi(root0);
        cJSON_Delete(root0);

        searchMessage_t msg = {};
        xQueueReset(searchQueue);
        // Cevap için 3 saniye bekle
        if (xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(3000)) == pdPASS) {
            ret = msg.name[0];
            if (cmd0 == 0xFF && ret > 0) {
                ESP_LOGI("AN_ACTION", "Basarili: Cihaz ACILDI (Adr:%d, Ret:%d)", adr, ret);
                break;
            }
            if (cmd0 == 0x00 && ret < 10) {
                ESP_LOGI("AN_ACTION", "Basarili: Cihaz KAPANDI (Adr:%d, Ret:%d)", adr, ret);
                break;
            }
        }
        
        if (retry < AN_RETRY_COUNT - 1) {
            ESP_LOGW("AN_ACTION", "Retry %d... Adr:%d Komut:%02X", retry + 1, adr, cmd0);
            vTaskDelay(DEL_TM / portTICK_PERIOD_MS);
        }
    }
}

void vFileScanTask(void *pvParameters) {
    finder_t *finder= (finder_t *)pvParameters;
    EventBits_t bitToSet = (1 << (finder->task_id - 1)); // 1 için TASK_1_BIT, 2 için TASK_2_BIT...

    for (uint8_t i=0;i<MAX_GEAR;i++) {
         gear_t gg={};
         finder->gear->read_gear(i,&gg);
         vTaskDelay(pdMS_TO_TICKS(5));
        if (gg.anahtar==finder->id) {
            finder_data_t item = {gg.kanal, gg.short_addr};
            xQueueSend(xDataQueue, &item, portMAX_DELAY);
        }
    }
    xEventGroupSetBits(fileEventGroup, bitToSet);   
    vTaskDelete(NULL);
}

static void anahtar_on_task(void *pvParameters)
{
     task_par_t *ff = (task_par_t *)pvParameters;

     fileEventGroup = xEventGroupCreate();;
     xDataQueue = xQueueCreate(20, sizeof(finder_data_t));
     EventBits_t expected_bits = 0;

     if (GlobalConfig.kanal1==1) {
        static finder_t f1 = {&gear01, ff->id, 1};
        f1.id = ff->id;
        expected_bits |= TASK_1_BIT;
        xTaskCreatePinnedToCore(
            vFileScanTask,           // Task fonksiyonunun adı
            "scan1",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü
            &f1,               // Task'a parametre gönderiliyor
            1,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            1                   // Çalışacağı çekirdek (0 veya 1)
        );
    }
    if (GlobalConfig.kanal2==1) {
        static finder_t f2 = {&gear02, ff->id, 2};
        f2.id = ff->id;
        expected_bits |= TASK_2_BIT;
        xTaskCreatePinnedToCore(
            vFileScanTask,           // Task fonksiyonunun adı
            "scan2",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü
            &f2,               // Task'a parametre gönderiliyor
            1,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            1                   // Çalışacağı çekirdek (0 veya 1)
        );
    }
    if (GlobalConfig.kanal3==1) {
        static finder_t f3 = {&gear03, ff->id, 3};
        f3.id = ff->id;
        expected_bits |= TASK_3_BIT;
        xTaskCreatePinnedToCore(
            vFileScanTask,           // Task fonksiyonunun adı
            "scan3",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü
            &f3,               // Task'a parametre gönderiliyor
            1,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            0                   // Çalışacağı çekirdek (0 veya 1)
        );
    } 
    if (GlobalConfig.kanal4==1) {
        static finder_t f4 = {&gear04, ff->id, 4};
        f4.id = ff->id;
        expected_bits |= TASK_4_BIT;
        xTaskCreatePinnedToCore(
            vFileScanTask,           // Task fonksiyonunun adı
            "scan4",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü
            &f4,               // Task'a parametre gönderiliyor
            1,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            0                   // Çalışacağı çekirdek (0 veya 1)
        );
    }   
    {
        static finder_t f5 = {&gear10, ff->id, 5};
        f5.id = ff->id;
        expected_bits |= TASK_5_BIT;
        xTaskCreatePinnedToCore(
            vFileScanTask,           // Task fonksiyonunun adı
            "scan5",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü
            &f5,               // Task'a parametre gönderiliyor
            1,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            1                   // Çalışacağı çekirdek (0 veya 1)
        );
    }   
    

    while (1) {
        EventBits_t current_bits = xEventGroupGetBits(fileEventGroup);

        if ((current_bits & expected_bits) == expected_bits) 
        {
            // Döngüden çıkmadan önce kuyrukta kalan son veriler varsa onları da eritelim
            if (uxQueueMessagesWaiting(xDataQueue) == 0) {
                xEventGroupClearBits(fileEventGroup, expected_bits);
                break; // Döngüden güvenli çıkış
            }
        }

        finder_data_t Item;
        if (xQueueReceive(xDataQueue, &Item, pdMS_TO_TICKS(50)) == pdPASS) {
            gear_t gg = {};
            
           // printf("                 Channel: %d, Short Address: %d", Item.channel, Item.short_addr);

            // İlk okuma
            if (Item.channel == 1)  gear01.read_gear(Item.short_addr, &gg); 
            else if (Item.channel == 2)  gear02.read_gear(Item.short_addr, &gg); 
            else if (Item.channel == 3)  gear03.read_gear(Item.short_addr, &gg); 
            else if (Item.channel == 0)  gear04.read_gear(Item.short_addr, &gg); // HATA DÜZELTİLDİ: 0 -> 4 yapıldı
            else if (Item.channel == 10) gear10.read_gear(Item.short_addr, &gg);
            
            // Aksiyon alınıyor
            anahtar_onaction(ff->command, ff->level, &gg);   

            // İkinci okuma (Eğer aksiyon sonrası durum değişiyorsa kalabilir, değişmiyorsa bu bloğu silebilirsiniz)
            if (Item.channel == 1)  gear01.read_gear(Item.short_addr, &gg); 
            else if (Item.channel == 2)  gear02.read_gear(Item.short_addr, &gg); 
            else if (Item.channel == 3)  gear03.read_gear(Item.short_addr, &gg); 
            else if (Item.channel == 0)  gear04.read_gear(Item.short_addr, &gg); // HATA DÜZELTİLDİ: 0 -> 4 yapıldı
            else if (Item.channel == 10) gear10.read_gear(Item.short_addr, &gg);
            
            //printf("Channel: %d %d, Short Address: %d %d level:%d\n", Item.channel, gg.kanal, Item.short_addr, gg.short_addr, gg.spec.status);
            
            // JSON işlemleri
            cJSON *pay0 = cJSON_CreateObject();  
            if (pay0 != NULL) {
                cJSON_AddStringToObject(pay0, "com", "get_level");
                cJSON_AddNumberToObject(pay0, "adres", Item.short_addr);
                cJSON_AddNumberToObject(pay0, "kanal", Item.channel);
                cJSON_AddNumberToObject(pay0, "power", gg.spec.status);
                
                char *dat = cJSON_PrintUnformatted(pay0);
                if (dat != NULL) {
                    udp_server.send_unicast_all((uint8_t *)dat, strlen(dat));
                    //printf("Udp >> %s\n", dat);
                    cJSON_free(dat); 
                }
                cJSON_Delete(pay0);
            }
            vTaskDelay(pdMS_TO_TICKS(100)); 
        } else {
            // Kuyruk boşsa ve tasklar henüz bitmediyse biraz dinlen
            vTaskDelay(pdMS_TO_TICKS(200)); 
        }
    }    

    // Temizlik
    vEventGroupDelete(fileEventGroup);
    vQueueDelete(xDataQueue);
 
    if(ff->task != NULL) {
        xTaskNotifyGive(ff->task);
    }
    //printf("anahtar on TAsk BİTTİ\n");
    
    vTaskDelete(NULL);
}


//----------------------
static void bekle_anahtar_on_task(void *pvParameters)
{

     task_par_t *ff = (task_par_t *)pvParameters;
     
     // 1. Ana yapıyı ayır ve basit verileri kopyala
     task_par_t *param = (task_par_t *)malloc(sizeof(task_par_t));
     if (!param) { vTaskDelete(NULL); return; }
     
     param->id = ff->id;
     param->level = ff->level;
     param->command = ff->command;
     param->is_mqtt = ff->is_mqtt;
     param->task = xTaskGetCurrentTaskHandle();

     // 2. pck_t yapısını ve içeriğini derin kopyala
     param->pck = (pck_t *)malloc(sizeof(pck_t));
     param->pck->rem = (remote_t *)malloc(sizeof(remote_t));
     if (param->pck->rem) memcpy(param->pck->rem, ff->pck->rem, sizeof(remote_t));
     param->pck->pck = cJSON_Duplicate(ff->pck->pck, true);
     
     xTaskCreatePinnedToCore(
            anahtar_on_task,           // Task fonksiyonunun adı
            "ont",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü artırıldı
            param,               // Task'a kopyalanan parametre gönderiliyor
            2,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            1                   // Çalışacağı çekirdek (0 veya 1)
        );  

        cJSON *pay = cJSON_CreateObject();  
        cJSON_AddStringToObject(pay, "com", "an_level");
        cJSON_AddNumberToObject(pay, "id", ff->id);
        cJSON_AddNumberToObject(pay, "level", ff->level);
        send_AK(pay,param->pck,param->is_mqtt); 

        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000));

        pay = cJSON_CreateObject();  
        cJSON_AddStringToObject(pay, "com", "an_okey");
        cJSON_AddNumberToObject(pay, "id", ff->id);
       // send_AK(pay,ff->pck,is_mqtt); 
        send_AK(pay, param->pck,param->is_mqtt); 

    // 3. Task bitişinde tüm derin kopyalanmış alanları temizle
    if (param->pck) {
        if (param->pck->pck) cJSON_Delete(param->pck->pck);
        if (param->pck->rem) free(param->pck->rem);
        free(param->pck);
    }
    free(param);

  //  printf("BEKLE TAsk BİTTİ\n");

    vTaskDelete(NULL);

}

//----------------------
void command_an_save(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, room = 0;
    char *nm = (char*)calloc(1,25);

    JSON_getstring(payload,"nm", nm,24);    
    JSON_getint(payload, "id", &adr);
    JSON_getint(payload, "room", &room);

    switch_t sw = {};
    anahtar.get_switch(adr,&sw);
    sw.room = room;
    strcpy((char *)sw.name,nm);
    anahtar.set_switch(adr,&sw);   
    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "an_save");
    send_AK(pay,pck,is_mqtt);
}

//----------------------
void command_an_level(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0;
    
    /*switch levelını okuyup bildiriyoruz.*/
    JSON_getint(payload, "id", &adr);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "an_level");
    cJSON_AddNumberToObject(pay, "id", adr);
    cJSON_AddNumberToObject(pay, "level", anahtar.get_level(adr));
    send_AK(pay,pck,is_mqtt);
}
//----------------------
void command_an_on(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0;
    
    JSON_getint(payload, "id", &adr);

    switch_t sw = {};
    anahtar.get_switch(adr,&sw);
    
    if (sw.type==0) {
        if (sw.set_level<50) {
            sw.set_level = 200;
        }
        sw.level=sw.set_level;
        anahtar.set_switch(adr,&sw);
        vTaskDelay(5/portTICK_PERIOD_MS);
        
        static task_par_t aktar = {};
        aktar.command = 1; //Levela göre lambaları yak
        aktar.id = adr;
        aktar.level = sw.level;
        aktar.task = NULL;
        aktar.is_mqtt = is_mqtt;
        aktar.pck = pck;

        xTaskCreatePinnedToCore(
            bekle_anahtar_on_task,           // Task fonksiyonunun adı
            "ont0",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü artırıldı
            &aktar,               // Task'a kopyalanan parametre gönderiliyor
            2,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            1                   // Çalışacağı çekirdek (0 veya 1)
        );      

    }   
}
//----------------------
void command_an_off(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0;
    
    JSON_getint(payload, "id", &adr);

    switch_t sw = {};
    anahtar.get_switch(adr,&sw);
    
    if (sw.type==0) {
        sw.relay_id = 2; //Komut olarak ON 2 olursa OFF
        sw.level = 0;
        //lambayı levela göre yak
        anahtar.set_switch(adr,&sw);
        vTaskDelay(5/portTICK_PERIOD_MS);

        static task_par_t aktar = {};
        aktar.command = 2;
        aktar.id = adr;
        aktar.level = sw.level;
        aktar.task = NULL;
        aktar.is_mqtt = is_mqtt;
        aktar.pck = pck;

        xTaskCreatePinnedToCore(
            bekle_anahtar_on_task,           // Task fonksiyonunun adı
            "ont0",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü artırıldı
            &aktar,               // Task'a kopyalanan parametre gönderiliyor
            2,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            1                   // Çalışacağı çekirdek (0 veya 1)
        ); 

    }   
}

void command_an_power(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, level = 0;
    
    JSON_getint(payload, "id", &adr);
    JSON_getint(payload, "power", &level);

    switch_t sw = {};
    anahtar.get_switch(adr,&sw);
    
    if (sw.type==0) {
        sw.relay_id = 3; //Komut olarak ON 2 olursa OFF
        sw.level = level;
        sw.set_level = level;
        //lambayı levela göre yak
        anahtar.set_switch(adr,&sw);
        vTaskDelay(5/portTICK_PERIOD_MS);
        static task_par_t aktar = {};
        aktar.command = 3;
         aktar.id = adr;
        aktar.level = sw.level;
        aktar.task = NULL;
        aktar.is_mqtt = is_mqtt;
        aktar.pck = pck;

        xTaskCreatePinnedToCore(
            bekle_anahtar_on_task,           // Task fonksiyonunun adı
            "ont0",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü artırıldı
            &aktar,               // Task'a kopyalanan parametre gönderiliyor
            2,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            1                   // Çalışacağı çekirdek (0 veya 1)
        ); 
    }   
}

void command_an_toggle(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0;
    
    JSON_getint(payload, "id", &adr);

    switch_t sw = {};
    anahtar.get_switch(adr,&sw);
    
    if (sw.type==0) {
        //sw.relay_id = 1; //Komut olarak ON 2 olursa OFF
        if (sw.set_level<50) {
            sw.set_level = 200;
        }
        sw.level=sw.set_level;
        anahtar.set_switch(adr,&sw);
        vTaskDelay(50/portTICK_PERIOD_MS);
        
        static task_par_t aktar = {};
        aktar.command = 4; //TOGGLE
         aktar.id = adr;
        aktar.level = sw.level;
        aktar.task = NULL;
        aktar.is_mqtt = is_mqtt;
        aktar.pck = pck;

        xTaskCreatePinnedToCore(
            bekle_anahtar_on_task,           // Task fonksiyonunun adı
            "ont0",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü artırıldı
            &aktar,               // Task'a kopyalanan parametre gönderiliyor
            2,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            1                   // Çalışacağı çekirdek (0 veya 1)
        ); 
    }   
}