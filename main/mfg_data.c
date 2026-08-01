#include "mfg_data.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"


#define TAG "MFG_DATA"


static char default_device_id[] = "dev_000000000000";

static char* device_id = NULL;
static char* ca_cert = NULL;
static char* client_cert = NULL;
static char* client_key = NULL;
static char* server_uri = NULL;

char *mfg_data_get_device_id(void)
{
    if(device_id == NULL) {
        return default_device_id;
    }
    return device_id;
}
char *mfg_data_get_ca_cert(void)
{
    if(ca_cert == NULL) {
        return NULL;
    }
    return ca_cert;
}
char *mfg_data_get_client_cert(void)
{
    if(client_cert == NULL) {
        return NULL;
    }
    return client_cert;
}
char *mfg_data_get_client_key(void)
{
    if(client_key == NULL) {
        return NULL;
    }
    return client_key;
}
char *mfg_data_get_server_uri(void)
{
    if(server_uri == NULL) {
        return NULL;
    }
    return server_uri;
}


void mfg_data_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init_partition("mfg_data"));
    
    nvs_handle_t mfg_handle;
    if (nvs_open_from_partition("mfg_data", "mfg_ns", NVS_READONLY, &mfg_handle) == ESP_OK) {
        
        /* Retrive device id */
        size_t required_size = 0;
        nvs_get_str(mfg_handle, "device_id", NULL, &required_size);
        device_id = malloc(required_size);
        if(device_id == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for device_id");
            goto failed;          
        }
        nvs_get_str(mfg_handle, "device_id", device_id, &required_size);
        printf("Device ID: %s\n", device_id);

        /* Retrive CA certificate */
        required_size = 0;
        nvs_get_str(mfg_handle, "ca_cert", NULL, &required_size);
        ca_cert = malloc(required_size);
        if(ca_cert == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for ca_cert");
            goto failed;
        }
        nvs_get_str(mfg_handle, "ca_cert", ca_cert, &required_size);
        
        /* Retrieve client certificate */
        required_size = 0;
        nvs_get_str(mfg_handle, "client_cert", NULL, &required_size);
        client_cert = malloc(required_size);
        if(client_cert == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for client_cert");
            goto failed;
        }
        nvs_get_str(mfg_handle, "client_cert", client_cert, &required_size);

        /* Retrive client key */
        required_size = 0;
        nvs_get_str(mfg_handle, "private_key", NULL, &required_size);
        client_key = malloc(required_size);
        if(client_key == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for private_key");
            goto failed;
        }
        nvs_get_str(mfg_handle, "private_key", client_key, &required_size);

        /* retrive server uri */
        required_size = 0;
        nvs_get_str(mfg_handle, "server", NULL, &required_size);
        server_uri = malloc(required_size);
        if(server_uri == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for server_uri");
            goto failed;
        }
        nvs_get_str(mfg_handle, "server", server_uri, &required_size);


        failed:
        nvs_close(mfg_handle);
    }
}