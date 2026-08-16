# Yerel Anahtarlar — Nasıl Çalışır, Ne Yapmanız Gerekir

Bu doküman, kutuya doğrudan bağlı (DALI olmayan, "yerel"/kanal=10) fiziksel anahtar, switch ve
hareket sensörlerinin nasıl tanımlandığını, hedeflerinin nasıl atandığını ve tuşa
basıldığında sistemin ne yaptığını anlatır.

## 1. Kaynak: `device.json`

Yerel anahtarların tek kaynağı `/config/device.json` dosyasındaki `hardware_devices[].functions[]`
listesidir. Her fonksiyon şu alanları taşır:

```json
{
  "f_id": 155,
  "f_type": "SWITCH",
  "hw_pin": 7,
  "is_assigned": false
}
```

- **`f_id`** — global olarak **tekil** olmalı (aynı id'yi başka bir fonksiyonda, hatta `ROLE`
  tipinde bile tekrar kullanmayın — firmware bunu doğrudan `adr` olarak kullanıyor).
- **`f_type`** — üç değerden biri anlamlı:
  | f_type | Anlamı |
  |---|---|
  | `ANAHTAR` | Momentary (anlık) tuş — basılı tutulduğu sürece açık |
  | `SWITCH` | Maintained (sabit konumlu) anahtar — her basışta toggle |
  | `SENSOR` | Hareket sensörü — basışta açar, "retrigger" timer'ıyla açık tutar |

  (`ROLE` tipi röle çıkışları için, bu sistemin dışında.)
- **`is_assigned`** — bu pin zaten doğrudan bir cihaza/devreye kablolanmışsa `true` yapın.
  Böyle bir pin **pasif** sayılır, telefon05'te gösterilmez ve hedef ataması yapılamaz —
  çünkü zaten kendi devresini kontrol ediyor demektir.
- **`device_id`** — `1` (Merkezi Kontrolör) olan cihazların fonksiyonları bu sisteme hiç
  dahil edilmez. Anahtarlarınızı diğer donanım kartlarında (ör. "Ek Röle" kartı) tanımlayın.

## 2. Senkronizasyon — kutu her açıldığında otomatik

Kutu her boot olduğunda (`Local_Device_Read()`, `define_unit.cpp`) `device.json`'daki
ANAHTAR/SWITCH/SENSOR fonksiyonlarını okuyup kendi kalıcı kataloğuyla (`/config/instanceL.bin`)
karşılaştırır:

- **Yeni bir fonksiyon** eklendiyse → kataloğa yeni kayıt olarak eklenir (hedefsiz, boş).
- **Var olan bir fonksiyon** güncellendiyse (ör. `is_assigned` değişti) → sadece envanter
  bilgisi (tip, aktif/pasif) güncellenir, **daha önce atanmış hedef bozulmaz**.
- **device.json'dan silinen bir fonksiyon** → kataloğdaki kaydı da silinir.

Yani: `device.json`'u düzenleyip kutuyu yeniden başlattığınızda değişiklikler otomatik
yansır — telefon05'te ayrıca bir şey yapmanıza gerek yok, sadece `ins_intro` ile taze liste
çekilir (uygulama zaten bunu kendisi yapar).

## 3. Hedef atama — telefon05

**Cihaz Kurulumu → Oda Yönetimi → Yerel Anahtarlar**

Bu ekran, `device.json`'dan senkronize olmuş ve **aktif** (pasif olmayan) tüm yerel
anahtarları listeler. Bir anahtara dokunduğunuzda tipine göre bir popup açılır:

- **ANAHTAR / SWITCH** → anahtar ayar popup'u: hedef (Lamba / Grup / Senaryo / Anahtar) ve
  hedefin id'si kaydedilir (process alanı önemsiz, davranış tipe göre sabittir).
- **SENSOR (Hareket)** → hareket ayar popup'u: hedef seçilir, ayrıca **"Süre (dk)"** alanından
  retrigger timer'ın kaç dakika süreceği ayarlanır (bkz. bölüm 4).

Bir fiziksel anahtarın **birden fazla lambayı** kontrol etmesini istiyorsanız: hedef olarak
"Anahtar" (virtual switch) seçin — o virtual anahtara önceden atanmış tüm lambalar birlikte
tetiklenir.

**Not:** DALI instance'larından farklı olarak yerel anahtarlarda Aktif/Pasif durumu ve
Filtre/Zaman/C.Hold gibi DALI'ye özgü ayarlar bu popup'larda **gösterilmez** — çünkü
aktiflik `device.json`'daki `is_assigned`'dan geliyor, filtre/zaman ise DALI protokolüne özgü
kavramlar.

## 4. Tuşa basınca ne oluyor — tip tip davranış

| Tip | Basma (stat=1) | Bırakma (stat=0) |
|---|---|---|
| **ANAHTAR** (momentary) | Hedef **açılır** (Max Level) | Hedef **kapanır** |
| **SWITCH** (maintained) | Hedefin **anlık durumu kontrol edilip tersi** gönderilir (gerçek toggle) | Hiçbir şey olmaz |
| **SENSOR** (hareket) | **`stat` değeri (0/1) fark etmeksizin**, gelen her `tus` mesajında hedef **açılır**, popup'ta ayarlanan **dakika** kadar bir timer başlar/sıfırlanır (ışık açık kalır) | — |

**Not:** SENSOR pinleri her basışta tek bir `tus` mesajı gönderiyor ama `stat` değeri
basma/bırakma anlamına gelmiyor (0 ya da 1 olabilir) — bu yüzden ANAHTAR/SWITCH'ten farklı
olarak stat'a bakılmıyor, mesajın kendisi yeterli tetikleyici sayılıyor.

Hedef DALI'de bir lamba/grup/senaryo/virtual-anahtar olabilir, ya da kutuya bağlı yerel bir
röle/lamba olabilir — davranış aynı şekilde çalışır, fark etmez.

**SENSOR süresi:** telefon05'teki hareket popup'unda "Süre (dk)" alanından ayarlanır
(varsayılan 1 dakika), firmware'de `instance_t.temp_set` alanında dakika olarak saklanır.
Hiç ayarlanmamışsa (0) firmware sabit **10 saniyelik** bir güvenlik varsayılanına düşer.

**Pasif (`is_assigned:true`) pinler** bu sistemin tamamen dışındadır — onlar eski/klasik
mekanizmayla (`device.json`'daki `actions`/`mapped_inputs` eşlemesi) doğrudan kendi
cihazlarını kontrol etmeye devam eder.

## 5. Kullanıcının yapması gerekenler — adım adım

1. `device.json`'da yeni anahtarı tanımlayın: tekil bir `f_id`, doğru `f_type`
   (ANAHTAR/SWITCH/SENSOR), doğru `hw_pin`, `is_assigned:false` (boşta olsun istiyorsanız).
2. Kutuyu yeniden başlatın (ya da tam flash sonrası ilk açılış) — otomatik senkronize olur.
3. Telefon05'te **Cihaz Kurulumu → Oda Yönetimi → Yerel Anahtarlar**'ı açın, yeni anahtarı
   bulun.
4. Anahtara dokunup hedefini (Lamba/Grup/Senaryo/Anahtar) seçip kaydedin.
5. Fiziksel tuşa basıp test edin; tipine göre yukarıdaki tabloya bakarak beklenen davranışı
   doğrulayın.

## 6. Bilinen sınırlamalar (henüz yapılmadı)

- **DALI'nin kendi fiziksel hareket/buton sensörleri** bu retrigger-timer / toggle
  sistemine henüz bağlı değil — sadece yerel (`kanal=10`) tuşlar için çalışıyor.
- **Arc Power** process'i (doğrudan seviye ayarı) henüz desteklenmiyor.
- `f_id` çakışmaları (aynı id iki farklı fonksiyonda) sessizce yanlış davranışa yol açar —
  device.json'u elle düzenlerken dikkatli olun.
