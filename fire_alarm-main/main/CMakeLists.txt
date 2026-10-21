idf_component_register(SRCS "main.c" "oled_ssd1306.c" "dht22.c" "mq135.c" "flame.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES driver esp_driver_gpio esp_adc esp_timer spi_flash)