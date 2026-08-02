#include "esp_log.h"
#include "nvs_flash.h"
#include "provision_data.h"

static const char *TAG = "provision_data";
static char *ssid = NULL;
static char *password = NULL;
static char *hostname = NULL;


char *provision_data_get_ssid(void)
{
    return ssid;
}

char *provision_data_get_password(void)
{
    return password;
}

char *provision_data_get_hostname(void)
{
    return hostname;
}


void provision_data_init(void)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle!");
        return;
    }

    /* Get ssid from NVS */
    size_t required_len = 0;
    nvs_get_str(my_handle, "ssid", NULL, &required_len);
    ssid = malloc(required_len);
    if (ssid == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for SSID");
        nvs_close(my_handle);
        return;
    }
    nvs_get_str(my_handle, "ssid", ssid, &required_len);

    /* Get password from NVS */
    nvs_get_str(my_handle, "password", NULL, &required_len);
    password = malloc(required_len);
    if (password == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for password");
        nvs_close(my_handle);
        return;
    }
    nvs_get_str(my_handle, "password", password, &required_len);

    /* Get hostname from NVS */
    nvs_get_str(my_handle, "hostname", NULL, &required_len);
    hostname = malloc(required_len);
    if (hostname == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for hostname");
        nvs_close(my_handle);
        return;
    }
    nvs_get_str(my_handle, "hostname", hostname, &required_len);

    nvs_close(my_handle);
}
