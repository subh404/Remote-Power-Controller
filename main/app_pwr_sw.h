#ifndef _APP_PWR_SW_H_
#define _APP_PWR_SW_H_
#include <stdint.h>

struct pwr_sw_event_t {
    uint8_t gpio_num;
    uint8_t req_level;
    uint32_t toggle_time;
};

void app_pwr_sw_init(void);
#endif /* _APP_PWR_SW_H_ */