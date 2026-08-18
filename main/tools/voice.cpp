
#include "ArduinoJson.h"
#include "gear/gear.h"
#include <ctype.h>





//void system_commans(uint8_t cmm, uint8_t yr, asio_param_t *sparam);
char *sonek(uint8_t kod, const char *on0, const char* on1, char *tmp);


char *birlestir(const char*txt, const char*txt1, char*ret)
{
    sprintf(ret,"%s %s",txt,txt1);
    return ret;
}
uint8_t adres_bul(char *txt, const char*aranan)
{
    // Rakamla yazılmış sayılar için kısayol (örn. "%80" -> replacestr sonrası "yuzde 80"):
    // "aranan" kelimesinin hemen ardından gelen 1-3 haneli rakamı doğrudan sayıya çevir.
    // Aşağıdaki yazıyla-sayı eşleştirmesi (seksen/yetmis vb.) bunu bulamıyordu.
    char *bul = strstr(txt, aranan);
    if (bul != NULL) {
        char *p2 = bul + strlen(aranan);
        while (*p2 == ' ') p2++;
        if (isdigit((unsigned char)*p2)) {
            int deger = atoi(p2);
            if (deger >= 0 && deger <= 254) return (uint8_t)deger;
        }
    }

    uint8_t ret = 254;
    char *p = (char*)calloc(1,30);
    if (strstr(txt,birlestir(aranan,"sifir",p))!=NULL) ret=0;
    if (strstr(txt,birlestir(aranan,"bir",p))!=NULL) ret=1;
    if (strstr(txt,birlestir(aranan,"iki",p))!=NULL) ret=2;
    if (strstr(txt,birlestir(aranan,"uc",p))!=NULL) ret=3;
    if (strstr(txt,birlestir(aranan,"dort",p))!=NULL) ret=4;
    if (strstr(txt,birlestir(aranan,"bes",p))!=NULL) ret=5;
    if (strstr(txt,birlestir(aranan,"alti",p))!=NULL) ret=6;
    if (strstr(txt,birlestir(aranan,"yedi",p))!=NULL) ret=7;
    if (strstr(txt,birlestir(aranan,"sekiz",p))!=NULL) ret=8;
    if (strstr(txt,birlestir(aranan,"dokuz",p))!=NULL) ret=9;
    if (strstr(txt,birlestir(aranan,"on",p))!=NULL) ret=10;
    if (strstr(txt,birlestir(aranan,"onbir",p))!=NULL) ret=11;
    if (strstr(txt,birlestir(aranan,"oniki",p))!=NULL) ret=12;
    if (strstr(txt,birlestir(aranan,"onuc",p))!=NULL) ret=13;
    if (strstr(txt,birlestir(aranan,"ondort",p))!=NULL) ret=14;
    if (strstr(txt,birlestir(aranan,"onbes",p))!=NULL) ret=15;
    if (strstr(txt,birlestir(aranan,"onalti",p))!=NULL) ret=16;
    if (strstr(txt,birlestir(aranan,"onyedi",p))!=NULL) ret=17;
    if (strstr(txt,birlestir(aranan,"onsekiz",p))!=NULL) ret=18;
    if (strstr(txt,birlestir(aranan,"ondokuz",p))!=NULL) ret=19;
    if (strstr(txt,birlestir(aranan,"yirmi",p))!=NULL) ret=20;
    if (strstr(txt,birlestir(aranan,"yirmibir",p))!=NULL) ret=21;
    if (strstr(txt,birlestir(aranan,"yirmiiki",p))!=NULL) ret=22;
    if (strstr(txt,birlestir(aranan,"yirmiuc",p))!=NULL) ret=23;
    if (strstr(txt,birlestir(aranan,"yirmidort",p))!=NULL) ret=24;
    if (strstr(txt,birlestir(aranan,"yirmibes",p))!=NULL) ret=25;
    if (strstr(txt,birlestir(aranan,"yirmialti",p))!=NULL) ret=26;
    if (strstr(txt,birlestir(aranan,"yirmiyedi",p))!=NULL) ret=27;
    if (strstr(txt,birlestir(aranan,"yirmisekiz",p))!=NULL) ret=28;
    if (strstr(txt,birlestir(aranan,"yirmidokuz",p))!=NULL) ret=29;
    if (strstr(txt,birlestir(aranan,"otuz",p))!=NULL) ret=30;
    if (strstr(txt,birlestir(aranan,"otuzbir",p))!=NULL) ret=31;
    if (strstr(txt,birlestir(aranan,"otuziki",p))!=NULL) ret=32;
    if (strstr(txt,birlestir(aranan,"otuzuc",p))!=NULL) ret=33;
    if (strstr(txt,birlestir(aranan,"otuzdort",p))!=NULL) ret=34;
    if (strstr(txt,birlestir(aranan,"otuzbes",p))!=NULL) ret=35;
    if (strstr(txt,birlestir(aranan,"otuzalti",p))!=NULL) ret=36;
    if (strstr(txt,birlestir(aranan,"otuzyedi",p))!=NULL) ret=37;
    if (strstr(txt,birlestir(aranan,"otuzsekiz",p))!=NULL) ret=38;
    if (strstr(txt,birlestir(aranan,"otuzdokuz",p))!=NULL) ret=39;
    if (strstr(txt,birlestir(aranan,"kirk",p))!=NULL) ret=40;
    if (strstr(txt,birlestir(aranan,"kirkbir",p))!=NULL) ret=41;
    if (strstr(txt,birlestir(aranan,"kirkiki",p))!=NULL) ret=42;
    if (strstr(txt,birlestir(aranan,"kirkuc",p))!=NULL) ret=43;
    if (strstr(txt,birlestir(aranan,"kirkdort",p))!=NULL) ret=44;
    if (strstr(txt,birlestir(aranan,"kirkbes",p))!=NULL) ret=45;
    if (strstr(txt,birlestir(aranan,"kirkalti",p))!=NULL) ret=46;
    if (strstr(txt,birlestir(aranan,"kirkyedi",p))!=NULL) ret=47;
    if (strstr(txt,birlestir(aranan,"kirksekiz",p))!=NULL) ret=48;
    if (strstr(txt,birlestir(aranan,"kirkdokuz",p))!=NULL) ret=49; 
    if (strstr(txt,birlestir(aranan,"elli",p))!=NULL) ret=50;
    if (strstr(txt,birlestir(aranan,"ellibir",p))!=NULL) ret=51;
    if (strstr(txt,birlestir(aranan,"elliiki",p))!=NULL) ret=52;
    if (strstr(txt,birlestir(aranan,"elliuc",p))!=NULL) ret=53;
    if (strstr(txt,birlestir(aranan,"ellidort",p))!=NULL) ret=54;  
    if (strstr(txt,birlestir(aranan,"ellibes",p))!=NULL) ret=55;
    if (strstr(txt,birlestir(aranan,"ellialti",p))!=NULL) ret=56;
    if (strstr(txt,birlestir(aranan,"elliyedi",p))!=NULL) ret=57;
    if (strstr(txt,birlestir(aranan,"ellisekiz",p))!=NULL) ret=58;
    if (strstr(txt,birlestir(aranan,"ellidokuz",p))!=NULL) ret=59;
    if (strstr(txt,birlestir(aranan,"altmis",p))!=NULL) ret=60;
    if (strstr(txt,birlestir(aranan,"altmisbir",p))!=NULL) ret=61;
    if (strstr(txt,birlestir(aranan,"altmisiki",p))!=NULL) ret=62;
    if (strstr(txt,birlestir(aranan,"altmisuc",p))!=NULL) ret=63;
    if (strstr(txt,birlestir(aranan,"altmisdort",p))!=NULL) ret=64;
    if (strstr(txt,birlestir(aranan,"altmisbes",p))!=NULL) ret=65;
    if (strstr(txt,birlestir(aranan,"altmisalti",p))!=NULL) ret=66;
    if (strstr(txt,birlestir(aranan,"altmisyedi",p))!=NULL) ret=67;
    if (strstr(txt,birlestir(aranan,"altmissekiz",p))!=NULL) ret=68;
    if (strstr(txt,birlestir(aranan,"altmisdokuz",p))!=NULL) ret=69;
    if (strstr(txt,birlestir(aranan,"yetmis",p))!=NULL) ret=70;
    if (strstr(txt,birlestir(aranan,"yetmisbir",p))!=NULL) ret=71;
    if (strstr(txt,birlestir(aranan,"yetmisiki",p))!=NULL) ret=72;
    if (strstr(txt,birlestir(aranan,"yetmisuc",p))!=NULL) ret=73;
    if (strstr(txt,birlestir(aranan,"yetmisdort",p))!=NULL) ret=74;
    if (strstr(txt,birlestir(aranan,"yetmisbes",p))!=NULL) ret=75;
    if (strstr(txt,birlestir(aranan,"yetmisalti",p))!=NULL) ret=76;
    if (strstr(txt,birlestir(aranan,"yetmisyedi",p))!=NULL) ret=77;
    if (strstr(txt,birlestir(aranan,"yetmissekiz",p))!=NULL) ret=78;
    if (strstr(txt,birlestir(aranan,"yetmisdokuz",p))!=NULL) ret=79;

    if (strstr(txt,birlestir(aranan,"seksen",p))!=NULL) ret=80;
    if (strstr(txt,birlestir(aranan,"seksenbir",p))!=NULL) ret=81;
    if (strstr(txt,birlestir(aranan,"sekseniki",p))!=NULL) ret=82;
    if (strstr(txt,birlestir(aranan,"seksenuc",p))!=NULL) ret=83;
    if (strstr(txt,birlestir(aranan,"seksendort",p))!=NULL) ret=84;
    if (strstr(txt,birlestir(aranan,"seksenbes",p))!=NULL) ret=85;
    if (strstr(txt,birlestir(aranan,"seksenalti",p))!=NULL) ret=86;
    if (strstr(txt,birlestir(aranan,"seksenyedi",p))!=NULL) ret=87;
    if (strstr(txt,birlestir(aranan,"seksensekiz",p))!=NULL) ret=88;
    if (strstr(txt,birlestir(aranan,"seksendokuz",p))!=NULL) ret=89;

    if (strstr(txt,birlestir(aranan,"doksan",p))!=NULL) ret=90;
    if (strstr(txt,birlestir(aranan,"doksanbir",p))!=NULL) ret=91;
    if (strstr(txt,birlestir(aranan,"doksaniki",p))!=NULL) ret=92;
    if (strstr(txt,birlestir(aranan,"doksanuc",p))!=NULL) ret=93;
    if (strstr(txt,birlestir(aranan,"doksandort",p))!=NULL) ret=94;
    if (strstr(txt,birlestir(aranan,"doksanbes",p))!=NULL) ret=95;
    if (strstr(txt,birlestir(aranan,"doksanalti",p))!=NULL) ret=96;
    if (strstr(txt,birlestir(aranan,"doksanyedi",p))!=NULL) ret=97;
    if (strstr(txt,birlestir(aranan,"doksansekiz",p))!=NULL) ret=98;
    if (strstr(txt,birlestir(aranan,"doksandokuz",p))!=NULL) ret=99;

    if (strstr(txt,birlestir(aranan,"yuz",p))!=NULL) ret=100;

    free(p);
    return ret;
}




int replacestr(char *line, const char *search, const char *replace)
{
   int count;
   char *sp; // start of pattern

   //printf("replacestr(%s, %s, %s)\n", line, search, replace);
   if ((sp = strstr(line, search)) == NULL) {
      return(0);
   }
   count = 1;
   int sLen = strlen(search);
   int rLen = strlen(replace);
   if (sLen > rLen) {
      // move from right to left
      char *src = sp + sLen;
      char *dst = sp + rLen;
      while((*dst = *src) != '\0') { dst++; src++; }
   } else if (sLen < rLen) {
      // move from left to right
      int tLen = strlen(sp) - sLen;
      char *stop = sp + rLen;
      char *src = sp + sLen + tLen;
      char *dst = sp + rLen + tLen;
      while(dst >= stop) { *dst = *src; dst--; src--; }
   }
   memcpy(sp, replace, rLen);

   count += replacestr(sp + rLen, search, replace);

   return(count);
}

    uint8_t guc_seviyesi = 254;
    uint8_t guc_yuzde = 254;

void room_action(uint8_t addr, uint8_t comm,uint8_t power)
{
/*     for (int i=0;i<64;i++)
    {
        gear_t kk = {};
        disk.read_file(GEAR_DALI1_FILE,&kk,sizeof(gear_t),i); 
        //printf("ADDR %02X %02X Vroom:%d Addr:%d\n",kk.short_addr,i,kk.vroom,addr);
        if (kk.vroom==addr) {
            //printf("%d Aktif\n",kk.short_addr);
            if (comm==1) arc_power(kk.short_addr,0,power,DALI_KANAL1);
            if (comm==2) off(kk.short_addr,0,DALI_KANAL1);
            if (comm==3) command_action(kk.short_addr,0,0x03,DALI_KANAL1);
            if (comm==4) command_action(kk.short_addr,0,0x02,DALI_KANAL1);
            if (comm==5) arc_power(kk.short_addr,0,power,DALI_KANAL1);
            vTaskDelay(100/portTICK_PERIOD_MS);
        }
    }
    for (int i=0;i<64;i++)
    {
        gear_t kk = {};
        disk.read_file(GEAR_DALI2_FILE,&kk,sizeof(gear_t),i); 
        if (kk.vroom==addr) {
            if (comm==1) arc_power(kk.short_addr,0,power,DALI_KANAL2);
            if (comm==2) off(kk.short_addr,0,DALI_KANAL2);
            if (comm==3) command_action(kk.short_addr,0,0x03,DALI_KANAL2);
            if (comm==4) command_action(kk.short_addr,0,0x02,DALI_KANAL2);
            vTaskDelay(100/portTICK_PERIOD_MS);
        }
    }
    for (int i=0;i<64;i++)
    {
        gear_t kk = {};
        disk.read_file(GEAR_WIFI_FILE,&kk,sizeof(gear_t),i); 
        if (kk.vroom==addr) {
            if (comm==1) arc_power(kk.short_addr,0,power,DALI_WIRELESS);
            if (comm==2) off(kk.short_addr,0,DALI_WIRELESS);
            if (comm==3) command_action(kk.short_addr,0,0x03,DALI_WIRELESS);
            if (comm==4) command_action(kk.short_addr,0,0x02,DALI_WIRELESS);
            
            vTaskDelay(100/portTICK_PERIOD_MS);
        }
    } */
}

uint8_t find_devicename(std::vector<find_param_t> lst, char *txt) {
   for (find_param_t par : lst) {
            char *en0 = (char *) calloc(1,50);
            ing_yap(par.name,en0);
            to_lowercase(en0);
            printf("%s - %s - %s\n",txt, par.name, en0);

            // Boş isimli (henüz adlandırılmamış) kayıtları hiç eşleştirmeye
            // çalışma: strnstr boş bir alt-diziyi HER metnin başında "bulur",
            // bu da isimsiz her grup/senaryo/cihaz kaydının rastgele her sesli
            // komutla "eşleşmesine" yol açıyordu.
            if (en0[0] == '\0') { free(en0); continue; }

            char *eslesme = strnstr(txt,en0,100);
            if (eslesme != NULL) {
                // Eşleşme bir kelime ORTASINDAN başlamamalı (örn. "tum lambalari"
                // içinde "m lamba" adlı bir cihazın yanlışlıkla eşleşmesi — "tuM" +
                // "LAMBAlari" sınırını aşarak). Sadece BAŞLANGIÇ sınırını kontrol
                // ediyoruz; SONA dokunmuyoruz çünkü Türkçe ekler ("lambayı",
                // "lambasını") isimden hemen sonra, boşluksuz gelir — bu hâlihazırda
                // çalışan ve istenen bir davranış.
                bool bas_sinir = (eslesme == txt) || !isalnum((unsigned char)*(eslesme - 1));
                if (bas_sinir) {
                    printf("bulundu INDEX:%d %s\n",par.index,par.name);
                    free(en0);
                    return par.index;
                }
            }
            free(en0);
        }
    return 255;
}


/*
    Kategori (ext_type) bazlı fan-out: yerel cihazlar (kanal 10) + DALI 1-3.
    oda_index==255 ise oda filtresi YOK (3c / AI "isAll" senaryosu — TÜM cihazlar).
    oda_index<255 ise sadece o odadaki cihazlara uygulanır (3b senaryosu).
    Hem yerel parser (new_voice_parse_task) hem AI-cevap işleyicisi (firebase_tool.cpp)
    bunu kullanır — aynı fan-out mantığı iki yerde ayrı ayrı yazılmasın diye.
    Dönüş değeri: gerçekten komut gönderilen cihaz sayısı — çağıran taraf 0 ise
    ("eşleşen cihaz yok") kullanıcıya yanlış bir "yapıyorum" onayı vermemeli.
*/
static int apply_category_fanout(uint8_t ext_type_beklenen, uint8_t oda_index,
                                  altkomuttipi_t altkomut, uint8_t guc_yuzde, uint8_t guc_seviyesi,
                                  pck_t *pck, bool is_mqtt)
{
    int gonderilen = 0;
    for (Base_Device* cihaz : cihaz_listesi) {
        if ((oda_index==255 || cihaz->device_room==oda_index) && cihaz->device_exttype==ext_type_beklenen) {
            cJSON *pl = cJSON_CreateObject();
            cJSON_AddNumberToObject(pl, "adres", cihaz->device_id);
            cJSON_AddNumberToObject(pl, "gurup", 0);
            cJSON_AddNumberToObject(pl, "kanal", 10);

            if (altkomut==KAPAT) {
                command_off(pl, pck, is_mqtt, false);
                vTaskDelay(500/portTICK_PERIOD_MS);
            } else {
                uint8_t power = 127;
                if (altkomut==GUC && guc_yuzde!=254) power = guc_seviyesi;
                if (altkomut==ENYUKSEK) power = 254;
                if (altkomut==ENDUSUK) power = 1;
                if (altkomut==ORTALA) power = 127;
                cJSON_AddNumberToObject(pl, "power", power);
                command_arc(pl, pck, is_mqtt, false);
                vTaskDelay(500/portTICK_PERIOD_MS);
            }
            cJSON_Delete(pl);
            gonderilen++;
        }
    }

    // LAMBA için yerel (cihaz_listesi) ve DALI tarafı farklı ext_type konvansiyonu kullanıyor:
    // yerel röle-lamba cihazları 0x76 (projenin kendi kodu), gerçek DALI balastları ise
    // DALI bus üzerinden kendi tipini 0x06 olarak raporluyor (gerçek cihazda doğrulandı).
    uint8_t ext_type_dali_beklenen = (ext_type_beklenen==0x76) ? 0x06 : ext_type_beklenen;

    // gear04 kanal numarası 0'dır (bkz. command_save_dev/wifi_search_device) ve WiFi
    // üzerinden giden ayrı bir DALI hattıdır — oda+kategori fan-out'unda (grup/senaryo
    // broadcast'inden farklı olarak) hariç tutmak için bir sebep yok, o kanalda da
    // gerçek lambalar olabilir.
    Gear *dali_kanallari[4] = { &gear01, &gear02, &gear03, &gear04 };
    uint8_t dali_kanal_no[4] = { 1, 2, 3, 0 };
    for (uint8_t k=0; k<4; k++) {
        std::vector<gear_room_entry_t> kayitlar = dali_kanallari[k]->get_room_entries();
        for (gear_room_entry_t &re : kayitlar) {
            // Role tipi (0x07) bir cihaz, kullanıcı ikonunu lamba yaptıysa (lamp_override)
            // LAMBA kategori fan-out'una da dahil olsun — bkz. gear.h dev_spec_t.reserved[0].
            bool ext_eslesiyor = (re.ext_type==ext_type_dali_beklenen) ||
                (ext_type_dali_beklenen==0x06 && re.ext_type==0x07 && re.lamp_override==1);
            if ((oda_index==255 || re.room==oda_index) && ext_eslesiyor) {
                cJSON *pl = cJSON_CreateObject();
                cJSON_AddNumberToObject(pl, "adres", re.short_addr);
                cJSON_AddNumberToObject(pl, "gurup", 0);
                cJSON_AddNumberToObject(pl, "kanal", dali_kanal_no[k]);

                if (altkomut==KAPAT) {
                    command_off(pl, pck, is_mqtt, false);
                    vTaskDelay(500/portTICK_PERIOD_MS);
                } else {
                    uint8_t power = 127;
                    if (altkomut==GUC && guc_yuzde!=254) power = guc_seviyesi;
                    if (altkomut==ENYUKSEK) power = 254;
                    if (altkomut==ENDUSUK) power = 1;
                    if (altkomut==ORTALA) power = 127;
                    cJSON_AddNumberToObject(pl, "power", power);
                    command_arc(pl, pck, is_mqtt, false);
                    vTaskDelay(500/portTICK_PERIOD_MS);
                }
                cJSON_Delete(pl);
                gonderilen++;
            }
        }
    }

    return gonderilen;
}

/*
    Tek bir cihazı isimle (gear10 -> gear01..04, ilk eşleşmede dur) bulup komut
    gönderir. search_text bir cümle (3a'daki gibi tüm sözcük öbeği) olabileceği
    gibi AI'nin çıkardığı tek bir cihaz adı da olabilir — find_devicename() zaten
    substring arıyor, ikisi için de çalışır. Bulunamazsa false döner.
*/
static bool apply_named_device_command(const char *search_text, altkomuttipi_t altkomut,
                                        uint8_t guc_yuzde, uint8_t guc_seviyesi,
                                        pck_t *pck, bool is_mqtt)
{
    // find_devicename() cihaz isimlerini ing_yap+to_lowercase ile normalize
    // edip karşılaştırıyor; AI'den gelen search_text (örn. "kırmızı lamba")
    // normalize edilmeden gönderilirse Türkçe karakterli isimler hiç eşleşmiyordu.
    char *norm = (char*)calloc(1, strlen(search_text) + 32);
    ing_yap(search_text, norm);
    to_lowercase(norm);

    uint8_t index = 255, location = 0xFF;
    Gear *matched_gear = nullptr;

    if (index==255) {
        std::vector<find_param_t> liste = gear10.get_name();
        if (liste.size()>0) index = find_devicename(liste, norm);
        if (index<255) { location = 10; matched_gear = &gear10; }
    }
    if (index==255) {
        std::vector<find_param_t> liste = gear01.get_name();
        if (liste.size()>0) index = find_devicename(liste, norm);
        if (index<255) { location = 1; matched_gear = &gear01; }
    }
    if (index==255) {
        std::vector<find_param_t> liste = gear02.get_name();
        if (liste.size()>0) index = find_devicename(liste, norm);
        if (index<255) { location = 2; matched_gear = &gear02; }
    }
    if (index==255) {
        std::vector<find_param_t> liste = gear03.get_name();
        if (liste.size()>0) index = find_devicename(liste, norm);
        if (index<255) { location = 3; matched_gear = &gear03; }
    }
    if (index==255) {
        std::vector<find_param_t> liste = gear04.get_name();
        if (liste.size()>0) index = find_devicename(liste, norm);
        if (index<255) { location = 4; matched_gear = &gear04; }
    }

    free(norm);

    if (index==255 || matched_gear==nullptr) return false;

    gear_t kayit = {};
    matched_gear->read_gear(index, &kayit);

    cJSON *pl = cJSON_CreateObject();
    cJSON_AddNumberToObject(pl, "adres", kayit.short_addr);
    cJSON_AddNumberToObject(pl, "gurup", 0);
    cJSON_AddNumberToObject(pl, "kanal", location);

    if (altkomut==KAPAT) {
        command_off(pl, pck, is_mqtt);
    } else {
        uint8_t power = 127;
        if (altkomut==GUC && guc_yuzde!=254) power = guc_seviyesi;
        if (altkomut==ENYUKSEK) power = 254;
        if (altkomut==ENDUSUK) power = 1;
        if (altkomut==ORTALA) power = 127;
        cJSON_AddNumberToObject(pl, "power", power);
        command_arc(pl, pck, is_mqtt);
    }
    cJSON_Delete(pl);
    return true;
}

/*
    YENİ NESİL sesli komut ayrıştırıcı — 3a/3b/grup/senaryo adım(lar)ı test aşamasında,
    3d (kategori+numara son çare) henüz yazılmadı.
    Strateji (kullanıcıyla üzerinde anlaşıldı):
      1) AKSİYON (ucuz: aç/kapat/güç vb.) — bulunamazsa hedef aramaya HİÇ girmeden
         AI katmanına düşülür (pahalı disk/isim aramasını boşa harcamamak için).
      2) KAPSAM (ucuz: tüm/sistem).
      3) HEDEF (pahalı, öncelik sırasıyla): tam cihaz adı > oda adı + kategori
         kelimesi > grup/senaryo adı veya numarası > kategori + numara (son çare).
         Not: eski voice_parse_task'taki bug burada yok — kategori kelimesi artık
         isim aramasını bloke etmeyecek (adım 3 kendi içinde önceliklendirilecek).
*/
static void new_voice_parse_task(void *args)
{
    voice_task_param_t *tt = (voice_task_param_t *)args;
    // replacestr() metni yerinde büyütebiliyor (" 0 "->" sifir ", "%"->" yuzde ");
    // asprintf tam payload uzunluğu kadar ayırdığından bu büyüme heap taşmasına
    // yol açıyordu (TLSF metadata bozulması, çok sonra ilgisiz bir malloc'ta patlıyordu).
    // Fazladan pay bırakarak büyümeye yer açıyoruz.
    char *txt = (char*)calloc(1, strlen(tt->payload) + 64);
    strcpy(txt, tt->payload);

    // Cihaz/oda/grup isimleri ing_yap+to_lowercase ile normalize edilip
    // karşılaştırılıyor (örn. "Kırmızı" -> "kirmizi"); gelen metin de aynı
    // normalizasyondan geçmezse Türkçe karakterli kelimeler hiç eşleşmiyordu
    // ("kırmızı"/"yeşil" başarısız, özel karakter içermeyen "mavi" tesadüfen
    // çalışıyordu). ing_yap çıktısı girişten uzun olamayacağından taşma yok.
    char *norm = (char*)calloc(1, strlen(tt->payload) + 64);
    ing_yap(txt, norm);
    to_lowercase(norm);
    free(txt);
    txt = norm;
    ESP_LOGI("VOICE", "Ham:'%s' -> Normalize:'%s'", tt->payload, txt);

    replacestr(txt," 0 "," sifir ");
    replacestr(txt,"%"," yuzde ");

    voice_sonuc_t sonuc = {};
    sonuc.komut = BOS;
    sonuc.altkomut = ABOS;
    sonuc.index = 255;
    sonuc.location = 0xFF;
    sonuc.room = 255;

    // "refresh" / "durumu yenile" / "ev hakkında bilgi ver": mevcut
    // refresh_device task'ını (UDP'deki "refresh" komutuyla aynı) tetikler —
    // cihaz durumlarını tazeleyip lamba/perde/priz açık-kapalı sayısını "say"
    // ile bildirir. tt->pck.rem/pck DERİN KOPYALANIR (udp_events.cpp'deki
    // "refresh" tetikleyicisiyle aynı desen) — aksi hâlde refresh_device
    // sonunda yaptığı free/cJSON_Delete, bu task'ın paylaştığı orijinal
    // pck/rem'i erken serbest bırakıp firebase_voice_task'ın kendi
    // temizliğinde double-free'ye yol açardı.
    if (strstr(txt,"refresh")!=NULL ||
        (strstr(txt,"durum")!=NULL && strstr(txt,"yenile")!=NULL) ||
        (strstr(txt,"ev")!=NULL && strstr(txt,"bilgi")!=NULL)) {
        search_task_param_t *rparam = (search_task_param_t *)malloc(sizeof(search_task_param_t));
        rparam->pck.rem = (remote_t *)malloc(sizeof(remote_t));
        memcpy(rparam->pck.rem, tt->pck.rem, sizeof(remote_t));
        rparam->pck.pck = cJSON_Duplicate(tt->pck.pck, true);
        rparam->payload = cJSON_CreateObject();
        rparam->is_mqtt = tt->is_mqtt;

        xTaskCreatePinnedToCore(
            refresh_device,
            "rdev_voice",
            4096,
            rparam,
            1,
            NULL,
            1
        );

        // sonuc.komut BOS (0) kalırsa firebase_voice_task "lokal katman bir
        // şey yapmadı" sanıp komutu AYRICA AI'ye de gönderiyordu — TAMAM
        // (zaten var olan "işlem yapıldı" değeri) ile bunu engelliyoruz.
        // devam_ediyor=true: tarama arka planda sürüyor, "Komutunuz uygulandı"
        // yerine kısa bir bekleme mesajı söylenecek (asıl özet ayrıca gelecek).
        sonuc.komut = TAMAM;
        sonuc.devam_ediyor = true;
        free(txt);
        xQueueSend(xvoiceQueue, &sonuc, portMAX_DELAY);
        vTaskDelete(NULL);
        return;
    }

    // ================================================================
    // 1. ADIM: AKSİYON (ucuz) — bulunamazsa hedef aramaya hiç girmeden çık.
    // ================================================================
    altkomuttipi_t altkomut = ABOS;
    uint8_t guc_yuzde = 254, guc_seviyesi = 254; // local: eski koddaki stale-state bug'ı burada yok

    if (strstr(txt,"ac")!=NULL) altkomut=AC;
    if (strstr(txt,"yak")!=NULL) altkomut=AC;
    if (strstr(txt,"kapat")!=NULL) altkomut=KAPAT;
    if (strstr(txt,"sondur")!=NULL) altkomut=KAPAT;
    if (strstr(txt,"uygula")!=NULL) altkomut=UYGULA;
    if (strstr(txt,"en dusuk")!=NULL) altkomut=ENDUSUK;
    if (strstr(txt,"en yuksek")!=NULL) altkomut=ENYUKSEK;
    if (strstr(txt,"orta")!=NULL) altkomut=ORTALA;
    if (strstr(txt,"guc")!=NULL) altkomut=GUC;
    if (strstr(txt,"yuzde")!=NULL) altkomut=GUC;
    // Senaryo dalı (SENARYO) altkomut'a bakmadan her zaman "goto scene" gönderiyor;
    // "gece modu" gibi aksiyon fiili içermeyen senaryo isimlerinin de 1. adım
    // gate'ini geçebilmesi için "modu"/"moduna" kelimesini örtük aksiyon sayıyoruz.
    if (altkomut==ABOS && strstr(txt,"modu")!=NULL) altkomut=UYGULA;

    if (altkomut==GUC) {
        uint8_t guc_degeri = adres_bul(txt,"guc");
        if (guc_degeri==254) guc_degeri = adres_bul(txt,"yuzde");
        if (guc_degeri!=254) {
            guc_yuzde = guc_degeri;
            guc_seviyesi = (uint8_t)((254.0/100.0) * guc_degeri);
        }
    }

    if (altkomut==ABOS) {
        // Aksiyon bulunamadı: pahalı hedef aramaya hiç girmeden AI katmanına düşülecek.
        free(txt);
        xQueueSend(xvoiceQueue, &sonuc, portMAX_DELAY);
        vTaskDelete(NULL);
        return;
    }

    sonuc.altkomut = altkomut;

    // ================================================================
    // 2. ADIM: KAPSAM (ucuz) — tüm / sistem
    // ================================================================
    bool broadcast = false, sistem = false;
    if (strstr(txt,"tum")!=NULL) broadcast=true;
    if (strstr(txt,"sistem")!=NULL) sistem=true;

    // ================================================================
    // 3. ADIM: HEDEF (pahalı)
    //   3a) Tam cihaz adı (gear10 -> gear01..04), ilk eşleşmede dur.
    //       (Eski koddaki bug burada YOK: orijinali "index<0xFF" ile bulunca bile
    //        aramaya devam edip üzerine yazıyordu; doğrusu "index==255" — henüz
    //        bulunmadıysa devam.)
    //   (3a'dan hemen sonra) Grup/senaryo adı — DALI protokol seviyesi, üyelik modeli gerekmiyor.
    //   3b) Oda adı + kategori kelimesi (fan-out: cihaz_listesi + gear01-03, device_room eşleşenler)
    //   3c) TÜM + kategori kelimesi (oda filtresi yok, kategoriye uyan HER cihaza fan-out)
    //   3d) Kategori + numara (son çare, örn. "3. grubu kapat") — TODO, henüz yazılmadı
    // ================================================================
    komuttipi_t komut = BOS;
    uint8_t index = 255, location = 0xFF;
    Gear *matched_gear = nullptr;

    if (index==255) {
        std::vector<find_param_t> liste = gear10.get_name();
        if (liste.size()>0) index = find_devicename(liste,txt);
        if (index<255) { location = 10; matched_gear = &gear10; }
    }
    if (index==255) {
        std::vector<find_param_t> liste = gear01.get_name();
        if (liste.size()>0) index = find_devicename(liste,txt);
        if (index<255) { location = 1; matched_gear = &gear01; }
    }
    if (index==255) {
        std::vector<find_param_t> liste = gear02.get_name();
        if (liste.size()>0) index = find_devicename(liste,txt);
        if (index<255) { location = 2; matched_gear = &gear02; }
    }
    if (index==255) {
        std::vector<find_param_t> liste = gear03.get_name();
        if (liste.size()>0) index = find_devicename(liste,txt);
        if (index<255) { location = 3; matched_gear = &gear03; }
    }
    if (index==255) {
        std::vector<find_param_t> liste = gear04.get_name();
        if (liste.size()>0) index = find_devicename(liste,txt);
        if (index<255) { location = 4; matched_gear = &gear04; }
    }

    // Cihaz adında bulunamadıysa grup/senaryo adında ara (numara değil, isim — örn.
    // "sinema senaryosunu aç"). find_devicename() burada da aynen kullanılabiliyor
    // çünkü zaten generic bir std::vector<find_param_t> arayıcısı.
    uint8_t gurup_senaryo_index = 255;
    komuttipi_t gurup_senaryo_komut = BOS;
    if (index==255) {
        std::vector<find_param_t> liste = gurup.get_name();
        if (liste.size()>0) gurup_senaryo_index = find_devicename(liste,txt);
        if (gurup_senaryo_index<255) gurup_senaryo_komut = GRUP;
    }
    if (index==255 && gurup_senaryo_index==255) {
        std::vector<find_param_t> liste = scene.get_name();
        if (liste.size()>0) gurup_senaryo_index = find_devicename(liste,txt);
        if (gurup_senaryo_index<255) gurup_senaryo_komut = SENARYO;
    }

    if (gurup_senaryo_index<255) {
        komut = gurup_senaryo_komut;
        index = gurup_senaryo_index;

        ESP_LOGI("VOICE", "Lokal sesli komut %s hedefliyor -> NO:%d",
                 (gurup_senaryo_komut==GRUP) ? "GRUP" : "SENARYO", gurup_senaryo_index);

        // Grup/senaryo numarası kanaldan bağımsızdır: 1, 2, 3 DALI kanallarının hepsine
        // gönderiyoruz, 4. kanala gitmiyoruz (kullanıcı onayı).
        for (uint8_t kanal=1; kanal<=3; kanal++) {
            cJSON *pl = cJSON_CreateObject();

            if (gurup_senaryo_komut==GRUP) {
                cJSON_AddNumberToObject(pl, "adres", gurup_senaryo_index);
                cJSON_AddNumberToObject(pl, "gurup", 1);
                cJSON_AddNumberToObject(pl, "kanal", kanal);
                if (altkomut==KAPAT) {
                    command_off(pl, &tt->pck, tt->is_mqtt);
                } else {
                    uint8_t power = 127;
                    if (altkomut==GUC && guc_yuzde!=254) power = guc_seviyesi;
                    if (altkomut==ENYUKSEK) power = 254;
                    if (altkomut==ENDUSUK) power = 1;
                    if (altkomut==ORTALA) power = 127;
                    cJSON_AddNumberToObject(pl, "power", power);
                    command_arc(pl, &tt->pck, tt->is_mqtt);
                }
            } else {
                // SENARYO: "goto scene" broadcast — sahnesi tanımlı her cihaz kendi
                // grubu/adresinden bağımsız tepki verir, bu yüzden adres=0xFF (broadcast).
                cJSON_AddNumberToObject(pl, "adres", 0xFF);
                cJSON_AddNumberToObject(pl, "gurup", 0);
                cJSON_AddNumberToObject(pl, "kanal", kanal);
                cJSON_AddNumberToObject(pl, "komut", gurup_senaryo_index + 6);
                command_action(pl, &tt->pck, tt->is_mqtt);
            }

            cJSON_Delete(pl);
        }
    }

    // ================================================================
    //   3b) Oda adı + kategori kelimesi (fan-out): "mutfak lambalarını aç" vb.
    //       Sadece 3a (cihaz adı) ve grup/senaryo adı hiçbir şey bulamadıysa denenir.
    //       Şimdilik SADECE yerel cihazlar (cihaz_listesi) — DALI kanallarındaki
    //       (gear01-04) oda bazlı cihazlar ayrı bir adım (disk maliyeti daha yüksek).
    //       Kategori olarak sadece LAMBA/PERDE/KAPI destekleniyor (komuttipi_t'nin
    //       şu an karşılığı olan tipler; diğer ext_type'lar enum genişleyince eklenecek).
    // ================================================================
    if (index==255 && gurup_senaryo_index==255) {
        uint8_t oda_index = 255;
        std::vector<find_param_t> oda_liste = room.get_name();
        if (oda_liste.size()>0) oda_index = find_devicename(oda_liste,txt);

        if (oda_index<255) {
            komuttipi_t kategori = BOS;
            uint8_t ext_type_beklenen = 0;
            if (strstr(txt,"lamba")!=NULL) { kategori=LAMBA; ext_type_beklenen=0x76; }
            if (strstr(txt,"perde")!=NULL) { kategori=PERDE; ext_type_beklenen=0x77; }
            if (strstr(txt,"priz")!=NULL)  { kategori=PRIZ;  ext_type_beklenen=0x7C; }

            if (kategori!=BOS) {
                komut = kategori;
                sonuc.room = oda_index;
                apply_category_fanout(ext_type_beklenen, oda_index, altkomut, guc_yuzde, guc_seviyesi, &tt->pck, tt->is_mqtt);
            }
        }
    }

    // ================================================================
    //   3c) TÜM + kategori kelimesi (özel/genel broadcast komutları):
    //       "tüm lambaları aç", "tüm perdeleri kapat", "tüm prizleri aç" vb.
    //       3b ile aynı mantık, tek fark: oda filtresi YOK — kategoriye uyan
    //       HER cihaza (yerel + DALI 1-3) uygulanır. Sadece 3a/grup-senaryo/3b
    //       hiçbir şey bulamadıysa VE "tüm" kapsam kelimesi varsa denenir.
    // ================================================================
    if (index==255 && gurup_senaryo_index==255 && komut==BOS && broadcast) {
        komuttipi_t kategori = BOS;
        uint8_t ext_type_beklenen = 0;
        if (strstr(txt,"lamba")!=NULL) { kategori=LAMBA; ext_type_beklenen=0x76; }
        if (strstr(txt,"perde")!=NULL) { kategori=PERDE; ext_type_beklenen=0x77; }
        if (strstr(txt,"priz")!=NULL)  { kategori=PRIZ;  ext_type_beklenen=0x7C; }

        if (kategori!=BOS) {
            komut = kategori;
            apply_category_fanout(ext_type_beklenen, 255, altkomut, guc_yuzde, guc_seviyesi, &tt->pck, tt->is_mqtt);
        }
    }

    if (index<255 && matched_gear!=nullptr) {
        // TODO: gear01-04 için doğru varsayım (DALI = her zaman lamba). gear10 (location==10)
        // için ise ext_type okuyup gerçek kategoriyi (blind/door/vb.) belirlemek gerekecek —
        // bu ayrım henüz onaylanmadı, komut=LAMBA sabit kalıyor.
        komut = LAMBA;

        // location==10 (yerel) ve location==1-4 (DALI) için TEK bir yol: command_arc/command_off
        // zaten kanal'a göre doğru tarafa (cihaz_listesi ya da gerçek DALI frame) yönlendiriyor.
        gear_t kayit = {};
        matched_gear->read_gear(index, &kayit);

        cJSON *arc_payload = cJSON_CreateObject();
        cJSON_AddNumberToObject(arc_payload, "adres", kayit.short_addr);
        cJSON_AddNumberToObject(arc_payload, "gurup", 0);
        cJSON_AddNumberToObject(arc_payload, "kanal", location);

        if (altkomut==KAPAT) {
            command_off(arc_payload, &tt->pck, tt->is_mqtt);
        } else {
            uint8_t power = 127; // AC / UYGULA / varsayılan seviye
            if (altkomut==GUC && guc_yuzde!=254) power = guc_seviyesi;
            if (altkomut==ENYUKSEK) power = 254;
            if (altkomut==ENDUSUK) power = 1;
            if (altkomut==ORTALA) power = 127;
            cJSON_AddNumberToObject(arc_payload, "power", power);
            command_arc(arc_payload, &tt->pck, tt->is_mqtt);
        }

        cJSON_Delete(arc_payload);
    }

    sonuc.komut = komut;
    sonuc.index = index;
    sonuc.location = location;

    (void)sistem;

    free(txt);
    xQueueSend(xvoiceQueue, &sonuc, portMAX_DELAY);
    vTaskDelete(NULL);
}

static void voice_parse_task(void *args)
{
    voice_task_param_t *tt = (voice_task_param_t *)args;
    // bkz. new_voice_parse_task'taki aynı fix: replacestr() büyüyen değişimlerde
    // (" 0 "->" sifir ", "%"->" yuzde ") yerinde taşma yapabiliyor, fazladan pay lazım.
    char *txt = (char*)calloc(1, strlen(tt->payload) + 64);
    strcpy(txt, tt->payload);
    //uint8_t yer = tt->yer;

   // uint8_t tmp_grp_adr = 255;
   // uint8_t tmp_sen_adr = 255;


    replacestr(txt," 0 "," sifir ");
    replacestr(txt,"%"," yuzde ");
    printf("First Voice  %s\n", txt);   
   // command_parse(txt);

    komuttipi_t komut = BOS;
    altkomuttipi_t altkomut = ABOS;
    
    bool broadcast = false;
    bool sistem = false;
    uint8_t index=255, location = 0xFF;


    /*  =======================================
        ONCELİKLE AkSIYON KELIMELERINI ARAYALIM 
        =======================================
    */
    if (strstr(txt,"ac")!=NULL) altkomut=AC;
    if (strstr(txt,"yak")!=NULL) altkomut=AC;
    if (strstr(txt,"kapat")!=NULL) altkomut=KAPAT;
    if (strstr(txt,"sondur")!=NULL) altkomut=KAPAT;
    if (strstr(txt,"uygula")!=NULL) altkomut=UYGULA;
    if (strstr(txt,"en dusuk")!=NULL) altkomut=ENDUSUK;
    if (strstr(txt,"en yuksek")!=NULL) altkomut=ENYUKSEK;
    if (strstr(txt,"orta")!=NULL) altkomut=ORTALA;
    if (strstr(txt,"guc")!=NULL) altkomut=GUC;
    if (strstr(txt,"yuzde")!=NULL) altkomut=GUC;
    if (altkomut==GUC) {
        uint8_t guc_degeri = 254 ;
        guc_degeri = adres_bul(txt,"guc");
        printf("GUC DEGERI GUC %s %d\n",txt, guc_degeri);
        if (guc_degeri==254) {
            guc_degeri= adres_bul(txt,"yuzde");
            printf("GUC DEGERI YUZDE %s %d\n",txt,guc_degeri);
        }
        if (guc_degeri!=254) {
            guc_yuzde = guc_degeri;
            guc_seviyesi = (254.0/100.0) * guc_degeri;
        } 
    }
    
   
    if (strstr(txt,"tum")!=NULL) broadcast=true;
    if (strstr(txt,"sistem")!=NULL) sistem=true;

    //ilk önce string içinde odaları arayalım
    //--------------------------------------------------
    

    //ilk önce string içinde anahtar kelimeleri arayalım
    //--------------------------------------------------
    if (strnstr(txt,"grup",9)!=NULL) komut=GRUP;
    if (strnstr(txt,"bolum",9)!=NULL) komut=GRUP;
    if (strnstr(txt,"senaryo",9)!=NULL && komut==BOS) komut=SENARYO;
    if (strnstr(txt,"lamba",9)!=NULL && komut==BOS) komut=LAMBA;
    if (strnstr(txt,"lambalari",9)!=NULL && komut==BOS) komut=LAMBA;
    if (strnstr(txt,"cihaz",9)!=NULL && komut==BOS) komut=LAMBA;
    if (strnstr(txt,"cihazlari",9)!=NULL && komut==BOS) komut=LAMBA;
    if (strnstr(txt,"radyo",9)!=NULL && komut==BOS) komut=RADYO;
    if (strnstr(txt,"perde",9)!=NULL && komut==BOS) komut=PERDE;
    if (strnstr(txt,"kapi",9)!=NULL && komut==BOS) komut=KAPI;

    if (komut==BOS) {

        std::vector<find_param_t> liste;
        liste = gear10.get_name();
        if (liste.size()>0) {
            index = find_devicename(liste,txt);
            if (index<255) {
                  komut=LAMBA;
                  location=10;
          }
        }

        if (index<0xFF)
        {
            liste = gear01.get_name();
            if (liste.size()>0) {
                index = find_devicename(liste,txt);
                if (index<255) {
                    komut=LAMBA;
                    location=1;
            }
            }
        }

        if (index<0xFF)
        {
            liste = gear02.get_name();
            if (liste.size()>0) {
                index = find_devicename(liste,txt);
                if (index<255) {
                    komut=LAMBA;
                    location=2;
            }
            }
        }

        if (index<0xFF)
        {
            liste = gear03.get_name();
            if (liste.size()>0) {
                index = find_devicename(liste,txt);
                if (index<255) {
                    komut=LAMBA;
                    location=3;
            }
            }
        }

        if (index<0xFF)
        {
            liste = gear04.get_name();
            if (liste.size()>0) {
                index = find_devicename(liste,txt);
                if (index<255) {
                    komut=LAMBA;
                    location=4;
            }
            }
        }
              
        
      } 

      printf("Komut %d (Aksiyon)Alt Komut %d Index:%d Broadcast:%d\n",komut,altkomut,index,broadcast);


      voice_sonuc_t sonuc = {};

      if (komut!=BOS && altkomut!=ABOS) 
       {    
          sonuc.komut = komut;
          sonuc.altkomut = altkomut;
          sonuc.index = index;
          sonuc.location = location;
          printf("UYGULANABILIR KOMUT\n");
          
       } else sonuc.komut=BOS;


       xQueueSend(xvoiceQueue, &sonuc, portMAX_DELAY); 

/*         if (gear01.find_name(txt)) komut=LAMBA;
        if (gear02.find_name(txt)) komut=LAMBA;
        if (gear03.find_name(txt)) komut=LAMBA;
        if (gear10.find_name(txt)) komut=LAMBA; */
    

    printf("Anahtar1 Kelime %d\n", komut);

/* 
    if (komut==BOS) {
        //Ana komut bulunamadı. Guruplar içinde ara
        for (int i=0;i<MAX_CRON;i++)
          {
            gurup_t ff={};
            disk.read_file(GURUP_FILE,&ff,sizeof(gurup_t),i); 
            if (strstr(txt,(char*)ff.name)!=NULL)
              {
                komut = GRUP;
                tmp_grp_adr = i;
                break;
              }
          }
        if (komut==BOS) 
          {
            for (int i=0;i<MAX_CRON;i++)
            {
                gurup_t ff={};
                disk.read_file(SCENE_FILE,&ff,sizeof(gurup_t),i); 
                if (strstr(txt,(char*)ff.name)!=NULL)
                {
                    komut = SENARYO;
                    tmp_sen_adr = i;
                    break;
                }
            }
          }   
        if (komut==BOS) 
          {
            for (int i=0;i<MAX_ROOM;i++)
            {
                room_t ff={};
                disk.read_file(ROOM_FILE,&ff,sizeof(room_t),i); 
                //printf("ARA %s %s %i\n",txt, ff.name,i);
                if (strstr(txt,(char*)ff.name)!=NULL)
                {
                    komut = ROOM;
                    tmp_sen_adr = i;
                    //printf("BUL %s %i %d\n",ff.name,i,tmp_sen_adr);
                    break;
                }
            }
          }         
    }

    if (strstr(txt,"ac")!=NULL) altkomut=AC;
    if (strstr(txt,"yak")!=NULL) altkomut=AC;
    if (strstr(txt,"kapat")!=NULL) altkomut=KAPAT;
    if (strstr(txt,"sondur")!=NULL) altkomut=KAPAT;
    if (strstr(txt,"uygula")!=NULL) altkomut=UYGULA;
    if (strstr(txt,"en dusuk")!=NULL) altkomut=ENDUSUK;
    if (strstr(txt,"en yuksek")!=NULL) altkomut=ENYUKSEK;
    if (strstr(txt,"orta")!=NULL) altkomut=ORTA;
    if (strstr(txt,"guc")!=NULL) altkomut=GUC;
    if (strstr(txt,"yuzde")!=NULL) altkomut=GUC;
    if (altkomut==GUC) {
        uint8_t guc_degeri = 254 ;
        guc_degeri = adres_bul(txt,"guc");
        printf("GUC DEGERI GUC %s %d\n",txt, guc_degeri);
        if (guc_degeri==254) {
            guc_degeri= adres_bul(txt,"yuzde");
            printf("GUC DEGERI YUZDE %s %d\n",txt,guc_degeri);
        }
        if (guc_degeri!=254) {
            guc_yuzde = guc_degeri;
            guc_seviyesi = (254.0/100.0) * guc_degeri;
        } 
    }
    if (strstr(txt,"sonra")!=NULL) altkomut=SONRA;
    if (strstr(txt,"once")!=NULL) altkomut=ONCE;
    if (strstr(txt,"sus")!=NULL) altkomut=SESSUS;
    if (strstr(txt,"dev")!=NULL) altkomut=SESDEVAM;
    if (strstr(txt,"ses")!=NULL) {
        if (strstr(txt,"ar")!=NULL) altkomut=SESARTI;
        if (strstr(txt,"ac")!=NULL) altkomut=SESARTI;
        if (strstr(txt,"ek")!=NULL) altkomut=SESEKSI;
        if (strstr(txt,"kap")!=NULL) altkomut=SESEKSI;
    }
   
    if (strstr(txt,"tum")!=NULL) broadcast=true;
    if (strstr(txt,"sistem")!=NULL) sistem=true;

    if (sistem) {
        //Sistem komutları
        bool tara = false, unknown = true;
        uint8_t kablo = 0;
        if (strstr(txt,"tara")!=NULL) tara=true;
        if (tara) {
            if (strstr(txt,"kablolu")!=NULL) kablo=1;
            if (strstr(txt,"kablo")!=NULL) kablo=1;
            if (strstr(txt,"kablosuz")!=NULL) kablo=2;
            if (kablo>0) {system_commans(kablo, yer, tt->tcp_param);unknown=false;} 
          }
        if (strstr(txt,"test")!=NULL) {
            system_commans(3, yer, tt->tcp_param);
            unknown=false;
        }  
        if (strstr(txt,"intro")!=NULL) {
            system_commans(4, yer, tt->tcp_param);
            unknown=false;
        }
        if (strstr(txt,"yenile")!=NULL) {
            system_commans(5, yer, tt->tcp_param);
            unknown=false;
        }
        if (strstr(txt,"yardim")!=NULL) {
            system_commans(6, yer, tt->tcp_param);
            unknown=false;
        }
        if (strstr(txt,"numara sil")!=NULL) {
            system_commans(7, yer, tt->tcp_param);
            unknown=false;
        }
        if (unknown) uygulandi("Sistem Komutunu anlamad+im. tekrarlay+in+iz",par,yer);  
    }

    if (komut==TAMAM) {
        if (strlen(last_command)<1) uygulandi("Uygulayabilecegim komut bulunmuyor.",par,yer); else 
        {
            uygulandi("Komutu uyguluyorum.",par,yer);
            strcpy(last_command,"");
        }
    }

    if (komut==VAZGEC) {
        strcpy(last_command,"");
        uygulandi("Komutu iptal ettim.",par,yer); 
    }

    
    if (komut==SENARYO)
    {
        uint8_t senadr = adres_bul(txt,"senaryo");
        uint8_t grpadr = adres_bul(txt,"grup");
        if (tmp_sen_adr!=255) {senadr = tmp_sen_adr;broadcast=true;}

        printf("%d %d\n",senadr,grpadr);

        bool tamam = false;
        if (grpadr==254 && broadcast && (senadr<=16))
          {
             //senaryo tüm lambalara uygulanacak
             command_action(255,0,senadr+6);
             char *pp = (char*)calloc(1,255);
             char *on = (char*)calloc(1,255);
             asprintf(&on,"%d nolu senaryo t+um cihazlara",senadr);
             strcpy(pp,sonek(4,on,NULL,pp));
             uygulandi(pp,par,yer);
             vTaskDelay(100/portTICK_PERIOD_MS);
             free(on);
             free(pp);
             tamam = true;
          } else {
            if ((senadr<17) && (grpadr<17)) {
                //senaryoyu guruba uygula
                command_action(grpadr,1,senadr+6);
             char *pp = (char*)calloc(1,255);
             char *on = (char*)calloc(1,255);
             asprintf(&on,"%d nolu senaryo %d nolu gruba",senadr,grpadr);
             strcpy(pp,sonek(4,on,NULL,pp));
             uygulandi(pp,par,yer);
             vTaskDelay(100/portTICK_PERIOD_MS);
             free(on);
             free(pp);
             tamam=true;
            }
          }
        if (!tamam)
        {
            bool aaa = true;
            if (senadr==254) {uygulandi("Senaryo numaras+in+i anlamad+im",par,yer);aaa=false;}
            if (senadr!=254 && !broadcast) {uygulandi("grup numaras+in+i anlamad+im",par,yer);aaa=false;}
            if (aaa) uygulandi("Komutu anlamad&im. L+utfen tekrarlay+in+iz",par,yer);
        }  
    }

    if (komut==LAMBA)
    {
        uint8_t adres = 254;
        bool pro = false; 
        if (broadcast) adres = 255;
        if (!broadcast) {
                adres = adres_bul(txt,"lamba");  
                if (adres==254) adres = adres_bul(txt,"cihaz"); 
        }       
        if ((adres!=254 && adres<64) || adres==255)
          {  
            char *pp = (char*)calloc(1,255);
            char *on = (char*)calloc(1,255);
             if (adres==255) strcpy(on,"t+um cihazlar "); else sprintf(on,"%d nolu cihaz ",adres);
             if (altkomut==AC) {strcpy(pp,sonek(1,on,NULL,pp));pro=true;arc_power(adres,0,127,DALI_ALL);}
             if (altkomut==KAPAT) {strcpy(pp,sonek(2,on,NULL,pp));pro=true;off(adres,0,DALI_ALL);}
             if (altkomut==ENYUKSEK) {strcpy(pp,sonek(3,on,"en y+uksek seviyeye",pp));pro=true;command_action(adres,0,0x03);}
             if (altkomut==ENDUSUK) {strcpy(pp,sonek(3,on,"en d+u+s+uk seviyeye",pp));pro=true;command_action(adres,0,0x02);}
             if (altkomut==ORTA) {strcpy(pp,sonek(3,on,"orta seviyeye",pp));pro=true;arc_power(adres,0,127,DALI_ALL);}
             if (altkomut==GUC) {
                if (guc_yuzde!=254)
                {
                    arc_power(adres,0,guc_seviyesi,DALI_ALL);
                    char *rr;
                    asprintf(&rr,"%% %d seviyesine",guc_yuzde);
                    strcpy(pp,sonek(3,on,rr,pp));
                    free(rr);
                    pro=true; 
                } else {
                    uygulandi("seviye 1 ile 100 aras+inda olmal+id+ir.",par,yer);
                    pro = true;
                }

                
             }
             if (pro)
             {
                if (sesli_komut_onay) strcat(pp,". Onayl+iyormusunuz?");
                if (!sesli_komut_onay) strcat(pp,".");
                uygulandi(pp,par,yer);
                vTaskDelay(100/portTICK_PERIOD_MS);
                
             }  
             free(pp);
             free(on);
          }
        //if (!pro) uygulandi("Komutu anlamad+im. L+utfen tekrarlay+in+iz",yer); 
    }

    if (komut==GRUP)
    {
        uint8_t adres = 254;
        bool pro = false; 
        adres = adres_bul(txt,"grup");  
        if (tmp_grp_adr!=255) {adres = tmp_grp_adr;}    
        printf("Gurup Adres : %d\n",adres);  
        if (adres<=16)
          {  
            char *pp = (char*)calloc(1,255);
            char *on = (char*)calloc(1,255);
             sprintf(on,"%d nolu gurup ",adres);
             if (altkomut==AC) {strcpy(pp,sonek(1,on,NULL,pp));pro=true;arc_power(adres,1,127,DALI_ALL);}
             if (altkomut==KAPAT) {strcpy(pp,sonek(2,on,NULL,pp));pro=true;off(adres,1,DALI_ALL);}
             if (altkomut==ENYUKSEK) {strcpy(pp,sonek(3,on,"en y+uksek seviyeye",pp));pro=true;command_action(adres,1,0x03);}
             if (altkomut==ENDUSUK) {strcpy(pp,sonek(3,on,"en d+u+s+uk seviyeye",pp));pro=true;command_action(adres,1,0x02);}
             if (altkomut==ORTA) {strcpy(pp,sonek(3,on,"orta seviyeye",pp));pro=true;arc_power(adres,1,127,DALI_ALL);}

             if (altkomut==GUC) {
                if (guc_yuzde!=254)
                {
                    arc_power(adres,1,guc_seviyesi,DALI_ALL);
                    char *rr;
                    asprintf(&rr,"%% %d seviyesine",guc_yuzde);
                    strcpy(pp,sonek(3,on,rr,pp));
                    free(rr);
                    pro=true; 
                } else {
                    uygulandi("seviye 1 ile 100 aras+inda olmal+id+ir.",par,yer);
                    pro = false;
                }
            }
            
             free(pp);
             free(on);  
          }
        if (pro) uygulandi("komut uyguland+i.",par,yer);  
        if (!pro) uygulandi("Gurup Komutunu anlamad+im. L+utfen tekrarlay+in+iz",par,yer);  
    }

    if (komut==ROOM)
    {
        if (altkomut==AC || altkomut==KAPAT || altkomut==GUC || altkomut==ENYUKSEK || altkomut==ENDUSUK)
        {
            if (altkomut==AC) 
            {
               printf("Room ON %d\n",tmp_sen_adr);
               room_action(tmp_sen_adr,1,200);
               uygulandi("Lambalar a+c+ild+i.",par,yer);  
            }
            if (altkomut==KAPAT) 
            {
               printf("Room OFF %d\n",tmp_sen_adr);
               room_action(tmp_sen_adr,2,0);
               uygulandi("Lambalar kapat+ild+i.",par,yer);
            }
            if (altkomut==ENYUKSEK) 
            {
               printf("Room ENYUKSEK %d\n",tmp_sen_adr);
               room_action(tmp_sen_adr,3,0);
               uygulandi("Lambalar en y+uksek seviyede.",par,yer);
            }
            if (altkomut==ENDUSUK) 
            {
               printf("Room ENDUSUK %d\n",tmp_sen_adr);
               room_action(tmp_sen_adr,4,0);
               uygulandi("Lambalar en d+u+s+uk seviyede.",par,yer);
            }
            if (altkomut==GUC) 
            {
               printf("Room GUC %d %d %d\n",tmp_sen_adr,guc_yuzde,guc_seviyesi);
               room_action(tmp_sen_adr,1,guc_seviyesi);
               uygulandi("Lambalar ayarland+i.",par,yer);
            }

        }
    }

    if (komut==RADYO)
    {
        if (altkomut==AC || altkomut==KAPAT || altkomut==ONCE || altkomut==SONRA || altkomut==SESARTI 
             || altkomut==SESEKSI || altkomut==SESSUS || altkomut==SESDEVAM) {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "com", "radio");
            if (altkomut==AC) cJSON_AddNumberToObject(root,"alt",1);
            if (altkomut==KAPAT) cJSON_AddNumberToObject(root,"alt",2);
            if (altkomut==ONCE) cJSON_AddNumberToObject(root,"alt",3);
            if (altkomut==SONRA) cJSON_AddNumberToObject(root,"alt",4);
            if (altkomut==SESARTI) cJSON_AddNumberToObject(root,"alt",5);
            if (altkomut==SESEKSI) cJSON_AddNumberToObject(root,"alt",6);
            if (altkomut==SESSUS) cJSON_AddNumberToObject(root,"alt",7);
            if (altkomut==SESDEVAM) cJSON_AddNumberToObject(root,"alt",8);
            char *dat = cJSON_PrintUnformatted(root);
            TCPServer_Send(dat,par);
            ESP_LOGI(TAG,"Giden %s",dat);
            cJSON_free(dat);
            cJSON_Delete(root);
            uygulandi("Komut ilgili cihazlara iletildi.",par,yer);
        }
    } */
 

    free(txt);
    vTaskDelete(NULL);
}



/* char *sonek(uint8_t kod, const char *on0, const char* on1, char *tmp)
{
    sprintf(tmp,"%s %s ",(on0!=NULL)?on0:"", (on1!=NULL)?on1:"");
    switch(kod)
    {
        case 1:{
            if (sesli_komut_onay) strcat(tmp,"a+c+ilacak"); else strcat(tmp,"a+c+ild+i");
        } break;
        case 2:{
            if (sesli_komut_onay) strcat(tmp,"kapat+ilacak"); else strcat(tmp,"kapat+ild+i");
        } break;
        case 3:{
            if (sesli_komut_onay) strcat(tmp,"al+inacacak"); else strcat(tmp,"al+ind+i");
        } break;
        case 4:{
            if (sesli_komut_onay) strcat(tmp,"uygulanacak"); else strcat(tmp,"uyguland+i");
        } break;
    }
    return tmp;
} */

/*
void system_commans(uint8_t cmm, uint8_t yr, asio_param_t *sparam)
{
    switch(cmm)
    {
        case 1: {
                uygulandi("dali kablosu taramas+i ba+slat+ild+i",sparam,yr);
                uint8_t kk = Device_Control_cable(1,NULL);
                char *pp;
                asprintf(&pp,"kablo taramas+i tamamland+i. %d adet cihaz bulundu.",kk);
                uygulandi(pp,sparam,yr);
                vTaskDelay(500/portTICK_PERIOD_MS);
                free(pp);
                }
                break;
        case 2: {
                uygulandi("Kablosuz network taramas+i ba+slat+il+iyor. L+utfen bekleyiniz.",sparam,yr);
                uint8_t kk = Device_Control_Wireless(1,NULL);
                char *pp;
                asprintf(&pp,"Kablosuz network taramas+i tamamland+i. %d adet cihaz bulundu.",kk);
                uygulandi(pp,sparam,yr);
                vTaskDelay(500/portTICK_PERIOD_MS);
                free(pp);
                }
                break; 
        case 3: {
                uygulandi("Sistem sesli komut almak i+cin haz+ir.",sparam,yr);  
                }
                break;
        case 4: {
                    intro_gonder(0,sparam);
                    uygulandi("intro haz+irland+i ve ekranlara g+onderildi.",sparam,yr);
                }   
                break;
        case 5: {
                    intro_gonder(5,sparam);
                    uygulandi("soft intro haz+irland+i ve ekranlara g+onderildi.",sparam,yr);
                }             
                break;  
        case 6: {
            
            uygulandi("Komutlar+im ",sparam,yr,2000);
            uygulandi("lamba, gurup, sistem, senaryo ve varsa +ozel komutlar.",sparam,yr,5500);
            uygulandi("her bir komutun alt detay+i vard+ir. Detaylar i+cin kullan+im k+ilavuzuna bak+in.",sparam,yr,8000);
            uygulandi("bir ka+c +ornek vereyim.",sparam,yr,3000);
            uygulandi("lamba 1 a+c.",sparam,yr);
            uygulandi("t+um lambalar+i kapat.",sparam,yr);
            uygulandi("senaryo 1 grup +u+ce uygula",sparam,yr,3000);
            uygulandi("gurup biri kapat",sparam,yr);
            uygulandi("bunlar genel komutlar+im. +özel komutlara da birka+c +ornek vereyim",sparam,yr,8000);
            uygulandi("mutfak lambalar+in+i a+c",sparam,yr);
            uygulandi("salon perdelerini kapat",sparam,yr);
            uygulandi("yemek i+cin haz+irlan",sparam,yr);
            uygulandi("gece moduna ge+c",sparam,yr);
            uygulandi("daha detayl+i bilgi almak istiyorsan+iz kullan+im k+ilavuzunu okumal+i veya yard+im almal+is+in+iz.",sparam,yr);
        }                           
        break;
        case 7: {
            
            uygulandi("tan+iml+i cihazlar silindi.",sparam,yr); 

        } break;
    }
}

*/