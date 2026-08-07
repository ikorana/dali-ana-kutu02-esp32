#pragma once

#include "base.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

class Movement : public Base_Device {
   public:
      // 1. Base_Port artık bir işaretçi (pointer) olarak alınıyor
      Movement(uint8_t id, const char *nm, uint8_t chn) : Base_Device() {
        device_type = 0x07;
        device_id = id; 
        device_exttype = 0x80;
        device_channel = chn;
        
        // Güvenli: Dışarıda yaşayan gerçek port nesnesinin adresi atanıyor       
        last_state=0;
        state=0;

        // name için dinamik bellek açılıyor
        asprintf(&name, "%s", nm); 
      }

      // 2. ÇOK KRİTİK: Hafıza sızıntısını önlemek için yıkıcı (Destructor)
      virtual ~Movement() {
          if (name != nullptr) {
              free(name); // asprintf ile açılan belleği temizle
          }
      }

      void init(void) {
        gear_t gr={};
        gear->read_gear(device_id,&gr);
        state=gr.spec.status;
        gunes = gr.spec.reserved[0];
      }

      void set_event_proc(send_events_t par) {
          send_event_json = par; // Fonksiyon göstericisini ata
      }

      void set_event_id(uint8_t id) {
          event_id = id;
      }

      void set_power(uint8_t pwr, bool say=true) {
      }

      void off(bool say=true) {
      }

      void send_event(uint8_t stat) 
      {
           if (send_event_json!=nullptr) {
                cJSON *root = cJSON_CreateObject();
                cJSON_AddStringToObject(root, "com","event");
                cJSON_AddStringToObject(root, "event","MOVEMENT_EVENT");
                cJSON_AddNumberToObject(root, "event_id",event_id);
                cJSON_AddNumberToObject(root, "room",device_room);
                cJSON_AddNumberToObject(root, "state",stat);
                send_event_json(root);
                cJSON_Delete(root);
           }
      }

      void set_action(uint8_t cmd, bool say=true) {
        switch (cmd)
        {
            case 1 : break; //Son durum;
            case 2 : break; //En Düşük
            case 3 : break; //En Yüksek
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
            send_event(2); //Toggle
        } 
        if (port->button_type==BUTTON_SWITCH) {
            if (port->active_state==1) {
              if (stat==1) {
                 send_event(1); //1 oldu
              }              
            } else {
              if (stat==0) {
                 send_event(0); //0 oldu
              }
            }
        }
        
        if (port->button_type==BUTTON_SENSOR) {
            if (port->active_state==1) {
                send_event(1); //1 oldu
              } else {
                send_event(0); //0 oldu
              }            
        } 
    } 

    void sunset_event(uint8_t state, event_state_t* ev) override {
          //Güneş batınca ne yapacagını buraya yaz
          gunes = 0;
          gear_t gr={};
          gear->read_gear(device_id,&gr);
          gr.spec.reserved[0] = gunes;
          gear->write_gear(device_id,&gr);
          
    }

    void sunrise_event(uint8_t state, event_state_t* ev) override {
        gunes = 1;
        gear_t gr={};
        gear->read_gear(device_id,&gr);
        gr.spec.reserved[0] = gunes;
        gear->write_gear(device_id,&gr);
          
    }

    void alarm_state_event(uint8_t state, uint8_t source, event_state_t* ev) override {
        //source 1=yangın 2=su baskını 3=gaz 4=hırsızlık
        ESP_LOGI("RLAMP","%s ALARM STATE EVENT %d %d",name,state,source);
        //Eger yangın varsa kapanacak  
        if (source==1) {
           off();
        }
        //Eger hırsızlık ise ve gece ise röle açılacak ve alarm kalktıgında otomatik açılmışsa röle kapanacak
        if (source==4) {
            if (state==1) {
                if (gunes==0) {
                  set_power(0xFE); 
                  auto_on=1;
                }
            } else {
                if (auto_on==1) {
                    off();
                    auto_on=0;
                }
            }
      }
          
    }

    
    private:
       uint8_t gunes=0, auto_on=0;
       send_events_t send_event_json = nullptr;
       uint8_t event_id=0x00;

       void write_state(uint8_t pwr, const char *txt=nullptr) {
            gear_t gr={};
            gear->read_gear(device_id,&gr);
            gr.spec.status = pwr;
            gear->write_gear(device_id,&gr);
            if (txt!=nullptr) {
                if (saycallback) {
                  char *mm;
                  asprintf(&mm,"%s %s",gr.name,txt);
                  saycallback(this,device_id,mm);
                  free(mm);
                }
            }
       }
            
};