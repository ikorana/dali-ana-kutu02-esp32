#pragma once

#include "base.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

class Socket : public Base_Device {
   public:
      // 1. Base_Port artık bir işaretçi (pointer) olarak alınıyor
      Socket(uint8_t id, const char *nm, uint8_t chn) : Base_Device() {
        device_type = 0x07;
        device_id = id; 
        device_exttype = 0x7c;
        device_channel = chn;
        
        // Güvenli: Dışarıda yaşayan gerçek port nesnesinin adresi atanıyor
        
        last_state=0;
        state=0;

        // name için dinamik bellek açılıyor
        asprintf(&name, "%s", nm); 
      }

      // 2. ÇOK KRİTİK: Hafıza sızıntısını önlemek için yıkıcı (Destructor)
      virtual ~Socket() {
          if (name != nullptr) {
              free(name); // asprintf ile açılan belleği temizle
          }
      }

      void init(void) {
        gear_t gr={};
        gear->read_gear(device_id,&gr);
        state=gr.spec.status;
        initstate = 1;       
        if (state>0) set_power(0xFE); else off();
        initstate = 0;
      }

      void set_power(uint8_t pwr, bool say=true) {
        last_state=state;
        for (Base_Port* port : ports) {
            port->pin_on();
        }
        state=1;
        temp_say = say;
        write_state(state,"açıldı");
      }


      void off(bool say=true) {
        last_state=state;
        for (Base_Port* port : ports) {
            port->pin_off();
        }
        state=0;
        temp_say = say;
        write_state(state,"kapatıldı");
        //printf("off\n");

      }

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

    void in_callback(In_Base_Port* port, uint8_t stat) override {
        // Giriş durum değişikliğinde yapılacak işlemler
        // Örneğin, bir sensör tetiklenirse bu fonksiyon çağrılabilir
        if (port->button_type==BUTTON_ANAHTAR) {
            //Anahtarda kenara bakmaz state durumuna bakar
            //stat her degiştiginde durum degişir
            if (state==1) off(); else set_power(0xFE);
        } 
        if (port->button_type==BUTTON_SWITCH) {
            if (port->active_state==1) {
              if (stat==1) {
                 if (state==1) off(); else set_power(0xFE);
              }              
            } else {
              if (stat==0) {
                 if (state==1) off(); else set_power(0xFE);
              }
            }
        }
        
        if (port->button_type==BUTTON_SENSOR) {
            if (port->active_state==1) {
                if (stat==1) set_power(0xFE);
                if (stat==0) off();
              } else {
                if (stat==0) set_power(0xFE);
                if (stat==1) off();
              }
            
        } 
    } 

    void alarm_state_event(uint8_t state, uint8_t source, event_state_t* ev) override {
        //source 1=yangın 2=su baskını 3=gaz 4=hırsızlık
        ESP_LOGI("PRIZ","%s ALARM STATE EVENT %d %d",name,state,source);
        //Eger yangın varsa kapanacak yangın kalktıgında otomatik kapanmışsa açılacak 
        if (source==1) {
           if (state==1) { 
                  off(); 
                  auto_on=1;
            } else {
                if (auto_on==1) {
                    set_power(0xFE);
                    auto_on=0;
                }
            }
        }       
    }
    
    
    private:
       uint8_t auto_on=0;
       uint8_t initstate=0;
       bool temp_say = true;



       void write_state(uint8_t pwr, const char *txt=nullptr) {
            gear_t gr={};
            gear->read_gear(device_id,&gr);
            gr.spec.status = pwr;
            gear->write_gear(device_id,&gr);
            if (txt!=nullptr) {
                if (saycallback && initstate==0 && temp_say) {
                  char *mm;
                  asprintf(&mm,"%s %s",gr.name,txt);
                  saycallback(this,device_id,mm);
                  free(mm);                  
                }
            }
            temp_say = true;
       }
      

       
};