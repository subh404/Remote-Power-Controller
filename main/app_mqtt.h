#ifndef _APP_MQTT_H_
#define _APP_MQTT_H_

#include <stdbool.h>
struct mqtt_event_loop_pub_data_t {
    char topic[256];
    char data[256];
    int qos;
    bool retain;
};
char* app_mqtt_get_keystroke_data(void);
void app_mqtt_clear_keystroke_data(void);
void app_mqtt_init(void);
#endif /* _APP_MQTT_H_ */