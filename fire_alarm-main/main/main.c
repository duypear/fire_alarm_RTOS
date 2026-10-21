#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "config.h"
#include "dht22.h"
#include "mq135.h"
#include "flame.h"
#include "oled_ssd1306.h"

static const char *TAG = "FIRE_SAFETY_SYSTEM";


// KHAI BÁO CÁC HÀNG ĐỢI FREERTOS 

static QueueHandle_t xSystemEventQueue = NULL; 
static QueueHandle_t xAlarmLevelQueue  = NULL; 
static QueueHandle_t xDisplayQueue     = NULL; 


static int64_t last_btn_time = 0;

static void IRAM_ATTR button_isr_handler(void *arg) {
    int64_t now = esp_timer_get_time() / 1000;
    if (now - last_btn_time > 250) { 
        last_btn_time = now;
        SystemEvent_t btn_evt = { .type = EVT_BTN_TRIGGER_EMERGENCY };
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        
        xQueueSendToFrontFromISR(xSystemEventQueue, &btn_evt, &xHigherPriorityTaskWoken);
        
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

//TASK 1 - ĐỊNH KỲ LẤY MẪU CẢM BIẾN 


static void Task_SensorSampler(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); 

    int dht_cycle = 0;
    int temp = 30, humi = 65;

    while (1) {
       
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        
        bool flame = flame_sensor_is_detected();

     
        int gas_adc = mq135_read_raw();

        if (++dht_cycle >= 20) {
            dht_cycle = 0;
            int t, h;
            if (dht22_read(&t, &h)) {
                temp = t;
                humi = h;
            }
        }


        SystemEvent_t sensor_evt = {
            .type = EVT_SENSOR_DATA,
            .temperature = temp,
            .humidity = humi,
            .gas_raw = gas_adc,
            .flame_detected = flame
        };

        xQueueSend(xSystemEventQueue, &sensor_evt, 0);
    }
}


//  TASK 2 - BỘ NÃO ĐIỀU KHIỂN FSM (ƯU TIÊN 4 - CAO NHẤT)
// CHỨC NĂNG: SENSOR FUSION & CƠ CHẾ KHÓA CẢNH BÁO (ALARM LATCHING)

static void Task_FireController(void *pvParameters) {
    SystemEvent_t evt;
    AlarmLevel_t current_state = LEVEL_SAFE;
    AlarmLevel_t previous_state = LEVEL_SAFE;
    int oled_refresh_counter = 0;
    int log_counter = 0;
    static int flame_hold = 0;

 
    bool alarm_latched = false;
    int reset_cooldown = 0; 

   
    int last_temp = 30;
    int last_gas = 0;
    bool last_flame = false;

    while (1) {
       
        if (xQueueReceive(xSystemEventQueue, &evt, portMAX_DELAY) == pdPASS) {
            bool state_changed = false;

            if (evt.type == EVT_BTN_TRIGGER_EMERGENCY) {

                if (current_state == LEVEL_SAFE) {
                    current_state = LEVEL_MANUAL_EMERGENCY;
                    alarm_latched = true;
                    ESP_LOGW(TAG, ">>> [INTERRUPT] Kich hoat BAO DONG KHAN CAP thu cong! <<<");
                } else {
                    current_state = LEVEL_SAFE;
                    alarm_latched = false;
                    flame_hold = 0;
                    reset_cooldown = 30; 
                    ESP_LOGI(TAG, ">>> [INTERRUPT] DA BAM NUT: TAT CANH BAO, HE THONG TRO VE AN TOAN! <<<");
                }
                state_changed = true;

            } else if (evt.type == EVT_SENSOR_DATA) {
                last_temp = evt.temperature;
                last_gas = evt.gas_raw;
                last_flame = evt.flame_detected;

                if (reset_cooldown > 0) {
                    reset_cooldown--;
                }


                if (reset_cooldown == 0) {
                    if (evt.flame_detected) flame_hold++;
                    else                    flame_hold = 0;

                    if (evt.flame_detected )
                    {
                        if (current_state != LEVEL_FIRE_ALERT && current_state != LEVEL_MANUAL_EMERGENCY) {
                            current_state = LEVEL_FIRE_ALERT;
                            alarm_latched = true; 
                            state_changed = true;
                            ESP_LOGW(TAG, ">>> PHAT HIEN CO CHAY! KHOA CANH BAO SUOT (CHO BAM NUT DE TAT) <<<");
                        }
                    }
                    else if (evt.gas_raw >= GAS_THRESHOLD_ALERT) 
                    {
                        if (current_state == LEVEL_SAFE) {
                            current_state = LEVEL_GAS_ALERT;
                            alarm_latched = true;
                            state_changed = true;
                            ESP_LOGW(TAG, ">>> PHAT HIEN KHI GAS! KHOA CANH BAO SUOT (CHO BAM NUT DE TAT) <<<");
                        }
                    }

                }

                if (++log_counter >= 10) {
                    log_counter = 0;
                    ESP_LOGI(TAG, "[%s] Nhiet do: %d C | Gas ADC: %4d | Co lua: %s%s",
                             (current_state == LEVEL_SAFE) ? "AN TOAN" :
                             (current_state == LEVEL_GAS_ALERT) ? "RO RI GAS" :
                             (current_state == LEVEL_FIRE_ALERT) ? "CO CHAY!" : "KHAN CAP",
                             last_temp, last_gas, last_flame ? "CO (!)" : "Khong",
                             alarm_latched ? " [CANH BAO SUOT]" : "");
                }
            } 
            //  GỬI LỆNH CHO TASK CÒI & ĐÈN (MAILBOX PATTERN)
           
            if (state_changed || current_state != previous_state) {
                previous_state = current_state;
                xQueueOverwrite(xAlarmLevelQueue, &current_state);
            }

  
            // GỬI DỮ LIỆU SANG OLED (MAILBOX PATTERN)

            if (state_changed || (++oled_refresh_counter >= 5)) {
                oled_refresh_counter = 0;
                DisplayMailbox_t disp = {
                    .level = current_state,
                    .temperature = last_temp,
                    .gas_raw = last_gas,
                    .flame = last_flame
                };
                xQueueOverwrite(xDisplayQueue, &disp);
            }
        }
    }
}

//  CƠ CẤU CHẤP HÀNH CÒI & ĐÈN 


static void Task_AlarmActuator(void *pvParameters) {
    AlarmLevel_t active_level = LEVEL_SAFE;
    int step = 0;

    while (1) {
        AlarmLevel_t new_level;
        if (xQueueReceive(xAlarmLevelQueue, &new_level, pdMS_TO_TICKS(50)) == pdPASS) {
            active_level = new_level;
            step = 0; 
        }
        step++;

        switch (active_level) {
            case LEVEL_SAFE:
                gpio_set_level(PIN_BUZZER, 0);
                gpio_set_level(PIN_LED, 0);
                break;

            case LEVEL_GAS_ALERT:

                if (step % 12 < 3) {
                    gpio_set_level(PIN_BUZZER, 1);
                    gpio_set_level(PIN_LED, 1);
                } else if (step % 12 < 6) {
                    gpio_set_level(PIN_BUZZER, 0);
                    gpio_set_level(PIN_LED, 1);
                } else {
                    gpio_set_level(PIN_BUZZER, 0);
                    gpio_set_level(PIN_LED, 0);
                }
                break;

            case LEVEL_FIRE_ALERT:
            case LEVEL_MANUAL_EMERGENCY:
                if (step % 2 == 1) {
                    gpio_set_level(PIN_BUZZER, 1);
                    gpio_set_level(PIN_LED, 1);
                } else {
                    gpio_set_level(PIN_BUZZER, 0);
                    gpio_set_level(PIN_LED, 0);
                }
                break;
        }
    }
}


//  HIỂN THỊ GIAO DIỆN OLED (ƯU TIÊN 1 - THẤP NHẤT)

static void Task_DisplayOLED(void *pvParameters) {
    DisplayMailbox_t disp_data;

    while (1) {
        if (xQueueReceive(xDisplayQueue, &disp_data, portMAX_DELAY) == pdPASS) {
            oled_render_dashboard(&disp_data);
        }
    }
}


void app_main(void) {
    ESP_LOGI(TAG, "Khoi tao he thong Smart Fire & Hazard Warning System...");

    // 1. Cấu hình chân GPIO đầu ra: Còi & LED
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << PIN_BUZZER) | (1ULL << PIN_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    gpio_config(&out_conf);
    gpio_set_level(PIN_BUZZER, 0);
    gpio_set_level(PIN_LED, 0);

    // 2. Cấu hình chân GPIO đầu vào: NÚT NHẤN với NGẮT CẠNH XUỐNG (GPIO_INTR_NEGEDGE)
    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL << PIN_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE // Kích hoạt ngắt khi bấm nút
    };
    gpio_config(&in_conf);

    // 3. Đăng ký dịch vụ Ngắt phần cứng và liên kết hàm button_isr_handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_BUTTON, button_isr_handler, NULL);

    // 4. Khởi tạo các module phần cứng
    flame_sensor_init();
    mq135_init();
    oled_init_driver();

    
    // Khởi tạo các queues
    
    xSystemEventQueue = xQueueCreate(10, sizeof(SystemEvent_t));
    xAlarmLevelQueue  = xQueueCreate(1, sizeof(AlarmLevel_t));  
    xDisplayQueue     = xQueueCreate(1, sizeof(DisplayMailbox_t)); 

    
    //  KHỞI TẠO CÁC Task FREERTOS
   
  
    xTaskCreate(Task_FireController, "Task_FireController", 4096, NULL, 4, NULL);
    xTaskCreate(Task_AlarmActuator,  "Task_AlarmActuator",  2048, NULL, 3, NULL);
    xTaskCreate(Task_SensorSampler,  "Task_SensorSampler",  3072, NULL, 2, NULL);
    xTaskCreate(Task_DisplayOLED,    "Task_DisplayOLED",    4096, NULL, 1, NULL);

    ESP_LOGI(TAG, "He thong san sang hoat dong (Che do Khoa bao dong Latch-Alarm)!");
}