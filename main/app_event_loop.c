#include "app_event_loop.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event_base.h"


static const char* TAG = "app_event_loop";

// Event loops
esp_event_loop_handle_t evt_loop_task_handle = NULL;

int app_event_loop_instance_register(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void *event_handler_arg, esp_event_handler_instance_t* handler_ctx_arg)
{
    return esp_event_handler_instance_register_with(evt_loop_task_handle, event_base, event_id, event_handler, event_handler_arg, handler_ctx_arg);
}

int app_event_loop_post(esp_event_base_t event_base, int32_t event_id, const void *event_data, size_t event_data_size, TickType_t ticks_to_wait)
{
    return esp_event_post_to(evt_loop_task_handle, event_base, event_id, event_data, event_data_size, ticks_to_wait);
}

void app_event_loop_init(void)
{
    ESP_LOGI(TAG, "Initializing application event loop");
    
    esp_event_loop_args_t evt_loop_args = {
        .queue_size = 10,
        .task_name = "app_event_loop", 
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 3072,
        .task_core_id = tskNO_AFFINITY
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&evt_loop_args, &evt_loop_task_handle));

}