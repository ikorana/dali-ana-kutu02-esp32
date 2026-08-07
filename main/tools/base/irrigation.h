

#pragma once

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include <errno.h>
#include <storage_little.h>
#include <cJSON.h>
#include "core.h"
#include "base.h"
#include <vector>

#include "time.h"

// firebase_tool.cpp içinde tanımlanır (aynı çeviri birimine daha sonra dahil edilir).
// Ağ/parse hatasında -1 döner, çağıran yerel plana düşmelidir.
int firebase_get_irrigation_duration(const char *time_str, uint16_t requested_minutes);

struct SulamaAdimi {
    uint8_t sira;
    uint8_t relay_id;
    uint16_t duration;
    Base_Port* port;
};


class Schedule {

public:
    Schedule() {};
    ~Schedule() {};

   // void time_update_event(uint8_t hour, uint8_t minute, uint8_t day);
   // void set_events(cJSON *pay);
    void set_time(const char *time_str) {
        int hour = 0;
        int minute = 0;
        if (sscanf(time_str, "%d:%d", &hour, &minute) == 2) {
            saat = hour;
            dakika = minute;
        } else {
            saat=99;
            dakika=99;
        }
    };
    void add_day(uint8_t day) {
        active_days.push_back(day);
    };
    void add_adim(uint8_t id, uint8_t relay_id, uint16_t time_in_seconds, Base_Port* port) {
        SulamaAdimi adim = {id, relay_id, time_in_seconds, port};
        durations.push_back(adim);  
    }
    uint8_t saat, dakika, sira;    
    std::vector<uint8_t> active_days; // 1-7 (Pazartesi-Pazar)
    std::vector<SulamaAdimi> durations; // relay_id, time_in_seconds
};

class Irrigation {
    public:
        Irrigation() {};
        ~Irrigation() {};

        void send_sulama_status(void) {
            
            cJSON *pay = cJSON_CreateObject();  
            cJSON_AddStringToObject(pay, "com", "get_irrigation");
            cJSON_AddNumberToObject(pay,"running",get_status());
            char *dat = cJSON_PrintUnformatted(pay);
            uint8_t cnt = udp_server->send_unicast_all_count((uint8_t *)dat,strlen(dat));
            ESP_LOGI("SULAMA", "\033[1;35mDurum %d terminale gönderildi\033[0m",cnt);
            vTaskDelay(100/portTICK_PERIOD_MS);
            free(dat);
            cJSON_Delete(pay);
        }

        void set_udp_server(Udp_Server_Gem *srv) {udp_server = srv;}


        bool get_active(void) {return active;}
        void set_active(bool a) {active=a;}
        void set_smart(bool a) {smart=a;}
        
        uint8_t get_status(void) {return is_running;}
        void add_events(event_state_t *ev) {
            events.push_back(ev);
        };
        
        void run_pin(uint8_t seg, uint8_t pin,uint8_t tm);

        void add_schedule(Schedule *sch) {
            schedules.push_back(sch);

        };
        void add_port(Base_Port *prt) {
            ports.push_back(prt);

        }

        void set_motor(uint8_t motor, uint16_t time, Base_Port *prt) {
            motor_relay = motor;
            motor_time = time;
            ports.push_back(prt);

        }
        

        void set_events(cJSON *pay);
        void sunset_event(uint8_t state, event_state_t* ev);
        void sunrise_event(uint8_t state, event_state_t* ev);
        void time_update_event(uint8_t hour, uint8_t minute, uint8_t daynum);

        void schedule_stop(void) {
            if (is_running) stop_requested = true;
        };


        void schedule_run(char *tm) {
            int hour = 0;
            int minute = 0;
            if (sscanf(tm, "%d:%d", &hour, &minute) == 2) {
                for (Schedule *sch : schedules) {
                    
                    printf("Hedef %02d:%02d Aranan: %02d:%02d\n",sch->saat,sch->dakika,hour,minute);

                    if (sch->saat==hour && sch->dakika==minute) {
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
        };


        void list(void) {
             ESP_LOGI("SULAMA","\033[1;34mSulama Sistemi");
             ESP_LOGI("SULAMA","\033[1;34m==========================================");
             ESP_LOGI("SULAMA","\033[1;34mMotor Relay ID : %d",motor_relay);
             ESP_LOGI("SULAMA","\033[1;34mMotor Time     : %d",motor_time);
             ESP_LOGI("SULAMA","\033[1;34mActive         : %d",active);

             ESP_LOGI("SULAMA","\033[1;34mZamanlamalar");
             ESP_LOGI("SULAMA","\033[1;34m==============");
             for (Schedule *sch : schedules) {
                ESP_LOGI("SULAMA","\033[1;34m     Saat   : %02d:%02d",sch->saat,sch->dakika);

                char *mm = (char*) calloc(1,30);
                char *kk;
                for (uint8_t day : sch->active_days) {
                    asprintf(&kk,"%d,",day);
                    strcat(mm,kk);
                    free(kk);
                }

                ESP_LOGI("SULAMA","\033[1;34m     Gunler : %s",mm);
                free(mm);

                ESP_LOGI("SULAMA","\033[1;34m     Segmentler");
                ESP_LOGI("SULAMA","\033[1;34m     ==========");
                for (SulamaAdimi& seg: sch->durations) {
                    // Burada doğrudan değişken isimlerini kullanabilirsin
                    ESP_LOGI("SULAMA","\033[1;34m     SegmentID: %d, RoleID: %d, Sure: %d", seg.sira, seg.relay_id, seg.duration);
                } 

                ESP_LOGI("SULAMA","\033[1;34m     Events");
                ESP_LOGI("SULAMA","\033[1;34m     ==========");
                for (event_state_t* ev : events) {
                    ESP_LOGI("SULAMA","\033[1;34m          %d",ev->event_id);     
                }

                ESP_LOGI("SULAMA","\033[1;34m==========================================\033[0m");
                
             }   
        }

        uint16_t calc_time(uint16_t tm, const char *time_str);
        void apply_ai_adjustment(Schedule *sch);
   
    private:
       Udp_Server_Gem *udp_server;
       bool active;
       bool smart;
       uint8_t motor_relay;
       uint16_t motor_time;
       uint8_t gunes;
       uint8_t Ay=0;
       float sulama_orani=1.0f; // apply_ai_adjustment tarafından hesaplanır, sch->durations'a hiç yazılmaz
       volatile Schedule* current_schedule;
    
       volatile uint8_t current_pin=255;
       volatile uint8_t current_time=0;

       std::vector<event_state_t*> events;  
       std::vector<Schedule*> schedules;

        volatile bool stop_requested = false;
        volatile bool is_running = false;
        void set_relay(uint8_t relay_id, bool state);
        
        std::vector<Base_Port*> ports;  

       static void sulama_task(void *arg);
       static void sulama_pin_task(void *arg);
};

#include "irrigation.cpp"

