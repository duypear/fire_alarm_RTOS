#include "oled_ssd1306.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"

static uint8_t s_buf[OLED_WIDTH * OLED_HEIGHT / 8];

static esp_err_t oled_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    return i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, buf, sizeof(buf), pdMS_TO_TICKS(50));
}

esp_err_t oled_init_driver(void) {
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_OLED_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = PIN_OLED_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &i2c_conf);
    i2c_driver_install(I2C_MASTER_NUM, i2c_conf.mode, 0, 0, 0);

    // Chuỗi lệnh khởi động SSD1306
    oled_cmd(0xAE);
    oled_cmd(0xD5); oled_cmd(0x80);
    oled_cmd(0xA8); oled_cmd(0x3F);
    oled_cmd(0xD3); oled_cmd(0x00);
    oled_cmd(0x40);
    oled_cmd(0x8D); oled_cmd(0x14); // Bật Charge Pump
    oled_cmd(0x20); oled_cmd(0x00);
    oled_cmd(0xA1);
    oled_cmd(0xC8);
    oled_cmd(0xDA); oled_cmd(0x12);
    oled_cmd(0x81); oled_cmd(0xCF);
    oled_cmd(0xD9); oled_cmd(0xF1);
    oled_cmd(0xDB); oled_cmd(0x40);
    oled_cmd(0xA4);
    oled_cmd(0xA6);
    oled_cmd(0xAF);
    return ESP_OK;
}

static void oled_clear(void) {
    memset(s_buf, 0, sizeof(s_buf));
}

static void oled_set_pixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    if (color) s_buf[x + (y / 8) * OLED_WIDTH] |= (1 << (y % 8));
    else       s_buf[x + (y / 8) * OLED_WIDTH] &= ~(1 << (y % 8));
}

static void oled_draw_rect(int x, int y, int w, int h, uint8_t color, bool fill) {
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            if (fill || i == 0 || i == w - 1 || j == 0 || j == h - 1) {
                oled_set_pixel(x + i, y + j, color);
            }
        }
    }
}

// Bảng font chữ 5x7 chuẩn sắc nét
static void get_glyph_5x7(char c, uint8_t rows[7]) {
    switch (c) {
        case 'A': { uint8_t r[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}; memcpy(rows, r, 7); break; }
        case 'B': { uint8_t r[7] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}; memcpy(rows, r, 7); break; }
        case 'C': { uint8_t r[7] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}; memcpy(rows, r, 7); break; }
        case 'D': { uint8_t r[7] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}; memcpy(rows, r, 7); break; }
        case 'E': { uint8_t r[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}; memcpy(rows, r, 7); break; }
        case 'F': { uint8_t r[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}; memcpy(rows, r, 7); break; }
        case 'G': { uint8_t r[7] = {0x0E, 0x11, 0x10, 0x13, 0x11, 0x11, 0x0F}; memcpy(rows, r, 7); break; }
        case 'H': { uint8_t r[7] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}; memcpy(rows, r, 7); break; }
        case 'I': { uint8_t r[7] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}; memcpy(rows, r, 7); break; }
        case 'K': { uint8_t r[7] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}; memcpy(rows, r, 7); break; }
        case 'L': { uint8_t r[7] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}; memcpy(rows, r, 7); break; }
        case 'M': { uint8_t r[7] = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}; memcpy(rows, r, 7); break; }
        case 'N': { uint8_t r[7] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}; memcpy(rows, r, 7); break; }
        case 'O': { uint8_t r[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}; memcpy(rows, r, 7); break; }
        case 'P': { uint8_t r[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}; memcpy(rows, r, 7); break; }
        case 'Q': { uint8_t r[7] = {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}; memcpy(rows, r, 7); break; }
        case 'R': { uint8_t r[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}; memcpy(rows, r, 7); break; }
        case 'S': { uint8_t r[7] = {0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E}; memcpy(rows, r, 7); break; }
        case 'T': { uint8_t r[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}; memcpy(rows, r, 7); break; }
        case 'U': { uint8_t r[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}; memcpy(rows, r, 7); break; }
        case 'V': { uint8_t r[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}; memcpy(rows, r, 7); break; }
        case 'X': { uint8_t r[7] = {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}; memcpy(rows, r, 7); break; }
        case 'Y': { uint8_t r[7] = {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}; memcpy(rows, r, 7); break; }
        case '0': { uint8_t r[7] = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}; memcpy(rows, r, 7); break; }
        case '1': { uint8_t r[7] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}; memcpy(rows, r, 7); break; }
        case '2': { uint8_t r[7] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}; memcpy(rows, r, 7); break; }
        case '3': { uint8_t r[7] = {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}; memcpy(rows, r, 7); break; }
        case '4': { uint8_t r[7] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}; memcpy(rows, r, 7); break; }
        case '5': { uint8_t r[7] = {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}; memcpy(rows, r, 7); break; }
        case '6': { uint8_t r[7] = {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E}; memcpy(rows, r, 7); break; }
        case '7': { uint8_t r[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}; memcpy(rows, r, 7); break; }
        case '8': { uint8_t r[7] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}; memcpy(rows, r, 7); break; }
        case '9': { uint8_t r[7] = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}; memcpy(rows, r, 7); break; }
        case ':': { uint8_t r[7] = {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00}; memcpy(rows, r, 7); break; }
        case '!': { uint8_t r[7] = {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}; memcpy(rows, r, 7); break; }
        case '[': { uint8_t r[7] = {0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E}; memcpy(rows, r, 7); break; }
        case ']': { uint8_t r[7] = {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E}; memcpy(rows, r, 7); break; }
        case '(': { uint8_t r[7] = {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}; memcpy(rows, r, 7); break; }
        case ')': { uint8_t r[7] = {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}; memcpy(rows, r, 7); break; }
        case '|': { uint8_t r[7] = {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}; memcpy(rows, r, 7); break; }
        case '-': { uint8_t r[7] = {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}; memcpy(rows, r, 7); break; }
        default:  { memset(rows, 0, 7); break; }
    }
}

static void oled_draw_char(int x, int y, char c) {
    uint8_t rows[7] = {0};
    get_glyph_5x7(c, rows);
    for (int r = 0; r < 7; r++) {
        for (int col = 0; col < 5; col++) {
            if ((rows[r] >> (4 - col)) & 1) {
                oled_set_pixel(x + col, y + r, 1);
            }
        }
    }
}

static void oled_draw_str(int x, int y, const char *str) {
    while (*str) {
        oled_draw_char(x, y, *str);
        x += 6;
        str++;
    }
}

static void oled_refresh(void) {
    oled_cmd(0x21); oled_cmd(0); oled_cmd(127);
    oled_cmd(0x22); oled_cmd(0); oled_cmd(7);
    for (int p = 0; p < 8; p++) {
        uint8_t packet[129];
        packet[0] = 0x40;
        memcpy(&packet[1], &s_buf[p * 128], 128);
        i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, packet, sizeof(packet), pdMS_TO_TICKS(50));
    }
}

void oled_render_dashboard(const DisplayMailbox_t *disp_data) {
    char text_buf[32];
    oled_clear();
    oled_draw_rect(0, 0, 128, 64, 1, false);

    if (disp_data->level == LEVEL_SAFE) {
        // Trạng thái bình thường
        oled_draw_str(16, 7, "HE THONG AN TOAN");
        oled_draw_rect(8, 18, 112, 1, 1, true);

        snprintf(text_buf, sizeof(text_buf), "NHIET DO: %2d C", disp_data->temperature);
        oled_draw_str(14, 25, text_buf);

        snprintf(text_buf, sizeof(text_buf), "KHI GAS : %4d", disp_data->gas_raw);
        oled_draw_str(14, 38, text_buf);

        oled_draw_rect(14, 51, 100, 7, 1, false);
        int bar_w = (disp_data->gas_raw * 96) / 4095;
        if (bar_w > 96) bar_w = 96;
        if (bar_w > 0) oled_draw_rect(16, 53, bar_w, 3, 1, true);

    } else if (disp_data->level == LEVEL_GAS_ALERT) {
        // Cảnh báo rò rỉ gas
        oled_draw_rect(2, 2, 124, 60, 1, false);
        oled_draw_str(16, 10, "CO RO RI KHI GA!");
        oled_draw_rect(10, 22, 108, 1, 1, true);

        snprintf(text_buf, sizeof(text_buf), "GAS: %4d (NGUY HIEM)", disp_data->gas_raw);
        oled_draw_str(6, 30, text_buf);

        snprintf(text_buf, sizeof(text_buf), "NHIET DO: %2d C", disp_data->temperature);
        oled_draw_str(20, 44, text_buf);

    } else if (disp_data->level == LEVEL_FIRE_ALERT) {
        // Cảnh báo có cháy
        oled_draw_rect(2, 2, 124, 60, 1, false);
        oled_draw_str(40, 10, "CO CHAY!");
        oled_draw_rect(10, 22, 108, 1, 1, true);

        if (disp_data->flame) {
            oled_draw_str(10, 30, "CO LUA & NHIET CAO");
        } else {
            oled_draw_str(14, 30, "NHIET DO QUA CAO");
        }

        snprintf(text_buf, sizeof(text_buf), "T: %2d C | GAS: %4d", disp_data->temperature, disp_data->gas_raw);
        oled_draw_str(10, 44, text_buf);

    } else if (disp_data->level == LEVEL_MANUAL_EMERGENCY) {
        // Báo động khẩn cấp thủ công
        oled_draw_rect(2, 2, 124, 60, 1, false);
        oled_draw_str(14, 10, "BAO DONG KHAN CAP!");
        oled_draw_rect(10, 22, 108, 1, 1, true);

        oled_draw_str(16, 30, "KICH HOAT THU CONG");
        oled_draw_str(16, 44, "(NHAN NUT DE TAT)");
    }

    oled_refresh();
}