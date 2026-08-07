

#include <stdbool.h>
#include <stdint.h>

// index.js şemasındaki "targetType" (yapısal kategori) — device/scene/group/location/weather.
// NOT: "mode" kaldırıldı, ev modu komutları artık "scene" (isimle) olarak geliyor.
typedef enum {
    STRUCTTYPE_UNKNOWN = 0,
    STRUCTTYPE_DEVICE,
    STRUCTTYPE_SCENE,
    STRUCTTYPE_GROUP,
    STRUCTTYPE_LOCATION,
    STRUCTTYPE_WEATHER
} smartq_structtype_t;

// index.js şemasındaki "deviceType" (sadece targetType device/group ise anlamlı).
// NOT: "rgb" kaldırıldı (hiç karşılığı yoktu).
typedef enum {
    TARGET_UNKNOWN = 0,
    TARGET_BLIND, TARGET_DOOR, TARGET_ELEVATOR, TARGET_ENERGY,
    TARGET_GARAGE, TARGET_GAS, TARGET_WATER, TARGET_SOCKET,
    TARGET_LAMP, TARGET_CLIMATE
} smartq_target_t;

// Eylem Türleri Enum Yapısı
typedef enum {
    ACTION_UNKNOWN = 0,
    ACTION_ON,
    ACTION_OFF,
    ACTION_SET_VALUE,
    ACTION_STATUS_CHECK
} smartq_action_t;

// Ana Komut Struct Yapısı
typedef struct {
    bool is_success;                     // İstek başarılı mı?
    char error_message[128];             // Hata varsa mesajı

    // Komut Detayları (Sadece is_success true ise geçerli)
    smartq_structtype_t struct_type;     // targetType (device/scene/group/location/weather)
    smartq_target_t target_type;         // deviceType (blind/door/.../lamp/climate) — sadece device/group'ta anlamlı
    smartq_action_t action;
    bool is_all;
    bool has_value;                      // Value değerinin gelip gelmediğini anlamak için
    double value;

    uint8_t scene_no;                    // sceneNo (0 = belirtilmemiş)
    uint8_t group_no;                    // groupNo (0 = belirtilmemiş)

    char name[64];                       // Opsiyonel: Cihaz/senaryo/grup özel adı
    char target_zone[64];                // Opsiyonel: Oda/Bölge
    char response_message[256];          // Mobil uygulama/Asistan yanıtı
} smartq_command_t;

