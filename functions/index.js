const { onRequest } = require("firebase-functions/v2/https");
const admin = require("firebase-admin");
const jwt = require("jsonwebtoken");

// AI ENTEGRASYONU İÇİN GEREKLİ KÜTÜPHANELER
const { genkit } = require("genkit");
const { googleAI } = require("@genkit-ai/googleai");
const { z } = require("zod");

admin.initializeApp();

// Genkit'i model belirtmeden, sadece plugin ile yalın başlatıyoruz
const aiInstance = genkit({
  plugins: [
    googleAI({ apiKey: process.env.GEMINI_API_KEY })
  ],
});

/**
 * YARDIMCI FONKSİYON: Open-Meteo Geocoding API ile Türkçe il/ilçe/mahalle adını lat/lng'e çevirir.
 * API key gerektirmez. updateDeviceLocation, getIrrigationDecision ve interpretVoiceCommand
 * (konum kaydetme) tarafından kullanılır.
 *
 * 'location' EN SPESİFİK yer adı olmalıdır (örn. "Maltepe") — Open-Meteo birleşik
 * "İl İlçe" string'lerinde ("İstanbul Maltepe") çoğu zaman sonuç döndürmüyor.
 * 'region' opsiyoneldir; aynı isimde birden fazla yer varsa (örn. başka bir ildeki
 * "Maltepe") doğru sonucu seçmek için Open-Meteo'nun döndürdüğü admin1/admin2
 * alanlarıyla karşılaştırılır. Bulunamazsa Open-Meteo'nun ilk (en yüksek nüfuslu) sonucuna düşülür.
 */
async function geocodeLocation(location, region = null) {
  const geoUrl = `https://geocoding-api.open-meteo.com/v1/search?name=${encodeURIComponent(location)}&count=10&language=tr&country=TR`;
  const geoResp = await fetch(geoUrl);
  if (!geoResp.ok) throw new Error(`Geocoding servisi hata döndürdü: ${geoResp.status}`);
  const geoData = await geoResp.json();
  if (!geoData.results || geoData.results.length === 0) {
    throw new Error(`'${location}' için konum bulunamadı.`);
  }

  let best = geoData.results[0];
  if (region) {
    const regionLower = region.toLocaleLowerCase("tr-TR");
    const match = geoData.results.find(r =>
      (r.admin1 && r.admin1.toLocaleLowerCase("tr-TR").includes(regionLower)) ||
      (r.admin2 && r.admin2.toLocaleLowerCase("tr-TR").includes(regionLower))
    );
    if (match) best = match;
  }

  return {
    lat: best.latitude,
    lng: best.longitude,
    name: region ? `${region} ${best.name}` : best.name
  };
}

/**
 * 1. FONKSİYON: registerHardwareDevice
 * Ana kutunun (Hardware) kendini veritabanına kaydetmesini / güncellemesini sağlar.
 * GÜNCELLEME: Sayısal 0 (sıfır) değerlerinin hata fırlatması engellendi.
 */
exports.registerHardwareDevice = onRequest(async (req, res) => {
  try {
    const { lisansKodu, projeNo, binaNo, katNo, daireNo, version, ip } = req.body.data || {};

    // GÜVENLİ KONTROL: Değerlerin varlığı explicit (null/undefined) olarak kontrol ediliyor, 0 artık yasal.
    if (
      lisansKodu === undefined || lisansKodu === null || lisansKodu === "" ||
      projeNo === undefined || projeNo === null ||
      binaNo === undefined || binaNo === null ||
      daireNo === undefined || daireNo === null
    ) {
      return res.status(400).send({
        data: { status: "error", message: "lisansKodu, projeNo, binaNo ve daireNo alanları zorunludur." }
      });
    }

    const db = admin.firestore();

    // Doküman ID'si (anahtar) olarak doğrudan lisansKodu kullanılıyor
    await db.collection("cihaz_kayitlari").doc(lisansKodu).set({
      // String Değerler
      lisansKodu: String(lisansKodu),
      version: version ? String(version) : "0.0.0",
      ip: ip ? String(ip) : "0.0.0.0",

      // Number (Sayısal) Değerler
      projeNo: Number(projeNo),
      binaNo: Number(binaNo),
      katNo: Number(katNo),
      daireNo: Number(daireNo),

      // Zaman Damgası
      sonKayitTarihi: admin.firestore.FieldValue.serverTimestamp()
    }, { merge: true });

    return res.status(200).send({ data: { status: "success", message: "Ana kutu kaydı/güncellemesi başarılı." } });

  } catch (error) {
    return res.status(500).send({ data: { status: "error", message: error.message } });
  }
});

/**
 * 2. FONKSİYON: getHardwareDevices
 * Yönetim programının filtre göndererek daire/cihaz listesi almasını sağlar.
 */
exports.getHardwareDevices = onRequest(async (req, res) => {
  try {
    const { projeNo, binaNo, katNo, daireNo, lisansKodu } = req.body.data || {};

    const db = admin.firestore();
    let query = db.collection("cihaz_kayitlari");

    if (projeNo !== undefined && projeNo !== null) {
      query = query.where("projeNo", "==", Number(projeNo));
    }
    if (binaNo !== undefined && binaNo !== null) {
      query = query.where("binaNo", "==", Number(binaNo));
    }
    if (katNo !== undefined && katNo !== null) {
      query = query.where("katNo", "==", Number(katNo));
    }
    if (daireNo !== undefined && daireNo !== null) {
      query = query.where("daireNo", "==", Number(daireNo));
    }
    if (lisansKodu) {
      query = query.where("lisansKodu", "==", String(lisansKodu));
    }

    const snapshot = await query.get();
    const cihazListesi = [];

    snapshot.forEach(doc => {
      const data = doc.data();
      if (data.sonKayitTarihi) {
        data.sonKayitTarihi = data.sonKayitTarihi.toDate().toISOString();
      }
      cihazListesi.push(data);
    });

    return res.status(200).send({
      data: {
        status: "success",
        totalCount: cihazListesi.length,
        devices: cihazListesi
      }
    });

  } catch (error) {
    console.error("Listeleme Hatası:", error);
    return res.status(500).send({ data: { status: "error", message: error.message } });
  }
});

/**
 * 3. FONKSİYON: kaydetCihazToken
 */
exports.kaydetCihazToken = onRequest(async (req, res) => {
  try {
    const { cihazId, platform, fcmToken, license } = req.body.data || {};
    if (!cihazId || !fcmToken) return res.status(400).send({ data: { status: "error", message: "Eksik parametre." } });
    const db = admin.firestore();
    await db.collection("cihazlar").doc(cihazId).set({
      cihazId, platform, fcmToken, license: license || "free_trial", sonGuncelleme: admin.firestore.FieldValue.serverTimestamp()
    }, { merge: true });
    return res.status(200).send({ data: { status: "success", message: "Cihaz başarıyla kaydedildi." } });
  } catch (error) {
    return res.status(500).send({ data: { status: "error", message: error.message } });
  }
});

/**
 * 4. FONKSİYON: sendAlarmNotification
 */
exports.sendAlarmNotification = onRequest(async (req, res) => {
  try {
    const { type, target, title, body, text } = req.body.data || {};
    const db = admin.firestore();
    let tokens = [];

    if (!type) return res.status(400).send({ data: { status: "error", message: "Eksik 'type' parametresi." } });

    if (type === "single") {
      if (!target) return res.status(400).send({ data: { status: "error", message: "target gerekli." } });
      tokens.push(target);
    } else if (type === "license") {
      if (!target) return res.status(400).send({ data: { status: "error", message: "target gerekli." } });
      const snapshot = await db.collection("cihazlar").where("license", "==", target).get();
      snapshot.forEach(doc => { if (doc.data().fcmToken) tokens.push(doc.data().fcmToken); });
    } else if (type === "all") {
      const snapshot = await db.collection("cihazlar").get();
      snapshot.forEach(doc => { if (doc.data().fcmToken) tokens.push(doc.data().fcmToken); });
    }

    if (tokens.length === 0) return res.status(200).send({ data: { status: "success", message: "Cihaz bulunamadı." } });

    const customData = {};
    if (text) {
      customData.com = "say";
      customData.txt = String(text);
    }

    const message = {
      notification: {
        title: title || "🚨 SİSTEM ALARMI!",
        body: body || "Donanım üzerinden kritik bir alarm tetiklendi.",
      },
      data: Object.keys(customData).length > 0 ? customData : undefined,
      tokens: tokens,
      android: {
        priority: "high",
        notification: { sound: "default", channelId: "alarm_channel", priority: "high" }
      },
      apns: {
        headers: { "apns-priority": "10" },
        payload: { aps: { sound: "default" } }
      }
    };

    const response = await admin.messaging().sendEachForMulticast(message);
    return res.status(200).send({ data: { status: "success", sentCount: response.successCount } });

  } catch (error) {
    console.error("Hata:", error);
    return res.status(500).send({ data: { status: "error", message: error.message } });
  }
});

/**
 * 5. FONKSİYON: interpretVoiceCommand
 * Doğal dil komutlarını SmartQ cihazlarının anlayacağı kesin JSON şemasına çevirir.
 */
exports.interpretVoiceCommand = onRequest({ secrets: ["GEMINI_API_KEY"] }, async (req, res) => {
  try {
    const { userPrompt, projeNo, binaNo, daireNo, lisansKodu } = req.body.data || {};

    if (!userPrompt) {
      return res.status(400).send({ data: { status: "error", message: "userPrompt alanı zorunludur." } });
    }

    // GÜNLÜK AI KULLANIM LİMİTİ (lisans başına) — maliyeti kontrol altında tutar.
    // 'cihaz_kayitlari/{lisansKodu}' dokümanında 'aiLimitsiz:true' olan lisanslar
    // bu limitten muaftır (ör. premium/özel anlaşmalı lisanslar). Limit dolduysa
    // Gemini'ye HİÇ gidilmez (maliyet orada da kesilir), client'ın zaten bildiği
    // targetType="unknown" + responseMessage şekliyle aynı sonucu döneriz.
    // Aynı dokümanda kısa süreli SOHBET HAFIZASI da tutulur (son 3 tur) — AI
    // "peki tatlı olarak ne önerirsin" gibi önceki soruya atıf yapan takip
    // sorularını anlayabilsin diye. Bağlamsız çok eski geçmiş taşınmaz, sadece
    // son birkaç tur — hem token maliyeti hem de eski konuya saplanma riski düşük kalır.
    const AI_DAILY_LIMIT = 50;
    const SOHBET_GECMISI_LIMIT = 3;
    let deviceRef = null;
    let sohbetGecmisi = [];
    let kullaniciAdi = null;
    let aiLimitsizMi = false;
    let aiGuncelSayac = 0; // bu sorgu dahil, "kaç kontörüm var" cevabı için
    if (lisansKodu) {
      deviceRef = admin.firestore().collection("cihaz_kayitlari").doc(String(lisansKodu));
      const deviceSnap = await deviceRef.get();
      const deviceData = deviceSnap.exists ? deviceSnap.data() : {};
      sohbetGecmisi = deviceData.sohbetGecmisi || [];
      kullaniciAdi = deviceData.kullaniciAdi || null;
      aiLimitsizMi = deviceData.aiLimitsiz === true;

      if (!aiLimitsizMi) {
        const bugun = new Date().toISOString().slice(0, 10); // YYYY-MM-DD
        const kullanim = deviceData.aiKullanim || {};
        const oncekiSayac = kullanim.tarih === bugun ? (kullanim.sayac || 0) : 0;

        if (oncekiSayac >= AI_DAILY_LIMIT) {
          return res.status(200).send({
            data: {
              status: "success",
              command: {
                targetType: "unknown",
                deviceType: "none",
                action: "STATUS_CHECK",
                sceneNo: 0,
                groupNo: 0,
                isAll: false,
                responseMessage: "Günlük yapay zeka kullanım limitinize ulaştınız, lütfen yarın tekrar deneyin."
              }
            }
          });
        }

        aiGuncelSayac = oncekiSayac + 1;
        await deviceRef.set({
          aiKullanim: { tarih: bugun, sayac: aiGuncelSayac }
        }, { merge: true });
      }
    }

    const gecmisMetni = sohbetGecmisi.length > 0
      ? "Önceki Konuşma (eskiden yeniye sıralı, sadece bağlam için):\n" +
        sohbetGecmisi.map(h => `Kullanıcı: ${h.soru}\nAsistan: ${h.cevap}`).join("\n") +
        "\n\n"
      : "";

    const kullaniciAdiMetni = kullaniciAdi
      ? `Kullanıcının Adı: ${kullaniciAdi} (uygun ve doğal olduğunda responseMessage içinde ismiyle hitap edebilirsin, her cümlede zorunlu değil)\n\n`
      : "";

    // Gelişmiş Akıllı Ev Cihaz, Mod, Senaryo ve Grup Şeması (Zod)
    const CommandSchema = z.object({
      targetType: z.enum([
        "device", "scene", "group", "location", "weather", "name", "aiUsage", "unknown"
      ]).describe("Komutun doğrudan hedef aldığı ana yapı türü. Doğrudan bir cihazsa 'device', 16 senaryodan biriyse 'scene', bir cihaz grubu hedefliyse 'group', kullanıcı kendi konumunu kaydetmek/güncellemek istiyorsa 'location', güncel hava durumunu soruyorsa 'weather', kendi adını kaydetmek/tanıtmak istiyorsa 'name', kendi yapay zeka kullanım hakkını/kontörünü soruyorsa 'aiUsage' seçilmelidir. NOT: 'gece moduna geç', 'misafir modu' gibi ev modu komutları da 'scene' olarak ele alınır (bkz. aşağıdaki SENARYO KURALLARI) — ayrı bir 'mode' kategorisi yoktur."),

      deviceType: z.enum([
        "blind", "door", "elevator", "energy", "garage",
        "gas", "water", "socket", "lamp", "climate", "none"
      ]).default("none").describe("Eğer targetType 'device' veya 'group' ise, hedef alınan akıllı ev cihazının türü. Senaryolarda 'none' olmalıdır."),

      action: z.enum(["ON", "OFF", "SET_VALUE", "STATUS_CHECK"])
        .describe("Cihaza, senaryoya veya gruba yaptırılacak eylem. Senaryo aktivasyonları için her zaman 'ON' kullanılmalıdır."),

      // 1. 16 ADET ÖZEL SENARYO (Scene) — ev modu komutları da buraya dahildir
      sceneNo: z.number().min(0).max(16).default(0)
        .describe("Kullanıcı 16 senaryodan birini tetiklediyse senaryo numarası (1-16 arası). Eğer senaryo tetiklenmediyse 0 olmalıdır. Örn: 'Sinema senaryosunu aç' (Eğer sinema 1. senaryoysa) -> 1, 'Senaryo 5'i aktif et' -> 5"),

      // 3. GRUP YÖNETİMİ (Group)
      groupNo: z.number().default(0)
        .describe("Kullanıcı belirli bir grubu hedeflediyse grup numarası (Örn: 'Grup 2'yi kapat' -> 2, '3. grup ışıkları aç' -> 3). Grup belirtilmediyse 0 olmalıdır."),

      isAll: z.boolean()
        .describe("Eğer kullanıcı 'tüm', 'hepsi', 'her yer' gibi bir kelime kullanarak o gruptaki veya evdeki tüm cihazları hedeflediyse true, tek bir cihazı/grubu hedeflediyse false olmalıdır. Çoğul ekleri (lambalar, perdeler) tek başına grubu veya senaryoyu ifade etmiyorsa true yapabilir."),

      value: z.number().optional()
        .describe("Dimmer yüzdesi veya termostat (climate) sıcaklık derecesi (Varsa)"),

      name: z.string().optional()
        .describe("Eğer kullanıcı 'mavi led', 'mutfak lambası', 'sinema senaryosu', 'bahçe grubu' gibi özel bir isim belirtmişse buraya aynen yazılmalıdır."),

      targetZone: z.string().optional()
        .describe("Komutun hedeflediği genel oda veya bölge (salon, mutfak, koridor vb.)"),

      locationSpecific: z.string().optional()
        .describe("SADECE targetType 'location' ise doldurulur. Kullanıcının söylediği EN SPESİFİK yer adı (ilçe/mahalle/semt), il adı OLMADAN, çekim eki almadan sade hâliyle (Örn: 'istanbulun maltepe ilçesindeyim' -> 'Maltepe', 'izmir çiğli seyrekteyim' -> 'Seyrek'). Bu, gerçek bir coğrafi arama servisine gönderilecektir; birleşik 'İl İlçe' yazma."),

      locationRegion: z.string().optional()
        .describe("SADECE targetType 'location' ise ve kullanıcı bir il/şehir de belirtmişse doldurulur (Örn: 'İstanbul', 'İzmir'). Aynı isimde birden fazla yer olma ihtimaline karşı doğru olanı seçmek için kullanılır, arama sorgusuna dahil edilmez."),

      userName: z.string().optional()
        .describe("SADECE targetType 'name' ise doldurulur. Kullanıcının söylediği KENDİ adı, sade hâliyle (Örn: 'ben ahmet, ismimi kaydet' -> 'Ahmet', 'adım ayşe' -> 'Ayşe'). Bu, cihaz/senaryo/grup ismi olan 'name' alanıyla KARIŞTIRILMAMALIDIR."),

      responseMessage: z.string()
        .describe("Kullanıcıya mobil uygulamada dönecek sesli/yazılı Türkçe yanıt")
    });

    // Gemini ile şemalı analiz çağrısı
    const aiResponse = await aiInstance.generate({
      model: 'googleai/gemini-2.5-flash',
      prompt: `${gecmisMetni}${kullaniciAdiMetni}Kullanıcı Komutu: "${userPrompt}"\nBağlam -> Proje: ${projeNo || 1}, Bina: ${binaNo || 0}, Daire: ${daireNo || 0}`,
      output: {
        schema: CommandSchema,
      },
      system: `Sen Smart Q akıllı ev otomasyon sisteminin yapay zeka motorusun.
               Kullanıcılardan gelen Türkçe doğal dil komutlarını cihazların, senaryoların ve grupların işleyebileceği saf verilere dönüştürürsün.

               SOHBET HAFIZASI: Eğer "Önceki Konuşma" bölümü varsa, bu SADECE bağlam
               içindir — kullanıcı "peki ya tatlı?", "onu da yap" gibi önceki soruya atıfta
               bulunan bir takip sorusu sorarsa bunu anlamak için kullan. Önceki turdaki bir
               cihaz/senaryo/konu, kullanıcı yeni cümlede AÇIKÇA ATIFTA BULUNMADIKÇA yeni
               komuta taşınmaz — her komutu öncelikle kendi içeriğine göre değerlendir.

               KRİTİK HEDEF AYRIMI KURALLARI:
               1. TARGET_TYPE SEÇİMİ:
                  - Eğer komut tekil bir cihazı veya genel cihaz sınıfını hedefliyorsa (Örn: "ışığı yak", "klimayı 25 dereceye ayarla"): targetType = "device"
                  - Eğer komut 16 senaryodan birini hedefliyorsa VEYA bir ev moduna geçişse (Örn: "1. senaryoyu çalıştır", "kitap okuma senaryosunu aç", "gece moduna geç", "misafir moduna geç", "evden çıkış modu"): targetType = "scene". AYRI BİR "mode" KATEGORİSİ YOKTUR — ev modu komutları da senaryo olarak ele alınır (kurulumda bu isimlerle — gece, gündüz, misafir vb. — gerçek senaryolar tanımlanmış olabilir).
                  - Eğer komut tanımlı bir grubu hedefliyorsa (Örn: "2. grubu kapat", "bahçe grubunu aç"): targetType = "group"
                  - Eğer kullanıcı kendi bulunduğu konumu kaydetmek/güncellemek/hatırlatmak istiyorsa (Örn: "ben izmir çiğli seyrekteyim, konumumu kaydet", "koordinatlarımı bul ve hatırla"): targetType = "location"

               2. SENARYO (SCENE) KURALLARI:
                  - Kullanıcı ismen veya numarayla bir senaryo tetiklediğinde targetType="scene", action="ON" olmalı ve 'sceneNo' alanına 1-16 arası ilgili sayı yazılmalıdır. Cümlede numara geçmiyorsa (Örn: Sinema, Romantik, Kitap Okuma, Parti VEYA gece/gündüz/misafir/dışarı gibi bir "ev modu" ifadesi), sayıyı tahmin etmeye ÇALIŞMA — bunun yerine 'name' alanına kullanıcının söylediği ismi (Örn: "gece", "misafir") AYNEN yaz; gerçek senaryo numarası bu isimden sunucu/cihaz tarafında çözülecektir.

               3. GRUP (GROUP) KURALLARI:
                  - Kullanıcı grup belirttiğinde targetType="group", 'groupNo' alanına ilgili grup numarası yazılmalıdır. Eyleme göre action "ON" veya "OFF" olur.

               4. GENEL CİHAZ VE EYLEM KURALLARI:
                  - deviceType alanı sadece targetType 'device' veya 'group' olduğunda doldurulur (blind, lamp, climate vb.). Senaryolarda "none" kalır.
                  - action alanı KESİNLİKLE çıktıya eklenmelidir. Açma/Aktif etme="ON", Kapatma="OFF", Derece/Yüzde="SET_VALUE", Durum sorma="STATUS_CHECK" olmalıdır.
                  - Klima (climate) için sıcaklık derecesi istendiğinde action="SET_VALUE", value alanına istenen derece (Örn: 25) yazılmalıdır.
                  - isAll alanı; "tüm lambalar", "her yeri kapat" gibi toplu cihaz komutlarında true olmalıdır.
                  - responseMessage alanına her zaman doğal, onaylayıcı ve asistan üslubuna uygun Türkçe bir cümle yaz.

               5. KONUM (LOCATION) KURALLARI:
                  - targetType="location" ise action="ON" olmalı.
                  - locationSpecific alanına SADECE en spesifik yer adı (ilçe/mahalle/semt) yazılır, il adı KATILMAZ ve çekim eki ("-deyim", "-ındayım" vb.) TEMİZLENİR. Örn: "istanbulun maltepe ilçesindeyim" -> locationSpecific="Maltepe". "izmir çiğli seyrekteyim" -> locationSpecific="Seyrek".
                  - locationRegion alanına, kullanıcı bir il/şehir belirtmişse o il adı sade hâliyle yazılır (Örn: "İstanbul", "İzmir"). Belirtilmemişse boş bırakılır.
                  - SEN (yapay zeka) ASLA enlem/boylam/koordinat ÜRETMEZSİN ve tahmin etmezsin. Koordinat çözümlemesi tamamen ayrı, gerçek bir coğrafi veritabanı tarafından yapılacaktır. Senin tek görevin cümledeki yer adını doğru ayrıştırmaktır.
                  - responseMessage alanına konum kaydıyla ilgili genel bir onay cümlesi yaz (örn. "Konumunuzu kaydediyorum."); kesin başarı/hata mesajı ayrıca sunucu tarafından güncellenecektir.

               6. HAVA DURUMU (WEATHER) KURALLARI:
                  - Kullanıcı güncel/genel hava durumunu soruyorsa (Örn: "hava durumu nasıl", "bugün hava nasıl", "dışarısı soğuk mu") targetType="weather", action="STATUS_CHECK" olmalıdır.
                  - SEN (yapay zeka) hava durumu verisine SAHİP DEĞİLSİN, ASLA sıcaklık/nem/yağış gibi değerler UYDURMAZSIN. Senin görevin sadece niyeti tespit etmektir; gerçek veri ve kesin cevap sunucu tarafından ayrıca eklenecektir.
                  - responseMessage alanına genel bir onay cümlesi yaz (örn. "Hava durumunu kontrol ediyorum."); kesin cevap sunucu tarafından güncellenecektir.

               7. GENEL SOHBET / ÖNERİ KURALLARI:
                  - Kullanıcı cihaz/senaryo/grup/konum/hava durumuyla ilgisi olmayan genel bir soru soruyorsa (Örn: "ne yesem", "akşam yemeği için fikir ver", "bana bir tarif öner"): targetType="unknown", action="STATUS_CHECK" olmalıdır.
                  - BU KATEGORİDE (SADECE bu kategoride), diğer kurallardaki "veri uydurma" kısıtı GEÇERLİ DEĞİLDİR — çünkü yemek/genel öneri doğası gereği öznel bir tavsiyedir, doğrulanması gereken bir gerçek değildir. Kendi bilgi ve yaratıcılığınla, samimi ve yardımsever bir Türkçe cevap üret; responseMessage alanına doğrudan bu cevabı yaz.
                  - Cevabı kısa ve sesli okunmaya uygun tut (2-3 cümleyi geçmesin).

               8. KULLANICI ADI KURALLARI:
                  - Kullanıcı kendi adını kaydetmek/tanıtmak isterse (Örn: "ben ahmet, ismimi kaydet", "adımı ayşe olarak kaydet", "beni ahmet diye hatırla"): targetType="name", action="ON", userName alanına kullanıcının söylediği adı sade hâliyle (çekim eki temizlenmiş) yaz.
                  - responseMessage alanına ismi kullanarak kısa bir onay cümlesi yaz (Örn: "Memnun oldum Ahmet, seni hatırlayacağım.").
                  - Eğer prompt başında "Kullanıcının Adı" bilgisi verilmişse (daha önce kaydedilmiş), diğer TÜM komut türlerinde (device/scene/group/unknown vb.) responseMessage'da uygun ve doğal düştüğü yerde kullanıcıya ismiyle hitap edebilirsin — ama bunu her cümlede zorunlu tutma, doğal bir sohbet gibi davran.

               9. YAPAY ZEKA KULLANIM HAKKI (KONTÖR) KURALLARI:
                  - Kullanıcı kendi yapay zeka kullanım hakkını/kontörünü/limitini soruyorsa (Örn: "kaç kontörüm var", "ne kadar hakkım kaldı", "bugün kaç kez yapay zeka kullandım", "günlük limitim ne kadar"): targetType="aiUsage", action="STATUS_CHECK" olmalıdır.
                  - SEN (yapay zeka) kaç hak kullanıldığını veya kaldığını BİLMİYORSUN, ASLA bir sayı UYDURMAZSIN. Görevin sadece niyeti tespit etmektir; responseMessage alanına genel bir onay cümlesi yaz (örn. "Kullanım hakkınızı kontrol ediyorum."), kesin sayı sunucu tarafından ayrıca eklenecektir.`
    });

    const command = aiResponse.output;

    // KONUM KAYDETME: koordinatı AI değil, gerçek geocoding servisi üretir.
    // updateDeviceLocation ile AYNI alanlara (lat/lng/locationName) yazılır ki tek bir
    // konum kaynağı olsun (getIrrigationDecision de bunu okur).
    if (command && command.targetType === "location" && command.locationSpecific) {
      if (!lisansKodu) {
        command.responseMessage = "Konumunuzu kaydedemedim, cihaz kimliği bulunamadı.";
      } else {
        try {
          const resolved = await geocodeLocation(command.locationSpecific, command.locationRegion || null);
          await admin.firestore().collection("cihaz_kayitlari").doc(String(lisansKodu)).set({
            lat: resolved.lat,
            lng: resolved.lng,
            locationName: resolved.name,
            locationGuncelleme: admin.firestore.FieldValue.serverTimestamp()
          }, { merge: true });
          command.responseMessage = `${resolved.name} konumunuz olarak kaydedildi.`;
        } catch (geoError) {
          console.error("Konum (voice) Hatası:", geoError);
          command.responseMessage = `${command.locationSpecific} konumunu bulamadım, lütfen tekrar deneyin.`;
        }
      }
    }

    // HAVA DURUMU SORGUSU: AI'ye hava verisi ürettirilmez, cihazın kayıtlı konumu için
    // gerçek Open-Meteo verisi çekilip cevap sunucu tarafında oluşturulur.
    if (command && command.targetType === "weather") {
      if (!lisansKodu) {
        command.responseMessage = "Hava durumunu kontrol edemedim, cihaz kimliği bulunamadı.";
      } else {
        try {
          const deviceSnap = await admin.firestore().collection("cihaz_kayitlari").doc(String(lisansKodu)).get();
          const d = deviceSnap.exists ? deviceSnap.data() : null;
          if (!d || d.lat === undefined || d.lng === undefined) {
            command.responseMessage = "Kayıtlı bir konumunuz olmadığı için hava durumunu öğrenemedim.";
          } else {
            const weatherUrl = `https://api.open-meteo.com/v1/forecast?latitude=${d.lat}&longitude=${d.lng}` +
              `&current=temperature_2m,relative_humidity_2m,precipitation,wind_speed_10m&timezone=Europe%2FIstanbul`;
            const weatherResp = await fetch(weatherUrl);
            if (!weatherResp.ok) throw new Error(`Hava durumu servisi hata döndürdü: ${weatherResp.status}`);
            const weatherData = await weatherResp.json();
            const cur = weatherData.current;
            const yerAdi = d.locationName || "bulunduğunuz konumda";
            const yagisCumlesi = cur.precipitation > 0 ? "Şu anda yağış var." : "Yağış yok.";
            command.responseMessage = `${yerAdi} şu anda sıcaklık ${cur.temperature_2m} derece, nem yüzde ${cur.relative_humidity_2m}, rüzgar ${cur.wind_speed_10m} kilometre saat. ${yagisCumlesi}`;
          }
        } catch (weatherError) {
          console.error("Hava Durumu (voice) Hatası:", weatherError);
          command.responseMessage = "Hava durumu bilgisini şu anda alamadım, lütfen tekrar deneyin.";
        }
      }
    }

    // KULLANICI ADI KAYDETME
    if (command && command.targetType === "name" && command.userName) {
      if (!lisansKodu) {
        command.responseMessage = "Adınızı kaydedemedim, cihaz kimliği bulunamadı.";
      } else {
        await deviceRef.set({ kullaniciAdi: command.userName }, { merge: true });
      }
    }

    // YAPAY ZEKA KULLANIM HAKKI (KONTÖR) SORGUSU: gerçek sayı AI'ye değil,
    // Firestore'dan (bu isteğin başında zaten okunmuş veriden) gelir.
    if (command && command.targetType === "aiUsage") {
      if (!lisansKodu) {
        command.responseMessage = "Kullanım hakkınızı kontrol edemedim, cihaz kimliği bulunamadı.";
      } else if (aiLimitsizMi) {
        command.responseMessage = "Sınırsız yapay zeka kullanım hakkınız var, günlük bir limitiniz yok.";
      } else {
        const kalanHak = Math.max(0, AI_DAILY_LIMIT - aiGuncelSayac);
        command.responseMessage = `Bugün için ${kalanHak} hakkınız kaldı (günlük limit ${AI_DAILY_LIMIT}, şu ana kadar ${aiGuncelSayac} kez kullandınız).`;
      }
    }

    // SOHBET HAFIZASINI GÜNCELLE: bu turu ekleyip son 3 turla sınırlıyoruz (token
    // maliyeti ve eski konuya saplanma riskini düşük tutmak için sınırsız değil).
    if (deviceRef && command) {
      const yeniGecmis = [...sohbetGecmisi, { soru: userPrompt, cevap: command.responseMessage || "" }].slice(-SOHBET_GECMISI_LIMIT);
      await deviceRef.set({ sohbetGecmisi: yeniGecmis }, { merge: true });
    }

    return res.status(200).send({
      data: {
        status: "success",
        command: command
      }
    });

  } catch (error) {
    console.error("AI Yorumlama Hatası:", error);
    return res.status(500).send({ data: { status: "error", message: error.message } });
  }
});

/**
 * 6. FONKSİYON: updateDeviceLocation
 * Cihazın sulama konumunu AÇIKÇA ve KOŞULSUZ olarak günceller/kaydeder.
 * İki senaryoda çağrılmalıdır:
 *   1) İlk kurulum sırasında (embedded veya web arayüzünden)
 *   2) Cihaz fiziksel olarak taşındığında
 * getIrrigationDecision bu fonksiyonun yazdığı cache'i SADECE OKUR, kendisi konum
 * değişikliğini tespit etmeye çalışmaz — sorumluluk ayrımı burada net tutulmuştur.
 *
 * Girdi JSON örnekleri:
 *   {"lisansKodu":"ABC123","location":"istanbul maltepe"}
 *   {"lisansKodu":"ABC123","lat":40.93,"lng":29.15}
 */
exports.updateDeviceLocation = onRequest(async (req, res) => {
  try {
    const { lisansKodu, location, lat, lng } = req.body.data || {};

    if (!lisansKodu) {
      return res.status(400).send({ data: { status: "error", message: "'lisansKodu' alanı zorunludur." } });
    }
    if ((lat === undefined || lng === undefined) && !location) {
      return res.status(400).send({ data: { status: "error", message: "'location' ya da 'lat'/'lng' ikilisi zorunludur." } });
    }

    let resolved;
    if (lat !== undefined && lng !== undefined) {
      resolved = { lat: Number(lat), lng: Number(lng), name: location || `${lat},${lng}` };
    } else {
      resolved = await geocodeLocation(location);
    }

    const db = admin.firestore();
    await db.collection("cihaz_kayitlari").doc(String(lisansKodu)).set({
      lat: resolved.lat,
      lng: resolved.lng,
      locationName: resolved.name,
      locationGuncelleme: admin.firestore.FieldValue.serverTimestamp()
    }, { merge: true });

    return res.status(200).send({
      data: { status: "success", message: "Konum güncellendi.", lat: resolved.lat, lng: resolved.lng, location: resolved.name }
    });

  } catch (error) {
    console.error("Konum Güncelleme Hatası:", error);
    return res.status(500).send({ data: { status: "error", message: error.message } });
  }
});

/**
 * 7. FONKSİYON: getIrrigationDecision
 * Cihazdan gelen konum + planlanan sulama saati/süresine göre, GERÇEK hava durumu
 * verisini (Open-Meteo, API key gerektirmez) çekip AI'a yorumlatarak nihai sulama
 * süresini belirler. AI'a "hava nasıl?" diye SORULMAZ; ham hava verisi ona verilir,
 * o sadece bu veriyi yorumlar. Böylece halüsinasyon riski ortadan kalkar.
 *
 * 'duration' bir BAZ/ORTALAMA süredir, sabit tavan değildir — AI mevsimsel/hava koşullarına
 * göre bu süreyi artırabilir veya azaltabilir. GÜVENLİK KELEPÇESİ: AI'ın döndürdüğü süre
 * (a) girdideki 'duration'ın en fazla 2 katı olabilir, (b) sistem genelinde tanımlı mutlak
 * bir tavanı (AI_MAX_ABSOLUTE_MINUTES) hiçbir koşulda aşamaz. Bu kod-seviyesi kelepçe,
 * modelin hatalı/aşırı bir değer üretmesi durumunda fiziksel taşkın/su israfı riskini
 * prompt'a değil, koda bağlı sabit bir kurala dayandırır.
 *
 * KONUM KAYNAĞI: Bu fonksiyon konum kıyaslaması YAPMAZ, sadece okur.
 *   - lat/lng doğrudan gönderilirse onu kullanır (tek seferlik override, cache'e yazmaz).
 *   - lisansKodu gönderilirse Firestore cache'inden (cihaz_kayitlari/{lisansKodu}) okur.
 *   - Cache boşsa ve 'location' de gönderilmemişse hata döner — önce updateDeviceLocation
 *     çağrılmalıdır (ilk kurulum) ya da cihaz taşındıysa yine updateDeviceLocation.
 *
 * Girdi JSON örnekleri:
 *   {"lisansKodu":"ABC123","time":"05:20","duration":15}   // konum cache'ten okunur
 *   {"lat":40.93,"lng":29.15,"time":"05:20","duration":15} // tek seferlik override
 */
exports.getIrrigationDecision = onRequest({ secrets: ["GEMINI_API_KEY"] }, async (req, res) => {
  try {
    const { lisansKodu, lat, lng, time, duration } = req.body.data || {};

    // GİRDİ DOĞRULAMA
    const requestedDuration = Number(duration);
    if (!time || typeof time !== "string" || !/^\d{2}:\d{2}$/.test(time)) {
      return res.status(400).send({ data: { status: "error", message: "'time' alanı HH:MM formatında zorunludur." } });
    }
    if (!Number.isFinite(requestedDuration) || requestedDuration < 0) {
      return res.status(400).send({ data: { status: "error", message: "'duration' alanı zorunlu ve sıfır veya pozitif bir sayı olmalıdır." } });
    }
    if ((lat === undefined || lng === undefined) && !lisansKodu) {
      return res.status(400).send({ data: { status: "error", message: "'lisansKodu' ya da 'lat'/'lng' ikilisi zorunludur." } });
    }

    // 1. ADIM: KONUM ÇÖZÜMLEME — sadece override ya da cache okuma, geocode YOK
    let resolvedLat = lat !== undefined ? Number(lat) : null;
    let resolvedLng = lng !== undefined ? Number(lng) : null;
    let resolvedName = (lat !== undefined && lng !== undefined) ? `${lat},${lng}` : null;

    if (resolvedLat === null || resolvedLng === null) {
      const deviceSnap = await admin.firestore().collection("cihaz_kayitlari").doc(String(lisansKodu)).get();
      const d = deviceSnap.exists ? deviceSnap.data() : null;
      if (!d || d.lat === undefined || d.lng === undefined) {
        return res.status(400).send({
          data: { status: "error", message: "Bu lisans için kayıtlı konum yok. Önce 'updateDeviceLocation' çağrılmalıdır." }
        });
      }
      resolvedLat = d.lat;
      resolvedLng = d.lng;
      resolvedName = d.locationName;
    }

    // 2. ADIM: GERÇEK SAATLİK HAVA VERİSİ ÇEKME (Open-Meteo, key gerektirmez)
    const weatherUrl = `https://api.open-meteo.com/v1/forecast?latitude=${resolvedLat}&longitude=${resolvedLng}` +
      `&hourly=temperature_2m,relative_humidity_2m,precipitation_probability,precipitation,wind_speed_10m` +
      `&timezone=Europe%2FIstanbul&forecast_days=2`;
    const weatherResp = await fetch(weatherUrl);
    if (!weatherResp.ok) throw new Error(`Hava durumu servisi hata döndürdü: ${weatherResp.status}`);
    const weatherData = await weatherResp.json();

    // İstenen 'time' saatine en yakın saatlik veri noktasını bul (bugünün tarihiyle)
    const todayStr = new Date().toISOString().slice(0, 10);
    const targetTimestamp = new Date(`${todayStr}T${time}:00`).getTime();
    const hourlyTimes = weatherData.hourly.time;
    let closestIdx = 0;
    let closestDiff = Infinity;
    hourlyTimes.forEach((t, idx) => {
      const diff = Math.abs(new Date(t).getTime() - targetTimestamp);
      if (diff < closestDiff) { closestDiff = diff; closestIdx = idx; }
    });

    const weatherFacts = {
      sicaklikC: weatherData.hourly.temperature_2m[closestIdx],
      nemYuzde: weatherData.hourly.relative_humidity_2m[closestIdx],
      yagisIhtimaliYuzde: weatherData.hourly.precipitation_probability[closestIdx],
      yagisMm: weatherData.hourly.precipitation[closestIdx],
      ruzgarKmh: weatherData.hourly.wind_speed_10m[closestIdx]
    };

    // 3. ADIM: AI'A HAM VERİYİ VERİP SADECE YORUMLATMA (veri üretmesi değil, yorumlaması isteniyor)
    const IrrigationSchema = z.object({
      duration: z.number().min(0).describe("Önerilen sulama süresi (dakika). Asla girdideki istenen süreyi aşmamalı, sadece azaltılabilir veya 0 yapılabilir."),
      reason: z.string().describe("Kararın kısa Türkçe gerekçesi (örn. 'Yüksek yağış ihtimali nedeniyle sulama iptal edildi.')")
    });

    const aiResponse = await aiInstance.generate({
      model: 'googleai/gemini-2.5-flash',
      prompt: `Konum: ${resolvedName}\nPlanlanan sulama saati: ${time}\nİstenen (maksimum) sulama süresi: ${requestedDuration} dakika\n` +
        `Güncel hava verisi -> Sıcaklık: ${weatherFacts.sicaklikC}°C, Nem: %${weatherFacts.nemYuzde}, ` +
        `Yağış ihtimali: %${weatherFacts.yagisIhtimaliYuzde}, Yağış miktarı: ${weatherFacts.yagisMm}mm, Rüzgar: ${weatherFacts.ruzgarKmh}km/s`,
      output: { schema: IrrigationSchema },
      system: `Sen bir akıllı bahçe/çim sulama sisteminin karar destek motorusun.
               Sana verilen GERÇEK hava verisine dayanarak, planlanan (baz) sulama süresini artırıp azaltmaya karar verirsin.
               Girdideki süre bir ORTALAMA/BAZ değerdir, sabit bir tavan değildir; mevsimsel kuraklık veya yüksek sıcaklıkta artırabilirsin.
               KURALLAR:
               1. Yağış ihtimali yüksekse (%60 üzeri) veya son/yakın saatlerde belirgin yağış (1mm üzeri) varsa süreyi düşür veya 0 yap.
               2. Nem oranı çok yüksekse (%85 üzeri) süreyi azalt.
               3. Rüzgar çok yüksekse (30km/s üzeri) sulama verimsiz ve savurma riskli olur, süreyi azalt.
               4. Hava çok sıcak (30°C üzeri) ve kuruysa (nem %40 altı, yağış ihtimali düşük), baz süreyi makul ölçüde artırabilirsin.
               5. Normal/ortalama koşullarda baz süreyi olduğu gibi koru.
               6. reason alanına kısa, anlaşılır bir Türkçe gerekçe yaz.`
    });

    // 4. ADIM: GÜVENLİK KELEPÇESİ — kod seviyesinde sabit kurallar, prompt'a güvenilmiyor.
    // AI baz süreyi en fazla 2 katına çıkarabilir, AMA sistem genelinde mutlak bir tavanı
    // (AI_MAX_ABSOLUTE_MINUTES) hiçbir koşulda aşamaz. Bu ikinci sınır, cihazdan anormal
    // düşük/yüksek bir 'duration' gelse bile fiziksel taşkın riskine karşı sabit bir korumadır.
    const AI_MAX_ABSOLUTE_MINUTES = 60;
    const aiDuration = Number(aiResponse.output?.duration);
    const safeDuration = Number.isFinite(aiDuration)
      ? Math.min(Math.max(0, aiDuration), requestedDuration * 2, AI_MAX_ABSOLUTE_MINUTES)
      : requestedDuration; // AI/parse hatasında güvenli varsayılan: orijinal planlanan süreyi koru

    return res.status(200).send({
      data: {
        status: "success",
        duration: safeDuration,
        reason: aiResponse.output?.reason || "Değerlendirme tamamlandı.",
        location: resolvedName,
        weather: weatherFacts
      }
    });

  } catch (error) {
    console.error("Sulama Kararı Hatası:", error);
    // HATA DURUMUNDA GÜVENLİ VARSAYILAN: hava verisi alınamazsa orijinal planı uygula,
    // sistemi tamamen durdurmak (çimin susuz kalması) yerine normal sulamaya izin ver.
    const fallbackDuration = Number(req.body?.data?.duration);
    return res.status(200).send({
      data: {
        status: "fallback",
        duration: Number.isFinite(fallbackDuration) ? fallbackDuration : 0,
        message: "Hava verisi alınamadı, planlanan süre uygulanıyor: " + error.message
      }
    });
  }
});