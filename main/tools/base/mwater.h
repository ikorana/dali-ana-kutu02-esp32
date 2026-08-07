#pragma once

#include "base.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

const char *MWATERAG = "MWATER";

class MWater : public Base_Device {
   public:
      // 1. Base_Port artık bir işaretçi (pointer) olarak alınıyor
      MWater(uint8_t id, const char *nm, uint8_t chn) : Base_Device() {
        device_type = 0x07;
        device_id = id; 
        device_exttype = 0x7D;
        device_channel = chn;
        moving_direction = 0;
        last_state=0;
        state=0;
        // name için dinamik bellek açılıyor
        asprintf(&name, "%s", nm); 
      }

      // 2. ÇOK KRİTİK: Hafıza sızıntısını önlemek için yıkıcı (Destructor)
      virtual ~MWater() {
          if (name != nullptr) {
              free(name); // asprintf ile açılan belleği temizle
          }
      }

      static void tim_callback(void *arg) {
        MWater *self = (MWater *)arg;
        self->movement_complete();
      }

      void init(void) {
        gear_t gr={};
        gear->read_gear(device_id,&gr);
        state=gr.spec.status;    
        esp_timer_create_args_t tm_args = {};
        tm_args.callback = &tim_callback; 
        tm_args.arg = (void *)this;   
        tm_args.name = "ms_timer";
        ESP_ERROR_CHECK(esp_timer_create(&tm_args, &tim));
        if (time<1 || time>120) time=5;    
      }

      void up(void) {
        // Hareket halindeyken gelen herhangi bir hareket komutu perdeyi durdurur.
        if (moving_direction != 0) {
            printf("Motor durduruldu\n");
            if (callback) callback(this, device_id, "Motor Durduruldu.");
            stop();
            return;
        }

        // Perde zaten tam açıksa, komutu görmezden gel.
        if (state == 1) {
            printf("Su Zaten Açık durumda.\n");
            if (callback) callback(this, device_id, "Su Zaten Açık durumda");
            say(nullptr, "su açık durumda");
            return;
        }

        printf("Motor moving up.\n");
        if (callback) callback(this, device_id, "Motor Açılıyor");
        relays_off();
        moving_direction = 1;
        state = 2; // Hareket başladığı için "arada" kabul edilir
        bool found = false;
        for (Base_Port* port : ports) {
          if (port->get_desc() && strcmp(port->get_desc(), "up") == 0) {
              port->pin_on();
              found = true;
          }
        }
        if (!found) ESP_LOGE(MWATERAG,"up port not found");
        write_state(state);
        say("açılıyor", nullptr);
        esp_timer_start_once(tim, time*1000000);
      }

      void down(void) {
        // Hareket halindeyken gelen herhangi bir hareket komutu perdeyi durdurur.
        if (moving_direction != 0) {
            printf("Motor is moving, stopping now.\n");
            if (callback) callback(this, device_id, "Motor Durduruldu");
            stop();
            return;
        }

        // Perde zaten tam kapalıysa, komutu görmezden gel.
        if (state == 0) {
            printf("Water already fully down. No action needed.\n");
            if (callback) callback(this, device_id, "Su Zaten Kapalı Durumda");
            say(nullptr, "su kapalı durumda");
            return;
        }

        printf("Motor moving down.\n");
        if (callback) callback(this, device_id, "Motor Kapatılıyor");
        relays_off();
        moving_direction = 2;
        state = 2; // Hareket başladığı için "arada" kabul edilir
        bool found = false;
        for (Base_Port* port : ports) {
          if (port->get_desc() && strcmp(port->get_desc(), "down") == 0) {
              port->pin_on();
              found = true;
          }
        }
        if (!found) ESP_LOGE(MWATERAG,"down port not found");
        write_state(state);
        say("kapatılıyor", nullptr);
        esp_timer_start_once(tim, time*1000000);
      }

      void stop(void) {
        if (moving_direction == 0) return;
        printf("Motor manual stop\n");        
        relays_off();
        state = 2; // Elle durdurulduğu için arada kaldı
        write_state(state);
        if (callback) callback(this, device_id, "Motor Durduruldu");
        say(nullptr, "hareket durduruldu");
      }

      void set_power(uint8_t pwr, bool say=true) override {
        if (pwr < 100) down();
        else up();
      }

      void off(bool say=true) override {
        down();
      }

      void set_action(uint8_t cmd, bool say=true) override {
        switch (cmd)
        {
            case 0 : stop(); break; //Stop
            case 1 : break; //Son durum;
            case 2 : down(); break; //En Düşük
            case 3 : up(); break; //En Yüksek AÇ
            case 4 :  break; //on and up
            case 5 :  break; //down and off
            case 6 ... 21: {
              //  hexcom = cmd + 10; //Goto scene 1 (Hex 0x10=dec 16)
            } break;
            case 22 :  break; //up
            case 23 :  break; //down
            case 24 :  break; //step up
            case 25 :  break; //step down
            case 26 :  break; //enable dapc
            break;
        } 
      }

      void say(const char *txt = nullptr, const char *txt1 = nullptr) 
       {
           if (txt!=nullptr && txt1==nullptr) {
                if (saycallback) {  
                  gear_t gr={};
                  gear->read_gear(device_id,&gr);         
                  char *mm;
                  asprintf(&mm,"%s %s",gr.name,txt);
                  saycallback(this,device_id,mm);
                  free(mm);
                }
            }

            if (txt==nullptr && txt1!=nullptr) {
                  char *mm;
                  asprintf(&mm,"%s",txt1);
                  saycallback(this,device_id,mm);
                  free(mm);
            }
       }

    void in_callback(In_Base_Port* port, uint8_t stat) override {
        // Push button (BUTTON_SWITCH) mantığı: Sadece belirlenen aktif seviyede işlem yapılır
        if (port->button_type == BUTTON_SWITCH) {
            // Gelen sinyal portun aktif seviyesiyle (0 veya 1) eşleşiyor mu?
            if (stat == port->active_state) {
                char* description = port->get_desc();
                if (description != nullptr) {
                    if (strcmp(description, "up") == 0) {
                        up(); // up() metodu hareket halindeyken durdurma mantığını içerir
                    } else if (strcmp(description, "down") == 0) {
                        down(); // down() metodu hareket halindeyken durdurma mantığını içerir
                    }
                }
            }
        }
    }

    void alarm_state_event(uint8_t state, uint8_t source, event_state_t* ev) override {
        //source 1=yangın 2=su baskını 3=gaz 4=hırsızlık
        ESP_LOGI("SU","%s ALARM STATE EVENT %d %d",name,state,source);
        //Eger Su baskını varsa su kapatılır
        if (source==2) {
           if (state==1) { 
                  off();
            } 
        }       
    }  
    
    
    private:
       esp_timer_handle_t tim;
       uint8_t moving_direction; // 0: Sabit, 1: Yukarı, 2: Aşağı

       void relays_off(void) {
        if (tim != nullptr) {
            esp_timer_stop(tim);
        }
        for (Base_Port* port : ports) {
          if (port->get_desc() && strcmp(port->get_desc(), "up") == 0) {
              port->pin_off();
              vTaskDelay(100/portTICK_PERIOD_MS);
          }
          if (port->get_desc() && strcmp(port->get_desc(), "down") == 0) {
              port->pin_off();
              vTaskDelay(100/portTICK_PERIOD_MS);
          }
        }
        moving_direction = 0;
       }

       void movement_complete(void) {
         if (moving_direction == 1) state = 1; // Tam Açık
         else if (moving_direction == 2) state = 0; // Tam Kapalı
         
         relays_off();
         write_state(state);
         printf("Motor travel complete. Final State: %d\n", state);
         if (callback) callback(this, device_id, "Hareket Tamamlandı");
         say(nullptr,"hareket tamamlandı");
       
       }


       void write_state(uint8_t pwr) {
            gear_t gr={};
            gear->read_gear(device_id,&gr);
            gr.spec.status = pwr;
            gear->write_gear(device_id,&gr);
       }
             
};