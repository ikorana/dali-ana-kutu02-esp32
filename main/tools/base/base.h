#pragma once


#include <stdint.h>
#include <string> // std::string için
#include <vector> // std::vector için
#include <functional> // std::function için
#include <stdio.h>

#include "freertos/FreeRTOS.h" // SemaphoreHandle_t için
#include "freertos/task.h" // esp_timer_get_time için
#include "freertos/semphr.h" // SemaphoreHandle_t için
#include "esp_timer.h" // esp_timer_get_time için
#include "esp_log.h" // ESP_LOGW için
#include "cJSON.h" // cJSON için
#include "tools/Uart.hpp" // UartDma için
#include "cJSON.h"
#include "jsontool.h"

struct PendingCommand {
    std::string raw_data;
    int pin;
    int target_stat;
    int64_t last_sent_ms;
    int retry_count;
    bool active;
};

class ReliableUartManager {
private:
    std::vector<PendingCommand> _pending_list;
    SemaphoreHandle_t _mutex;
    const int MAX_RETRIES = 3;
    const int TIMEOUT_MS = 500; // Cevap bekleme süresi

public:
    ReliableUartManager() {
        _mutex = xSemaphoreCreateMutex();
    }

    // Komutu kuyruğa ekle ve gönder
    void send_role_command(UartDma* uart, int pin, int stat) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "com", "role");
        cJSON_AddNumberToObject(obj, "pin", pin);
        cJSON_AddNumberToObject(obj, "stat", stat);
        char *dat = cJSON_PrintUnformatted(obj);
        std::string data_str(dat);

        xSemaphoreTake(_mutex, portMAX_DELAY);
        // Aynı pin ve hedef durum için bekleyen bir komut varsa güncelle, yoksa yeni ekle
        bool found = false;
        for (auto &cmd : _pending_list) {
            if (cmd.pin == pin && cmd.target_stat == stat && cmd.active) {
                cmd.raw_data = data_str;
                cmd.last_sent_ms = esp_timer_get_time() / 1000;
                cmd.retry_count = 0;
                found = true;
                break;
            }
        }
        if (!found) {
            _pending_list.push_back({data_str, pin, stat, esp_timer_get_time() / 1000, 0, true});
        }
        xSemaphoreGive(_mutex);

        uart->send(data_str);
        cJSON_free(dat);
        cJSON_Delete(obj);
    }
    // Gelen UART mesajlarını kontrol et ve eşleşen varsa listeden sil
    void process_response(int pin, int stat) {
        xSemaphoreTake(_mutex, portMAX_DELAY);
        for (auto it = _pending_list.begin(); it != _pending_list.end(); ) {
            if (it->pin == pin && it->target_stat == stat) {
                ESP_LOGI("RELIABLE_UART", "ACK received for Pin: %d, Stat: %d", pin, stat);
                it = _pending_list.erase(it); // ACK alındı, listeden çıkar
            } else {
                ++it;
            }
        }
        xSemaphoreGive(_mutex);
    }

    // Zaman aşımına uğrayanları tekrar gönder (Bir Task içinde çalışmalı)
    void check_timeouts(UartDma* uart) {
        int64_t now = esp_timer_get_time() / 1000;
        xSemaphoreTake(_mutex, portMAX_DELAY);
        for (auto &cmd : _pending_list) {
            if (cmd.active && (now - cmd.last_sent_ms > TIMEOUT_MS)) {
                if (cmd.retry_count < MAX_RETRIES) {
                    uart->send(cmd.raw_data);
                    cmd.last_sent_ms = now;
                    cmd.retry_count++;
                    ESP_LOGW("RELIABLE_UART", "Retrying Pin: %d, Stat: %d", cmd.pin, cmd.target_stat);
                } else {
                    cmd.active = false; // Maksimum denemeye ulaşıldı
                    ESP_LOGE("RELIABLE_UART", "Command failed after %d retries for Pin: %d, Target Stat: %d", MAX_RETRIES, cmd.pin, cmd.target_stat);
                    // İsteğe bağlı: Üst katmanlara hatayı bildir
                }
            }
        }
        xSemaphoreGive(_mutex);
    }
};

typedef enum 
{
    BUTTON_ANAHTAR=0,
    BUTTON_SWITCH,
    BUTTON_SENSOR 
} button_tip_t;

// ==========================================
// 1. BASE_EVENT SINIFI
// ==========================================

class Base_Event {
    public:
        Base_Event(const char *nm, const char *cat, uint8_t uid) { 
            asprintf(&type, "%s", nm); 
            asprintf(&category, "%s", cat);
            id = uid;
         }
        virtual ~Base_Event() {}

   // protected:
        uint8_t id;
        char *type;    
        char *category;
};

uint8_t find_event_id(const char *type) {
    for (Base_Event* ev : event_list) {
        if (strcmp(ev->type, type) == 0) {
            return ev->id;
        }
    }
    return 0xFF; // Bulunamadı
}


class In_Base_Port;
// Fonksiyon göstericisi yerine std::function kullanıyoruz
typedef std::function<void(In_Base_Port* port, uint8_t stat)> in_callback_t;


// ==========================================
// 1. IN_BASE_PORT SINIFI
// ==========================================
class In_Base_Port {
    public:
        // Kurucu fonksiyon
        In_Base_Port(uint8_t pin, uint8_t astat, char *typ, char *dsc = nullptr) {
            pin_id = pin;
            active_state = astat;
            asprintf(&type, "%s", typ); // Tip bilgisini dinamik olarak sakla
            asprintf(&desc, "%s", dsc ? dsc : "Unnamed Port");
        
            if (strcmp(type, "button") == 0) {
                button_type = BUTTON_ANAHTAR;
            } else if (strcmp(type, "switch") == 0) {
                button_type = BUTTON_SWITCH;
            } else if (strcmp(type, "sensor") == 0) {
                button_type = BUTTON_SENSOR;
            } else {
                button_type = BUTTON_SWITCH; // Varsayılan
            }
        }
        
        // Çok biçimlilik (Polymorphism) için sanal yıkıcı
        virtual ~In_Base_Port() {
            free(type);
        }

        void set_callback(in_callback_t cb) {
            callback = cb;
        }

        // Pin ID'sini güvenli okumak için getter
        uint8_t get_pin_id(void) const { return pin_id; }
        char *get_desc(void) { return desc; }
        

        in_callback_t callback = nullptr; // Giriş durum değişikliğinde çağrılacak geri arama fonksiyonu  
        uint8_t active_state; // 1: Aktif yüksek, 0: Aktif düşük  
        button_tip_t button_type; // Buton tipi (örneğin, anahtar veya switch)

    private :
       

    protected:
        uint8_t pin_id;        
        char *type; // Port tipi (örneğin "relay", "sensor" vb.)
        uint8_t state; // 0: Kapalı, 1: Açık
        char *desc; // Port açıklaması
                
};

// ==========================================
// 1. BASE_PORT SINIFI
// ==========================================
class Base_Port {
    public:
        // Kurucu fonksiyon
        Base_Port(uint8_t pin, uint8_t astat, char *typ, UartDma *urt, ReliableUartManager* manager, char *dsc = nullptr) {
            pin_id = pin;
            active_state = astat;
            asprintf(&type, "%s", typ); // Tip bilgisini dinamik olarak sakla
            uart = urt;
            _reliable_manager = manager; // Manager'ı sakla
            asprintf(&desc, "%s", dsc ? dsc : "Unnamed Port");
        }
        
        // Çok biçimlilik (Polymorphism) için sanal yıkıcı
        virtual ~Base_Port() {
            free(type);
            free(desc);
        }

        // Port donanım kontrol metotları (Donanım katmanına bağlanacak)
        void pin_on(void) {
            uint8_t target_stat = (active_state == 1) ? 1 : 0;
            _reliable_manager->send_role_command(uart, pin_id, target_stat);
        }

        void pin_off(void) {
            uint8_t target_stat = (active_state == 1) ? 0 : 1;
            _reliable_manager->send_role_command(uart, pin_id, target_stat);
        }

        void pin_toggle(void) {

            //_reliable_manager->send_role_command(uart, pin_id, target_stat);
            // Örn: gpio_set_level((gpio_num_t)pin_id, !gpio_get_level(...));
            //send_role_command(uart, pin_id, !get_status());
            //printf("GPIO %d -> TOGGLED\n", pin_id);
        }

        // Pin ID'sini güvenli okumak için getter
        uint8_t get_pin_id(void) const { return pin_id; }
        char *get_desc(void) { return desc; }
        

    protected:
        uint8_t pin_id;
        uint8_t active_state; // 1: Aktif yüksek, 0: Aktif düşük
        char *type; // Port tipi (örneğin "relay", "sensor" vb.) 
        char *desc; // Port açıklaması       
        UartDma *uart;
        ReliableUartManager* _reliable_manager; // Yeni üye
        
};

// ==========================================
// 2. BASE_DEVICE SINIFI
// ==========================================
class Base_Device {
    public:
        // Varsayılan kurucu fonksiyon, göstericiyi güvenli başlatır
        Base_Device() {device_type=0; device_exttype=0; device_id=0; device_channel=0; name=nullptr; }
        
        // ÇOK KRİTİK: Alt sınıfların (Relay vb.) hafızada kalmasını önleyen sanal yıkıcı
        virtual ~Base_Device() {}

        // Port atama fonksiyonu (İşaretçi olarak adres alır)
        void add_out_port(Base_Port *prt) {
            ports.push_back(prt);
        };

        void add_events(event_state_t *ev) {
            events.push_back(ev);
        };

        virtual void in_callback(In_Base_Port* port, uint8_t stat) {}; 

        void add_in_port(In_Base_Port *prt) {
            // Lambda kullanarak 'this' işaretçisini yakalıyoruz
            prt->set_callback([this](In_Base_Port* port, uint8_t stat) {
                this->in_callback(port, stat);
            });
            in_ports.push_back(prt);
        }

        void set_gear(Gear *gr) {
            this->gear = gr;
        };

        void set_callback(base_callback_t cb) {
            callback = cb;
        }

        void set_saycallback(base_say_callback_t cb) {
            saycallback = cb;
        }


        const char* get_name() const { return name; }

        virtual void set_power(uint8_t pwr, bool say=true) {};
        virtual void off(bool say=true) {};
        virtual void set_action(uint8_t cmd, bool say=true) {};
        virtual void init(void) {};
        
        void button_process(uint8_t pin, uint8_t stat) {
            for (In_Base_Port* in_port : in_ports) {
                if (in_port->get_pin_id() == pin) {
                    in_port->callback(in_port, stat);
                    break;
                }
            }
        };

        void get_disk(gear_t *gr)
        {
            gear->read_gear(device_id,gr);
        
        }


        uint8_t get_status(void) { return (state==1)?0x04:0x00; }
        uint8_t get_level(void) { return (state==1)?0xFE:0x00; }

        void set_room(uint8_t rm) { 
            device_room=rm; 
        }

        void set_time(uint16_t tm) { 
            time=tm; 
        }

        virtual void time_update_event(uint8_t hour, uint8_t minute) {
            // Alt sınıflar bu metodu geçersiz kılabilir
        }
        virtual void sunset_event(uint8_t state, event_state_t* ev ) {
            // Alt sınıflar bu metodu geçersiz kılabilir
        }
        virtual void sunrise_event(uint8_t state, event_state_t* ev ) {
            // Alt sınıflar bu metodu geçersiz kılabilir
        }

        virtual void alarm_state_event(uint8_t state, uint8_t source, event_state_t* ev) {
            // Alt sınıflar bu metodu geçersiz kılabilir
        }

        virtual void movement_event(uint8_t state, uint8_t room, event_state_t* ev) {
            // Alt sınıflar bu metodu geçersiz kılabilir
        }

        void set_events(cJSON *pay) {
           uint8_t eid = 0; 
           if (pay==NULL) return; 
           JSON_getint(pay,"event_id", &eid);
           for (event_state_t* ev : events) {
                if (ev->event_id == eid) {
                   //Event bulundu, işleme al
                   char * event_type = (char *)calloc(1,32);
                   JSON_getstring(pay,"event", event_type,32);
                   //Time UPDATE EVENT
                   if (strcmp(event_type, "CLOCK_UPDATE_EVENT") == 0) {
                       uint8_t hour = 0, minute = 0;
                       JSON_getint(pay,"hour", &hour);
                       JSON_getint(pay,"minute", &minute);
                       time_update_event(hour, minute);
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
                   if (strcmp(event_type, "ALARM_STATE_EVENT") == 0) {
                       uint8_t state = 0, source = 0;
                       JSON_getint(pay,"state", &state);
                       JSON_getint(pay,"source", &source);
                       alarm_state_event(state, source, ev);
                   }

                   if (strcmp(event_type, "MOVEMENT_EVENT") == 0) {
                       uint8_t state = 0, room = 0;
                       JSON_getint(pay,"state", &state);
                       JSON_getint(pay,"room", &room);                       
                       movement_event(state, room, ev);
                   }


                   free(event_type);
                   break;
                
                }
            }
        }

        // Donanım kimlik ve tip parametreleri
        uint8_t device_type;
        uint8_t device_exttype;
        uint8_t device_id;
        uint8_t device_channel;

        uint8_t device_icon;
        uint8_t device_room;
        uint8_t device_anahtar;
        char *name;  
        uint8_t last_state;
        uint8_t state;
        uint16_t time;

    protected:
        Gear *gear;   
        // Cihazın kontrol ettiği fiziksel portların listesi
        std::vector<Base_Port*> ports;  
        std::vector<In_Base_Port*> in_ports;  
        std::vector<event_state_t*> events;  
        base_callback_t callback;
        base_say_callback_t saycallback;

        
    private:
        
           
               
};