#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include "config.h"
#include "esp_err.h"

// Khởi tạo I2C và màn hình SSD1306
esp_err_t oled_init_driver(void);

// Vẽ giao diện trạng thái hệ thống
void oled_render_dashboard(const DisplayMailbox_t *data);

#endif // OLED_SSD1306_H