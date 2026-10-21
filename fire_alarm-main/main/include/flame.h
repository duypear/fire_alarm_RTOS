#ifndef FLAME_H
#define FLAME_H

#include <stdbool.h>
#include "esp_err.h"

// Cấu hình chân GPIO cảm biến lửa
esp_err_t flame_sensor_init(void);

// Trả về true nếu phát hiện có lửa (mức LOW)
bool flame_sensor_is_detected(void);

#endif // FLAME_H