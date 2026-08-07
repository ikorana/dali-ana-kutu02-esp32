


// Tüm firebase task'larının ortak TLS bypass + header kurulumu
static esp_http_client_handle_t firebase_client_init(const char *url, uint32_t timeout_ms,
                                                      http_event_handle_cb event_handler = NULL,
                                                      void *user_data = NULL)
{
    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = timeout_ms;
    config.transport_type = HTTP_TRANSPORT_OVER_SSL;

    // TLS bypass ayarlarımız (menuconfig ile tam uyumlu)
    config.cert_pem = NULL;
    config.crt_bundle_attach = NULL;
    config.use_global_ca_store = false;
    config.skip_cert_common_name_check = true;

    if (event_handler) {
        config.event_handler = event_handler;
        config.user_data = user_data;
    }

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Firebase Callable için en kararlı Content-Type başlığı
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_header(client, "User-Agent", "curl/8.1.2");

    return client;
}

// content-length bazlı basit response okuma/loglama (chunked olmayan cevaplar için)
static void firebase_log_response(esp_http_client_handle_t client)
{
    int64_t content_length = esp_http_client_get_content_length(client);
    if (content_length > 0) {
        char *response_buffer = (char *)malloc(content_length + 1);
        if (response_buffer) {
            int read_len = esp_http_client_read_response(client, response_buffer, content_length);
            if (read_len >= 0) {
                response_buffer[read_len] = '\0';
                ESP_LOGI(TAG, "Firebase Yanıt Gövdesi: %s", response_buffer);
            }
            free(response_buffer);
        }
    }
}

/*
    ilgili degişkenleri kontrol eder eğer degişmiş ise firebase e kaydeder.
*/
void firebase_register_task(void *param)
{
    ESP_LOGI(TAG, "Firebase Register işlemleri yapılıyor");

    bool a0 = (GlobalConfig.Aproject_number==GlobalConfig.project_number);
    bool a1 = (GlobalConfig.AbinaNo==GlobalConfig.binaNo);
    bool a2 = (GlobalConfig.AkatNo==GlobalConfig.katNo);
    bool a3 = (GlobalConfig.AdaireNo==GlobalConfig.daireNo);
    bool a4 = (strcmp((char*)GlobalConfig.Alicense,(char *)GlobalConfig.license)==0);
    bool a5 = (strcmp((char*)GlobalConfig.Aip,(char *)NetworkConfig.ip)==0);

    if (!a0 || !a1 || !a2 || !a3 || !a4 || !a5) {
        GlobalConfig.Aproject_number=GlobalConfig.project_number;
        GlobalConfig.AbinaNo=GlobalConfig.binaNo;
        GlobalConfig.AkatNo=GlobalConfig.katNo;
        GlobalConfig.AdaireNo=GlobalConfig.daireNo;
        strcpy((char*)GlobalConfig.Alicense,(char *)GlobalConfig.license);
        strcpy((char*)GlobalConfig.Aip,(char *)NetworkConfig.ip);
        disk.file_control(GLOBAL_FILE);
        disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
    } else {
       vTaskDelete(NULL);  
       return;
    }


    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);

    // Curl şeması ile birebir uyumlu dinamik veri enjeksiyonu
    cJSON_AddStringToObject(data, "lisansKodu", (char *)GlobalConfig.license);
    cJSON_AddNumberToObject(data, "projeNo", GlobalConfig.project_number);
    cJSON_AddNumberToObject(data, "binaNo", GlobalConfig.binaNo);
    cJSON_AddNumberToObject(data, "katNo", GlobalConfig.katNo);
    cJSON_AddNumberToObject(data, "daireNo", GlobalConfig.daireNo);
    cJSON_AddStringToObject(data, "version", (char *)GlobalConfig.version);
    cJSON_AddStringToObject(data, "ip", (char *)NetworkConfig.ip);

    char *post_data = cJSON_PrintUnformatted(root);
    ESP_LOGI(TAG, "Gönderilecek JSON Verisi: %s", post_data);

    esp_http_client_handle_t client = firebase_client_init(
        "https://us-central1-telefon05.cloudfunctions.net/registerHardwareDevice", 5000);

    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    // Gönderme ve Yanıt Kontrolü
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        firebase_log_response(client);

        ESP_LOGI(TAG, "HTTP POST Gerçekleşti. Durum Kodu: %d", status_code);

        if (status_code == 200) {
            // Tam bu noktada cihaz belleğe "Kayıt Başarılı" flag'ini yazabilir
            ESP_LOGI(TAG, "Ana Kutu Firebase'e başarıyla register oldu.");
        } else {
            ESP_LOGE(TAG, "Firebase uygulama seviyesinde hata döndürdü. Kod: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST İsteği Başarısız! Hata Kodu: %s", esp_err_to_name(err));
    }

    // Bellek Temizliği
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(post_data);

    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "Firebase Register TAMAMLANDI");

    vTaskDelete(NULL); 
}

/*
    Sadece lat/lon degismisse Firebase'e konum guncellemesi gonderir.
*/
void firebase_update_location_task(void *param)
{
    ESP_LOGI(TAG, "Firebase Konum Güncelleme kontrol ediliyor");

    bool a6 = (GlobalConfig.Alat==GlobalConfig.lat);
    bool a7 = (GlobalConfig.Alon==GlobalConfig.lon);

    if (a6 && a7) {
        vTaskDelete(NULL);
        return;
    }

    GlobalConfig.Alat=GlobalConfig.lat;
    GlobalConfig.Alon=GlobalConfig.lon;
    disk.file_control(GLOBAL_FILE);
    disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);

    cJSON_AddStringToObject(data, "lisansKodu", (char *)GlobalConfig.license);
    cJSON_AddNumberToObject(data, "lat", GlobalConfig.lat);
    cJSON_AddNumberToObject(data, "lng", GlobalConfig.lon);

    char *post_data = cJSON_PrintUnformatted(root);
    ESP_LOGI(TAG, "Gönderilecek JSON Verisi: %s", post_data);

    esp_http_client_handle_t client = firebase_client_init(
        "https://us-central1-telefon05.cloudfunctions.net/updateDeviceLocation", 5000);

    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        firebase_log_response(client);

        ESP_LOGI(TAG, "HTTP POST Gerçekleşti. Durum Kodu: %d", status_code);

        if (status_code == 200) {
            ESP_LOGI(TAG, "Konum Firebase'e güncellendi.");
        } else {
            ESP_LOGE(TAG, "Firebase uygulama seviyesinde hata döndürdü. Kod: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST İsteği Başarısız! Hata Kodu: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(post_data);

    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "Firebase Konum Güncelleme TAMAMLANDI");

    vTaskDelete(NULL);
}

void firebase_notification_task(void *param)
{
    char *txt = (char *)param;

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);

    // Curl şeması ile birebir uyumlu dinamik veri enjeksiyonu
    cJSON_AddStringToObject(data, "type", "license");
    cJSON_AddStringToObject(data, "target", (char *)GlobalConfig.license);
    cJSON_AddStringToObject(data, "title", "🚨 ALARM 🚨");
    cJSON_AddStringToObject(data, "body", txt);
    cJSON_AddStringToObject(data, "text", txt);
    char *post_data = cJSON_PrintUnformatted(root);
    ESP_LOGI(TAG, "Gönderilecek JSON Verisi: %s", post_data);


    esp_http_client_handle_t client = firebase_client_init(
        "https://us-central1-telefon05.cloudfunctions.net/sendAlarmNotification", 5000);

    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    // Gönderme ve Yanıt Kontrolü
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        firebase_log_response(client);

        ESP_LOGI(TAG, "HTTP POST Gerçekleşti. Durum Kodu: %d", status_code);

        if (status_code == 200) {
            // Tam bu noktada cihaz belleğe "Kayıt Başarılı" flag'ini yazabilir
            ESP_LOGI(TAG, "Gönderildi.");
        } else {
            ESP_LOGE(TAG, "Uygulama seviyesinde hata döndürdü. Kod: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST İsteği Başarısız! Hata Kodu: %s", esp_err_to_name(err));
    }

    // Bellek Temizliği
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(post_data);

    vTaskDelay(pdMS_TO_TICKS(100));

    vTaskDelete(NULL); 
}

typedef struct {
    char *buffer;
    int buffer_len;
} http_response_t;

static esp_err_t _http0_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Chunked değilse normal okuma
            }
            if (evt->data_len > 0) {
                http_response_t *res = (http_response_t *)evt->user_data;
                char *new_ptr = (char *)realloc(res->buffer, res->buffer_len + evt->data_len + 1);
                if (new_ptr == NULL) {
                    ESP_LOGE("HTTP_EVENT", "Bellek yetersiz!");
                    return ESP_FAIL;
                }
                res->buffer = new_ptr;
                memcpy(res->buffer + res->buffer_len, evt->data, evt->data_len);
                res->buffer_len += evt->data_len;
                res->buffer[res->buffer_len] = '\0';
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}



/*
    getIrrigationDecision'ı çağırır. Konum lisansKodu üzerinden Firestore cache'inden
    okunduğu için burada lat/lng göndermeye gerek yok. AI'nin önerdiği süreyi (dakika)
    döner; ağ hatası/parse hatası durumunda -1 döner (çağıran taraf yerel plana düşmeli).
*/
int firebase_get_irrigation_duration(const char *time_str, uint16_t requested_minutes)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);

    cJSON_AddStringToObject(data, "lisansKodu", (char *)GlobalConfig.license);
    cJSON_AddStringToObject(data, "time", (char *)time_str);
    cJSON_AddNumberToObject(data, "duration", requested_minutes);

    char *post_data = cJSON_PrintUnformatted(root);
    ESP_LOGI(TAG, "Gönderilecek JSON Verisi: %s", post_data);

    http_response_t response_data = { .buffer = NULL, .buffer_len = 0 };

    esp_http_client_handle_t client = firebase_client_init(
        "https://us-central1-telefon05.cloudfunctions.net/getIrrigationDecision", 15000,
        _http0_event_handler, &response_data);

    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    int result_minutes = -1;

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST Gerçekleşti. Durum Kodu: %d", status_code);

        if (response_data.buffer != NULL && response_data.buffer_len > 0) {
            ESP_LOGI(TAG, "Firebase Yanıt Gövdesi: %s", response_data.buffer);

            cJSON *res_root = cJSON_Parse(response_data.buffer);
            if (res_root) {
                cJSON *res_data = cJSON_GetObjectItem(res_root, "data");
                cJSON *dur_item = res_data ? cJSON_GetObjectItem(res_data, "duration") : NULL;
                cJSON *reason_item = res_data ? cJSON_GetObjectItem(res_data, "reason") : NULL;

                if (cJSON_IsNumber(dur_item)) {
                    result_minutes = (int)dur_item->valuedouble;
                    ESP_LOGI(TAG, "AI Sulama Kararı: %d dakika. Gerekçe: %s",
                             result_minutes, cJSON_IsString(reason_item) ? reason_item->valuestring : "-");
                } else {
                    ESP_LOGW(TAG, "Yanıtta 'duration' alanı bulunamadı.");
                }
                cJSON_Delete(res_root);
            }
        } else {
            ESP_LOGW(TAG, "Firebase'den gövde (body) gelmedi veya okunamadı.");
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST İsteği Başarısız! Hata Kodu: %s", esp_err_to_name(err));
    }

    if (response_data.buffer) {
        free(response_data.buffer);
    }
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(post_data);

    return result_minutes;
}

// firebase_voice_task'ın hem "yerel katman başardı" erken dönüşünde hem de AI
// yolunun sonunda ihtiyaç duyduğu ortak temizlik — tek yerde tutulursa erken
// dönüş dalında unutulmaz (daha önce tam da bu unutulmuştu: txt/usr/param
// hiç serbest bırakılmıyordu, yerel katman başarılı olduğunda sızıyordu).
static void firebase_voice_common_cleanup(char *txt, char *usr, search_task_param_t *param)
{
    if (txt) free(txt);
    if (usr) free(usr);
    if (param) {
        if (param->pck.pck) cJSON_Delete(param->pck.pck);
        if (param->pck.rem) free(param->pck.rem);
        if (param->payload) cJSON_Delete(param->payload);
        free(param);
    }
}

void firebase_voice_task(void *pvParameters)
{
    search_task_param_t *param = (search_task_param_t *)pvParameters;

    char *txt = (char*)calloc(1, 255);
    char *usr = (char*)calloc(1, 64);
    JSON_getstring(param->payload, "txt", txt, 254);
    JSON_getstring(param->payload, "user", usr, 63);

    printf("task : %s\n", txt);

    voice_task_param_t prm = {};
    // new_voice_parse_task kendi içinde ing_yap+to_lowercase ile normalize
    // ediyor (bkz. o fonksiyondaki not); client'ın hiç göndermediği ayrı bir
    // "eng" alanına artık ihtiyaç yok — ham txt doğrudan yeterli.
    prm.payload = txt;
    prm.pck = param->pck;
    prm.is_mqtt = param->is_mqtt;

    // Kuyruğu SADECE ilk seferde oluşturuyoruz. Her çağrıda yeniden oluşturmak
    // (eski hâli) hem eski kuyruk nesnesini sızdırıyordu hem de bir önceki
    // çağrının (zaman aşımına uğramış) task'ı geç kalıp yeni oluşturulan
    // kuyruğa yazınca sonucun karışmasına yol açabiliyordu.
    if (xvoiceQueue == NULL) {
        xvoiceQueue = xQueueCreate(1, sizeof(voice_sonuc_t));
    } else {
        // Önceki çağrıdan kalmış olabilecek bayat bir sonucu (zaman aşımından
        // sonra geç gelmiş) temizle ki yeni komutla karışmasın.
        voice_sonuc_t bayat;
        while (xQueueReceive(xvoiceQueue, &bayat, 0) == pdPASS) { }
    }

    xTaskCreatePinnedToCore(
            new_voice_parse_task,           // Task fonksiyonunun adı (YENİ ayrıştırıcı — test aşaması)
            "vptsk",          // Task'ın ismi (debug için)
            8192,               // Stack büyüklüğü (yeni ayrıştırıcı daha fazla vector/cJSON kullanıyor)
            &prm,               // Task'a kopyalanan parametre gönderiliyor
            1,                  // Öncelik sırası (0 en düşük)
            NULL,               // Task handle (referans gerekmiyorsa NULL)
            1                   // Çalışacağı çekirdek (0 veya 1)
        );

    // 20 saniye: 3b/3c fan-out'ta cihaz başına 500ms gecikme var, gerçekçi bir
    // evdeki cihaz sayısında (onlarca değil, birkaç cihaz/kanal) 5sn yetersiz
    // kalabiliyordu. Yine de sınırsız değil — çok büyük bir fan-out bu süreyi de
    // aşarsa firebase_voice_task AI'ye düşer ama yerel task arka planda çalışmaya
    // devam eder (bu durum ayrı, daha kapsamlı bir tasarım gerektirir).
    voice_sonuc_t sonuc = {};
    if (xQueueReceive(xvoiceQueue, &sonuc, pdMS_TO_TICKS(20000)) == pdPASS) {
        // Kod buraya geldiyse diğer task bitmiş ve veriyi başarıyla göndermiştir.
        printf("--- Sonuç Alındı ---\n");
        printf("Komut    : %d\n", sonuc.komut);
        printf("Alt Komut: %d\n", sonuc.altkomut);
        printf("Index    : %d\n", sonuc.index);
    } 

    if (sonuc.komut!=0) {
        char *txt1 = NULL;
        cJSON *pay = cJSON_CreateObject();
        cJSON_AddStringToObject(pay, "com", "say");
        // "refresh" gibi kendi taraması arka planda devam eden komutlarda
        // "Komutunuz uygulandı" yerine kısa bir bekleme mesajı söylenir —
        // asıl sonuç (say) tarama bitince ayrıca gelecek, erken "uygulandı"
        // mesajıyla çakışmasın diye.
        if (sonuc.devam_ediyor) {
            asprintf(&txt1, "Lütfen bekleyiniz.");
        } else {
            asprintf(&txt1, "Komutunuz uygulandı.");
        }
        cJSON_AddStringToObject(pay, "txt", txt1);
        send_AK(pay, &param->pck, param->is_mqtt);
        if(txt1) free(txt1);
        vTaskDelay(pdMS_TO_TICKS(100));
        firebase_voice_common_cleanup(txt, usr, param);
        vTaskDelete(NULL);
        return;
    }

    cJSON *pay = cJSON_CreateObject();  
    cJSON_AddStringToObject(pay, "com", "voice");
    char *txt1 = NULL;
    asprintf(&txt1, "%s komutunuz degerlendiriliyor. Lütfen bekleyiniz.", txt);
    cJSON_AddStringToObject(pay, "txt", txt1);
    send_AK(pay, &param->pck, param->is_mqtt);
    if(txt1) free(txt1);

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);

    cJSON_AddStringToObject(data, "userPrompt", txt);
    cJSON_AddStringToObject(data, "lisansKodu", (char *)GlobalConfig.license);
    cJSON_AddNumberToObject(data, "projeNo", GlobalConfig.project_number);
    cJSON_AddNumberToObject(data, "binaNo", GlobalConfig.binaNo);
    cJSON_AddNumberToObject(data, "daireNo", GlobalConfig.daireNo);
    char *post_data = cJSON_PrintUnformatted(root);
    ESP_LOGI(TAG, "Gönderilecek JSON Verisi: %s", post_data);

    // Yanıtı saklayacağımız yapıyı hazırlıyoruz
    http_response_t response_data = { .buffer = NULL, .buffer_len = 0 };

    esp_http_client_handle_t client = firebase_client_init(
        "https://us-central1-telefon05.cloudfunctions.net/interpretVoiceCommand", 12000,
        _http0_event_handler, &response_data);

    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST Gerçekleşti. Durum Kodu: %d", status_code);
        
        // YANIT BURADA KONTROL EDİLİYOR
        if (response_data.buffer != NULL && response_data.buffer_len > 0) {
            ESP_LOGI(TAG, "Firebase Yanıt Gövdesi: %s", response_data.buffer);
           
            smartq_command_t my_command;


            if (parse_smartq_response(response_data.buffer, &my_command)) {
            if (my_command.is_success) {
                printf("ACTION : %d\n", my_command.action);
                printf("IS_ALL : %d\n", my_command.is_all);

                // TANI (test aşaması): struct_type/target_type/sceneNo/groupNo doğru mu geldi bak.
                ESP_LOGI(TAG, "STRUCT_TYPE:%d DEVICE_TYPE:%d SCENE_NO:%d GROUP_NO:%d NAME:%s ZONE:%s",
                         my_command.struct_type, my_command.target_type,
                         my_command.scene_no, my_command.group_no,
                         my_command.name, my_command.target_zone);

                // responseMessage'ı EN BAŞTA (uygulama sonucunu bilmeden) söylemiyoruz.
                // AI "yapıyorum" dese bile biz yerelde başaramazsak kullanıcıya yanlış bir
                // onay vermiş oluruz — önce dene, GERÇEK sonuca göre konuş.
                const char *soylenecek = my_command.response_message;
                char yerel_mesaj[160] = {0};

                if (my_command.struct_type == STRUCTTYPE_DEVICE) {
                    altkomuttipi_t altkomut = ABOS;
                    if (my_command.action == ACTION_ON) altkomut = AC;
                    if (my_command.action == ACTION_OFF) altkomut = KAPAT;
                    if (my_command.action == ACTION_SET_VALUE) altkomut = GUC;

                    uint8_t guc_yuzde = 254, guc_seviyesi = 254;
                    if (my_command.action == ACTION_SET_VALUE && my_command.has_value) {
                        guc_yuzde = (uint8_t)my_command.value;
                        guc_seviyesi = (uint8_t)((254.0/100.0) * my_command.value);
                    }

                    // targetZone doluysa isAll'a bakmadan HER ZAMAN gerçek bir oda aranır.
                    // Bulunamazsa "tüm cihazlara uygula"ya sessizce düşmüyoruz — "lambaları aç"
                    // ile "oda lambalarını aç" aynı niyet değil; oda belirtilmiş ama tanınamamışsa
                    // komut hiç uygulanmamalı.
                    uint8_t oda_index = 255; // varsayılan: targetZone boşsa oda filtresi yok
                    bool oda_gerekli_ama_bulunamadi = false;
                    if (strlen(my_command.target_zone) > 0) {
                        std::vector<find_param_t> oda_liste = room.get_name();
                        if (oda_liste.size() > 0) oda_index = find_devicename(oda_liste, my_command.target_zone);
                        if (oda_index == 255) oda_gerekli_ama_bulunamadi = true;
                    }

                    uint8_t ext_type = 0;
                    bool tip_destekleniyor = true;
                    switch (my_command.target_type) {
                        case TARGET_LAMP:     ext_type = 0x76; break;
                        case TARGET_BLIND:    ext_type = 0x77; break;
                        case TARGET_ENERGY:   ext_type = 0x78; break;
                        case TARGET_WATER:    ext_type = 0x79; break;
                        case TARGET_GAS:      ext_type = 0x7A; break;
                        case TARGET_ELEVATOR: ext_type = 0x7B; break;
                        case TARGET_SOCKET:   ext_type = 0x7C; break;
                        case TARGET_DOOR:     ext_type = 0x7E; break;
                        case TARGET_GARAGE:   ext_type = 0x7F; break;
                        case TARGET_CLIMATE:
                            tip_destekleniyor = false;
                            ESP_LOGW(TAG, "CLIMATE: ESP tarafı henüz eklenmedi (ext_type ayrılmadı).");
                            break;
                        default:
                            tip_destekleniyor = false;
                            ESP_LOGW(TAG, "Bilinmeyen/desteklenmeyen deviceType.");
                            break;
                    }

                    if (oda_gerekli_ama_bulunamadi) {
                        snprintf(yerel_mesaj, sizeof(yerel_mesaj), "%s adında bir yer bulamadım.", my_command.target_zone);
                        soylenecek = yerel_mesaj;
                    } else if (!tip_destekleniyor) {
                        snprintf(yerel_mesaj, sizeof(yerel_mesaj), "Bu cihaz türünü henüz desteklemiyorum.");
                        soylenecek = yerel_mesaj;
                    } else if (altkomut == ABOS) {
                        snprintf(yerel_mesaj, sizeof(yerel_mesaj), "Ne yapmamı istediğinizi anlayamadım.");
                        soylenecek = yerel_mesaj;
                    } else if (strlen(my_command.name) > 0) {
                        bool bulundu = apply_named_device_command(my_command.name, altkomut,
                                                                   guc_yuzde, guc_seviyesi,
                                                                   &param->pck, param->is_mqtt);
                        if (!bulundu) {
                            snprintf(yerel_mesaj, sizeof(yerel_mesaj), "%s adında bir cihaz bulamadım.", my_command.name);
                            soylenecek = yerel_mesaj;
                        }
                        // bulunduysa soylenecek zaten AI'nin responseMessage'ı olarak kalıyor.
                    } else {
                        int gonderilen = apply_category_fanout(ext_type, oda_index, altkomut, guc_yuzde, guc_seviyesi,
                                                                &param->pck, param->is_mqtt);
                        if (gonderilen == 0) {
                            snprintf(yerel_mesaj, sizeof(yerel_mesaj), "Eşleşen bir cihaz bulamadım.");
                            soylenecek = yerel_mesaj;
                        }
                    }
                } else if (my_command.struct_type == STRUCTTYPE_SCENE || my_command.struct_type == STRUCTTYPE_GROUP) {
                    ESP_LOGI(TAG, "Sesli komut %s hedefliyor -> NAME:%s NO:%d",
                             (my_command.struct_type == STRUCTTYPE_SCENE) ? "SENARYO" : "GRUP",
                             my_command.name,
                             (my_command.struct_type == STRUCTTYPE_SCENE) ? my_command.scene_no : my_command.group_no);

                    // Grup/senaryo dispatch'i henüz yazılmadı — AI'nin iyimser mesajını
                    // (örn. "gece moduna geçiyorum") söylemek yalan olur.
                    snprintf(yerel_mesaj, sizeof(yerel_mesaj), "Bu özelliği henüz eklemedik.");
                    soylenecek = yerel_mesaj;
                }
                // STRUCTTYPE_UNKNOWN / LOCATION / WEATHER: soylenecek zaten AI'nin (veya konum/
                // hava durumu akışında sunucunun gerçek sonuca göre güncellediği) responseMessage'ı
                // — bunlar bir cihaz eylemi vaat etmiyor, dokunmuyoruz.

                cJSON *pay0 = cJSON_CreateObject();
                cJSON_AddStringToObject(pay0, "com", "say");
                cJSON_AddStringToObject(pay0, "txt", soylenecek);
                send_AK(pay0, &param->pck, param->is_mqtt);
                ESP_LOGI(TAG, "Asistan Diyor ki: %s", soylenecek);
            } else {
                ESP_LOGE(TAG, "API Hatası: %s", my_command.error_message);
                cJSON *pay0 = cJSON_CreateObject();
                cJSON_AddStringToObject(pay0, "com", "say");
                cJSON_AddStringToObject(pay0, "txt", "Komutunuzu anlayamadım, lütfen tekrar deneyin.");
                send_AK(pay0, &param->pck, param->is_mqtt);
            }
        }

            
            // İleride gelen JSON'ı parse etmek isterseniz response_data.buffer'ı kullanabilirsiniz:
            // cJSON *res_root = cJSON_Parse(response_data.buffer);
            // ...
        } else {
            ESP_LOGW(TAG, "Firebase'den gövde (body) gelmedi veya okunamadı.");
        }

        if (status_code == 200) {
            ESP_LOGI(TAG, "Gönderildi ve yanıt alındı.");
        } else {
            ESP_LOGE(TAG, "Uygulama seviyesinde hata döndürdü. Kod: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST İsteği Başarısız! Hata Kodu: %s", esp_err_to_name(err));
    }

    // BELLEK TEMİZLİĞİ
    if (response_data.buffer) {
        free(response_data.buffer); // Event handler'ın ayırdığı belleği temizliyoruz
    }
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(post_data);

    firebase_voice_common_cleanup(txt, usr, param);

    vTaskDelay(pdMS_TO_TICKS(100));
    vTaskDelete(NULL);
}