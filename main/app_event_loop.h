#ifndef _APP_EVENT_LOOP_H_
#define _APP_EVENT_LOOP_H_
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "esp_event_base.h"


int app_event_loop_instance_register(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void *event_handler_arg, esp_event_handler_instance_t* handler_ctx_arg);

int app_event_loop_post(esp_event_base_t event_base, int32_t event_id, const void *event_data, size_t event_data_size, TickType_t ticks_to_wait);

void app_event_loop_init(void);

#endif /* _APP_EVENT_LOOP_H_ */