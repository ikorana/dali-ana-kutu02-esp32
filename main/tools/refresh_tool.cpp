



void rFileScanTask(void *pvParameters) {
    finder_t *finder= (finder_t *)pvParameters;
    EventBits_t bitToSet = (1 << (finder->task_id - 1)); // 1 için TASK_1_BIT, 2 için TASK_2_BIT...

    for (uint8_t i=0;i<MAX_GEAR;i++) {
         gear_t gg={};
         finder->gear->read_gear(i,&gg);
         vTaskDelay(pdMS_TO_TICKS(5));
        if (gg.short_addr!=0xFF) {
            finder_data_t item = {gg.kanal, gg.short_addr};
            xQueueSend(xDataQueue, &item, portMAX_DELAY);
        }
    }
    xEventGroupSetBits(fileEventGroup, bitToSet);   
    vTaskDelete(NULL);
}


// Dönüş: cihazın güncel parlaklık/seviyesi (0=kapalı) — refresh_device() bunu
// lamba/perde açık-kapalı sayımı için kullanır.
uint8_t sendStatus(uint8_t adr, uint8_t kanal, pck_t *pck, bool is_mqtt) {

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", ((adr<<1)|1));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    cJSON_AddNumberToObject(root, "special",14);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    xQueueReset(searchQueue);
    send_STM(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(20000));
    xSemaphoreGive(searchQueueMutex);
    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "status");
    cJSON_AddNumberToObject(pay, "adres", adr);
    cJSON_AddNumberToObject(pay, "kanal", kanal);
    cJSON_AddNumberToObject(pay, "grp0", msg.name[0]);
    cJSON_AddNumberToObject(pay, "grp1", msg.name[1]);
    cJSON_AddNumberToObject(pay, "level", msg.name[2]);
    send_AK(pay,pck,is_mqtt);

   if (kanal==1) gear01.set_status(adr,msg.name[0],msg.name[1],msg.name[2]);
   if (kanal==2) gear02.set_status(adr,msg.name[0],msg.name[1],msg.name[2]);
   if (kanal==3) gear03.set_status(adr,msg.name[0],msg.name[1],msg.name[2]);

   return msg.name[2];
}

void refresh_device(void *pvParameters) {
  search_task_param_t *param = (search_task_param_t *)pvParameters; 
  ESP_LOGI("REFRESH_TASK", "Refresh işlemi başlatıldı...");

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "refresh");
    send_AK(pay,&param->pck, param->is_mqtt);
    EventBits_t expected_bits = 0;

    fileEventGroup = xEventGroupCreate();;
    xDataQueue = xQueueCreate(50, sizeof(finder_data_t));

    if (GlobalConfig.kanal1==1) {
        static finder_t f1 = {&gear01, 0xFF, 1};
        expected_bits |= TASK_1_BIT;
        xTaskCreatePinnedToCore(
            rFileScanTask,           // Task fonksiyonunun adı
            "scan1",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü
            &f1,               // Task'a parametre gönderiliyor
            1,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            1                   // Çalışacağı çekirdek (0 veya 1)
        );
    }
    if (GlobalConfig.kanal2==1) {
        static finder_t f2 = {&gear02, 0xFF, 2};
        expected_bits |= TASK_2_BIT;
        xTaskCreatePinnedToCore(
            rFileScanTask,           // Task fonksiyonunun adı
            "scan2",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü
            &f2,               // Task'a parametre gönderiliyor
            1,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            1                   // Çalışacağı çekirdek (0 veya 1)
        );
    }
    if (GlobalConfig.kanal3==1) {
        static finder_t f3 = {&gear03, 0xFF, 3};
        expected_bits |= TASK_3_BIT;
        xTaskCreatePinnedToCore(
            rFileScanTask,           // Task fonksiyonunun adı
            "scan3",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü
            &f3,               // Task'a parametre gönderiliyor
            1,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            0                   // Çalışacağı çekirdek (0 veya 1)
        );
    } 
    if (GlobalConfig.kanal4==1) {
        static finder_t f4 = {&gear04, 0xFF, 4};
        expected_bits |= TASK_4_BIT;
        xTaskCreatePinnedToCore(
            rFileScanTask,           // Task fonksiyonunun adı
            "scan4",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü
            &f4,               // Task'a parametre gönderiliyor
            1,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            0                   // Çalışacağı çekirdek (0 veya 1)
        );
    } 

    {
        static finder_t f5 = {&gear10, 0xFF, 5};
        expected_bits |= TASK_5_BIT;
        xTaskCreatePinnedToCore(
            rFileScanTask,           // Task fonksiyonunun adı
            "scan5",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü
            &f5,               // Task'a parametre gönderiliyor
            1,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            1                   // Çalışacağı çekirdek (0 veya 1)
        );
    } 
    

    uint8_t count=0;
    // Lamba/perde/priz açık-kapalı sayımı — tarama bitince tek bir "say"
    // özetinde bildirilir. Yerel (kanal 10) ext_type projenin kendi kodu
    // (lamba=0x76, perde=0x77, priz=0x7C); gerçek DALI kanallarında (1-3)
    // lamba balastı 0x06 raporlar, perde ise (özel/tip 0x07 cihaz) yine 0x77
    // kullanır — bkz. apply_category_fanout. Priz şu an DALI tarafında
    // wifi_search_device'de tanımlı bir tip olmadığından pratikte hep 0
    // kalabilir, kontrol yine de zararsız/ileriye dönük bırakıldı.
    uint16_t lamba_acik=0, lamba_kapali=0, perde_acik=0, perde_kapali=0, priz_acik=0, priz_kapali=0;
    xEventGroupClearBits(fileEventGroup, expected_bits);

    while (1) {
        EventBits_t current_bits = xEventGroupGetBits(fileEventGroup);
        if ((current_bits & expected_bits) == expected_bits)
        {
            xEventGroupClearBits(fileEventGroup, expected_bits);
            break; // Döngüden çık
        }
        int toplam_sonuc = uxQueueMessagesWaiting(xDataQueue);
        if (toplam_sonuc > 0) {
            finder_data_t Item;
            while (xQueueReceive(xDataQueue, &Item, 0) == pdPASS) {
                count++;
                if (Item.channel>0 && Item.channel<4) {
                    uint8_t level = sendStatus(Item.short_addr, Item.channel, &param->pck, param->is_mqtt);
                    Gear *kanal_gear = (Item.channel==1) ? &gear01 : (Item.channel==2) ? &gear02 : &gear03;
                    uint8_t ext = 0xFF;
                    for (gear_room_entry_t &re : kanal_gear->get_room_entries()) {
                        if (re.short_addr == Item.short_addr) { ext = re.ext_type; break; }
                    }
                    bool acik = level > 0;
                    if (ext == 0x06) { if (acik) lamba_acik++; else lamba_kapali++; }
                    if (ext == 0x77) { if (acik) perde_acik++; else perde_kapali++; }
                    if (ext == 0x7C) { if (acik) priz_acik++; else priz_kapali++; }
                }
                if (Item.channel==10) {
                    gear_t gg={};
                    gear10.read_gear(Item.short_addr,&gg);
                    cJSON *pay = cJSON_CreateObject();
                    cJSON_AddStringToObject(pay, "com", "status");
                    cJSON_AddNumberToObject(pay, "adres", Item.short_addr);
                    cJSON_AddNumberToObject(pay, "kanal", 10);
                    cJSON_AddNumberToObject(pay, "grp0", gg.spec.group[0]);
                    cJSON_AddNumberToObject(pay, "grp1", gg.spec.group[1]);
                    cJSON_AddNumberToObject(pay, "level", gg.spec.status);
                    send_AK(pay,&param->pck,param->is_mqtt);
                    vTaskDelay(50/portTICK_PERIOD_MS);
                    cJSON *pay0 = cJSON_CreateObject();
                    cJSON_AddStringToObject(pay0, "com", "search");
                    cJSON_AddNumberToObject(pay0, "count", count);
                    send_AK(pay0,&param->pck,param->is_mqtt);

                    bool acik10 = gg.spec.status > 0;
                    if (gg.ext_type == 0x76) { if (acik10) lamba_acik++; else lamba_kapali++; }
                    if (gg.ext_type == 0x77) { if (acik10) perde_acik++; else perde_kapali++; }
                    if (gg.ext_type == 0x7C) { if (acik10) priz_acik++; else priz_kapali++; }
                }
                //ESP_LOGI("REFRESH_TASK", "%d Cihaz bulundu - Kanal: %d, Short Address: %d", count, Item.channel, Item.short_addr);
            }
        } else vTaskDelay(pdMS_TO_TICKS(500)); 
    }
    

/* 
  uint8_t count=0;
  bool mq = param->is_mqtt;

  if (GlobalConfig.kanal1==1) gear01.refresh(&count, &param->pck, send_AK, sendStatus, mq);
  if (GlobalConfig.kanal2==1) gear02.refresh(&count, &param->pck, send_AK, sendStatus, mq);
  if (GlobalConfig.kanal3==1) gear03.refresh(&count, &param->pck, send_AK, sendStatus, mq);
  if (GlobalConfig.kanal4==1) gear04.refresh(&count, &param->pck, send_AK, sendStatus, mq);
  gear10.refresh(&count, &param->pck, send_AK, sendStatus, mq);

  pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "refresh");
    cJSON_AddNumberToObject(pay, "count", 255);
    send_AK(pay,&param->pck, param->is_mqtt);
     */

    // Tarama sonucunu (lamba/perde/priz açık-kapalı) tek bir "say" ile bildir.
    char *ozet = NULL;
    asprintf(&ozet, "Tarama tamamlandı. %d lamba açık, %d lamba kapalı, %d perde açık, %d perde kapalı, %d priz açık, %d priz kapalı.",
             lamba_acik, lamba_kapali, perde_acik, perde_kapali, priz_acik, priz_kapali);
    cJSON *sayPay = cJSON_CreateObject();
    cJSON_AddStringToObject(sayPay, "com", "say");
    cJSON_AddStringToObject(sayPay, "txt", ozet);
    send_AK(sayPay, &param->pck, param->is_mqtt);
    free(ozet);

    cJSON *pay2 = cJSON_CreateObject();
    cJSON_AddStringToObject(pay2, "com", "refresh");
    cJSON_AddNumberToObject(pay2, "count", 255);
    send_AK(pay2,&param->pck, param->is_mqtt);

  // Task bitişinde derin kopyalanmış verileri temizliyoruz
  cJSON_Delete(param->pck.pck);
  free(param->pck.rem);
  cJSON_Delete(param->payload);
  free(param);

  ESP_LOGI("REFRESH_TASK", "Refresh işlemi BITTI...");

  vTaskDelete(NULL);
}