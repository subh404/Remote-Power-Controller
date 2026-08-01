#include "app_usb_msc.h"

#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_partition.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tinyusb_msc.h"

static const char *TAG = "app_usb_msc";

/* Storage global variables */
tinyusb_msc_storage_handle_t storage_hdl = NULL;
tinyusb_msc_mount_point_t mp;


#define BASE_PATH "/data" // base path to mount the partition


static esp_err_t storage_init_spiflash(wl_handle_t *wl_handle)
{
    ESP_LOGI(TAG, "Initializing wear levelling");

    const esp_partition_t *data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);
    if (data_partition == NULL) {
        ESP_LOGE(TAG, "Failed to find FATFS partition. Check the partition table.");
        return ESP_ERR_NOT_FOUND;
    }

    return wl_mount(data_partition, wl_handle);
}

// Set mount point to the application and list files in BASE_PATH by filesystem API
static void app_usb_msc_mount(void)
{
    ESP_LOGI(TAG, "Mount storage...");
    ESP_ERROR_CHECK(tinyusb_msc_set_storage_mount_point(storage_hdl, TINYUSB_MSC_STORAGE_MOUNT_APP));

    // List all the files in this directory
    ESP_LOGI(TAG, "\nls command output:");
    struct dirent *d;
    DIR *dh = opendir(BASE_PATH);
    if (!dh) {
        if (errno == ENOENT) {
            // If the directory is not found
            ESP_LOGE(TAG, "Directory doesn't exist %s", BASE_PATH);
        } else {
            // If the directory is not readable then throw error and exit
            ESP_LOGE(TAG, "Unable to read directory %s", BASE_PATH);
        }
        return;
    }
    // While the next entry is not readable we will print directory files
    while ((d = readdir(dh)) != NULL) {
        printf("%s\n", d->d_name);
    }
    return;
}

static void storage_mount_changed_cb(tinyusb_msc_storage_handle_t handle, tinyusb_msc_event_t *event, void *arg)
{
    switch (event->id) {
    case TINYUSB_MSC_EVENT_MOUNT_START:
        // Verify that all the files are closed before unmounting
        break;
    case TINYUSB_MSC_EVENT_MOUNT_COMPLETE:
        ESP_LOGI(TAG, "Storage mounted to application: %s", (event->mount_point == TINYUSB_MSC_STORAGE_MOUNT_APP) ? "Yes" : "No");
        break;
    case TINYUSB_MSC_EVENT_MOUNT_FAILED:
    case TINYUSB_MSC_EVENT_FORMAT_REQUIRED:
        ESP_LOGE(TAG, "Storage mount failed or format required");
        break;
    default:
        break;
    }
}

void app_usb_msc_init(void)
{
     tinyusb_msc_storage_config_t storage_cfg = {
        .mount_point = TINYUSB_MSC_STORAGE_MOUNT_USB,       // Initial mount point to USB
        .fat_fs = {
            .base_path = BASE_PATH,                              // Use default base path
            .config.max_files = 5,                          // Maximum number of files that can be opened simultaneously
            .format_flags = 0,                              // No special format flags
        },
    };
    ESP_LOGI(TAG, "spi flash storage initialization");
    static wl_handle_t wl_handle = WL_INVALID_HANDLE;
    ESP_ERROR_CHECK(storage_init_spiflash(&wl_handle));
    // Set the storage medium to the wear leveling handle
    storage_cfg.medium.wl_handle = wl_handle;
    ESP_LOGI(TAG, "Creating new storage for SPI flash");
    ESP_ERROR_CHECK(tinyusb_msc_new_storage_spiflash(&storage_cfg, &storage_hdl));
    ESP_LOGI(TAG, "Configuring the callback for mount changed events");
    // Configure the callback for mount changed events
    ESP_ERROR_CHECK(tinyusb_msc_set_storage_callback(storage_mount_changed_cb, NULL));
    // Re-mount to the APP
    app_usb_msc_mount();
}