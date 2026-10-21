#include "dht22.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

bool dht22_read(int *temp_out, int *humi_out) {
    uint8_t data[5] = {0};

    gpio_set_direction(PIN_DHT22, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_DHT22, 0);
    esp_rom_delay_us(1200);
    gpio_set_level(PIN_DHT22, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(PIN_DHT22, GPIO_MODE_INPUT);

    int timeout = 0;
    while (gpio_get_level(PIN_DHT22) == 1) { if (++timeout > 100) return false; esp_rom_delay_us(1); }
    timeout = 0;
    while (gpio_get_level(PIN_DHT22) == 0) { if (++timeout > 100) return false; esp_rom_delay_us(1); }
    timeout = 0;
    while (gpio_get_level(PIN_DHT22) == 1) { if (++timeout > 100) return false; esp_rom_delay_us(1); }

    for (int i = 0; i < 40; i++) {
        timeout = 0;
        while (gpio_get_level(PIN_DHT22) == 0) { if (++timeout > 100) return false; esp_rom_delay_us(1); }
        esp_rom_delay_us(35);
        if (gpio_get_level(PIN_DHT22) == 1) data[i / 8] |= (1 << (7 - (i % 8)));
        timeout = 0;
        while (gpio_get_level(PIN_DHT22) == 1) { if (++timeout > 100) break; esp_rom_delay_us(1); }
    }

    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        *humi_out = ((data[0] << 8) | data[1]) / 10;
        *temp_out = (((data[2] & 0x7F) << 8) | data[3]) / 10;
        if (data[2] & 0x80) *temp_out = -(*temp_out);
        return true;
    }
    return false;
}