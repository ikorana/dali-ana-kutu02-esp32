#pragma once

#include "base.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

class Energy : public Base_Device {
   public:
      // 1. Base_Port artık bir işaretçi (pointer) olarak alınıyor
      Energy(uint8_t id, const char *nm, uint8_t chn) : Base_Device() {
        device_type = 0x07;
        device_id = id; 
        device_exttype = 0x78;
        device_channel = chn;
        
        // Güvenli: Dışarıda yaşayan gerçek port nesnesinin adresi atanıyor  
        last_state=0;
        state=0;

        // name için dinamik bellek açılıyor
        asprintf(&name, "%s", nm); 
      }

      // 2. ÇOK KRİTİK: Hafıza sızıntısını önlemek için yıkıcı (Destructor)
      virtual ~Energy() {
          if (name != nullptr) {
              free(name); // asprintf ile açılan belleği temizle
          }
      }

      void init(void) override {
        gear_t gr={};
        gear->read_gear(device_id,&gr);
        state=gr.spec.status;
        init_state = 1;        
        if (state>0) set_power(0xFE); else off();
        init_state = 0;
      }

      void set_power(uint8_t pwr, bool say=true) override {
        last_state=state;
        
        for (Base_Port* port : ports) {
            port->pin_on();
        }
        state=1;
        write_state(state,"açıldı");
      }

      void off(bool say=true) override {        
        last_state=state;
        state=0;
        
        for (Base_Port* port : ports) {
            port->pin_off();
        }
        write_state(state,"kapatıldı");
      }

      void set_action(uint8_t cmd, bool say=true) override {
        switch (cmd)
        {
            case 1 :  break; //Son durum;
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

      void alarm_state_event(uint8_t state, uint8_t source, event_state_t* ev) override {
        //source 1=yangın 2=su baskını 3=gaz 4=hırsızlık
        ESP_LOGI("ENERGY","%s ALARM STATE EVENT %d %d",name,state,source);
        //Eger yangın varsa enerjiyi kes
        if (source==1) {
           if (state==1) { 
                  off();
            } 
        }       
    }  
      
    private:
       uint8_t init_state=0;

       void write_state(uint8_t pwr, const char *txt = nullptr) {
            gear_t gr={};
            gear->read_gear(device_id,&gr);
            gr.spec.status = pwr;
            gear->write_gear(device_id,&gr);
            if (txt!=nullptr) {
                if (saycallback && init_state==0) {
                  char *mm;
                  asprintf(&mm,"%s %s",gr.name,txt);
                  saycallback(this,device_id,mm);
                  free(mm);
                }
            }
       }       
};