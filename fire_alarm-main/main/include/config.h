#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_adc/adc_oneshot.h"

// Cấu hình chân GPIO
#define PIN_MQ135_ADC_CHAN  ADC_CHANNEL_0  // GPIO 0
#define PIN_DHT22           1              // GPIO 1
#define PIN_FLAME_DO        2              // GPIO 2
#define PIN_BUZZER          3              // GPIO 3
#define PIN_OLED_SDA        4              // GPIO 4
#define PIN_OLED_SCL        5              // GPIO 5
#define PIN_LED             6              // GPIO 6
#define PIN_BUTTON          7              // GPIO 7

// Cấu hình I2C & OLED
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_FREQ_HZ  100000
#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_ADDR           0x3C

// Ngưỡng cảnh báo
#define GAS_THRESHOLD_ALERT  3000  
#define TEMP_THRESHOLD_ALERT 45    

// Cấp độ báo động
typedef enum {
    LEVEL_SAFE = 0,         
    LEVEL_GAS_ALERT,        
    LEVEL_FIRE_ALERT,       
    LEVEL_MANUAL_EMERGENCY  
} AlarmLevel_t;

// Loại sự kiện
typedef enum {
    EVT_SENSOR_DATA = 0,
    EVT_BTN_TRIGGER_EMERGENCY
} EventType_t;

// Gói tin sự kiện từ cảm biến / nút nhấn
typedef struct {
    EventType_t type;
    int temperature;
    int humidity;
    int gas_raw;
    bool flame_detected;
} SystemEvent_t;

// Gói tin Mailbox cập nhật OLED
typedef struct {
    AlarmLevel_t level;
    int temperature;
    int gas_raw;
    bool flame;
} DisplayMailbox_t;

#endif // CONFIG_H