#pragma once

#include "base.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <esp_timer.h>

class Elevator : public Base_Device {
   public:
      // 1. Base_Port artık bir işaretçi (pointer) olarak alınıyor
      Elevator(uint8_t id, const char *nm, uint8_t chn) : Base_Device() {
        device_type = 0x07;
        device_id = id; 
        device_exttype = 0x7B;
        device_channel = chn;
        
        // Güvenli: Dışarıda yaşayan gerçek port nesnesinin adresi atanıyor
 
        last_state=0;
        state=0;

        // name için dinamik bellek açılıyor
        asprintf(&name, "%s", nm); 
      }

      // 2. ÇOK KRİTİK: Hafıza sızıntısını önlemek için yıkıcı (Destructor)
      virtual ~Elevator() {
          if (name != nullptr) {
              free(name); // asprintf ile açılan belleği temizle
          }
      }

      void init(void) {
        esp_timer_create_args_t tm_args = {};
        tm_args.callback = &tim_callback; 
        tm_args.arg = (void *)this;   
        tm_args.name = "my_oneshot_timer";
        ESP_ERROR_CHECK(esp_timer_create(&tm_args, &tim));
        if (time<1 || time>60) time=3;
        off();
      }

      void set_power(uint8_t pwr, bool say=true) {
        last_state=state;
        state=1;
        for (Base_Port* port : ports) {
            port->pin_on();
        }
        write_state(state,"çağrıldı");
        esp_timer_start_once(tim, time*1000*1000);
      }

      void off(bool say=true) {
        last_state=state;
        state=0;
        for (Base_Port* port : ports) {
            port->pin_off();
        }
        write_state(state,nullptr);
      }

      static void tim_callback(void* arg) {
        Elevator *dev = (Elevator *)arg;
        dev->off();
      };

      void set_action(uint8_t cmd, bool say=true) {
        switch (cmd)
        {
            case 1 : {
              if (last_state==0) set_power(255);
                   else off();
            } break; //Son durum;
            case 2 : set_power(0xFE); break; //En Düşük
            case 3 : set_power(0xFE); break; //En Yüksek
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

    private:
        esp_timer_handle_t tim;
        
        void write_state(uint8_t pwr, const char *txt=nullptr) {
            if (txt!=nullptr) {
                if (saycallback) { 
                  gear_t gr={};
                  gear->read_gear(device_id,&gr);                 
                  char *mm;
                  asprintf(&mm,"%s %s",gr.name,txt);
                  saycallback(this,device_id,mm);
                  free(mm);
                }
            }
        }     
};