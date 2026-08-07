#include "base/irrigation.h"
#include "cJSON.h"
#include "jsontool.h"

#include "dali_tool.cpp"

EventGroupHandle_t alarm_group = xEventGroupCreate();
#define BIT_ALARM (1 << 0)


void alarm_active(void *pvParameters) {
  search_task_param_t *param = (search_task_param_t *)pvParameters; 
  
  ESP_LOGI("ALARM_TASK", "Alarm Açma Süreci Başlatıldı...");

    //Alarmın Açılması için beklenecek süre
    TickType_t xTicksToWait = pdMS_TO_TICKS(10000);

    // 10 saniye boyunca BIT_MY_VARIABLE_IS_ONE bitinin 1 olmasını BEKLE
    EventBits_t uxBits = xEventGroupWaitBits(
        alarm_group,
        BIT_ALARM,
        pdTRUE,        // Uyanınca biti otomatik temizle (Clear on exit)
        pdFALSE,       // Sadece bu bitin 1 olmasını bekle (Wait for all değil)
        xTicksToWait   // Zaman aşımı süresi
    );

    if ((uxBits & BIT_ALARM) != 0) {
            // Değişken 10 saniye içinde 1 oldu!
            ESP_LOGI("ALARM_TASK", "Alarm AÇILMADI...");
            vTaskDelete(NULL);
            return;
        } 

    GlobalConfig.alarm = 2;
    disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "set_alarm");
    cJSON_AddNumberToObject(pay, "state", 2);
    send_AK(pay,&param->pck, param->is_mqtt);
    ESP_LOGI("ALARM_TASK", "Alarm AÇILDI...");

    // Task bitişinde derin kopyalanmış verileri temizliyoruz
    cJSON_Delete(param->pck.pck);
    free(param->pck.rem);
    cJSON_Delete(param->payload);
    free(param);


    vTaskDelete(NULL);

}


void Alarm_ON(remote_t *rem, cJSON *new_pck, pck_t *n_pck, cJSON *payload_item, bool is_mqtt) {
    search_task_param_t *param = (search_task_param_t *)malloc(sizeof(search_task_param_t));                 
    // Derin Kopya (Hard Copy) oluşturuluyor
    param->pck.rem = (remote_t *)malloc(sizeof(remote_t));
    memcpy(param->pck.rem, rem, sizeof(remote_t));
    param->pck.pck = cJSON_Duplicate(new_pck, true);
    param->payload = cJSON_Duplicate(payload_item, true);
    param->is_mqtt = is_mqtt;

    xEventGroupSetBits(alarm_group, BIT_ALARM);
    vTaskDelay(100/portTICK_PERIOD_MS); 
    xEventGroupClearBits(alarm_group, BIT_ALARM);

    xTaskCreatePinnedToCore(
            alarm_active,           // Task fonksiyonunun adı
            "rdev",          // Task'ın ismi (debug için)
            4096,               // Stack büyüklüğü artırıldı
            param,               // Task'a kopyalanan parametre gönderiliyor
            1,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            1                   // Çalışacağı çekirdek (0 veya 1)
        );

    GlobalConfig.alarm = 1;
    disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "set_alarm");
    cJSON_AddNumberToObject(pay, "state", 1);
    send_AK(pay,n_pck, is_mqtt);    
}

void Alarm_OFF(pck_t *n_pck, bool is_mqtt) {

    xEventGroupSetBits(alarm_group, BIT_ALARM);
    vTaskDelay(100/portTICK_PERIOD_MS); 

    GlobalConfig.alarm = 0;
    GlobalConfig.alarm_aktif=0;
    disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "set_alarm");
    cJSON_AddNumberToObject(pay, "state", 0);
    send_AK(pay,n_pck, is_mqtt);

    cJSON *pay2 = cJSON_CreateObject(); 
    uint8_t evid = find_event_id("ALARM_STATE_EVENT"); 
    cJSON_AddStringToObject(pay2, "com", "EVENTS");
    cJSON_AddStringToObject(pay2, "event", "ALARM_STATE_EVENT");
    cJSON_AddNumberToObject(pay2, "event_id", evid);
    cJSON_AddNumberToObject(pay2, "state", 0);
    send_events(pay2);
    cJSON_Delete(pay2);

}



void com_process_task(void *pvParameters) {

    search_task_param_t *param = (search_task_param_t *)pvParameters; 

    char *command = (char *)calloc(1,25);    
    JSON_getstring(param->payload,"com", command,25);
    ESP_LOGI("COM_PROCESS","Command :%s",command);

    if (strcmp(command,"off")==0) command_off(param->payload,&param->pck,param->is_mqtt);
    if (strcmp(command,"arc_power")==0) command_arc(param->payload,&param->pck,param->is_mqtt);
    if (strcmp(command,"action")==0) command_action(param->payload,&param->pck,param->is_mqtt); 
    if (strcmp(command,"qstatus")==0) command_qstatus(param->payload,&param->pck,param->is_mqtt);
    if (strcmp(command,"get_level")==0) command_get_level(param->payload,&param->pck,param->is_mqtt); 
    if (strcmp(command,"identfy")==0) command_identfy(param->payload,&param->pck,param->is_mqtt);  
    
    if (strcmp(command,"get_gurup")==0) command_get_gurup(param->payload,&param->pck,param->is_mqtt);
    if (strcmp(command,"set_gurup")==0) command_set_gurup(param->payload,&param->pck,param->is_mqtt);
    if (strcmp(command,"get_scene")==0) command_get_scene(param->payload,&param->pck,param->is_mqtt);
    if (strcmp(command,"set_scene")==0) command_set_scene(param->payload,&param->pck,param->is_mqtt);
    if (strcmp(command,"del_scene")==0) command_del_scene(param->payload,&param->pck,param->is_mqtt);

    if (strcmp(command,"get_detail")==0) command_get_detail(param->payload,&param->pck,param->is_mqtt);   
    if (strcmp(command,"set_min")==0) command_set_level(param->payload,&param->pck, SET_MIN_LEVEL,"set_min",param->is_mqtt);
    if (strcmp(command,"set_max")==0) command_set_level(param->payload,&param->pck, SET_MAX_LEVEL,"set_max",param->is_mqtt);
    if (strcmp(command,"set_fail")==0) command_set_level(param->payload,&param->pck, SET_FAIL_LEVEL,"set_fail",param->is_mqtt);
    if (strcmp(command,"set_pwr")==0) command_set_level(param->payload,&param->pck,SET_POWERON_LEVEL,"set_pwr",param->is_mqtt);
    if (strcmp(command,"set_fade")==0) command_set_fade(param->payload,&param->pck, param->is_mqtt);
    if (strcmp(command,"set_efade")==0) command_set_efade(param->payload,&param->pck, param->is_mqtt);

    if (strcmp(command,"get_min")==0) command_get_single(param->payload,&param->pck,QUERY_MIN_LEVEL,"get_min",param->is_mqtt);
    if (strcmp(command,"get_max")==0) command_get_single(param->payload,&param->pck,QUERY_MAX_LEVEL,"get_max",param->is_mqtt);
    if (strcmp(command,"get_fail")==0) command_get_single(param->payload,&param->pck,QUERY_FAILURE_LEVEL,"get_fail",param->is_mqtt);
    if (strcmp(command,"get_pwr")==0) command_get_single(param->payload,&param->pck,QUERY_POWERON_LEVEL,"get_pwr",param->is_mqtt);

    if (strcmp(command,"modbus_adr")==0) command_modbus_adr(param->payload,&param->pck, false);
    if (strcmp(command,"modbus_getadr")==0) command_modbus_getadr(param->payload,&param->pck, false);
    if (strcmp(command,"modbus_identfy")==0) command_modbus_identfy(param->payload,&param->pck, false);
    if (strcmp(command,"modbus_ping")==0) command_modbus_ping(param->payload,&param->pck, false);
    if (strcmp(command,"pin")==0) command_pin(param->payload,&param->pck, false);

    if (strcmp(command,"weather")==0) command_weather(param->payload,&param->pck, param->is_mqtt);


    if (strcmp(command,"stop_irrigation")==0) {
            irrigation->schedule_stop();
            vTaskDelay(100/portTICK_PERIOD_MS); 
    };
    if (strcmp(command,"run_schedule")==0) {
            char *tm = (char *)calloc(1,25);    
            JSON_getstring(param->payload,"time", tm,25);
            irrigation->schedule_run(tm);
            vTaskDelay(100/portTICK_PERIOD_MS); 
            free(tm);
    };

    if (strcmp(command,"chg_irr")==0) {
            uint8_t sm=0;
            char *tm = (char *)calloc(1,25);    
            JSON_getstring(param->payload,"sistem", tm,25);
            JSON_getint(param->payload,"smart", &sm);
            //işlem yap
            vTaskDelay(100/portTICK_PERIOD_MS); 
            free(tm);
    };

    if (strcmp(command,"run_irrpin")==0) {
            uint8_t seg=0, rel=0,tm=0;
            JSON_getint(param->payload,"segment_id", &seg);
            JSON_getint(param->payload,"relay_id", &rel);
            JSON_getint(param->payload,"time",&tm);
            if (irrigation->get_status()==0) {
                irrigation->run_pin(seg,rel,tm);
            }
           
            //işlem yap
            vTaskDelay(100/portTICK_PERIOD_MS); 
  
    };

    if (strcmp(command,"get_irrigation")==0) {
        if (irrigation !=nullptr) {
            cJSON *pay = cJSON_CreateObject();  
            cJSON_AddStringToObject(pay, "com", "get_irrigation");
            cJSON_AddNumberToObject(pay,"running",irrigation->get_status());
            send_AK(pay,&param->pck, param->is_mqtt);
        }
    };
    

    // Task bitişinde derin kopyalanmış verileri temizliyoruz
    cJSON_Delete(param->pck.pck);
    free(param->pck.rem);
    cJSON_Delete(param->payload);
    free(param);

    free(command);
    vTaskDelete(NULL);
}



void udp_worker_task(void *pvParameters) {
    udp_msg_t *msg = NULL;
    ESP_LOGI("UDP_WORKER", "UDP İşleme Taskı Başlatıldı.");

    while (1) {
       // Kuyruktan veri gelmesini bekle
        if (xQueueReceive(udp_processing_queue, &msg, portMAX_DELAY))
        {
            ESP_LOGI("UDP_WORKER", "[%s:%d] %s %s-> Payload: %s", 
                     ipaddr_ntoa(&msg->remote->addr), 
                     msg->remote->port,
                     msg->is_broadcast ? "BROADCAST" : "UNICAST",
                     msg->is_mqtt ? "MQTT" : "UDP",
                     (char*)msg->payload);
                     
            cJSON *rcv_json = cJSON_Parse((const char *)msg->payload);
            if (rcv_json==nullptr) {
                ESP_LOGI("UDP WORKER","Gelen data JSON degil");
               
                
                if (strcmp((const char *)msg->payload,"list_ins")==0) {
                    instance.list_instance();
                }
                if (strcmp((const char *)msg->payload,"delete_ins")==0) {
                    instance.clear_file();
                }
                if (strcmp((const char *)msg->payload,"list_room")==0) {
                    room.list_room();
                }
                if (strcmp((const char *)msg->payload,"list_switch")==0) {
                    anahtar.list_switch();
                }

                if (strcmp((const char *)msg->payload,"list_gear1")==0) {
                       gear01.list_gear(); 
                }
                if (strcmp((const char *)msg->payload,"list_gear2")==0) {
                       gear02.list_gear();
                }
                if (strcmp((const char *)msg->payload,"list_gear3")==0) {
                       gear03.list_gear();
                }
                if (strcmp((const char *)msg->payload,"list_gear4")==0) {
                       gear04.list_gear();
                }
                if (strcmp((const char *)msg->payload,"list_gear10")==0) {
                       gear10.list_gear();
                }

                if (strcmp((const char *)msg->payload,"list_fcm")==0) {
                       fcm.list_fcm();
                }
                if (strcmp((const char *)msg->payload,"format_fcm")==0) {
                       fcm.file_format();
                }
                if (strcmp((const char *)msg->payload,"empty_fcm")==0) {
                       fcm.file_emty();
                }
                

                free(msg->remote);
                free(msg->payload);
                free(msg); 
                continue;
            }
            
            cJSON *pck_item = cJSON_GetObjectItem(rcv_json, "pck");
            cJSON *payload_item = cJSON_GetObjectItem(rcv_json, "payload"); 

            if (payload_item==nullptr && pck_item==nullptr) {
               char *command = (char *)calloc(1,25);    
               JSON_getstring(rcv_json,"com", command,25);
               //ESP_LOGI("UDP_WORKER_W","Command : %s",command);

               if (strcmp(command,"wanswer")==0) {
                  uint8_t ret = 0xff;
                  JSON_getint(rcv_json,"ret", &ret);
                  xQueueSend(IntQueue, (void *)&ret, (TickType_t)10);
                  searchMessage_t msg = {};
                  msg.name[0] = ret;
                  xQueueSend(searchQueue, (void *)&msg, pdMS_TO_TICKS(10));

               }
               if (strcmp(command,"wresponse")==0 || strcmp(command,"wsresponse")==0 ) {
                    searchMessage_t msg = {};
                    uint8_t a0=0;
                    if (JSON_getint(rcv_json,"A0", &a0)) msg.name[0] = a0;
                    if (JSON_getint(rcv_json,"A1", &a0)) msg.name[1] = a0;
                    if (JSON_getint(rcv_json,"A2", &a0)) msg.name[2] = a0;
                    if (JSON_getint(rcv_json,"A3", &a0)) msg.name[3] = a0;
                    if (JSON_getint(rcv_json,"A4", &a0)) msg.name[4] = a0;
                    if (JSON_getint(rcv_json,"A5", &a0)) msg.name[5] = a0;
                    if (JSON_getint(rcv_json,"A6", &a0)) msg.name[6] = a0;  
                    if (strcmp(command,"wresponse")==0) xQueueSend(searchQueue, (void *)&msg, pdMS_TO_TICKS(10));
                    if (strcmp(command,"wsresponse")==0) xQueueSend(wsearchQueue, (void *)&msg, pdMS_TO_TICKS(10));
                }

                free(msg->remote);
                free(msg->payload);
                free(msg); 
                continue;

            }

            if (pck_item==nullptr) {
                ESP_LOGE("UDP WORKER","pck verisi yok.");
            }
            if (payload_item==nullptr) {
                ESP_LOGE("UDP WORKER","payload verisi yok.");
            }   
            char *pkg_type=NULL;
            char *pkg_id=NULL;
            cJSON *new_pck = NULL;
            bool no_data_process = false;
            bool is_new_pck = false;

            if (pck_item!=nullptr) {
                pkg_type = (char *)calloc(1,5);
                pkg_id = (char *)calloc(1,25);
                JSON_getstring(pck_item,"type", pkg_type,5);
                JSON_getstring(pck_item,"id", pkg_id,25);

                //Gelen AK ise Paketi işleme
                if (strcmp(pkg_type,"AK")==0) {
                    free(pkg_type);
                    free(pkg_id);
                    no_data_process = true;
                }
                //Gelen Paket Unicast veya Broadcast ise yeni pck oluştur
                if (strcmp(pkg_type,"UN")==0 || strcmp(pkg_type,"BR")==0) {
                    new_pck = cJSON_CreateObject();
                    cJSON_AddStringToObject(new_pck, "type", "AK");
                    cJSON_AddStringToObject(new_pck, "id", pkg_id);
                    is_new_pck = true;
                }
                free(pkg_type);
                free(pkg_id);
            }

        
            if (!no_data_process && payload_item!=nullptr && is_new_pck) {
               /*payload var ve new_pck hazır durumda*/

               search_task_param_t *param = (search_task_param_t *)malloc(sizeof(search_task_param_t));
               // Derin Kopya (Hard Copy) oluşturuluyor
                  param->pck.rem = (remote_t *)malloc(sizeof(remote_t));
                  memcpy(param->pck.rem, msg->remote, sizeof(remote_t));
                  param->pck.pck = cJSON_Duplicate(new_pck, true);
                  param->payload = cJSON_Duplicate(payload_item, true);
                  param->is_mqtt = msg->is_mqtt;


                  xTaskCreatePinnedToCore(
                        com_process_task,           // Task fonksiyonunun adı
                        "cpt",          // Task'ın ismi (debug için)
                        4096,               // Stack büyüklüğü artırıldı
                        param,               // Task'a kopyalanan parametre gönderiliyor
                        1,                  // Öncelik sırası (0 en düşük)
                        NULL,               // Task handle (referans gerekmiyorsa NULL)
                        1                   // Çalışacağı çekirdek (0 veya 1)
                    );
               



               char *command = (char *)calloc(1,25);    
               JSON_getstring(payload_item,"com", command,25);
               ESP_LOGI("UDP_WORKER","Command :%s",command);
               
               
                pck_t *n_pck = (pck_t *)calloc(1,sizeof(pck_t));
                n_pck->rem = msg->remote;
                n_pck->pck = new_pck;
                 
                /* 
                   Bu iki komut mqtt den gönderilir. STM32 nin dali kanallarını açıp kapatmasını sağlar.
                   Default olarak kanallar kapalıdır. Açıldıgında esp32 bunu kaydeder ve ilk boot sırasında
                   kanalın tekrar açılmasını sağlar. 
                */
            /**/    if (strcmp(command,"dali")==0) channel_onoff(payload_item,n_pck, msg->is_mqtt);
            /**/    if (strcmp(command,"led")==0) led_onoff(payload_item,n_pck, msg->is_mqtt);


                
                /* Bu komutlar ESP32 ile ilgilidir*/
                if (strcmp(command,"dev_reset")==0) esp_restart();
                if (strcmp(command,"token")==0) set_token(payload_item,n_pck);
                
                if (strcmp(command,"alarm")==0) alarm_onoff(payload_item,n_pck);
                if (strcmp(command,"get_version")==0) send_version(n_pck);
                if (strcmp(command,"room_intro")==0) room_intro(n_pck);
                if (strcmp(command,"cre_oda")==0) room_create(payload_item,n_pck);
                if (strcmp(command,"del_oda")==0) room_delete(payload_item,n_pck);
                if (strcmp(command,"switch_intro")==0) switch_intro(n_pck);
                if (strcmp(command,"cre_switch")==0) switch_create(payload_item,n_pck);
                if (strcmp(command,"del_switch")==0) switch_delete(payload_item,n_pck);
                if (strcmp(command,"grpintro")==0) gurup_intro(n_pck);
                if (strcmp(command,"grpname")==0) gurup_name(payload_item,n_pck);
                if (strcmp(command,"scnintro")==0) scene_intro(n_pck);
                if (strcmp(command,"scnname")==0) scene_name(payload_item,n_pck);
                if (strcmp(command,"intro")==0) gear_intro(payload_item,n_pck);
                if (strcmp(command,"ins_intro")==0) instance_intro(n_pck);
                if (strcmp(command,"small_intro")==0) small_intro(payload_item,n_pck);

                if (strcmp(command,"voice")==0) {
                    search_task_param_t *param = (search_task_param_t *)malloc(sizeof(search_task_param_t));                 
                    // Derin Kopya (Hard Copy) oluşturuluyor
                    param->pck.rem = (remote_t *)malloc(sizeof(remote_t));
                    memcpy(param->pck.rem, msg->remote, sizeof(remote_t));
                    param->pck.pck = cJSON_Duplicate(new_pck, true);
                    param->payload = cJSON_Duplicate(payload_item, true);
                    param->is_mqtt = msg->is_mqtt;

                     xTaskCreatePinnedToCore(
                        firebase_voice_task,           // Task fonksiyonunun adı
                        "fbvdev",          // Task'ın ismi (debug için)
                        4096,               // Stack büyüklüğü artırıldı
                        param,               // Task'a kopyalanan parametre gönderiliyor
                        1,                  // Öncelik sırası (0 en düşük)
                        NULL,               // Task handle (referans gerekmiyorsa NULL)
                        1                   // Çalışacağı çekirdek (0 veya 1)
                    );



                }

                if (strcmp(command,"set_grp_time")==0) command_set_gurup_time(payload_item,n_pck, msg->is_mqtt);
                if (strcmp(command,"get_grp_time")==0) command_get_gurup_time(payload_item,n_pck, msg->is_mqtt);
                if (strcmp(command,"set_scn_time")==0) command_set_scene_time(payload_item,n_pck, msg->is_mqtt);
                if (strcmp(command,"get_scn_time")==0) command_get_scene_time(payload_item,n_pck, msg->is_mqtt);
                if (strcmp(command,"save_dev")==0) command_save_dev(payload_item,n_pck, msg->is_mqtt);
                

                /* Bu komutlar STM32 ile ilgilidir*/
            // /**/    if (strcmp(command,"off")==0) command_off(payload_item,n_pck, msg->is_mqtt);
            // /**/    if (strcmp(command,"arc_power")==0) command_arc(payload_item,n_pck, msg->is_mqtt);
            // /**/    if (strcmp(command,"action")==0) command_action(payload_item,n_pck, msg->is_mqtt); 
            // /**/    if (strcmp(command,"get_level")==0) command_get_level(payload_item,n_pck,msg->is_mqtt);              
            // /**/    if (strcmp(command,"qstatus")==0) command_qstatus(payload_item,n_pck,msg->is_mqtt);
            // /**/    if (strcmp(command,"identfy")==0) command_identfy(payload_item,n_pck,msg->is_mqtt);
                
            // /**/    if (strcmp(command,"get_gurup")==0) command_get_gurup(payload_item,n_pck,  msg->is_mqtt);
            // /**/    if (strcmp(command,"set_gurup")==0) command_set_gurup(payload_item,n_pck, msg->is_mqtt);
            // /**/    if (strcmp(command,"get_scene")==0) command_get_scene(payload_item,n_pck, msg->is_mqtt);
            // /**/    if (strcmp(command,"set_scene")==0) command_set_scene(payload_item,n_pck,msg->is_mqtt);
            // /**/    if (strcmp(command,"del_scene")==0) command_del_scene(payload_item,n_pck,msg->is_mqtt);
          
            // /**/    if (strcmp(command,"get_detail")==0) command_get_detail(payload_item,n_pck, msg->is_mqtt);         
            // /**/    if (strcmp(command,"set_min")==0) command_set_level(payload_item,n_pck, SET_MIN_LEVEL,"set_min",msg->is_mqtt);
            // /**/    if (strcmp(command,"set_max")==0) command_set_level(payload_item,n_pck, SET_MAX_LEVEL,"set_max",msg->is_mqtt);
            // /**/    if (strcmp(command,"set_fail")==0) command_set_level(payload_item,n_pck, SET_FAIL_LEVEL,"set_fail",msg->is_mqtt);
            // /**/    if (strcmp(command,"set_pwr")==0) command_set_level(payload_item,n_pck,SET_POWERON_LEVEL,"set_pwr",msg->is_mqtt);
            // /**/    if (strcmp(command,"set_fade")==0) command_set_fade(payload_item,n_pck, msg->is_mqtt);
            // /**/    if (strcmp(command,"set_efade")==0) command_set_efade(payload_item,n_pck, msg->is_mqtt);
           
            // /**/    if (strcmp(command,"get_min")==0) command_get_single(payload_item,n_pck,QUERY_MIN_LEVEL,"get_min",msg->is_mqtt);
            // /**/    if (strcmp(command,"get_max")==0) command_get_single(payload_item,n_pck,QUERY_MAX_LEVEL,"get_max",msg->is_mqtt);
            // /**/    if (strcmp(command,"get_fail")==0) command_get_single(payload_item,n_pck,QUERY_FAILURE_LEVEL,"get_fail",msg->is_mqtt);
            // /**/    if (strcmp(command,"get_pwr")==0) command_get_single(payload_item,n_pck,QUERY_POWERON_LEVEL,"get_pwr",msg->is_mqtt);
          
            /**/    if (strcmp(command,"go_scene")==0) command_go_scene(payload_item,n_pck, msg->is_mqtt);

            /**/    if (strcmp(command,"get_instance")==0) command_get_instance(payload_item,n_pck, msg->is_mqtt);
            /**/    if (strcmp(command,"get_amode")==0) command_get_amode(payload_item,n_pck, msg->is_mqtt);
                 /*Set AModda sorun var*/  
                 if (strcmp(command,"set_amode")==0) command_set_amode(payload_item,n_pck, msg->is_mqtt); 

            /**/        if (strcmp(command,"qinstance")==0) command_query_instance(payload_item,n_pck, msg->is_mqtt);      
            /**/        if (strcmp(command,"set_instance")==0) command_set_instance(payload_item,n_pck, msg->is_mqtt);
            /**/        if (strcmp(command,"qfilter")==0) command_query_ins_filter(payload_item,n_pck, msg->is_mqtt);
            /**/        if (strcmp(command,"sfilter")==0) command_set_ins_filter(payload_item,n_pck, msg->is_mqtt);
            /**/        if (strcmp(command,"qstimer")==0) command_query_ins_timer(payload_item,n_pck, msg->is_mqtt);
            /**/        if (strcmp(command,"sstimer")==0) command_set_ins_timer(payload_item,n_pck, msg->is_mqtt);
            /**/        if (strcmp(command,"put_dev")==0) command_put_device(payload_item,n_pck, msg->is_mqtt);
            /**/            if (strcmp(command,"get_temp")==0) command_get_temp(payload_item,n_pck,msg->is_mqtt);
            /**/            if (strcmp(command,"set_temp")==0) command_set_temp(payload_item,n_pck,msg->is_mqtt);
            /**/            if (strcmp(command,"set_tmode")==0) command_set_tmode(payload_item,n_pck,msg->is_mqtt);

       
                        if (strcmp(command,"get_qkanal")==0) command_get_qkanal(payload_item,n_pck, msg->is_mqtt);
                        if (strcmp(command,"get_color")==0) command_get_color(payload_item,n_pck, msg->is_mqtt);
                        if (strcmp(command,"set_color")==0) command_set_color(payload_item,n_pck, msg->is_mqtt);

                
            /**/        if (strcmp(command,"an_level")==0) command_an_level(payload_item,n_pck, msg->is_mqtt);
            /**/        if (strcmp(command,"an_on")==0) command_an_on(payload_item,n_pck, msg->is_mqtt);
            /**/        if (strcmp(command,"an_off")==0) command_an_off(payload_item,n_pck, msg->is_mqtt);
            /**/        if (strcmp(command,"an_power")==0) command_an_power(payload_item,n_pck, msg->is_mqtt);
            /**/        if (strcmp(command,"an_save")==0) command_an_save(payload_item,n_pck, msg->is_mqtt);
                        if (strcmp(command,"an_toggle")==0) command_an_toggle(payload_item,n_pck, msg->is_mqtt);
                
                
                if (strcmp(command,"get_mode")==0) {
                    cJSON *pay = cJSON_CreateObject();  
                    cJSON_AddStringToObject(pay, "com", "get_mode");
                    cJSON_AddNumberToObject(pay,"mode",GlobalConfig.mode);
                    cJSON_AddNumberToObject(pay,"alarm",GlobalConfig.alarm);
                    send_AK(pay,n_pck, msg->is_mqtt);
                }
                if (strcmp(command,"set_mode")==0) {
                    uint8_t md = 0;
                    JSON_getint(payload_item,"mode", &md);
                    GlobalConfig.mode = md;
                    //----- NORMAL MODE ------
                    if (md==0) {
                        //Alarm Açıksa alarmı kapat
                        if (GlobalConfig.alarm==2) {
                            Alarm_OFF(n_pck, msg->is_mqtt);
                        }
                    }
                    //----- EVDEN ÇIKIŞ MODU ------
                    if (md==1) {
                        //Evden çıkıyor. Tüm lambaları kapat alarmı cevaba göre aç.
                        uint8_t alr = 0;
                        JSON_getint(payload_item,"alarm", &alr);
                        if (alr==1) Alarm_ON(msg->remote, new_pck, n_pck, payload_item, msg->is_mqtt);
                       //Tüm lambaları kapat (DALI broadcast + yerel cihazlar tek noktadan: command_off)
                        cJSON *off_pl = cJSON_CreateObject();
                        cJSON_AddNumberToObject(off_pl, "adres", 0xff);
                        cJSON_AddNumberToObject(off_pl, "gurup", 0);
                        cJSON_AddNumberToObject(off_pl, "kanal", 9);
                        command_off(off_pl, n_pck, msg->is_mqtt, false);
                        cJSON_Delete(off_pl);

                    }
                    disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
                    cJSON *pay = cJSON_CreateObject();  
                    cJSON_AddStringToObject(pay, "com", "set_mode");
                    send_AK(pay,n_pck, msg->is_mqtt);
                }
                if (strcmp(command,"set_alarm")==0) {
                    uint8_t md = 0;
                    JSON_getint(payload_item,"state", &md);
                    if (md==1) {
                        Alarm_ON(msg->remote, new_pck, n_pck, payload_item, msg->is_mqtt);
                    }   
                    if (md==0) {
                        Alarm_OFF(n_pck, msg->is_mqtt);
                    }
                }


                if (strcmp(command,"set_admin")==0) {
                    char *adm = (char *)calloc(1,25);    
                    JSON_getstring(payload_item,"admin", adm,25);
                    strcpy((char*)GlobalConfig.admin,(char *)adm);
                    disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
                    cJSON *pay = cJSON_CreateObject();  
                    cJSON_AddStringToObject(pay, "com", "set_admin");
                    cJSON_AddStringToObject(pay, "admin", (char*)GlobalConfig.admin);
                    send_AK(pay,n_pck, msg->is_mqtt);
                    free(adm);
                }
                               
                if (strcmp(command,"clear_number")==0) {
                    clear_number(payload_item,n_pck);
                }

                if (strcmp(command,"adresle")==0) {
                    uint8_t kan = 0, val=0;
                    JSON_getint(payload_item,"kanal", &kan);
                    JSON_getint(payload_item,"value", &val);

                    cJSON *root = cJSON_CreateObject();
                    cJSON_AddStringToObject(root, "com", "query");
                    cJSON_AddNumberToObject(root, "hexcom", val);
                    cJSON_AddNumberToObject(root, "kanal",kan);
                    cJSON_AddNumberToObject(root, "bit",16);
                    cJSON_AddNumberToObject(root, "special",15);
                    send_STM(root);
                    cJSON_Delete(root);

                    cJSON *pay = cJSON_CreateObject();  
                    cJSON_AddStringToObject(pay, "com", "adresle");
                    send_AK(pay,n_pck, msg->is_mqtt);
                }

                if (strcmp(command,"refresh")==0) {    
                  search_task_param_t *param = (search_task_param_t *)malloc(sizeof(search_task_param_t));                 
                  // Derin Kopya (Hard Copy) oluşturuluyor
                  param->pck.rem = (remote_t *)malloc(sizeof(remote_t));
                  memcpy(param->pck.rem, msg->remote, sizeof(remote_t));
                  param->pck.pck = cJSON_Duplicate(new_pck, true);
                  param->payload = cJSON_Duplicate(payload_item, true);
                  param->is_mqtt = msg->is_mqtt;


                  xTaskCreatePinnedToCore(
                        refresh_device,           // Task fonksiyonunun adı
                        "rdev",          // Task'ın ismi (debug için)
                        4096,               // Stack büyüklüğü artırıldı
                        param,               // Task'a kopyalanan parametre gönderiliyor
                        1,                  // Öncelik sırası (0 en düşük)
                        NULL,               // Task handle (referans gerekmiyorsa NULL)
                        1                   // Çalışacağı çekirdek (0 veya 1)
                    );
               }

               /*OK Telde Mesajlara bakılacak*/
                if (strcmp(command,"search")==0) {    
                  search_task_param_t *param = (search_task_param_t *)malloc(sizeof(search_task_param_t));                 
                  // Derin Kopya (Hard Copy) oluşturuluyor
                  param->pck.rem = (remote_t *)malloc(sizeof(remote_t));
                  memcpy(param->pck.rem, msg->remote, sizeof(remote_t));
                  param->pck.pck = cJSON_Duplicate(new_pck, true);
                  param->payload = cJSON_Duplicate(payload_item, true);

                  xTaskCreatePinnedToCore(
                        search_device,           // Task fonksiyonunun adı
                        "sdev",          // Task'ın ismi (debug için)
                        4096,               // Stack büyüklüğü artırıldı
                        param,               // Task'a kopyalanan parametre gönderiliyor
                        1,                  // Öncelik sırası (0 en düşük)
                        NULL,               // Task handle (referans gerekmiyorsa NULL)
                        1                   // Çalışacağı çekirdek (0 veya 1)
                    );
               }

                vTaskDelay(200/portTICK_PERIOD_MS);
                free(n_pck);
                free(command);
            }
        cJSON_Delete(rcv_json); 
        // Derin kopya ile oluşturduğumuz tüm alanları temizliyoruz
        free(msg->remote);
        free(msg->payload);
        free(msg); 

        } 
    }
    vTaskDelete(NULL);
}


/*
   Udp mesajları serverdan buraya gelir. Bir hard kopyası alındıktan
   sonra udp_processing_queue ile udp_worker_task a aktarılarak 
   sıra ile burada işlenir. 
*/
void on_udp_message_received(udp_msg_t *msg, void *user_ctx) {
    if (msg == NULL || msg->payload == NULL || msg->remote == NULL) return;

    // udp_msg_t ve içeriği için derin kopya oluşturuyoruz
    udp_msg_t *msg_copy = (udp_msg_t*)malloc(sizeof(udp_msg_t));
    if (msg_copy != NULL) {
        msg_copy->remote = (remote_t*)malloc(sizeof(remote_t));
        msg_copy->payload = malloc(msg->len + 1);
        msg_copy->len = msg->len;
        msg_copy->is_broadcast = msg->is_broadcast;
        msg_copy->is_mqtt = false;

        if (msg_copy->remote && msg_copy->payload) {
            memcpy(msg_copy->remote, msg->remote, sizeof(remote_t));
            memcpy(msg_copy->payload, msg->payload, msg->len + 1);
        
            if (xQueueSend(udp_processing_queue, &msg_copy, 0) != pdPASS) {
                ESP_LOGE("MAIN", "Kuyruk dolu, paket düşürüldü!");
                free(msg_copy->remote);
                free(msg_copy->payload);
                free(msg_copy);
            }
        } else {
            if (msg_copy->remote) free(msg_copy->remote);
            if (msg_copy->payload) free(msg_copy->payload);
            free(msg_copy);
        }
    }
}