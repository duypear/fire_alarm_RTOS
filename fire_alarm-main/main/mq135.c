#include "mq135.h"
#include "config.h"
#include "esp_adc/adc_oneshot.h"

static adc_oneshot_unit_handle_t s_adc_handle;

esp_err_t mq135_init(void) {
    adc_oneshot_unit_init_cfg_t init_cfg = { .unit_id = ADC_UNIT_1 };
    esp_err_t err = adc_oneshot_new_unit(&init_cfg, &s_adc_handle);
    if (err != ESP_OK) return err;

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12, // Đo áp đến 3.3V
    };
    return adc_oneshot_config_channel(s_adc_handle, PIN_MQ135_ADC_CHAN, &chan_cfg);
}

int mq135_read_raw(void) {
    int val = 0;
    adc_oneshot_read(s_adc_handle, PIN_MQ135_ADC_CHAN, &val);
    return val;
}