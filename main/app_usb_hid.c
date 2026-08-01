#include "app_usb_hid.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"


#include "app_event_source.h"
#include "app_event_loop.h"


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


/* Application event handler for usb hid */
void app_usb_hid_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    switch (event_id) {
    case APP_HID_EVENT_GENERIC_KEYSTROKE:
        ESP_LOGI(TAG, "APP_HID_EVENT_GENERIC_KEYSTROKE");
        break;
    case APP_HID_EVENT_PRESS_ENTER:
        ESP_LOGI(TAG, "APP_HID_EVENT_PRESS_ENTER");
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