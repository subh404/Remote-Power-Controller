#include "app_usb_hid.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"


#include "app_event_source.h"
#include "app_event_loop.h"
#include "app_mqtt.h"


static const char *TAG = "app_usb_hid";

extern uint8_t hid_report_descriptor[];

ESP_EVENT_DEFINE_BASE(APP_HID_EVENT);
/********* TinyUSB HID callbacks ***************/

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    ESP_LOGI(TAG, "Received GET_REPORT request: report_id=%d, report_type=%d, reqlen=%d", report_id, report_type, reqlen);

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
    ESP_LOGI(TAG, "Received SET_REPORT request: report_id=%d, report_type=%d, bufsize=%d", report_id, report_type, bufsize);
}

static void generic_keystroke_handler(char *buffer)
{
    if (buffer == NULL || strlen(buffer) == 0) {
        ESP_LOGW(TAG, "Received empty keystroke report");
        return;
    }

    /* Convert hex encoded string to uint8_t array */
    size_t bufsize = strlen(buffer);
    ESP_LOGI(TAG, "Received keystroke report: %s, length: %zu", buffer, bufsize);
    uint8_t* keyarray = (uint8_t*)malloc(bufsize / 2);
    if (keyarray == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for keyarray");
        return;
    }

    for(int i = 0; i < bufsize / 2; i++) {
        sscanf((char*)&buffer[i * 2], "%2hhx", &keyarray[i]);
    }

    int i = 0;
    int k = 0;
    uint8_t keycode[6] = {0, 0, 0, 0, 0, 0};
    while( i < bufsize / 2) {
        ESP_LOGI(TAG, "Keystroke byte %d: 0x%02X", i, keyarray[i]);
        if (keyarray[i] == 0xFF)
        {
            i++;
            continue;
        }

        if (k < 6) {
            keycode[k++] = keyarray[i];
        } else {
            ESP_LOGW(TAG, "wrong format");
            return ;
        }
        
        if(i+1 < bufsize/2 && keyarray[i+1] == 0xFF) {
            i++;
            continue;
        }

        tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keycode);
        vTaskDelay(pdMS_TO_TICKS(50));
        tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);
        vTaskDelay(pdMS_TO_TICKS(50));
        i++;
        k = 0;
        memset(keycode, 0, sizeof(keycode));
    }

    free(keyarray);
}


/* Application event handler for usb hid */
void app_usb_hid_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    switch (event_id) {
    case APP_HID_EVENT_GENERIC_KEYSTROKE:
        ESP_LOGI(TAG, "APP_HID_EVENT_GENERIC_KEYSTROKE");
        char *buffer = app_mqtt_get_keystroke_data();
        generic_keystroke_handler(buffer);
        app_mqtt_clear_keystroke_data();
        break;
    case APP_HID_EVENT_PRESS_ENTER:
        ESP_LOGI(TAG, "APP_HID_EVENT_PRESS_ENTER");
        char *enter_keycode = "28";
        generic_keystroke_handler(enter_keycode);
        break;
    case APP_HID_EVENT_PRESS_PC_UNLOCK:
        ESP_LOGI(TAG, "APP_HID_EVENT_PRESS_PC_UNLOCK");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event_id);
        break;
    }
}

void app_usb_hid_init(void)
{
    ESP_LOGI(TAG, "app usb hid initialization");
    ESP_ERROR_CHECK(app_event_loop_instance_register(APP_HID_EVENT, APP_HID_EVENT_GENERIC_KEYSTROKE, app_usb_hid_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(app_event_loop_instance_register(APP_HID_EVENT, APP_HID_EVENT_PRESS_ENTER, app_usb_hid_event_handler, NULL, NULL));
    ESP_LOGI(TAG, "app usb hid initialization DONE");
}