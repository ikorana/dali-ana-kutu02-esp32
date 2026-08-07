
#include "esp_log_level.h"
#include "mqtt_client.h"
#include <stdio.h>



static esp_mqtt5_user_property_item_t user_property_arr[] = {
    {"board", "SMQ"},
    {"u", ""},
    {"p", ""}
};

#define USE_PROPERTY_ARR_SIZE   sizeof(user_property_arr)/sizeof(esp_mqtt5_user_property_item_t)
static bool subsribed = false;

static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    //ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t )event_data;
    esp_mqtt_client_handle_t client = event->client;
   // int msg_id;
    //ESP_LOGI(TAG, "free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(),
    //         esp_get_minimum_free_heap_size());

    char *tp;
    char *dt;        

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        printf("Will Topic             : %s\n",willTopic);
        printf("In Topic (Dinlenen)    : %s\n",inTopic);
        printf("Out Topic (Gönderilen) : %s\n",outTopic);
        esp_mqtt_client_publish(client, (char *)willTopic, "ONLINE", 6, 1, 1);
        esp_mqtt_client_subscribe(client, (char *)inTopic, 0);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        subsribed = false;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        subsribed = true;
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, reason code=0x%02x ", event->msg_id, (uint8_t)*event->data);
        /* print_user_property(event->property->user_property);
        esp_mqtt5_client_set_publish_property(client, &publish_property);
        msg_id = esp_mqtt_client_publish(client, "topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id); */
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        subsribed = false;
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
/*         print_user_property(event->property->user_property);
        esp_mqtt5_client_set_user_property(&disconnect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
        esp_mqtt5_client_set_disconnect_property(client, &disconnect_property);
        esp_mqtt5_client_delete_user_property(disconnect_property.user_property);
        disconnect_property.user_property = NULL; */
        esp_mqtt_client_disconnect(client);
        break;

    case MQTT_EVENT_PUBLISHED:
        //ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        //print_user_property(event->property->user_property);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
        
        asprintf(&tp,"%.*s", event->topic_len, event->topic);
        asprintf(&dt,"%.*s", event->data_len, event->data);

        if (strcmp(tp,(char*)inTopic)==0)
          {
             udp_msg_t *msg_copy = (udp_msg_t*)malloc(sizeof(udp_msg_t));
             if (msg_copy != NULL) {
                msg_copy->remote = (remote_t*)malloc(sizeof(remote_t));
                msg_copy->payload = malloc(event->data_len + 1);
                msg_copy->len = event->data_len;
                msg_copy->is_broadcast = false;
                msg_copy->is_mqtt = true;
                memcpy(msg_copy->payload, dt, event->data_len + 1);
                if (xQueueSend(udp_processing_queue, &msg_copy, 0) != pdPASS) {
                        ESP_LOGE("MAIN", "Kuyruk dolu, paket düşürüldü!");
                        free(msg_copy->remote);
                        free(msg_copy->payload);
                        free(msg_copy);
                } ;
             }
            }
        free(tp);
        free(dt);

        
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        //print_user_property(event->property->user_property);
        ESP_LOGI(TAG, "MQTT5 return code is %d", event->error_handle->connect_return_code);

        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            /* log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno); */
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }

        break;

    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_send(const char *data) {
    
    int qos = 1;               // Verinin ulaşacağından emin olmak istiyoruz
    int retain = 0;            // Bu mesaj kalıcı (retained) olmasın
    if (!subsribed) return ;
    // Fonksiyonu çağırma
    // 'len' parametresini 0 vererek uzunluğun otomatik hesaplanmasını sağlıyoruz
    int msg_id = esp_mqtt_client_publish(mqtt_client, (char *)outTopic, data, 0, qos, retain);
    // Sonucu kontrol etme
    if (msg_id == -1) {
        ESP_LOGE("MQTT", "Mesaj gönderilemedi (Bağlantı koptu veya hata oluştu)");
    } else if (msg_id == -2) {
        ESP_LOGW("MQTT", "Outbox dolu! Mesaj kuyruğa alınamadı.");
    } else {
        ESP_LOGI("MQTT", "MQTT Send [%s]-> %s", (char *)outTopic,data);
    }   
}

void mqtt_start(void)
{
    char *mm;
    asprintf(&mm,"%s/will",(char*)GlobalConfig.license);
    strcpy((char*)willTopic,mm);
    free(mm);

    //Bilgileri alacagım Topic
    asprintf(&mm,"%s/in",(char*)GlobalConfig.license);
    strcpy((char*)inTopic,mm);
    free(mm);

    //Bilgileri gönderecegim Topic
    asprintf(&mm,"%s/out",(char*)GlobalConfig.license);
    strcpy((char*)outTopic,mm);
    free(mm);

    char *broker;
    asprintf(&broker,"mqtt://%s",GlobalConfig.mqtt);

    subsribed = false;

    esp_mqtt5_connection_property_config_t connect_cfg = {};

        connect_cfg.session_expiry_interval = 10;
        connect_cfg.maximum_packet_size = 1024;
        connect_cfg.receive_maximum = 65535;
        connect_cfg.topic_alias_maximum = 2;
        connect_cfg.request_resp_info = true;
        connect_cfg.request_problem_info = true;
        connect_cfg.will_delay_interval = 10;
        connect_cfg.payload_format_indicator = true;
        connect_cfg.message_expiry_interval = 10;
        connect_cfg.response_topic = "test/response";
        connect_cfg.correlation_data = "123456";
        connect_cfg.correlation_data_len = 6;
  
    esp_mqtt_client_config_t cln_cfg = {};
        cln_cfg.broker.address.uri = broker;
        cln_cfg.session.protocol_ver = MQTT_PROTOCOL_V_5;
        cln_cfg.network.disable_auto_reconnect = false;
  
        //.credentials.username = "123",
        //.credentials.authentication.password = "456",
        cln_cfg.session.last_will.topic = (char*)willTopic,
        cln_cfg.session.last_will.msg = "OFFLINE";
        cln_cfg.session.last_will.msg_len = 7;
        cln_cfg.session.last_will.qos = 1;
        cln_cfg.session.last_will.retain = true;
        cln_cfg.session.keepalive = 30;
    
        mqtt_client = esp_mqtt_client_init(&cln_cfg);
        esp_mqtt5_client_set_user_property(&connect_cfg.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
        esp_mqtt5_client_set_user_property(&connect_cfg.will_user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
        esp_mqtt5_client_set_connect_property(mqtt_client, &connect_cfg);

        esp_mqtt5_client_delete_user_property(connect_cfg.user_property);
        esp_mqtt5_client_delete_user_property(connect_cfg.will_user_property);

        esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt5_event_handler, NULL);
        esp_mqtt_client_start(mqtt_client);
}