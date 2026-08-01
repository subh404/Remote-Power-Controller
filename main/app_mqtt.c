#include "app_mqtt.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_netif.h"

#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "class/hid/hid_device.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_system.h"

#include "app_event_source.h"
#include "app_event_loop.h"
#include "mfg_data.h"
#include "app_pwr_sw.h"

#define MQTT_BASE_TOPIC "/topic/device/"
/* Event source task related definitions */
ESP_EVENT_DEFINE_BASE(APP_MQTT_EVENT);

static const char *TAG = "mqtt_client";

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void subscribe_to_topics(esp_mqtt_client_handle_t client)
{
    char *device_id = mfg_data_get_device_id();
    if(device_id == NULL) {
        ESP_LOGE(TAG, "Device ID is not configured. Please configure the device ID in the manufacturing data.");
        return;
    }
    char dev_topic[256];
    
    /* Power sw toggle topic */
    memset(dev_topic, 0, sizeof(dev_topic));
    snprintf(dev_topic, sizeof(dev_topic), MQTT_BASE_TOPIC "%s/pwr_sw_toggle", device_id);
    
    int msg_id = esp_mqtt_client_subscribe(client, dev_topic, 0);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

    /* USB-HID key stroke topi*/
    memset(dev_topic, 0, sizeof(dev_topic));
    snprintf(dev_topic, sizeof(dev_topic), MQTT_BASE_TOPIC "%s/usb_hid_keystroke", device_id);
    msg_id = esp_mqtt_client_subscribe(client, dev_topic, 0);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

    /* PC unlock  topic */
    memset(dev_topic, 0, sizeof(dev_topic));
    snprintf(dev_topic, sizeof(dev_topic), MQTT_BASE_TOPIC "%s/pc_unlock", device_id);
    msg_id = esp_mqtt_client_subscribe(client, dev_topic, 0);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

}

static void power_switch_toggle_handler(bool is_true)
{
    if(is_true) {
      ESP_LOGI(TAG, "Power switch toggle event received. Posting event to event loop.");  
      app_event_loop_post(APP_POWER_SWITCH_OUT_EVENT, APP_POWER_SWITCH_OUT_EVENT_TOGGLE, NULL, 0, portMAX_DELAY);
    }
}

static void app_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        subscribe_to_topics(client);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, return code=0x%02x ", event->msg_id, (uint8_t)*event->data);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        char* device_id = mfg_data_get_device_id();
        if(device_id == NULL) {
            ESP_LOGE(TAG, "Device ID is not configured. Please configure the device ID in the manufacturing data.");
            return;
        }

        /* Check if the topic is one of the subscribed topics */
        char dev_topic[256];

        /* Power sw toggle topic */
        memset(dev_topic, 0, sizeof(dev_topic));
        snprintf(dev_topic, sizeof(dev_topic), MQTT_BASE_TOPIC "%s/pwr_sw_toggle", device_id);
        if (strncmp(event->topic, dev_topic, event->topic_len) == 0) {
            ESP_LOGI(TAG, "Received message for topic: %.*s", event->topic_len, event->topic);
            if (event->data_len > 0 && strncmp(event->data, "true", event->data_len) == 0) {
                power_switch_toggle_handler(true);
            }
            return;
        }

        memset(dev_topic, 0, sizeof(dev_topic));
        snprintf(dev_topic, sizeof(dev_topic), MQTT_BASE_TOPIC "%s/pc_unlock", device_id);

        if (strncmp(event->topic, dev_topic, event->topic_len) == 0) {
            ESP_LOGI(TAG, "Received message for topic: %.*s", event->topic_len, event->topic);
            return;
        }

        memset(dev_topic, 0, sizeof(dev_topic));
        snprintf(dev_topic, sizeof(dev_topic), MQTT_BASE_TOPIC "%s/usb_hid_keystroke", device_id);
        if (strncmp(event->topic, dev_topic, event->topic_len) == 0 && event->data_len > 0) {
            ESP_LOGI(TAG, "Received message for topic: %.*s", event->topic_len, event->topic);
            return;
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

/* Event hanlder for APP MQTT events */
static void app_mqtt_event_loop_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    switch (event_id) {
    case APP_MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "APP_MQTT_EVENT");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event_id);
        break;
    }
}

static void mqtt_connection_init(void)  
{
    char * ca_cert = mfg_data_get_ca_cert();
    char * client_cert = mfg_data_get_client_cert();
    char * client_key = mfg_data_get_client_key();
    char * server_uri = mfg_data_get_server_uri();
    
    if(server_uri == NULL) {
        
        ESP_LOGE(TAG, "Server is not configured. Please configure the server URI in the manufacturing data.");
        return;
    }

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = server_uri,
        .broker.verification.certificate = (const char *)ca_cert,
        .credentials = {
            .authentication.certificate = (const char *)client_cert,
            .authentication.key = (const char *)client_key,
        },
    };

    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, app_mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    ESP_ERROR_CHECK(app_event_loop_instance_register(APP_MQTT_EVENT, APP_MQTT_EVENT_PUBLISHED, app_mqtt_event_loop_handler, NULL, NULL));
    
}

static void mqtt_netif_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    switch (event_id) {
    case IP_EVENT_STA_GOT_IP:
        mqtt_connection_init();
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event_id);
        break;
    }
}

void app_mqtt_init(void)
{


     ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        mqtt_netif_event_handler,
                                                        NULL,
                                                        NULL));
}