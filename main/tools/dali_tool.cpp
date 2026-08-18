
#include "cJSON.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "jsontool.h"
#include "portmacro.h"
#include "tools/dali_command.h"
#include "tools/gear/gear.h"

// DALI (kanal 1-4) kayıtları instance'de, yerel (kanal 10) kayıtlar instanceL'de tutulur.
static inline Instance& pick_instance(uint8_t kanal) {
    return (kanal==10) ? instanceL : instance;
}
typedef union {
    uint16_t all;

    struct {
        // STM32 (Little-Endian) için sıralama:
        // İlk byte (LSB) Adres, İkinci byte (MSB) Komut olmalı ki
        // frame.all değeri 0xFEC8 şeklinde görünsün.
        uint8_t address; // Artık LSB (Düşük adres)
        uint8_t command; // Artık MSB (Yüksek adres)
    } byte;

    struct {
        // Bit-field sıralaması da LSB'den MSB'ye doğrudur:
        uint16_t data_val  : 8; // Önce ilk 8 biti (0-7) komut kaplamalı
        uint16_t selector  : 1; // 8. bit: Selector
        uint16_t addr_val   : 6; // 9-14. bitler: Adres
        uint16_t type_bit   : 1; // 15. bit: Type (Y)
    } bits;

} DaliFrame_t;

uint16_t create_command(uint8_t adr, bool is_group, bool is_arc, uint8_t cmd)
{
    DaliFrame_t frame;

    frame.all = 0; // Başlangıçta temizle
    frame.bits.selector = is_arc ? 0 : 1; //Eger Arc komutu ise 0 olmalı
    frame.bits.addr_val = (adr&0x3F);
    frame.bits.type_bit = is_group ? 1 : 0; //Eger gurup adresi ise 1 olmalı
    frame.bits.data_val = cmd;
    if (adr==0xFF || adr==0xFE) frame.bits.type_bit=1;
   // printf("Arc:%d %d Grp:%d %d\n",is_arc,frame.bits.selector,is_group,frame.bits.type_bit );
    return frame.all;
}

void send_wifi(cJSON *pay)
{  
    char *dat = cJSON_PrintUnformatted(pay);
    ESP_LOGI("SEND_UDP","GIDEN >> %s",dat);
    uint8_t cc = udp_server.send_unicast_all_count((uint8_t *)dat,strlen(dat));
    udp_server.send_broadcast((uint8_t *)dat,strlen(dat));
    ESP_LOGI(TAG,"%d Terminale gönderildi.",cc);   
    cJSON_free(dat);
}

void send_AK(cJSON *pay, pck_t *pck, bool is_mqtt)
{
    cJSON *root = cJSON_CreateObject();                      
    cJSON_AddItemReferenceToObject(root, "pck", pck->pck);
    cJSON_AddItemToObject(root, "payload", pay);   
    char *dat = cJSON_PrintUnformatted(root);
    ESP_LOGI("SEND_AK","%s GIDEN >> %s",is_mqtt?"MQTT":"UDP",dat);
    if (!is_mqtt) udp_server.send_unicast(pck->rem,(uint8_t *)dat,strlen(dat));
    if (is_mqtt) mqtt_send(dat);
    cJSON_free(dat);
    cJSON_Delete(root);
}

void send_STM(cJSON *obj)
{
    char *dat = cJSON_PrintUnformatted(obj);

    printf("Send STM >> %s\n",dat);

    myUart->send(dat);
    ESP_LOGW(TAG,"STM >> %s",dat);
    cJSON_free(dat);
}

void dali_channel_onoff(uint8_t chn, uint8_t stat)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"com","DALI");
    cJSON_AddNumberToObject(root,"kanal",chn);
    cJSON_AddNumberToObject(root,"stat",stat);
    send_STM(root);
    cJSON_Delete(root);
}

uint8_t dali_get_channel_onoff(uint8_t chn)
{
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    xQueueReset(searchQueue);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"com","get_DALI");
    cJSON_AddNumberToObject(root,"kanal",chn);
    send_STM(root);
    cJSON_Delete(root);
    searchMessage_t msg = {};
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(5000));
    xSemaphoreGive(searchQueueMutex);
    return msg.name[0];
}

void channel_onoff(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t kanal=0, stat=0;
    JSON_getint(payload,"kanal", &kanal);
    JSON_getint(payload,"stat", &stat);
    dali_channel_onoff(kanal,stat);
    if (kanal==1) GlobalConfig.kanal1=stat;
    if (kanal==2) GlobalConfig.kanal2=stat;
    if (kanal==3) GlobalConfig.kanal3=stat;
    disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
}

void led_onoff(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t stat=0;
    JSON_getint(payload,"stat", &stat);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"com","led");
    cJSON_AddNumberToObject(root,"stat",stat);
    send_STM(root);
    cJSON_Delete(root);
}


int send_wifi_query(uint32_t cmd, uint8_t *ret, uint16_t tm) {
    xSemaphoreTake(wsearchQueueMutex, portMAX_DELAY);
    xQueueReset(wsearchQueue);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"com","query");
    cJSON_AddNumberToObject(root,"hexcom",cmd);
    cJSON_AddNumberToObject(root,"bit",16);
    cJSON_AddNumberToObject(root,"special",0xAA);

    char *dat = cJSON_PrintUnformatted(root);
    udp_server.send_unicast_all_count((uint8_t *)dat,strlen(dat));
    cJSON_free(dat);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    TickType_t xTicksToWait = pdMS_TO_TICKS(tm);
    BaseType_t got = xQueueReceive(wsearchQueue, &msg, xTicksToWait);
    xSemaphoreGive(wsearchQueueMutex);
    if (got == pdPASS) {
        ret[0] = msg.name[0];
        ret[1] = msg.name[1];
        ret[2] = msg.name[2];
        ret[3] = msg.name[3];
        ret[4] = msg.name[4];
        ret[5] = msg.name[5];
        return 1;
        }
    return -1;
}

void wifi_search_device(void *pvParameters) {

    udp_server.client_list();

    xEventGroupSetBits(searchGROUP, BIT_START);
    vTaskDelay(500/portTICK_PERIOD_MS);

    for (uint8_t adr=0;adr<64;adr++) {

        if((adr%10)==0) printf("adr:%02d\n",adr);

        uint32_t cmd =  (((adr<<1)|1) << 8)| QUERY_DEVICE_TYPE;
        uint8_t ret[6];
        int typ =send_wifi_query(cmd, ret,300);
        if (typ==-1) {
            vTaskDelay(100/portTICK_PERIOD_MS);
            typ =send_wifi_query(cmd, ret,500);
        }
        if (typ!=-1) {
            
            uint32_t ra = 0x00FFFFFF;
            ra=(ret[2]<<16)| (ret[3]<<8) | (ret[4]);
            gear_t gr = {};
            gr.aktif = 1;
            gr.short_addr = adr;
            gr.random_addr = ra;
            gr.type = ret[0];
            gr.ext_type = ret[1];
            gr.kanal = (dev_sourge_t)0;
            gr.instance = 0;
            gr.ico = 0;
            gr.room = 0xFF;   
            gr.anahtar=0xff;        
            gr.spec.status = 0;
            gr.spec.group[0] = 0;
            gr.spec.group[1] = 0;
            

            char *txt;
            char *harf;
            asprintf(&harf,"W%02d",gr.short_addr);
            uint8_t fnd = 0;
            if (ret[0]==0x06) {asprintf(&txt,"Lamba%s",harf);fnd=1;} 
            if (ret[0]==0x08 ) {asprintf(&txt,"Rgb%s",harf); fnd=1;}
            if (ret[0]==0x07 && ret[1]==0x07) {asprintf(&txt,"Role%s",harf);fnd=1;} 
            if (ret[0]==0x07 && ret[1]==0x0B) {asprintf(&txt,"Anahtar%s",harf); fnd=1;}
            if (ret[0]==0x07 && ret[1]==0x77) {asprintf(&txt,"Perde%s",harf); fnd=1;}

            if (ret[0]==0x07 && ret[1]==0x78) {asprintf(&txt,"Elektrik%s",harf); fnd=1;}
            if (ret[0]==0x07 && ret[1]==0x79) {asprintf(&txt,"Walf%s",harf); fnd=1;}
            if (ret[0]==0x07 && ret[1]==0x7A) {asprintf(&txt,"Gaz%s",harf); fnd=1;}
            if (ret[0]==0x07 && ret[1]==0x7B) {asprintf(&txt,"Asansör%s",harf); fnd=1;}

            if (fnd==0) asprintf(&txt,"Bilinmeyen%s",harf);

            searchMessage_t msg = {};

            strcpy((char*)gr.name,txt);
            strcpy((char*)msg.name,txt);
            free(harf);
            free(txt);
           
            gear04.update_gear(adr,&gr);
            printf("ADR : %d TYPE : %02X EXT : %02X  %s\n",gr.short_addr, gr.type,gr.ext_type, gr.name);

            msg.addr = gr.short_addr;
            msg.type = gr.type;
            msg.exttype = gr.ext_type;    
            xQueueReset(searchQueue);
            xQueueSend(searchQueue, (void *)&msg, pdMS_TO_TICKS(10));        
            xEventGroupSetBits(searchGROUP, BIT_CONTINUE);
        
        }
    }
  
    xEventGroupSetBits(searchGROUP, BIT_STOP);
    vTaskDelay(500/portTICK_PERIOD_MS);


    vTaskDelete(NULL);
}

void search_device(void *pvParameters) {
  search_task_param_t *param = (search_task_param_t *)pvParameters; 
  ESP_LOGI("SEARCH_TASK", "Arama işlemi başlatıldı...");

    uint8_t kayit=9, cable = 9;
    JSON_getint(param->payload,"kayit",&kayit);
    JSON_getint(param->payload,"cable",&cable);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "search");
    cJSON_AddStringToObject(pay, "txt", "Arama için Hazırlanıyor...");
    send_AK(pay,&param->pck);
     
    
    char *txt;
    if (cable==0) asprintf(&txt,"Kablosuz sistemde Arama Başlatıldı");
    if (cable==1) {asprintf(&txt,"Kanal 1 Arama Başlatıldı");}
    if (cable==2) {asprintf(&txt,"Kanal 2 Arama Başlatıldı");}
    if (cable==3) {asprintf(&txt,"Kanal 3 Arama Başlatıldı");}
    ESP_LOGI("CABLE_CONTROL","%s",txt);

    // Arama boyunca (birden çok cihaz mesajı gelene kadar) searchQueue'yu tek
    // başımıza kullanıyoruz; başka bir sorgu bu sırada devreye giremesin.
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);

    // STM32'ye komutu göndermeden ÖNCE eski bit/kuyruk durumunu temizle.
    // Aksi halde STM32'nin "search"/"start" cevabı UART kesmesiyle çok hızlı
    // gelip send_STM'den hemen sonraki temizlemeyi yarışıp silebiliyor —
    // yeni aramanın START sinyali kayboluyor ve önceki aramadan kalan
    // kuyruk öğesi bir sonraki aramayı karıştırıyordu.
    xEventGroupClearBits(searchGROUP, BIT_START | BIT_STOP | BIT_CONTINUE);
    xQueueReset(searchQueue);

    // DALI_channel_search STM32'de uzun süre bloklayan bir döngü — bu sırada
    // STM32 ping/pong GPIO kontrolünü servis edemeyebilir. led_task bunu
    // "STM32 kilitlendi" sanıp STM32'yi taramanın ortasında resetliyordu.
    // Arama boyunca bu bekçiyi geçici olarak (diske yazmadan) kapatıyoruz.
    uint8_t ping_active_saved = GlobalConfig.ping_active;
    GlobalConfig.ping_active = 0;

    if (cable!=0) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root,"com","search");
        cJSON_AddNumberToObject(root,"kanal",cable);
        send_STM(root);
        cJSON_Delete(root);
    }

    if (cable==0) {
        //Kablosuz ARAMA
        xTaskCreatePinnedToCore(
                        wifi_search_device,           // Task fonksiyonunun adı
                        "sdev97",          // Task'ın ismi (debug için)
                        4096,               // Stack büyüklüğü artırıldı
                        NULL,               // Task'a kopyalanan parametre gönderiliyor
                        1,                  // Öncelik sırası (0 en düşük)
                        NULL,               // Task handle (referans gerekmiyorsa NULL)
                        1                   // Çalışacağı çekirdek (0 veya 1)
                    );
    }

   // if (cable==5) kk=kk+cable_channel_control(kayit,DALI_KANAL3,param);

    bool don = true;
    uint8_t sayi = 0;
    while (don)
       {
            EventBits_t uxBits = xEventGroupWaitBits(
            searchGROUP,
            BIT_START | BIT_CONTINUE | BIT_STOP, // Bu üç bitten herhangi birini bekle
            pdTRUE,        // Bitler gelince otomatik temizlensin (Clear on exit)
            pdFALSE,       // "Wait for ALL" false: Herhangi biri gelince uyan
            portMAX_DELAY  // Sonsuza kadar bekle
            );

            if (uxBits & BIT_START) {
                cJSON *pay = cJSON_CreateObject();  
                cJSON_AddStringToObject(pay, "com", "search");
                cJSON_AddStringToObject(pay, "txt", txt);
                send_AK(pay,&param->pck);
                free(txt);
            }

            if (uxBits & BIT_STOP) {
                cJSON *pay = cJSON_CreateObject();  
                cJSON_AddStringToObject(pay, "com", "search");
                asprintf(&txt,"%d cihaz bulundu. Arama Tamamlandı",sayi);
                cJSON_AddStringToObject(pay, "txt", txt);
                send_AK(pay,&param->pck);
                free(txt);
                GlobalConfig.ping_active = ping_active_saved;
                xSemaphoreGive(searchQueueMutex);
                don=false;
                break;  // Döngüden çık ve task'ı bitir veya bekleme modunu kapat
            }

            if (uxBits & BIT_CONTINUE) {
                searchMessage_t msg = {};
                if (xQueueReceive(searchQueue, &msg, portMAX_DELAY)) {
                    sayi=sayi+1;
                    asprintf(&txt,"%2d->%02d [%02d:%02d] %s", sayi,msg.addr,msg.type,msg.exttype,msg.name);
                    cJSON *pay = cJSON_CreateObject();  
                    cJSON_AddStringToObject(pay, "com", "search");
                    cJSON_AddStringToObject(pay, "txt", txt);
                    send_AK(pay,&param->pck);
                    free(txt);
                }
                
                // Devam işlemleri...
            }

       }
    
  // Task bitişinde derin kopyalanmış verileri temizliyoruz
  cJSON_Delete(param->pck.pck);
  free(param->pck.rem);
  cJSON_Delete(param->payload);
  free(param);

  vTaskDelete(NULL);
}

void clear_number(cJSON *payload, pck_t *pck)
{
    uint8_t id = 200, kanal = 255;
    JSON_getint(payload,"id", &id);
    JSON_getint(payload,"kanal", &kanal);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "search");
    cJSON_AddStringToObject(pay, "txt", "Silme İşlemi başlatıldı. Lütfen BEKLEYİNİZ.");
    send_AK(pay,pck);

    if (kanal>200) instance.clear_file();
    if (kanal==1 || kanal>200) gear01.clear_file();
    if (kanal==2 || kanal>200) gear02.clear_file();
    if (kanal==3 || kanal>200) gear03.clear_file();
    if (kanal==0 || kanal>200) gear04.clear_file();
    pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "search");
    cJSON_AddStringToObject(pay, "txt", "Silme İşlemi TAMAMLANDI.");
    send_AK(pay,pck);
        
    ESP_LOGI(TAG,"Silme işlemi Tamamlandı");
}


void command_set_gurup_time(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    uint8_t adr = 0;
    JSON_getint(payload,"adres", &adr);
    gurup.set_time(payload);
    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "set_grp_time");
    cJSON_AddNumberToObject(pay,"adres",adr);
    send_AK(pay,pck,is_mqtt);   
}

void command_set_scene_time(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    uint8_t adr = 0;
    JSON_getint(payload,"adres", &adr);
    scene.set_time(payload);
    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "set_scn_time");
    cJSON_AddNumberToObject(pay,"adres",adr);
    send_AK(pay,pck, is_mqtt);   
}

void command_get_scene_time(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    uint8_t adr = 0;
    JSON_getint(payload,"adres", &adr);
    zaman_t *z1 = (zaman_t *)calloc(1,sizeof(zaman_t));
    zaman_t *z2 = (zaman_t *)calloc(1,sizeof(zaman_t)); 
    scene.get_time(adr,z1,z2);

    cJSON *Lm, *ot, *ot1;
    cJSON *Gm, *gt, *gt1;

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "get_scn_time");
    cJSON_AddNumberToObject(pay,"adres",adr);

    cJSON_AddItemToObject(pay, "time1",Lm = cJSON_CreateObject());
    cJSON_AddNumberToObject(Lm,"onactive",z1->onactive);
    cJSON_AddItemToObject(Lm, "ontime",ot = cJSON_CreateObject());
    cJSON_AddNumberToObject(ot,"saat",z1->ontime.saat);
    cJSON_AddNumberToObject(ot,"dakika",z1->ontime.dakika);
    cJSON_AddNumberToObject(Lm,"offactive",z1->offactive);
    cJSON_AddItemToObject(Lm, "offtime",ot1 = cJSON_CreateObject());
    cJSON_AddNumberToObject(ot1,"saat",z1->ontime.saat);
    cJSON_AddNumberToObject(ot1,"dakika",z1->ontime.dakika);
    cJSON_AddNumberToObject(Lm,"power",z1->power);

    cJSON_AddItemToObject(pay, "time2",Gm = cJSON_CreateObject());
    cJSON_AddNumberToObject(Gm,"onactive",z2->onactive);
    cJSON_AddItemToObject(Gm, "ontime",gt = cJSON_CreateObject());
    cJSON_AddNumberToObject(gt,"saat",z2->ontime.saat);
    cJSON_AddNumberToObject(gt,"dakika",z2->ontime.dakika);
    cJSON_AddNumberToObject(Gm,"offactive",z2->offactive);
    cJSON_AddItemToObject(Gm, "offtime",gt1 = cJSON_CreateObject());
    cJSON_AddNumberToObject(gt1,"saat",z2->ontime.saat);
    cJSON_AddNumberToObject(gt1,"dakika",z2->ontime.dakika);
    cJSON_AddNumberToObject(Gm,"power",z2->power);

    send_AK(pay,pck, is_mqtt);   
}
void command_get_gurup_time(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    uint8_t adr = 0;
    JSON_getint(payload,"adres", &adr);
    zaman_t *z1 = (zaman_t *)calloc(1,sizeof(zaman_t));
    zaman_t *z2 = (zaman_t *)calloc(1,sizeof(zaman_t)); 
    gurup.get_time(adr,z1,z2);

    cJSON *Lm, *ot, *ot1;
    cJSON *Gm, *gt, *gt1;

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "get_grp_time");
    cJSON_AddNumberToObject(pay,"adres",adr);

    cJSON_AddItemToObject(pay, "time1",Lm = cJSON_CreateObject());
    cJSON_AddNumberToObject(Lm,"onactive",z1->onactive);
    cJSON_AddItemToObject(Lm, "ontime",ot = cJSON_CreateObject());
    cJSON_AddNumberToObject(ot,"saat",z1->ontime.saat);
    cJSON_AddNumberToObject(ot,"dakika",z1->ontime.dakika);
    cJSON_AddNumberToObject(Lm,"offactive",z1->offactive);
    cJSON_AddItemToObject(Lm, "offtime",ot1 = cJSON_CreateObject());
    cJSON_AddNumberToObject(ot1,"saat",z1->ontime.saat);
    cJSON_AddNumberToObject(ot1,"dakika",z1->ontime.dakika);
    cJSON_AddNumberToObject(Lm,"power",z1->power);

    cJSON_AddItemToObject(pay, "time2",Gm = cJSON_CreateObject());
    cJSON_AddNumberToObject(Gm,"onactive",z2->onactive);
    cJSON_AddItemToObject(Gm, "ontime",gt = cJSON_CreateObject());
    cJSON_AddNumberToObject(gt,"saat",z2->ontime.saat);
    cJSON_AddNumberToObject(gt,"dakika",z2->ontime.dakika);
    cJSON_AddNumberToObject(Gm,"offactive",z2->offactive);
    cJSON_AddItemToObject(Gm, "offtime",gt1 = cJSON_CreateObject());
    cJSON_AddNumberToObject(gt1,"saat",z2->ontime.saat);
    cJSON_AddNumberToObject(gt1,"dakika",z2->ontime.dakika);
    cJSON_AddNumberToObject(Gm,"power",z2->power);

    send_AK(pay,pck, is_mqtt);   
}

void command_get_single(cJSON *payload, pck_t *pck, uint8_t cmd, const char *txt,bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,false,false,cmd));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    if (kanal>3) send_STM(root);
    if (kanal==0) send_wifi(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(5000));
    xSemaphoreGive(searchQueueMutex);
    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", txt);
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"val",msg.name[0]);
    send_AK(pay,pck,is_mqtt);
}   


void command_modbus_adr(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    uint8_t id = 0, inp = 255, outp = 0xff;
    JSON_getint(payload,"id", &id);
    JSON_getint(payload,"first_out", &outp);
    JSON_getint(payload,"first_input", &inp);

    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    xQueueReset(searchQueue);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "adres");
    cJSON_AddNumberToObject(root, "id", id);
    cJSON_AddNumberToObject(root, "fout",outp);
    cJSON_AddNumberToObject(root, "fin",inp);
    send_STM(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(5000));
    xSemaphoreGive(searchQueueMutex);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "modbus_adr");
    cJSON_AddNumberToObject(pay,"id",msg.name[0]);
    cJSON_AddNumberToObject(pay,"first_out",msg.name[1]);
    cJSON_AddNumberToObject(pay,"first_input",msg.name[2]);
    send_AK(pay,pck,is_mqtt);
}  

void command_modbus_getadr(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    uint8_t id = 0;//, inp = 255;//, outp = 0xff;
    JSON_getint(payload,"id", &id);

    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    xQueueReset(searchQueue);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "getadres");
    cJSON_AddNumberToObject(root, "id", id);
    send_STM(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(5000));
    xSemaphoreGive(searchQueueMutex);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "modbus_adr");
    cJSON_AddNumberToObject(pay,"id",msg.name[0]);
    cJSON_AddNumberToObject(pay,"first_out",msg.name[1]);
    cJSON_AddNumberToObject(pay,"first_input",msg.name[2]);
    send_AK(pay,pck,is_mqtt);
} 


bool isCoordinatesInTurkey(double lat, double lon) {
  // Türkiye'nin coğrafi uç sınır kutusu
  const double minLat = 35.8; // Hatay'ın en güneyi
  const double maxLat = 42.1; // Sinop'un en kuzeyi
  const double minLon = 25.6; // Gökçeada'nın en batısı
  const double maxLon = 44.8; // Iğdır'ın en doğusu

  return (lat >= minLat && lat <= maxLat) && (lon >= minLon && lon <= maxLon);
}


void command_weather(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    float lat = 0, lon = 0, temp=0, rain=0.0;
    char *yer = (char *)calloc(1,60);

    JSON_getfloat(payload,"lat", &lat);
    JSON_getfloat(payload,"lon", &lon);
    JSON_getfloat(payload,"temp", &temp);
    JSON_getfloat(payload,"rain3h", &rain);
    JSON_getstring(payload,"city", yer,59);

    if (isCoordinatesInTurkey(lat,lon)) {
        GlobalConfig.lat = lat;
        GlobalConfig.lon = lon;
        GlobalConfig.temp = temp;
        GlobalConfig.rain = rain;
        strcpy((char*)GlobalConfig.yer,yer);
        disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);

        ESP_LOGI("WEATHER","Lat : %f Lon : %f  temp:%f %s Kaydedildi",lat,lon,temp,yer);

        if (Active_Network_connection>0 && Active_Network_connection<3) {
            xTaskCreatePinnedToCore(
                            firebase_update_location_task,
                            "fblocation",
                            8192,
                            NULL,
                            1,
                            NULL,
                            1
                        );
        }
    }
    free(yer);   
}

void command_modbus_ping(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    uint8_t id = 0;
    JSON_getint(payload,"id", &id);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    xQueueReset(searchQueue);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "ping");
    cJSON_AddNumberToObject(root, "id", id);
    send_STM(root);
    cJSON_Delete(root);
    searchMessage_t msg = {};
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(5000));
    xSemaphoreGive(searchQueueMutex);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "pong");
    cJSON_AddNumberToObject(pay,"id",msg.name[0]);
    send_AK(pay,pck,is_mqtt);
}  

void command_pin(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    uint8_t id = 0, state=0;
    JSON_getint(payload,"id", &id);
    JSON_getint(payload,"state", &state);

    reliableUartManager.send_role_command(myUart, id,state);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    xQueueReset(searchQueue);
    xSemaphoreGive(searchQueueMutex);
}

void command_modbus_identfy(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    uint8_t id = 0;
    JSON_getint(payload,"id", &id);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "modbus_identfy");
    cJSON_AddNumberToObject(root, "id", id);
    send_STM(root);
    cJSON_Delete(root);
} 

void command_set_efade(cJSON *payload, pck_t *pck,bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255, val=0xFF;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);
    JSON_getint(payload,"val", &val);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,false,false,SET_EFADE_TIME));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    cJSON_AddNumberToObject(root, "special",6);
    cJSON_AddNumberToObject(root, "val",val);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    if(kanal>0) send_STM(root);
    if (kanal==0) send_wifi(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(5000));
    xSemaphoreGive(searchQueueMutex);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "set_efade");
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"val",msg.name[0]);
    send_AK(pay,pck,is_mqtt);
}

void command_set_fade(cJSON *payload, pck_t *pck,bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255, val=0xFF;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);
    JSON_getint(payload,"val", &val);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,false,false,SET_FADE_TIME));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    cJSON_AddNumberToObject(root, "special",5);
    cJSON_AddNumberToObject(root, "val",val);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    if (kanal>0) send_STM(root);
    if (kanal==0) send_wifi(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(5000));
    xSemaphoreGive(searchQueueMutex);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "set_fade");
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"val",msg.name[0]);
    send_AK(pay,pck,is_mqtt);
}


void command_get_qkanal(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, kanal = 255;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);

    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    char *mm;
    asprintf(&mm,"%04X:10:01:%02d#",create_command(adr,false,false,QUERY_CHN_FEATURE),kanal);
    myUart->send(mm);
    free(mm);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(3000));
    xSemaphoreGive(searchQueueMutex);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "get_qkanal");
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"val",msg.name[0]>>5);
    send_AK(pay,pck,is_mqtt);
}

void command_set_color(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, kanal = 255, w=0, a=0, f=0, r=0, g=0, b=0, typ=0;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);
    JSON_getint(payload,"w", &w);
    JSON_getint(payload,"a", &a);
    JSON_getint(payload,"f", &f);
    JSON_getint(payload,"r", &r);
    JSON_getint(payload,"g", &g);
    JSON_getint(payload,"b", &b);
    JSON_getint(payload,"type", &typ);

  //  typ = 2; //0 Rgb 1 waf 2 rgbwaf

     char *mm;
    asprintf(&mm,"%04X:10:0C:%02X:07:%02X:%02X:%02X:%02X:%02X:%02X:%02X:99#",
        create_command(adr,false,false,typ),kanal,
        r,g,b,w,a,f,typ);
    myUart->send(mm);

    printf("%s\n",mm);

    free(mm); 

       
    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "get_color");
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"r",r);
    cJSON_AddNumberToObject(pay,"g",g);
    cJSON_AddNumberToObject(pay,"b",b);
    cJSON_AddNumberToObject(pay,"w",w);
    cJSON_AddNumberToObject(pay,"a",a);
    cJSON_AddNumberToObject(pay,"f",f);
    cJSON_AddNumberToObject(pay,"type",typ);
    send_AK(pay,pck,is_mqtt);
}

void command_get_color(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, kanal = 255, typ=0;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);
    JSON_getint(payload,"type", &typ);

    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    char *mm;
    asprintf(&mm,"%04X:10:0B:%02d:%02X#",create_command(adr,false,false,QUERY_COLOR_VALUE),kanal,typ);
    myUart->send(mm);
    free(mm);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(5000));
    xSemaphoreGive(searchQueueMutex);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "get_color");
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"r",msg.name[0]);
    cJSON_AddNumberToObject(pay,"g",msg.name[1]);
    cJSON_AddNumberToObject(pay,"b",msg.name[2]);
    cJSON_AddNumberToObject(pay,"w",msg.name[3]);
    cJSON_AddNumberToObject(pay,"a",msg.name[4]);
    cJSON_AddNumberToObject(pay,"f",msg.name[5]);
    send_AK(pay,pck,is_mqtt);
}

void command_go_scene(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255, grp=0xFF, scn=0xFF;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"gurup", &grp);
    JSON_getint(payload,"scene", &scn);
    JSON_getint(payload,"kanal", &kanal);

    uint8_t cmd = GOTO_SCENE+scn;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "send");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,(grp==1)?true:false,false,cmd));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    send_STM(root);
    cJSON_Delete(root);
        
    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "go_scene");
    send_AK(pay,pck, is_mqtt);
}

void command_set_level(cJSON *payload, pck_t *pck, uint8_t cmd, const char *txt,bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255, val=0xFF;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);
    JSON_getint(payload,"val", &val);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,false,false,cmd));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    cJSON_AddNumberToObject(root, "special",4);
    cJSON_AddNumberToObject(root, "val",val);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    if (kanal>0) send_STM(root);
    if (kanal==0) send_wifi(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(5000));
    xSemaphoreGive(searchQueueMutex);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", txt);
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"val",msg.name[0]);
    send_AK(pay,pck,is_mqtt);
}

void command_get_detail(cJSON *payload, pck_t *pck,bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,false,false,0x00));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    cJSON_AddNumberToObject(root, "special",3);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    if (kanal>0) send_STM(root);
    if (kanal==0) send_wifi(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(3000));
    xSemaphoreGive(searchQueueMutex);
    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "get_detail");
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"max",msg.name[0]);
    cJSON_AddNumberToObject(pay,"min",msg.name[1]);
    cJSON_AddNumberToObject(pay,"err",msg.name[2]);
    cJSON_AddNumberToObject(pay,"pwr",msg.name[3]);
    cJSON_AddNumberToObject(pay,"fade",msg.name[4]);
    cJSON_AddNumberToObject(pay,"efade",msg.name[5]);
    cJSON_AddNumberToObject(pay,"pmin",msg.name[6]);
    send_AK(pay,pck, is_mqtt);

}


uint8_t get_scn(uint8_t adr, uint8_t kanal, uint8_t scn)
{
    uint8_t cmd = QUERY_SCENE+(scn);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,false,false,cmd));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    if (kanal>0) send_STM(root);
    if (kanal==0) send_wifi(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(3000));
    xSemaphoreGive(searchQueueMutex);
    return msg.name[0];
}

void command_del_scene(cJSON *payload, pck_t *pck,bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255, scn=255;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);
    JSON_getint(payload,"scene", &scn);
    uint8_t cmd = REMOVE_SCENE + (scn);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "send");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,false,false,cmd));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    cJSON_AddNumberToObject(root, "double",1);
    if (kanal>0) send_STM(root);
    if (kanal==0) send_wifi(root);
    cJSON_Delete(root);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "scene_status");
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"scene",scn);
    cJSON_AddNumberToObject(pay,"value",0xFF);
    send_AK(pay,pck,is_mqtt);
}

void command_set_scene(cJSON *payload, pck_t *pck,bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255, scn=255, val = 0xFF;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);
    JSON_getint(payload,"scene", &scn);
    JSON_getint(payload,"value", &val);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,false,false,0x00));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    cJSON_AddNumberToObject(root, "special",2);
    cJSON_AddNumberToObject(root, "scene",scn);
    cJSON_AddNumberToObject(root, "val",val);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    if (kanal>0) send_STM(root);
    if (kanal==0) send_wifi(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(3000));
    xSemaphoreGive(searchQueueMutex);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "scene_status");
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"scene",scn);
    cJSON_AddNumberToObject(pay,"value",msg.name[0]);
    send_AK(pay,pck,is_mqtt);

}
void command_get_scene(cJSON *payload, pck_t *pck,bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255, scn=255;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);
    JSON_getint(payload,"scene", &scn);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "scene_status");
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"scene",scn);
    cJSON_AddNumberToObject(pay,"value",get_scn(adr,kanal,scn));
    send_AK(pay,pck,is_mqtt);
}

void command_identfy(cJSON *payload, pck_t *pck,bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "send");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,false,false,IDENTFY_DEVICE));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    cJSON_AddNumberToObject(root, "double",1);
    bool cm = (kanal>0 && kanal<10);
    if (cm) send_STM(root);
    if (cm|| kanal==0) send_wifi(root);
    cJSON_Delete(root);
    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "identfy");
    send_AK(pay,pck, is_mqtt);
}

/*
   Gear a ait gurupları geardan sorgulayarak gear dosyasına yazar
*/
int query_grp(uint8_t adr, uint8_t kanal, uint8_t *L, uint8_t *H)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,false,false,QUERY_GROUP_L));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    cJSON_AddNumberToObject(root, "special",1);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    if (kanal>0) send_STM(root);
    if (kanal==0) send_wifi(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    BaseType_t got = xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(3000));
    xSemaphoreGive(searchQueueMutex);
    if (got==pdTRUE) {
        uint8_t LL=msg.name[0], HH=msg.name[1];
       *L=LL;
       *H=HH;
       if (kanal==1) gear01.set_gurup(adr,LL,HH);
       if (kanal==2) gear02.set_gurup(adr,LL,HH);
       if (kanal==3) gear03.set_gurup(adr,LL,HH);

    } else return -1;
    return 1;
}

void command_set_gurup(cJSON *payload, pck_t *pck,bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255, grp=255, typ=255;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);
    JSON_getint(payload,"gurup", &grp);
    JSON_getint(payload,"type", &typ);
    uint8_t cmd = (typ==1)? ADD_GROUP+(grp) : REMOVE_GROUP + (grp);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "send");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,false,false,cmd));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    cJSON_AddNumberToObject(root, "double",1);
    if (kanal>0) send_STM(root);
    if (kanal==0) send_wifi(root);
    cJSON_Delete(root);

    vTaskDelay(200/portTICK_PERIOD_MS);
    uint8_t L=0, H=0;
    query_grp(adr,kanal,&L,&H);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "set_gurup");
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"low",L);
    cJSON_AddNumberToObject(pay,"high",H);
    send_AK(pay,pck,is_mqtt);
}

void command_get_gurup(cJSON *payload, pck_t *pck,bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);

    uint8_t L=0, H=0;
    query_grp(adr,kanal,&L,&H);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "get_gurup");
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"low",L);
    cJSON_AddNumberToObject(pay,"high",H);
    send_AK(pay,pck, is_mqtt);
}

void command_get_level(cJSON *payload, pck_t *pck,bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255, stt=0;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,false,false,QUERY_ACTUAL_LEVEL));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    bool cm = (kanal>0 && kanal<10);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    if (cm) send_STM(root);
    if (cm || kanal==0) send_wifi(root);
    if (kanal==10) {
        //bu cihaz lokaldedir. adr üzerinden cihazı bulup komutu ona gönder
        for (Base_Device* cihaz : cihaz_listesi)
        {
            if (cihaz->device_id==adr)
              {
                stt=cihaz->get_level();
              }
        }
    }
    cJSON_Delete(root);

    searchMessage_t msg = {};
    if (kanal!=10) {
        xQueueReset(searchQueue);
        xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(3000));
    } else {
        msg.name[0]=stt;
    }
    xSemaphoreGive(searchQueueMutex);
   
    
   // printf("GET LEVEL %d\n",msg.name[0]);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "get_level");
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"power",msg.name[0]);
    send_AK(pay,pck, is_mqtt);
}

void command_qstatus(cJSON *payload, pck_t *pck,bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255, stt=0;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"kanal", &kanal);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,false,false,QUERY_STATUS));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    bool cm = (kanal>0 && kanal<10);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    if (cm) send_STM(root);
    if (cm || kanal==0) send_wifi(root);
    if (kanal==10) {
        //bu cihaz lokaldedir. adr üzerinden cihazı bulup komutu ona gönder
        for (Base_Device* cihaz : cihaz_listesi)
        {
            if (cihaz->device_id==adr)
              {
                stt=cihaz->get_status();
              }
        }
    }

    cJSON_Delete(root);

    searchMessage_t msg = {};
    if (kanal!=10) {
      xQueueReset(searchQueue);
      xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(3000));
    } else {
      msg.name[0]=stt;
    }
    xSemaphoreGive(searchQueueMutex);


   // printf("QSTATUS %d\n",msg.name[0]);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "qstatus");
    cJSON_AddNumberToObject(pay,"adres",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"status",msg.name[0]);
    send_AK(pay,pck, is_mqtt);
}


void command_action(cJSON *payload, pck_t *pck, bool is_mqtt=false, bool say=true)
{
    uint8_t adr = 0, grp= 200, kanal = 255, cmd=255;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"gurup", &grp);
    JSON_getint(payload,"kanal", &kanal);
    JSON_getint(payload,"komut", &cmd);

    uint8_t hexcom = 0;
    switch (cmd)
     {
        case 1 : hexcom=0x0A; break; //Son durum;
        case 2 : hexcom=0x06; break; //En Düşük
        case 3 : hexcom=0x05; break; //En Yüksek
        case 4 : hexcom=0x08; break; //on and up
        case 5 : hexcom=0x07; break; //down and off
        case 6 ... 21: {
            hexcom = cmd + 10; //Goto scene 1 (Hex 0x10=dec 16)
        } break;
        case 22 : hexcom=0x01; break; //up
        case 23 : hexcom=0x02; break; //down
        case 24 : hexcom=0x03; break; //step up
        case 25 : hexcom=0x04; break; //step down
        case 26 : hexcom=0x09; break; //enable dapc
        break;
     } 

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "send");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,(grp==0)?false:true,false,hexcom));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    bool cm = (kanal>0 && kanal<10);
    if (cm) send_STM(root);
    if (cm || kanal==0) send_wifi(root);
    if (kanal==10) {
        //bu cihaz lokaldedir. adr üzerinden cihazı bulup komutu ona gönder 
        for (Base_Device* cihaz : cihaz_listesi) 
        {
            if (cihaz->device_id==adr || adr==0xff)
              {
                cihaz->set_action(cmd, say);
              }
        }
    }

    cJSON_Delete(root);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "action");
    send_AK(pay,pck, is_mqtt);
}

void command_arc(cJSON *payload, pck_t *pck, bool is_mqtt=false, bool say=true)
{
    uint8_t adr = 0, grp= 200, kanal = 255, pow=255;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"gurup", &grp);
    JSON_getint(payload,"kanal", &kanal);
    JSON_getint(payload,"power", &pow);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "send");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,(grp==0)?false:true,true,pow));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);

    bool cm = (kanal>0 && kanal<10);
    if (cm) send_STM(root);
    if (cm || kanal==0) send_wifi(root);
    if (kanal==10 || kanal==9) {
        //bu cihaz lokaldedir. adr üzerinden cihazı bulup komutu ona gönder
        //kanal==9: STM tarafındaki tüm DALI kanallarını hedefleyen broadcast — yerel
        //(DALI olmayan) cihazlar bu broadcast'in kapsamında olmadığından burada da uygulanır.
        //adr==0xff (broadcast) durumunda sadece LAMBA tipindeki (0x76) cihazlar hedeflenir;
        //gaz/su vanası, kontaktör, asansör, perde, priz gibi cihazlar toplu komutla
        //etkilenmemeli. Tekil hedefli çağrılarda (adr==device_id) tip filtresi uygulanmaz.
        for (Base_Device* cihaz : cihaz_listesi)
        {
            bool hedef = (cihaz->device_id==adr) ||
                         (adr==0xff && cihaz->device_exttype==0x76);
            bool syy = say;
            if (adr==255 && cihaz->device_exttype==0x76) syy = false; //broadcast ile tüm lambaları kapatırken sayma yapılmaz             
            if (hedef)
              {
                cihaz->set_power(pow, syy);
              }
        }
    }
    cJSON_Delete(root);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "arc_power");
    send_AK(pay,pck, is_mqtt);
}

void command_off(cJSON *payload, pck_t *pck, bool is_mqtt= false, bool say=true)
{
    uint8_t adr = 0, grp= 200, kanal = 255;
    JSON_getint(payload,"adres", &adr);
    JSON_getint(payload,"gurup", &grp);
    JSON_getint(payload,"kanal", &kanal);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "send");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,grp,false,0x00));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "bit",16);
    bool cm = (kanal>0 && kanal<10);
    if (cm) send_STM(root);
    if (cm || kanal==0) send_wifi(root);
    if (kanal==10 || kanal==9) {
        //bu cihaz lokaldedir. adr üzerinden cihazı bulup komutu ona gönder
        //kanal==9: STM tarafındaki tüm DALI kanallarını kapatan broadcast — yerel
        //(DALI olmayan) cihazlar bu broadcast'in kapsamında olmadığından burada da kapatılır.
        //adr==0xff (broadcast) durumunda sadece LAMBA tipindeki (0x76) cihazlar hedeflenir;
        //gaz/su vanası, kontaktör, asansör, perde, priz gibi cihazlar "tüm lambaları kapat"
        //ile kapanmamalı. Tekil hedefli çağrılarda (adr==device_id) tip filtresi uygulanmaz.
        for (Base_Device* cihaz : cihaz_listesi)
        {
            bool hedef = (cihaz->device_id==adr) ||
                         (adr==0xff && cihaz->device_exttype==0x76);
            bool syy = say;
            if (adr==255 && cihaz->device_exttype==0x76) syy = false; //broadcast ile tüm lambaları kapatırken sayma yapılmaz   

            if (hedef)
              {
                cihaz->off(syy);
              }
        }
    }
    cJSON_Delete(root);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "off");     
    send_AK(pay,pck, is_mqtt);
}

void room_delete(cJSON *payload, pck_t *pck)
{
    uint8_t id = 200;
    JSON_getint(payload,"id", &id);
    if (id<200) {
        cJSON *pay = cJSON_CreateObject();  
        cJSON_AddStringToObject(pay, "com", "del_oda");
        cJSON_AddNumberToObject(pay, "id", id);
        send_AK(pay,pck);
        room.room_delete(id);
    } 
}

void switch_delete(cJSON *payload, pck_t *pck)
{
    uint8_t id = 200;
    JSON_getint(payload,"id", &id);
    if (id<200) {
        cJSON *pay = cJSON_CreateObject();  
        cJSON_AddStringToObject(pay, "com", "del_switch");
        cJSON_AddNumberToObject(pay, "id", id);
        send_AK(pay,pck);
        anahtar.switch_delete(id);
    } 
}

void switch_create(cJSON *payload, pck_t *pck)
{
    uint8_t tp=0xff, sid=0xff, sin=0xff, rid=0xff, rka=0xff,schn=255;
    char *txt = (char*)calloc(1,17);
    JSON_getstring(payload,"txt", txt,16);
    JSON_getint(payload,"tip", &tp);
    JSON_getint(payload,"sid", &sid);
    JSON_getint(payload,"sin", &sin);
    JSON_getint(payload,"rid", &rid);
    JSON_getint(payload,"rka", &rka);
    JSON_getint(payload,"senschn", &schn);

    anahtar.switch_create(txt,tp,sid,sin,rid,rka,schn);
    free(txt);
    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "cre_switch");
    send_AK(pay,pck);
}




void room_create(cJSON *payload, pck_t *pck)
{
    char *txt = (char*)calloc(1,17);
    JSON_getstring(payload,"txt", txt,16);
    room.room_create(txt);
    free(txt);
    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "cre_oda");
    send_AK(pay,pck);
}




void small_intro(cJSON *payload, pck_t *pck)
{
    uint8_t room = 0;
    JSON_getint(payload,"room", &room);

    cJSON *pay = cJSON_CreateObject();                         
    cJSON_AddStringToObject(pay, "com", "small_intro");
    cJSON_AddNumberToObject(pay, "room", room);
    send_AK(pay,pck);


    cJSON *pay1 = cJSON_CreateObject();                         
    cJSON_AddStringToObject(pay1, "com", "small_intro");
    cJSON_AddNumberToObject(pay1, "room", room);

    cJSON *gear = cJSON_CreateArray();
    cJSON *sw = cJSON_CreateArray();
    gear01.gear_small_intro(room, gear);
    gear02.gear_small_intro(room, gear);
    gear03.gear_small_intro(room, gear);
    gear04.gear_small_intro(room, gear);
    anahtar.small_intro(room,sw);

    cJSON_AddItemToObject(pay1, "gear", gear);
    cJSON_AddItemToObject(pay1, "sw", sw);

    send_AK(pay1,pck);
}


void gear_intro(cJSON *payload, pck_t *pck)
{
    uint8_t typ = 0;
    JSON_getint(payload,"type", &typ);

    if (typ==1) {
        cJSON *pay1 = cJSON_CreateObject();                         
        cJSON_AddStringToObject(pay1, "com", "intro");
        cJSON_AddNumberToObject(pay1, "kanal", 10);
        cJSON_AddItemToObject(pay1, "gear", gear10.gear_intro());
        send_AK(pay1,pck);
        vTaskDelay(100/portTICK_PERIOD_MS);
    }


    cJSON *pay = cJSON_CreateObject();                         
    cJSON_AddStringToObject(pay, "com", "intro");
    cJSON_AddNumberToObject(pay, "kanal", typ);
    if (typ==1) cJSON_AddItemToObject(pay, "gear", gear01.gear_intro());
    if (typ==2) cJSON_AddItemToObject(pay, "gear", gear02.gear_intro());
    if (typ==3) cJSON_AddItemToObject(pay, "gear", gear03.gear_intro());
    if (typ==0) cJSON_AddItemToObject(pay, "gear", gear04.gear_intro());
    
    send_AK(pay,pck);

}

void room_intro(pck_t *pck)
{
    cJSON *pay = cJSON_CreateObject();                         
    cJSON_AddStringToObject(pay, "com", "room_intro");
    cJSON_AddItemToObject(pay, "room", room.room_intro());
    send_AK(pay,pck);
}

void switch_intro(pck_t *pck)
{
    cJSON *pay = cJSON_CreateObject();                         
    cJSON_AddStringToObject(pay, "com", "switch_intro");
    cJSON_AddItemToObject(pay, "switch", anahtar.switch_intro());
    send_AK(pay,pck);
}

void gurup_intro(pck_t *pck)
{
    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "grpintro");
    cJSON_AddItemToObject(pay, "grp", gurup.gurup_intro());
    send_AK(pay,pck);
}

void gurup_name(cJSON *payload, pck_t *pck)
{
    uint8_t grpid = 0;
    char *txt = (char*)calloc(1,21);
    JSON_getint(payload,"grpid", &grpid);
    JSON_getstring(payload,"name", txt,20);
    gurup.set_group_name(grpid, txt);
    free(txt);
    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "grpname");
    send_AK(pay,pck);
}

void scene_intro(pck_t *pck)
{
    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "scnintro");
    cJSON_AddItemToObject(pay, "scn", scene.gurup_intro());
    send_AK(pay,pck);
}

void scene_name(cJSON *payload, pck_t *pck)
{
    uint8_t sceneid = 0;
    char *txt = (char*)calloc(1,21);
    JSON_getint(payload,"sceneid", &sceneid);
    JSON_getstring(payload,"name", txt,20);
    scene.set_group_name(sceneid, txt);
    free(txt);
    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "scnname");
    send_AK(pay,pck);
}


void instance_intro(pck_t *pck)
{
    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "ins_intro");
    cJSON *scn = cJSON_CreateArray();
    instance.instance_intro(scn);  // DALI kayıtları
    instanceL.instance_intro(scn); // Yerel kayıtlar (device.json ile boot'ta senkronize edilir, bkz. Local_Device_Read)
    cJSON_AddItemToObject(pay, "scn", scn);
    send_AK(pay,pck);
}

void set_token(cJSON *payload,pck_t *pck)
{
    char *nm, *uid, *fc;
    nm = (char*)calloc(1,16);
    uid = (char*)calloc(1,64);
    fc = (char*)calloc(1,255);
    JSON_getstring(payload,"user", nm,15);
    JSON_getstring(payload,"UUID", uid,63);
    JSON_getstring(payload,"token", fc,250);
    fcm_t ff = {};
    strcpy((char *)ff.uuid,(char *)uid);
    strcpy((char *)ff.fcm,(char *)fc);
    strcpy((char *)ff.name,(char *)nm);
    fcm.update_fcm(&ff);
    free(nm);
    free(uid);
    free(fc);
}


void alarm_onoff(cJSON *payload,pck_t *pck)
{        
    uint8_t stat=0;

    JSON_getint(payload, "status", &stat);

    if (stat==9) {
        cJSON *pay = cJSON_CreateObject();                                   
        cJSON_AddStringToObject(pay, "com", "alarm");
        cJSON_AddNumberToObject(pay, "status", 9);           
        send_AK(pay,pck);

        cJSON *root = cJSON_CreateObject(); 
        cJSON_AddStringToObject(root, "com", "alarm");
        cJSON_AddNumberToObject(root, "status", 9);
        char *dat = cJSON_PrintUnformatted(root);
        udp_server.send_unicast_all((uint8_t *)dat,strlen(dat));
        cJSON_Delete(root);

        GlobalConfig.alarm_aktif = 0;
        disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
    }    

}

void send_version(pck_t *pck)
{
    uint8_t cs = 0;
    if (GlobalConfig.kanal1==1) cs++;
    if (GlobalConfig.kanal2==1) cs++;
    if (GlobalConfig.kanal3==1) cs++;
   // if (GlobalConfig.kanal4==1) cs++;

    vTaskDelay(100/portTICK_PERIOD_MS);
         
            cJSON *pay = cJSON_CreateObject();                                   
            cJSON_AddStringToObject(pay, "com", "get_version");
            cJSON_AddNumberToObject(pay, "id", GlobalConfig.device_id);
            cJSON_AddStringToObject(pay, "embeded", (char*)GlobalConfig.version);
            cJSON_AddNumberToObject(pay, "kanal", cs);
            cJSON_AddStringToObject(pay, "admin", (char*)GlobalConfig.admin);
            cJSON_AddStringToObject(pay, "mqtt", (char*)GlobalConfig.mqtt);
            cJSON_AddStringToObject(pay, "lic", (char*)GlobalConfig.license);
            cJSON_AddNumberToObject(pay, "ai", GlobalConfig.Ai);
            if (GlobalConfig.kanal4==1)
                 cJSON_AddNumberToObject(pay, "wifi", 1);
            cJSON_AddNumberToObject(pay, "sulama", GlobalConfig.sulama);   
            cJSON_AddNumberToObject(pay, "aydinlatma", GlobalConfig.aydinlatma);     
            send_AK(pay,pck);
}

// Telefon05: kullanıcı bir Role (ext_type==0x07) cihazın ikonunu ampul/aydınlatma
// temalı bir ikonla değiştirdiğinde gönderilir. İkonun kendisi kutuya gönderilmez
// (o telefonun kendi lokal tercihi) — sadece bu cihazın "tüm lambaları aç" gibi
// kategori fan-out'una dahil olup olmayacağı bilgisi (bkz. apply_category_fanout,
// gear.h dev_spec_t.reserved[0]) kutuya bildirilir.
void command_set_lamp_override(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    uint8_t adr = 0, kanal = 255, val = 0;
    JSON_getint(payload,"id", &adr);
    JSON_getint(payload,"kanal", &kanal);
    JSON_getint(payload,"val", &val);

    if (kanal==1) gear01.set_lamp_override(adr,val);
    if (kanal==2) gear02.set_lamp_override(adr,val);
    if (kanal==3) gear03.set_lamp_override(adr,val);
    if (kanal==0) gear04.set_lamp_override(adr,val);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "set_lamp_override");
    cJSON_AddNumberToObject(pay,"id",adr);
    cJSON_AddNumberToObject(pay,"kanal",kanal);
    cJSON_AddNumberToObject(pay,"val",val);
    send_AK(pay,pck,is_mqtt);
}

void command_save_dev(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, kanal = 255;
    char *nm = (char*)calloc(1,25);

    JSON_getstring(payload,"nm", nm,24);
    JSON_getint(payload, "id", &adr);
    JSON_getint(payload, "kanal", &kanal);

    gear_t gg = {};
    if (kanal==1) gear01.read_gear(adr,&gg);
    if (kanal==2) gear02.read_gear(adr,&gg);
    if (kanal==3) gear03.read_gear(adr,&gg);
    if (kanal==0) gear04.read_gear(adr,&gg);
    if (kanal==10) gear10.read_gear(adr,&gg);

    strcpy((char *)gg.name,nm);    
    JSON_getint(payload, "ico", &gg.ico);
    JSON_getint(payload, "vr", &gg.room);
    JSON_getint(payload, "an", &gg.anahtar);

    if (kanal==1) gear01.write_gear(adr,&gg);
    if (kanal==2) gear02.write_gear(adr,&gg);
    if (kanal==3) gear03.write_gear(adr,&gg);
    if (kanal==0) gear04.write_gear(adr,&gg);
    if (kanal==10) gear10.write_gear(adr,&gg);

    // İsim/oda değişikliği reset beklemeden sesli komut aramasına yansısın diye
    // RAM önbelleğini (names + room_entries) tazeliyoruz.
    if (kanal==1) gear01.refresh_names();
    if (kanal==2) gear02.refresh_names();
    if (kanal==3) gear03.refresh_names();
    if (kanal==0) gear04.refresh_names();
    if (kanal==10) gear10.refresh_names();

    free(nm);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "save_dev");
    send_AK(pay,pck,is_mqtt);
}

#include "tools/anahtar_tool.cpp"
#include "tools/refresh_tool.cpp"

#define CHECK_BIT(var, pos) (((var) >> (pos)) & 1)

uint8_t anahtar_onoff(uint8_t onoff, uint8_t adr, uint8_t kanal, uint8_t ins)
{
    // Yerel (kanal=10) instance'lar için fiziksel DALI hattı yok — doğrulama
    // sorgusu göndermeden istenen değeri doğrudan kabul et.
    if (kanal==10) return onoff;

    //Instance cihaz üzerinden kapatır veya açar
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", adr);
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "special",12);
    cJSON_AddNumberToObject(root, "par1",ins);
    cJSON_AddNumberToObject(root, "par2",onoff);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    send_STM(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(3000));
    xSemaphoreGive(searchQueueMutex);
    return msg.name[0];
}

void command_get_instance(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, kanal=0x00;
  
    JSON_getint(payload, "adres", &adr);
    JSON_getint(payload, "kanal", &kanal);
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", create_command(adr,false,false,0x00));
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "special",7);
    send_STM(root);
    cJSON_Delete(root);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "get_instance");
    cJSON_AddItemToObject(pay, "adr", cJSON_CreateNumber(adr));
    cJSON_AddItemToObject(pay, "kanal", cJSON_CreateNumber(kanal));   
    send_AK(pay,pck,is_mqtt);

    xEventGroupClearBits(searchGROUP, BIT_STOP);

    EventBits_t uxBits = xEventGroupWaitBits(
        searchGROUP,
        BIT_STOP, // Bu üç bitten herhangi birini bekle
        pdTRUE,        // Bitler gelince otomatik temizlensin (Clear on exit)
        pdFALSE,       // "Wait for ALL" false: Herhangi biri gelince uyan
        10000/portTICK_PERIOD_MS  // Sonsuza kadar bekle
        );

    if (uxBits & BIT_STOP) {
        cJSON *pay = cJSON_CreateObject();  
        cJSON_AddStringToObject(pay, "com", "search");
        cJSON_AddStringToObject(pay, "txt", "Arama bitti. Alt Cihaz Tanımı ile Sisteminizi yenileyiniz. ");
        send_AK(pay,pck,is_mqtt);
    } else {
        cJSON *pay = cJSON_CreateObject();  
        cJSON_AddStringToObject(pay, "com", "search");
        cJSON_AddStringToObject(pay, "txt", "Arama Başarısız");
        send_AK(pay,pck,is_mqtt);
    }
}

void command_set_instance(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, ins = 0, kan = 0xff, act=0xff;

    JSON_getint(payload, "adr", &adr); //Adres
    JSON_getint(payload, "ins", &ins); //Instance
    JSON_getint(payload, "kanal", &kan); //Kanal

    //Instance Oku (katalog kaydı) — yoksa (örn. henüz set edilmemiş yerel anahtar) ESP_ERR_NOT_FOUND döner
    instance_t fileins = {};
    esp_err_t found = pick_instance(kan).get_instance(kan, adr, ins, &fileins);
    fileins.channel = kan;
    fileins.dev_addr = adr;
    fileins.ins_addr = ins;

    uint8_t typ=0xff;
    JSON_getint(payload, "type", &typ); //Instance tipi (buton/ısı/hareket) - opsiyonel
    if (typ!=0xff) fileins.type = typ;
    else if (found!=ESP_OK && kan==10) fileins.type = INSTANCE_TYPE_BUTTON; // yeni yerel kayıt, tip belirtilmedi

    uint8_t cmtype=0xff;
    JSON_getint(payload, "cmtype", &cmtype); //Hedef türü: lamba/grup/senaryo/anahtar (opsiyonel)
    if (cmtype!=0xff) fileins.com = (instance_command_t)cmtype;

    uint8_t pro=0xff;
    JSON_getint(payload, "pro", &pro); //Hedefe uygulanacak eylem (opsiyonel)
    if (pro!=0xff) fileins.process = (instance_process_type_t)pro;

    JSON_getint(payload, "cmadr", &fileins.com_addr);      //Hedef adresi (opsiyonel)
    JSON_getint(payload, "ins_kanal", &fileins.lamp_channel); //Hedef kanalı (opsiyonel)
    JSON_getint(payload, "tset", &fileins.temp_set); //MOTION: retrigger timer süresi (sn, opsiyonel)

    JSON_getint(payload, "act", &act); //Instance Aktif/Pasif

    uint8_t act1 = anahtar_onoff(act,adr,kan,ins);
    if (act1>1) anahtar_onoff(act,adr,kan,ins);
    if (act1>1) anahtar_onoff(act,adr,kan,ins);
    if (act1>1) return;

    fileins.ins_active = act1;
    pick_instance(kan).set_instance(kan, adr, ins, &fileins);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "set_instance");
    cJSON_AddItemToObject(pay, "chn", cJSON_CreateNumber(fileins.channel));
    cJSON_AddItemToObject(pay, "adr", cJSON_CreateNumber(fileins.dev_addr));
    cJSON_AddItemToObject(pay, "ins", cJSON_CreateNumber(fileins.ins_addr));
    cJSON_AddItemToObject(pay, "type", cJSON_CreateNumber(fileins.type));
    cJSON_AddItemToObject(pay, "act", cJSON_CreateNumber(fileins.ins_active));
    cJSON_AddItemToObject(pay, "cmtype", cJSON_CreateNumber(fileins.com));
    cJSON_AddItemToObject(pay, "pro", cJSON_CreateNumber(fileins.process));
    cJSON_AddItemToObject(pay, "cmadr", cJSON_CreateNumber(fileins.com_addr));
    cJSON_AddItemToObject(pay, "ins_kanal", cJSON_CreateNumber(fileins.lamp_channel));
    cJSON_AddItemToObject(pay, "tset", cJSON_CreateNumber(fileins.temp_set));
    send_AK(pay,pck,is_mqtt);

}

void command_query_instance(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, ins = 0, kan = 0xff;

    JSON_getint(payload, "adr", &adr);
    JSON_getint(payload, "ins", &ins);
    JSON_getint(payload, "kanal", &kan);

    instance_t fileins = {};
    pick_instance(kan).get_instance(kan, adr, ins, &fileins);
    // Kayıt henüz katalogda yoksa (örn. hiç set edilmemiş yerel anahtar) fileins sıfır
    // kalır — sorgulanan gerçek adresi yine de yansıt ki app doğru kaydı eşleştirebilsin.
    fileins.channel = kan;
    fileins.dev_addr = adr;
    fileins.ins_addr = ins;

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "set_instance");
    cJSON_AddItemToObject(pay, "chn", cJSON_CreateNumber(fileins.channel));
    cJSON_AddItemToObject(pay, "adr", cJSON_CreateNumber(fileins.dev_addr));
    cJSON_AddItemToObject(pay, "ins", cJSON_CreateNumber(fileins.ins_addr));
    cJSON_AddItemToObject(pay, "type", cJSON_CreateNumber(fileins.type));
    cJSON_AddItemToObject(pay, "act", cJSON_CreateNumber(fileins.ins_active));
    cJSON_AddItemToObject(pay, "cmtype", cJSON_CreateNumber(fileins.com));
    cJSON_AddItemToObject(pay, "pro", cJSON_CreateNumber(fileins.process));
    cJSON_AddItemToObject(pay, "cmadr", cJSON_CreateNumber(fileins.com_addr));
    cJSON_AddItemToObject(pay, "ins_kanal", cJSON_CreateNumber(fileins.lamp_channel));
    send_AK(pay,pck,is_mqtt);
}


uint8_t instance_filter_set(uint8_t adr, uint8_t kanal, uint8_t ins, uint8_t val)
{

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", adr);
    cJSON_AddNumberToObject(root, "kanal",kanal);
    cJSON_AddNumberToObject(root, "scene",ins);
    cJSON_AddNumberToObject(root, "val",val);
    cJSON_AddNumberToObject(root, "special",9);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    send_STM(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(10000));
    xSemaphoreGive(searchQueueMutex);
    return msg.name[0];
}

void command_set_ins_filter(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, ins = 0, kan = 0xff, val=0x00;
  
    JSON_getint(payload, "adr", &adr);
    JSON_getint(payload, "ins", &ins);
    JSON_getint(payload, "kanal", &kan);
    JSON_getint(payload, "val", &val);

    instance_t fileins = {};
    pick_instance(kan).get_instance(kan, adr, ins, &fileins);
    fileins.filter = val;

        if (instance_filter_set(adr,kan,ins,fileins.filter)!=1)
          if (instance_filter_set(adr,kan,ins,fileins.filter)!=1)
            if (instance_filter_set(adr,kan,ins,fileins.filter)!=1) return;

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "sfilter");
    cJSON_AddItemToObject(pay, "adr", cJSON_CreateNumber(fileins.dev_addr));
    cJSON_AddItemToObject(pay, "ins", cJSON_CreateNumber(fileins.ins_addr));
    cJSON_AddItemToObject(pay, "filter", cJSON_CreateNumber(fileins.filter));
    send_AK(pay,pck,is_mqtt);
    pick_instance(kan).set_instance(kan, adr, ins, &fileins);
}

void command_query_ins_filter(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, ins = 0, kan = 0xff;
  
    JSON_getint(payload, "adr", &adr);
    JSON_getint(payload, "ins", &ins);
    JSON_getint(payload, "kanal", &kan);

    uint32_t cmm = (((adr<<1)|1) << 16)| (ins<<8) | (0x90) ; 

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", cmm);
    cJSON_AddNumberToObject(root, "kanal",kan);
    cJSON_AddNumberToObject(root, "bit",24);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    send_STM(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(5000));
    xSemaphoreGive(searchQueueMutex);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "qfilter");
    cJSON_AddItemToObject(pay, "adr", cJSON_CreateNumber(adr));
    cJSON_AddItemToObject(pay, "ins", cJSON_CreateNumber(ins));
    cJSON_AddItemToObject(pay, "kanal", cJSON_CreateNumber(kan));
    cJSON_AddItemToObject(pay, "filter", cJSON_CreateNumber(msg.name[0]));    
    send_AK(pay,pck,is_mqtt);
}

void command_query_ins_timer(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, ins = 0, kan = 0xff;
  
    JSON_getint(payload, "adr", &adr);
    JSON_getint(payload, "ins", &ins);
    JSON_getint(payload, "kanal", &kan);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", adr);
    cJSON_AddNumberToObject(root, "kanal",kan);
    cJSON_AddNumberToObject(root, "val",ins);
    cJSON_AddNumberToObject(root, "special",10);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    send_STM(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(10000));
    xSemaphoreGive(searchQueueMutex);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "qstimer");
    cJSON_AddItemToObject(pay, "adr", cJSON_CreateNumber(adr));
    cJSON_AddItemToObject(pay, "ins", cJSON_CreateNumber(ins));
    cJSON_AddItemToObject(pay, "short", cJSON_CreateNumber(msg.name[0]));  
    cJSON_AddItemToObject(pay, "long", cJSON_CreateNumber(msg.name[1])); 
    cJSON_AddItemToObject(pay, "stuck", cJSON_CreateNumber(msg.name[2]));   
    send_AK(pay,pck,is_mqtt);
}

void command_set_ins_timer(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, ins = 0, kan = 0xff, shrt=0xff, lng=0xff, stk=0xff;
  
    JSON_getint(payload, "adr", &adr);
    JSON_getint(payload, "ins", &ins);
    JSON_getint(payload, "kanal", &kan);
  
    JSON_getint(payload, "short", &shrt);
    JSON_getint(payload, "long", &lng);
    JSON_getint(payload, "stuck", &stk);


    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", adr);
    cJSON_AddNumberToObject(root, "kanal",kan);
    cJSON_AddNumberToObject(root, "val",ins);
    cJSON_AddNumberToObject(root, "special",11);
    cJSON_AddNumberToObject(root, "par1",shrt);
    cJSON_AddNumberToObject(root, "par2",lng);
    cJSON_AddNumberToObject(root, "par3",stk);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    send_STM(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(10000));
    xSemaphoreGive(searchQueueMutex);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "sstimer");
    cJSON_AddItemToObject(pay, "adr", cJSON_CreateNumber(adr));
    cJSON_AddItemToObject(pay, "ins", cJSON_CreateNumber(ins));   
    send_AK(pay,pck,is_mqtt);
}

void command_get_amode(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, kan = 0xff;
  
    JSON_getint(payload, "adr", &adr);
    JSON_getint(payload, "kanal", &kan);


    uint32_t cmm = (((adr<<1)|1) << 16)| (0xFE<<8) | (0x3E) ; 
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", cmm);
    cJSON_AddNumberToObject(root, "kanal",kan);
    cJSON_AddNumberToObject(root, "bit",24);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    send_STM(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(10000));
    xSemaphoreGive(searchQueueMutex);

    cJSON *pay = cJSON_CreateObject();
    cJSON_AddStringToObject(pay, "com", "get_amode");
    cJSON_AddItemToObject(pay, "adr", cJSON_CreateNumber(adr));
    cJSON_AddItemToObject(pay, "kanal", cJSON_CreateNumber(kan));
    cJSON_AddItemToObject(pay, "mode", cJSON_CreateNumber(msg.name[0]));
       
    send_AK(pay,pck,is_mqtt);
}

void command_set_amode(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, kan = 0xff, mode=0xff;
  
    JSON_getint(payload, "adr", &adr);
    JSON_getint(payload, "kanal", &kan);
    JSON_getint(payload, "mode", &mode);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", adr);
    cJSON_AddNumberToObject(root, "kanal",kan);
    cJSON_AddNumberToObject(root, "special",8);
    cJSON_AddNumberToObject(root, "value",mode);
    xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
    send_STM(root);
    cJSON_Delete(root);

    searchMessage_t msg = {};
    xQueueReset(searchQueue);
    xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(10000));
    xSemaphoreGive(searchQueueMutex);

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "get_amode");
    cJSON_AddItemToObject(pay, "adr", cJSON_CreateNumber(adr));
    cJSON_AddItemToObject(pay, "kanal", cJSON_CreateNumber(kan));
    cJSON_AddItemToObject(pay, "mode", cJSON_CreateNumber(msg.name[0]));
       
    send_AK(pay,pck,is_mqtt);
}

void command_put_device(cJSON *payload, pck_t *pck, bool is_mqtt)
{
    uint8_t adr = 0, kan = 0xff;
  
    /*Yapılan degişikliklerin kalıcı olmasını saglar*/

    JSON_getint(payload, "id", &adr);
    JSON_getint(payload, "kanal", &kan);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "query");
    cJSON_AddNumberToObject(root, "hexcom", adr);
    cJSON_AddNumberToObject(root, "kanal",kan);
    cJSON_AddNumberToObject(root, "special",13);
    send_STM(root);
    cJSON_Delete(root);
       
    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "put_dev");
    cJSON_AddItemToObject(pay, "adr", cJSON_CreateNumber(adr));
    send_AK(pay,pck,is_mqtt);
}

void fill_ins(instance_t *ins, cJSON *pay)
{
    cJSON_AddStringToObject(pay, "com", "push_temp");
    cJSON_AddNumberToObject(pay, "adres", ins->dev_addr); 
    cJSON_AddNumberToObject(pay, "ins", ins->ins_addr); 
    cJSON_AddNumberToObject(pay, "temp", ins->temp);
    cJSON_AddNumberToObject(pay, "relay", ins->status);
    cJSON_AddNumberToObject(pay, "set", ins->temp_set);
    cJSON_AddNumberToObject(pay, "error", 0);
    cJSON_AddNumberToObject(pay, "type", ins->temp_type);
}


void command_get_temp(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    uint8_t adr = 0, kan = 0xff, ains=0xff;
  
    JSON_getint(payload, "adres", &adr);
    JSON_getint(payload, "ins", &ains);
    JSON_getint(payload, "kanal", &kan);

    instance_t ins = {};
    esp_err_t kk= pick_instance(kan).get_instance(kan,adr,ains, &ins);
    if (kk==ESP_OK) {
        cJSON *pay = cJSON_CreateObject();

        uint32_t cmm = (((ins.dev_addr<<1)|1) << 16)| (ins.ins_addr<<8) | (0xBC) ;
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "query");
        cJSON_AddNumberToObject(root, "hexcom", cmm);
        cJSON_AddNumberToObject(root, "kanal",kan);
        cJSON_AddNumberToObject(root, "bit",24);
        xSemaphoreTake(searchQueueMutex, portMAX_DELAY);
        send_STM(root);
        cJSON_Delete(root);
        searchMessage_t msg = {};
        xQueueReset(searchQueue);
        xQueueReceive(searchQueue, &msg, pdMS_TO_TICKS(10000));
        xSemaphoreGive(searchQueueMutex);
        ins.temp = msg.name[0];
        temp_role_degerlendir(&ins);
        fill_ins(&ins,pay);
        send_AK(pay,pck, is_mqtt);
    }

}

void command_set_temp(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    uint8_t adr = 0, kan = 0xff, ains=0xff, set=0x00;
  
    JSON_getint(payload, "adres", &adr);
    JSON_getint(payload, "ins", &ains);
    JSON_getint(payload, "kanal", &kan);
    JSON_getint(payload, "set", &set);

    instance_t ins = {};
    esp_err_t kk= pick_instance(kan).get_instance(kan,adr,ains, &ins);
    if (kk==ESP_OK) {
        ins.temp_set = set;
        temp_role_degerlendir(&ins);
        pick_instance(kan).set_instance(kan,adr,ains, &ins);
        cJSON *pay = cJSON_CreateObject();                      
        fill_ins(&ins,pay);
        send_AK(pay,pck, is_mqtt);
    }
}
void command_set_tmode(cJSON *payload, pck_t *pck, bool is_mqtt=false)
{
    uint8_t adr = 0, kan = 0xff, ains=0xff, set=0x00;
  
    JSON_getint(payload, "adres", &adr);
    JSON_getint(payload, "ins", &ains);
    JSON_getint(payload, "kanal", &kan);
    JSON_getint(payload, "mode", &set);

    instance_t ins = {};
    esp_err_t kk= pick_instance(kan).get_instance(kan,adr,ains, &ins);
    if (kk==ESP_OK) {
        ins.temp_type = set;
        temp_role_degerlendir(&ins);
        pick_instance(kan).set_instance(kan,adr,ains, &ins);
        cJSON *pay = cJSON_CreateObject();                      
        fill_ins(&ins,pay);
        send_AK(pay,pck, is_mqtt);
    }
}