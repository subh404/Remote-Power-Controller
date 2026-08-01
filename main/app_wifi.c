#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define EXAMPLE_ESP_WIFI_SSID      "wifi"
#define EXAMPLE_ESP_WIFI_PASS      "wifi1234"
#define EXAMPLE_ESP_MAXIMUM_RETRY  20

static wifi_config_t wifi_config;


static int s_retry_num = 0;


static const char *TAG = "wifi station";

static void app_wifi_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            ESP_LOGI(TAG, "connect to the AP fail after maximum retries");
            /* TODO: Add a reboot logic */
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
    }
}

void app_wifi_init(void)
{

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        app_wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        app_wifi_event_handler,
                                                        NULL,
                                                        NULL));
                                                      
    
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        goto failure;
    }
    else
    {
        size_t ssid_len = sizeof(wifi_config.sta.ssid);
        size_t password_len = sizeof(wifi_config.sta.password);
        err = nvs_get_str(my_handle, "ssid", (char *)wifi_config.sta.ssid, &ssid_len);
        wifi_config.sta.ssid[ssid_len] = '\0'; // Ensure null-termination
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) reading SSID from NVS!", esp_err_to_name(err));
            goto failure;
        }
        err = nvs_get_str(my_handle, "password", (char *)wifi_config.sta.password, &password_len);
        wifi_config.sta.password[password_len] = '\0'; // Ensure null-termination
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) reading password from NVS!", esp_err_to_name(err));
            goto failure;
        }
        goto success_retrival;
    }

    failure:
    strcpy((char *)wifi_config.sta.ssid, EXAMPLE_ESP_WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, EXAMPLE_ESP_WIFI_PASS);

    success_retrival:
    /* TODO : Remove it in production */
    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", wifi_config.sta.ssid);
    ESP_LOGI(TAG, "Using WiFi Password: %s", wifi_config.sta.password);

    nvs_close(my_handle);
    
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());                                
}