
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "esp_log.h"
#include "sound_command.h" // Yukarıdaki struct tanımını buraya dahil ettiğini varsayıyoruz



// String'den (deviceType) Target Enum'a Dönüştürücü yardımcı fonksiyon
static smartq_target_t resolve_device_type(const char *str) {
    if (!str) return TARGET_UNKNOWN;
    if (strcmp(str, "blind") == 0)    return TARGET_BLIND;
    if (strcmp(str, "door") == 0)     return TARGET_DOOR;
    if (strcmp(str, "elevator") == 0) return TARGET_ELEVATOR;
    if (strcmp(str, "energy") == 0)   return TARGET_ENERGY;
    if (strcmp(str, "garage") == 0)   return TARGET_GARAGE;
    if (strcmp(str, "gas") == 0)      return TARGET_GAS;
    if (strcmp(str, "water") == 0)    return TARGET_WATER;
    if (strcmp(str, "socket") == 0)   return TARGET_SOCKET;
    if (strcmp(str, "lamp") == 0)     return TARGET_LAMP;
    if (strcmp(str, "climate") == 0)  return TARGET_CLIMATE;
    return TARGET_UNKNOWN;
}

// String'den (targetType) yapısal kategori enum'ına dönüştürücü.
// NOT: bu, eskiden yanlışlıkla resolve_target_type() ile (deviceType tablosuyla)
// çözülmeye çalışılıyordu — "device"/"scene"/"group" hiçbir zaman eşleşmediği
// için sonuç hep TARGET_UNKNOWN oluyordu. Şimdi ayrı, doğru bir çözümleyicisi var.
static smartq_structtype_t resolve_struct_type(const char *str) {
    if (!str) return STRUCTTYPE_UNKNOWN;
    if (strcmp(str, "device") == 0)   return STRUCTTYPE_DEVICE;
    if (strcmp(str, "scene") == 0)    return STRUCTTYPE_SCENE;
    if (strcmp(str, "group") == 0)    return STRUCTTYPE_GROUP;
    if (strcmp(str, "location") == 0) return STRUCTTYPE_LOCATION;
    if (strcmp(str, "weather") == 0)  return STRUCTTYPE_WEATHER;
    return STRUCTTYPE_UNKNOWN;
}

// String'den Action Enum'a Dönüştürücü yardımcı fonksiyon
static smartq_action_t resolve_action(const char *str) {
    if (!str) return ACTION_UNKNOWN;
    if (strcmp(str, "ON") == 0)           return ACTION_ON;
    if (strcmp(str, "OFF") == 0)          return ACTION_OFF;
    if (strcmp(str, "SET_VALUE") == 0)    return ACTION_SET_VALUE;
    if (strcmp(str, "STATUS_CHECK") == 0) return ACTION_STATUS_CHECK;
    return ACTION_UNKNOWN;
}

// ANA PARSE FONKSİYONU
bool parse_smartq_response(const char *json_string, smartq_command_t *cmd) {
    if (cmd == NULL || json_string == NULL) return false;
    
    // Struct içeriğini sıfırla (Garbage verileri temizle)
    memset(cmd, 0, sizeof(smartq_command_t));

    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        ESP_LOGE("JSON PARSE", "JSON Ayrıştırma Hatası!");
        cmd->is_success = false;
        snprintf(cmd->error_message, sizeof(cmd->error_message), "Geçersiz JSON formatı");
        return false;
    }

    cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (!cJSON_IsObject(data)) {
        cJSON_Delete(root);
        return false;
    }

    cJSON *status = cJSON_GetObjectItemCaseSensitive(data, "status");
    if (!cJSON_IsString(status) || (status->valuestring == NULL)) {
        cJSON_Delete(root);
        return false;
    }

    // --- HATA DURUMU ---
    if (strcmp(status->valuestring, "error") == 0) {
        cmd->is_success = false;
        cJSON *message = cJSON_GetObjectItemCaseSensitive(data, "message");
        if (cJSON_IsString(message) && (message->valuestring != NULL)) {
            snprintf(cmd->error_message, sizeof(cmd->error_message), "%s", message->valuestring);
        }
        cJSON_Delete(root);
        return true; // JSON başarıyla işlendi (içeriği hata olsa bile)
    }

    // --- BAŞARILI DURUM ---
    if (strcmp(status->valuestring, "success") == 0) {
        cmd->is_success = true;
        
        cJSON *command = cJSON_GetObjectItemCaseSensitive(data, "command");
        if (!cJSON_IsObject(command)) {
            cJSON_Delete(root);
            return false;
        }

        // 1. Zorunlu Alanlar
        cJSON *targetType = cJSON_GetObjectItemCaseSensitive(command, "targetType");
        if (cJSON_IsString(targetType)) {
            cmd->struct_type = resolve_struct_type(targetType->valuestring);
        }

        cJSON *deviceType = cJSON_GetObjectItemCaseSensitive(command, "deviceType");
        if (cJSON_IsString(deviceType)) {
            cmd->target_type = resolve_device_type(deviceType->valuestring);
        }

        cJSON *action = cJSON_GetObjectItemCaseSensitive(command, "action");
        if (cJSON_IsString(action)) {
            cmd->action = resolve_action(action->valuestring);
        }

        cJSON *isAll = cJSON_GetObjectItemCaseSensitive(command, "isAll");
        if (cJSON_IsBool(isAll)) {
            cmd->is_all = cJSON_IsTrue(isAll);
        }

        cJSON *responseMessage = cJSON_GetObjectItemCaseSensitive(command, "responseMessage");
        if (cJSON_IsString(responseMessage) && (responseMessage->valuestring != NULL)) {
            snprintf(cmd->response_message, sizeof(cmd->response_message), "%s", responseMessage->valuestring);
        }

        // 2. Opsiyonel Alanlar
        cJSON *value = cJSON_GetObjectItemCaseSensitive(command, "value");
        if (cJSON_IsNumber(value)) {
            cmd->has_value = true;
            cmd->value = value->valuedouble;
        } else {
            cmd->has_value = false;
        }

        cJSON *name = cJSON_GetObjectItemCaseSensitive(command, "name");
        if (cJSON_IsString(name) && (name->valuestring != NULL)) {
            snprintf(cmd->name, sizeof(cmd->name), "%s", name->valuestring);
        }

        cJSON *sceneNo = cJSON_GetObjectItemCaseSensitive(command, "sceneNo");
        if (cJSON_IsNumber(sceneNo)) {
            cmd->scene_no = (uint8_t)sceneNo->valuedouble;
        }

        cJSON *groupNo = cJSON_GetObjectItemCaseSensitive(command, "groupNo");
        if (cJSON_IsNumber(groupNo)) {
            cmd->group_no = (uint8_t)groupNo->valuedouble;
        }

        cJSON *targetZone = cJSON_GetObjectItemCaseSensitive(command, "targetZone");
        if (cJSON_IsString(targetZone) && (targetZone->valuestring != NULL)) {
            snprintf(cmd->target_zone, sizeof(cmd->target_zone), "%s", targetZone->valuestring);
        }
    }

    cJSON_Delete(root);
    return true;
}



/*

// Kullanımı:
smartq_command_t my_command;

if (parse_smartq_response(json_payload_string, &my_command)) {
    if (my_command.is_success) {
        ESP_LOGI(TAG, "Asistan Diyor ki: %s", my_command.response_message);
        
        // Enum'lar sayesinde switch-case rahatlığı:
        switch(my_command.target_type) {
            case TARGET_LAMP:
                if (my_command.action == ACTION_ON) {
                    // Işıkları açma pinini aktif et
                    ESP_LOGI(TAG, "Röle tetiklendi: Lambalar AÇILIYOR.");
                }
                break;
                
            case TARGET_CLIMATE:
                if (my_command.action == ACTION_SET_VALUE && my_command.has_value) {
                    ESP_LOGI(TAG, "Klima derecesi %.1f olarak ayarlanıyor.", my_command.value);
                }
                break;
                
            default:
                break;
        }
    } else {
        ESP_LOGE(TAG, "API Hatası: %s", my_command.error_message);
    }
}
*/