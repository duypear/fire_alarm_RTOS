#ifndef MQ135_H
#define MQ135_H

#include "esp_err.h"

// Khởi tạo kênh ADC Oneshot cho MQ-135
esp_err_t mq135_init(void);

// Đọc giá trị ADC thô (0 - 4095)
int mq135_read_raw(void);

#endif