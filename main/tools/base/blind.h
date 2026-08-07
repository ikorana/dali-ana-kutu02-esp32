#pragma once

#include "base.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

const char *PERDETAG = "BLIND";


class Blind : public Base_Device {
   public:
      // 1. Base_Port artık bir işaretçi (pointer) olarak alınıyor
      Blind(uint8_t id, const char *nm, uint8_t chn) : Base_Device() {
        device_type = 0x07;
        device_id = id; 
        device_exttype = 0x77;
        device_channel = chn;
        moving_direction = 0;
        last_state=0;
        state=0;
        // name için dinamik bellek açılıyor
        asprintf(&name, "%s", nm); 
      }

      // 2. ÇOK KRİTİK: Hafıza sızıntısını önlemek için yıkıcı (Destructor)
      virtual ~Blind() {
          if (name != nullptr) {
              free(name); // asprintf ile açılan belleği temizle
          }
      }

      static void tim_callback(void *arg) {
        Blind *self = (Blind *)arg;
        self->movement_complete();
        self->temp_say = true;
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
        if (time<1 || time>10000) time=5;    
      }

      void up(void) {
        // Hareket halindeyken gelen herhangi bir hareket komutu perdeyi durdurur.
        if (moving_direction != 0) {
            printf("Perde durduruldu\n");
            if (callback) callback(this, device_id, "Perde Durduruldu.");
            stop();
            temp_say=true;
            return;
        }

        // Perde zaten tam açıksa, komutu görmezden gel.
        if (state == 1) {
            printf("Perde Zaten Açık durumda.\n");
            if (callback) callback(this, device_id, "Perde Zaten Açık durumda");
            say(nullptr,"perde açık durumda");
            temp_say=true;
            return;
        }

        printf("Blind moving up.\n");
        if (callback) callback(this, device_id, "Perde Açılıyor");
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
        if (!found) ESP_LOGE(PERDETAG,"up port not found");
        write_state(state);
        say(nullptr,"perde açılıyor");
        esp_timer_start_once(tim, time*1000000);
      }

      void down(void) {
        // Hareket halindeyken gelen herhangi bir hareket komutu perdeyi durdurur.
        if (moving_direction != 0) {
            printf("Blind is moving, stopping now.\n");
            if (callback) callback(this, device_id, "Perde Durduruldu");
            stop();
            temp_say=true;
            return;
        }

        // Perde zaten tam kapalıysa, komutu görmezden gel.
        if (state == 0) {
            printf("Blind already fully down. No action needed.\n");
            if (callback) callback(this, device_id, "Perde Zaten Kapalı Durumda");
            say(nullptr,"perde kapalı durumda");
            temp_say=true;
            return;
        }

        printf("Blind moving down.\n");
        if (callback) callback(this, device_id, "Perde Kapatılıyor");
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
        if (!found) ESP_LOGE(PERDETAG,"down port not found");
        write_state(state);
        say(nullptr,"perde kapatılıyor");
        esp_timer_start_once(tim, time*1000000);
      }

      void stop(void) {
        if (moving_direction == 0) return;
        printf("Blind manual stop\n");        
        relays_off();
        state = 2; // Elle durdurulduğu için arada kaldı
        write_state(state);
        if (callback) callback(this, device_id, "Perde Durduruldu");
        say(nullptr,"perde durduruldu");
      }

      void set_power(uint8_t pwr, bool say=true) override {
        temp_say=say;
        if (pwr < 100) down();
        else up();
      }

      void off(bool say=true) override {
        temp_say=say;
        down();
      }

      void set_action(uint8_t cmd, bool say=true) override {
        temp_say=say;
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

    void sunset_event(uint8_t state, event_state_t* ev) override {
          //Güneş batınca açılır veya kapanır
          gunes = 0;
          gear_t gr={};
          gear->read_gear(device_id,&gr);
          gr.spec.reserved[0] = gunes;
          gear->write_gear(device_id,&gr);
          if (ev->action_state==1) up();
          if (ev->action_state==0) down();
          
    }

    void sunrise_event(uint8_t state, event_state_t* ev) override {
      //Güneş çıkınca açılır veya kapanır
        gunes = 1;
        gear_t gr={};
        gear->read_gear(device_id,&gr);
        gr.spec.reserved[0] = gunes;
        gear->write_gear(device_id,&gr);
        if (ev->action_state==1) up();
        if (ev->action_state==0) down();
    }
    
    
    private:
       esp_timer_handle_t tim;
       uint8_t gunes =0;
       uint8_t moving_direction; // 0: Sabit, 1: Yukarı, 2: Aşağı
       bool temp_say = true;

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
         printf("Blind travel complete. Final State: %d\n", state);
         if (callback) callback(this, device_id, "Hareket Tamamlandı");
         say(nullptr,"perde hareketi tamamlandı");
       }

       void say(const char *txt = nullptr, const char *txt1 = nullptr) 
       {
           if (!temp_say) return;

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

       void write_state(uint8_t pwr) {
            gear_t gr={};
            gear->read_gear(device_id,&gr);
            gr.spec.status = pwr;
            gear->write_gear(device_id,&gr);           
       }
             
};