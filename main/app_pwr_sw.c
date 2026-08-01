#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "app_pwr_sw.h"
#include "app_event_source.h"
#include "app_event_loop.h"

static const char *TAG = "app_pwr_sw";

ESP_EVENT_DECLARE_BASE(APP_POWER_SWITCH_OUT_EVENT);    

static QueueHandle_t pwr_sw_evt_queue = NULL;

#define CONFIG_GPIO_OUTPUT_IO_7    7
#define CONFIG_GPIO_INPUT_IO_13    13


static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    struct pwr_sw_event_t evt;
    evt.gpio_num = CONFIG_GPIO_OUTPUT_IO_7;
    /* Event is being created for map value of gp13 to gp7 */
    evt.req_level = gpio_get_level((uint32_t) arg);
    evt.toggle_time = 0; // Set the appropriate toggle time if needed
    xQueueSendFromISR(pwr_sw_evt_queue, &evt, NULL);

    /* TODO: Write a debounce logic */
    // start a timer with a delay of 50ms , and the debounce state 
    // If the state changes within the 50ms , then reset the timer and wait for another 
    // If the pin was stable for 50ms , then send the event to the queue
}

static void pwr_sw_task(void* arg)
{
    struct pwr_sw_event_t evt;
    for (;;) {
        if (xQueueReceive(pwr_sw_evt_queue, &evt, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", evt.gpio_num, evt.req_level);
            if(evt.toggle_time > 0) {
                gpio_set_level(evt.gpio_num, evt.req_level);
                vTaskDelay(pdMS_TO_TICKS(evt.toggle_time));
                gpio_set_level(evt.gpio_num, !evt.req_level);
            } else {
                gpio_set_level(evt.gpio_num, evt.req_level);
            }
        }
    }
}

static void app_pwr_sw_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    struct pwr_sw_event_t evt;
    evt.gpio_num = CONFIG_GPIO_OUTPUT_IO_7;
    switch (event_id) {
    case APP_POWER_SWITCH_OUT_EVENT_HIGH:
        ESP_LOGI(TAG, "APP_POWER_SWITCH_OUT_EVENT_HIGH");
        evt.req_level = 1;
        break;
    case APP_POWER_SWITCH_OUT_EVENT_LOW:
        ESP_LOGI(TAG, "APP_POWER_SWITCH_OUT_EVENT_LOW");
        evt.req_level = 0;
        break;
    case APP_POWER_SWITCH_OUT_EVENT_TOGGLE:
        ESP_LOGI(TAG, "APP_POWER_SWITCH_OUT_EVENT_TOGGLE");
        if(event_data != NULL) {
            evt.toggle_time = *((uint32_t*)event_data);
            evt.req_level = 1;
        } else {
            ESP_LOGW(TAG, "APP_POWER_SWITCH_OUT_EVENT_TOGGLE event_data is NULL");
             evt.toggle_time = 500; // Default toggle time in milliseconds
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event_id);
        return ;
    }

    xQueueSend(pwr_sw_evt_queue, &evt, portMAX_DELAY);
}


void app_pwr_sw_init(void) {

    /* Configure the Output pin to optocoupler */
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<CONFIG_GPIO_OUTPUT_IO_7);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);


    /* Configure the Input pin from power button */
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<CONFIG_GPIO_INPUT_IO_13);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr
    pwr_sw_evt_queue = xQueueCreate(10, sizeof(struct pwr_sw_event_t));
    //start gpio task
    xTaskCreate(pwr_sw_task, "pwr_sw_task", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(CONFIG_GPIO_INPUT_IO_13, gpio_isr_handler, (void*) CONFIG_GPIO_INPUT_IO_13);

    ESP_ERROR_CHECK(app_event_loop_instance_register(APP_POWER_SWITCH_OUT_EVENT, APP_POWER_SWITCH_OUT_EVENT_HIGH, app_pwr_sw_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(app_event_loop_instance_register(APP_POWER_SWITCH_OUT_EVENT, APP_POWER_SWITCH_OUT_EVENT_LOW, app_pwr_sw_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(app_event_loop_instance_register(APP_POWER_SWITCH_OUT_EVENT, APP_POWER_SWITCH_OUT_EVENT_TOGGLE, app_pwr_sw_event_handler, NULL, NULL));
}