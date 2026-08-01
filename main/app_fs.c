#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "sdkconfig.h"
#include "app_fs.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"


#define MAX_FILES   5

static const char *TAG = "app_fs";


// Mount path for the partition
const char *base_path = "/data";

// Handle of the wear levelling library instance
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

const char *wifi_creds_file_path = "/data/wifi_creds.json";




void app_fs_init(void)
{
   ESP_LOGI(TAG, "Mounting FAT filesystem");
    // To mount device we need name of device partition, define base_path
    // and allow format partition in case if it is new one and was not formatted before
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = MAX_FILES, // Number of files that can be open at a time
            .format_if_mount_failed = true, // If true, try to format the partition if mount fails
            .allocation_unit_size = CONFIG_WL_SECTOR_SIZE, // Size of allocation unit, cluster size.
            .use_one_fat = false, // Use only one FAT table (reduce memory usage), but decrease reliability of file system in case of power failure.
    };

    // Mount FATFS filesystem located on "storage" partition in read-write mode
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(base_path, "storage", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Filesystem mounted");

    // Try to read wifi credentials from the file
    FILE *file = fopen(wifi_creds_file_path, "r");
    if (file == NULL) {
        ESP_LOGW(TAG, "WiFi credentials file not found");
        return;
    }
    else
    {
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if(file_size <= 0) {
            ESP_LOGE(TAG, "WiFi credentials file is empty");
            fclose(file);
            return;
        }
        if(file_size > 1024) {
            ESP_LOGE(TAG, "WiFi credentials file is too large");
            fclose(file);
            return;
        }

        char *line = malloc(file_size + 1);
        if (line == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory");
            fclose(file);
            return;
        }

        fread(line, 1, file_size, file);
        line[file_size] = '\0';
        fclose(file);
        // Read the JSON to get the credentials and save it to NVS
        cJSON *json = cJSON_Parse(line);
        if (json == NULL) {
            ESP_LOGE(TAG, "Failed to parse JSON");
            goto clean;
        }

        cJSON *ssid = cJSON_GetObjectItemCaseSensitive(json, "ssid");
        if (cJSON_IsString(ssid) && (ssid->valuestring != NULL)) {
            ESP_LOGI(TAG, "Configured SSID: %s", ssid->valuestring);
        }
        else {
            ESP_LOGE(TAG, "SSID not found in JSON");
            goto clean;
        }

        cJSON *password = cJSON_GetObjectItemCaseSensitive(json, "password");
        if (cJSON_IsString(password) && (password->valuestring != NULL)) {
            ESP_LOGI(TAG, "Configured Password: %s", password->valuestring);
        }
        else {
            ESP_LOGE(TAG, "Password not found in JSON");
            goto clean;
        }
        // Open NVS handle
        ESP_LOGI(TAG, "\nOpening Non-Volatile Storage (NVS) handle...");
        nvs_handle_t my_handle;
        err = nvs_open("storage", NVS_READWRITE, &my_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
            goto clean;
        }

        err = nvs_set_str(my_handle, "ssid", ssid->valuestring);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write SSID!");
            goto clean;
        }

        err = nvs_set_str(my_handle, "password", password->valuestring);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write password!");
            goto clean;
        }

        ESP_LOGW(TAG, "WiFi credentials file found");
        // At last close the file and delete it to avoid leaving sensitive information on the filesystem
        nvs_close(my_handle);

        clean:
        cJSON_Delete(json);
        free(line);
        if (remove(wifi_creds_file_path) != 0) {
            ESP_LOGE(TAG, "Failed to delete WiFi credentials file");
        }
        else {
            ESP_LOGI(TAG, "WiFi credentials file deleted");
        }
    }

    // Unmount FATFS
    ESP_LOGI(TAG, "Unmounting FAT filesystem");

    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_unmount_rw_wl(base_path, s_wl_handle));

}

