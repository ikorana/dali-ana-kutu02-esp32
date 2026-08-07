
#include "irrigation.h"
#include "jsontool.h"


const float K_MEVSIM_TABLOSU[12] = {
    0.0,  // Ocak (Sulama kapalı)
    0.0,  // Şubat (Sulama kapalı)
    0.3,  // Mart (Erken ilkbahar, uyanış)
    0.7,  // Nisan (İlkbahar, dengeli)
    1.0,  // Mayıs (Referans ay - Taban süre tam uygulanır)
    1.3,  // Haziran (Yaz başlangıcı, sıcaklık artışı)
    1.6,  // Temmuz (Zirve sıcaklık, yüksek buharlaşma)
    1.5,  // Ağustos (Yüksek sıcaklık)
    1.1,  // Eylül (Sonbahar geçişi)
    0.6,  // Ekim (Serinleme, yağış başlangıcı)
    0.2,  // Kasım (Kışa geçiş)
    0.0   // Aralık (Sulama kapalı)
};

uint16_t Irrigation::calc_time(uint16_t tm, const char *time_str)
{

    if (smart>0) {
        int ai_minutes = firebase_get_irrigation_duration(time_str, tm);
        if (ai_minutes >= 0) {
            return (uint16_t)ai_minutes;
        }
        ESP_LOGW("SULAMA", "AI sulama kararı alınamadı, yerel mevsim tablosuna düşülüyor.");
    }

    float katsayi = K_MEVSIM_TABLOSU[Ay];
    if (smart>0) return tm  * katsayi;
        else return tm ;
}


/*
    Bir schedule'daki tüm segmentlerin sulama saati aynıdır, sadece süreleri farklıdır.
    Bu yüzden AI'ye/yerel tabloya sadece BİR referans segment üzerinden tek sorulur.
    Çıkan oran sch->durations'a hiç yazılmaz (kalıcı referans kirlenmesin diye);
    sadece sulama_orani alanına kaydedilir, uygulama noktasında (sulama_task) kullanılır.
*/
void Irrigation::apply_ai_adjustment(Schedule *sch)
{
    sulama_orani = 1.0f;

    if (smart==0 || sch==nullptr) return;

    uint16_t reference_tm = 0;
    for (SulamaAdimi &seg : sch->durations) {
        if (seg.duration > 0) { reference_tm = seg.duration; break; }
    }
    if (reference_tm == 0) return;

    char time_str[8];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", sch->saat, sch->dakika);

    uint16_t adjusted_tm = calc_time(reference_tm, time_str);

    sulama_orani = (float)adjusted_tm / (float)reference_tm;

    ESP_LOGI("SULAMA", "\033[1;35mAI/Mevsim katsayısı: %.2f (referans %d sn -> %d sn)", sulama_orani, reference_tm, adjusted_tm);
}

void Irrigation::sunset_event(uint8_t state, event_state_t* ev)  {
          //Güneş batınca ne yapacagını buraya yaz
          gunes = 0;          
    }

    void Irrigation::sunrise_event(uint8_t state, event_state_t* ev)  {
        gunes = 1;         
    }

void Irrigation::time_update_event(uint8_t hour, uint8_t minute, uint8_t daynum) {
           //Zamanlamaların içinde gün var mı
           for (Schedule *sch : schedules) {
                for (uint8_t day : sch->active_days) {
                    if (day == daynum) {
                        //Eger bir zamanlamada gün bulunuyorsa
                        //printf("Hedef %02d:%02d Su An: %02d:%02d\n",sch->saat,sch->dakika,hour,minute);
                        if (sch->saat==hour && sch->dakika==minute) {
                            if (!is_running)
                            {
                            //Sulamayı başlat
                            time_t now;
                            struct tm timeinfo;
                            time(&now);
                            localtime_r(&now, &timeinfo);
                            Ay = timeinfo.tm_mon+1;
                            current_schedule = sch;
                            is_running = true; // Flag'i set et                   
                            // FreeRTOS Taskını oluşturup tetikliyoruz
                            xTaskCreatePinnedToCore(
                                sulama_task,     // Task fonksiyonu
                                "sulama_task",   // Task adı
                                4096,            // Stack boyutu (JSON işlemleri veya loglar için 4KB ideal)
                                this,            // 'this' pointer'ı argüman olarak gidiyor (void *arg)
                                5,               // Task önceliği
                                NULL,            // Task handle gerekmiyor
                                1                // Core 1 (Core 0 genelde Wi-Fi/BT tarafından kullanılır)
                            );
                          }                           
                        }

                    }
                }
            }
        }    


void Irrigation::set_events(cJSON *pay) {
           uint8_t eid = 0; 
           if (pay==NULL) return; 
           if (!active) return;

           JSON_getint(pay,"event_id", &eid);
           for (event_state_t* ev : events) {
                if (ev->event_id == eid) {
                   //Event bulundu, işleme al
                   char * event_type = (char *)calloc(1,32);
                   JSON_getstring(pay,"event", event_type,32);
                   //Time UPDATE EVENT
                   if (strcmp(event_type, "CLOCK_UPDATE_EVENT") == 0) {
                       uint8_t hour = 0, minute = 0, daynum = 0;
                       JSON_getint(pay,"hour", &hour);
                       JSON_getint(pay,"minute", &minute);
                       JSON_getint(pay,"daynum", &daynum);
                       time_update_event(hour, minute,daynum);
                       //printf("CLOCK UPDATE EVENT %d %d %d\n",hour,minute,daynum);
                   }
                   if (strcmp(event_type, "SUNSET_EVENT") == 0) {
                       uint8_t state = 0;
                       JSON_getint(pay,"state", &state);
                       sunset_event(state, ev);
                   }
                   if (strcmp(event_type, "SUNRISE_EVENT") == 0) {
                       uint8_t state = 0;
                       JSON_getint(pay,"state", &state);
                       sunrise_event(state, ev);
                   }
                   if (strcmp(event_type, "RAIN_DETECTED_EVENT") == 0) {
                       uint8_t state = 0;
                       JSON_getint(pay,"state", &state);

                   }

                   free(event_type);
                   break;
                
                }
            }
        }

void Irrigation::sulama_task(void *arg) {
    Irrigation *irrigation = (Irrigation *)arg;
    Schedule *sch = const_cast<Schedule*>(irrigation->current_schedule);
    
    if (sch == nullptr || sch->durations.empty()) {
        ESP_LOGW("SULAMA", "Hata: Gecerli bir sulama programi bulunamadi.");
        irrigation->is_running = false;
        vTaskDelete(NULL);
        return;
    }

    uint8_t max_id = 0;
    for (SulamaAdimi& seg: sch->durations) {
        if (seg.sira > max_id) max_id = seg.sira;        
    }
    
    if (max_id == 0) {
        irrigation->is_running = false;
        vTaskDelete(NULL);
        return;
    }

    irrigation->apply_ai_adjustment(sch);

    irrigation->send_sulama_status();

    uint8_t current_id = 1;
    irrigation->stop_requested = false;
    bool motor_calisiyor = false;

    // Bir önceki adımdan açık kalan vanaları takip etmek için liste
    std::vector<uint8_t> previous_active_vanes; 

    ESP_LOGI("SULAMA", "\033[1;35m=== Basinc Korumali (Overlap) Sulama Basladi ===\033[0m");

    while (current_id <= max_id) {
        if (irrigation->stop_requested) break;

        // --- 1. AŞAMA: Mevcut adımdaki vanaları tespit et ---
        std::vector<std::pair<uint8_t, uint16_t>> active_relays;
        for (SulamaAdimi& seg : sch->durations) {
            if (seg.sira == current_id && seg.duration > 0) {
                // seg.duration dakika cinsindendir; countdown döngüsü saniyede bir çalıştığı için
                // burada saniyeye çevriliyor (son hamle).
                active_relays.push_back({seg.relay_id, (uint16_t)(seg.duration * irrigation->sulama_orani * 1)});
            }
        }

        // Eğer bu adım boşsa direkt geç
        if (active_relays.empty()) {
            current_id++;
            continue;
        }

        // --- 2. AŞAMA: ÖNCE YENİ ADIMIN VANALARINI AÇ (BASINÇ RAHATLATMA) ---
        for (const auto& [relay_id, duration] : active_relays) {
            irrigation->set_relay(relay_id, true);
            ESP_LOGI("SULAMA", "\033[1;35mAdim %d: Yeni Bolge Vanasi (Role %d) acildi.", current_id, relay_id);
        }

        // --- 3. AŞAMA: YENİ VANALAR AÇILDIKTAN SONRA ESKİ VANALARI KAPAT (OVERLAP) ---
        if (!previous_active_vanes.empty()) {
            // Mekanik vananın/selonoidin fiziksel olarak açılması için 1.5 saniye tolerans payı
            // Bu esnada hem eski hem yeni vanalar açık olduğundan hat basıncı güvenli sınırlarda kalır.
            vTaskDelay(pdMS_TO_TICKS(1500)); 
            
            for (uint8_t old_relay_id : previous_active_vanes) {
                irrigation->set_relay(old_relay_id, false);
                ESP_LOGI("SULAMA", "\033[1;35mGecis Tamamlandi: Eski Vana (Role %d) kapatildi.", old_relay_id);
            }
            previous_active_vanes.clear(); // Eski listeyi temizle
        }

        // --- 4. AŞAMA: MOTOR ILK KEZ CALISACAKSA TETIKLE ---
        if (irrigation->motor_relay != 0 && !motor_calisiyor) {
            irrigation->set_relay(irrigation->motor_relay, true);
            motor_calisiyor = true;
            ESP_LOGI("SULAMA", "\033[1;35m>> ANA MOTOR (Role %d) CALISTIRILDI. <<", irrigation->motor_relay);
            vTaskDelay(pdMS_TO_TICKS(irrigation->motor_time)); 
        }

        // --- 5. AŞAMA: ZAMAN TAKİBİ DÖNGÜSÜ ---
        while (!active_relays.empty()) {
            vTaskDelay(pdMS_TO_TICKS(1000));

            if (irrigation->stop_requested) {
                ESP_LOGW("SULAMA", "\033[1;35mAcil durdurma algilandi!");
                break;
            }

            for (auto it = active_relays.begin(); it != active_relays.end(); ) {
                it->second--;

                if (it->second == 0) {
                    // Süresi biten vanayı hemen kapatmıyoruz, bir sonraki adıma devrediyoruz ki overlap yapabilsin
                    // Ancak bu adım listenin son adımıysa (max_id) veya tek bir adımsa direkt kapatılmalı
                    if (current_id == max_id) {
                        irrigation->set_relay(it->first, false);
                        ESP_LOGI("SULAMA", "\033[1;35mSon Adim: Role %d kapatiliyor.", it->first);
                    } else {
                        // Bir sonraki adımda kapatılmak üzere yedekle
                        previous_active_vanes.push_back(it->first);
                        ESP_LOGI("SULAMA", "\033[1;35mRole %d suresi bitti, gecis icin bekletiliyor.", it->first);
                    }
                    it = active_relays.erase(it);
                } else {
                    it++;
                }
            }
        }

        if (irrigation->stop_requested) break;

        current_id++;
    }

    // --- SİSTEM KAPANIŞ GÜVENLİĞİ (ABORT VEYA BİTİŞ) ---
    ESP_LOGI("SULAMA", "\033[1;35mKapanis emniyeti devreye giriyor...");

    // 1. Önce Motoru Durdur (Su akışını ve basıncı kes)
    if (irrigation->motor_relay != 0) {
        ESP_LOGI("SULAMA", "\033[1;35mMotor Kapatılıyor\033[0m");
        irrigation->set_relay(irrigation->motor_relay, false);
        vTaskDelay(irrigation->motor_time);
    }
    
    // Hattın deşarj olması için yarım saniye bekle
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI("SULAMA", "\033[1;35mVanalar Kapatılıyor\033[0m");
    // 2. Kalan tüm vanaları ve yedekte bekleyen geçiş vanalarını kapat
    for (uint8_t old_relay_id : previous_active_vanes) {
        irrigation->set_relay(old_relay_id, false);
    }
    for (SulamaAdimi& seg: sch->durations) {
        irrigation->set_relay(seg.relay_id, false);
    }

    ESP_LOGI("SULAMA", "\033[1;35m=== Hidrolik Guvenlikli Kapanis Saglandi. ===\033[0m");
    irrigation->is_running = false;
    irrigation->send_sulama_status();
    vTaskDelete(NULL);
}

void Irrigation::set_relay(uint8_t relay_id, bool state)
{
    for (Base_Port* port : ports) {
        if (port->get_pin_id() == relay_id) {
            if (state==true) {
               port->pin_on();
            } else {
                port->pin_off();           
            } 
            vTaskDelay(pdMS_TO_TICKS(500));          
            return;
        }
    }  
}

void Irrigation::sulama_pin_task(void *arg) {
    Irrigation *irr = (Irrigation *)arg;

    if (irr->current_time == 0 || irr->current_pin == 255)
    {
        irr->is_running = false;
        vTaskDelete(NULL);
        return;
    }
    irr->send_sulama_status();


    ESP_LOGI("SULAMA", "\033[1;35m(Role %d) Valf Açıldı", irr->current_pin);
    irr->set_relay(irr->current_pin, true);


    irr->set_relay(irr->motor_relay, true);
    ESP_LOGI("SULAMA", "\033[1;35m>> ANA MOTOR (Role %d) CALISTIRILDI. <<", irr->motor_relay);
    vTaskDelay(pdMS_TO_TICKS(irr->motor_time)); 

    uint16_t ctime = irr->current_time;
    while (1) {
        if (irr->stop_requested) {
            ESP_LOGW("SULAMA", "\033[1;35mAcil durdurma algilandi!");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        ctime--;
        if (ctime == 0) break;
    }

    ESP_LOGI("SULAMA", "\033[1;35mKapanis emniyeti devreye giriyor...");
    irr->set_relay(irr->motor_relay, false);
    ESP_LOGI("SULAMA", "\033[1;35m>> ANA MOTOR (Role %d) DURDURULDU. <<", irr->motor_relay);
    vTaskDelay(pdMS_TO_TICKS(irr->motor_time)); 

    ESP_LOGI("SULAMA", "\033[1;35m(Role %d) Valf Kapatıldı", irr->current_pin);
    irr->set_relay(irr->current_pin, false);


    ESP_LOGI("SULAMA", "\033[1;35m=== Hidrolik Guvenlikli Kapanis Saglandi. ===\033[0m");
    irr->current_pin=255;
    irr->current_time=0;

    irr->is_running = false;
    irr->send_sulama_status();
    vTaskDelete(NULL);
}

void Irrigation::run_pin(uint8_t seg, uint8_t pin,uint8_t tm)
{
    for (Base_Port* port : ports) {
        if (port->get_pin_id() == pin) {
            is_running = true; // Flag'i set et    
            current_pin = pin;
            current_time = tm;
              
            // FreeRTOS Taskını oluşturup tetikliyoruz
            xTaskCreatePinnedToCore(
                sulama_pin_task,     // Task fonksiyonu
                "sulama_pin_task",   // Task adı
                4096,            // Stack boyutu (JSON işlemleri veya loglar için 4KB ideal)
                this,            // 'this' pointer'ı argüman olarak gidiyor (void *arg)
                5,               // Task önceliği
                NULL,            // Task handle gerekmiyor
                1                // Core 1 (Core 0 genelde Wi-Fi/BT tarafından kullanılır)
            );
            break;
        }
    }
}