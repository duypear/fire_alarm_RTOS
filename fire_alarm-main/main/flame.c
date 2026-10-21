#include "flame.h"
#include "config.h"
#include "driver/gpio.h"

esp_err_t flame_sensor_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_FLAME_DO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    return gpio_config(&io_conf);
}

bool flame_sensor_is_detected(void) {
    return (gpio_get_level(PIN_FLAME_DO) == 0);
}