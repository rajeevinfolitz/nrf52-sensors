#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_saadc.h"
#include "boards.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

// Constants
#define SAMPLE_BUFFER_LEN 5
const int Rx = 10000;
const float default_TempC = 24.0;
const long open_resistance = 35000;
const long short_resistance = 200;
const long short_CB = 240, open_CB = 255;
const float SupplyV = 3.3;
const float cFactor = 1.1;

// Global variables
volatile nrf_saadc_value_t latest_adc_val_A1 = 0;
volatile nrf_saadc_value_t latest_adc_val_A2 = 0;

int analog_read_on_nrf(int adc_channel, uint32_t excite_pin) {
    int result;

    nrf_gpio_pin_write(excite_pin, 1);  // Excite the sensor
    nrf_delay_us(90);                   // Delay for 90ms
    nrfx_saadc_sample_convert(adc_channel, &result);
    nrf_gpio_pin_write(excite_pin, 0);  // De-excite the sensor
    nrf_delay_ms(100);

    return result;
}

int myCBvalue(int res, float TC, float cF) {
    int WM_CB;
    res = res * cF;
    if (res > 550) {
        if (res > 8000) {
            WM_CB = (-2.246 - 5.239 * (res / 1000.0) * (1 + .018 * (TC - 24.0)) - .06756 * (res / 1000.0) * (res / 1000.0) * ((1.0 + 0.018 * (TC - 24.0)) * (1.0 + 0.018 * (TC - 24.0))));
        } else if (res > 1000) {
            WM_CB = (-3.213 * (res / 1000.0) - 4.093) / (1 - 0.009733 * (res / 1000.0) - 0.01205 * TC);
        } else {
            WM_CB = ((res / 1000.0) * 23.156 - 12.736) * (1.0 + 0.018 * (TC - 24.0));
        }
    } else {
        if (res > 300) {
            WM_CB = 0;
        } else if (res < 300 && res >= short_resistance) {
            WM_CB = short_CB;
            NRF_LOG_INFO("Sensor Short WM");
        }
    }
    if (res >= open_resistance || res == 0) {
        WM_CB = open_CB;
        NRF_LOG_INFO("Open or Fault for WM");
    }
    return WM_CB;
}

float readWMsensor() {
    float SenVWM1 = (latest_adc_val_A1 / 1024.0) * SupplyV;
    float SenVWM2 = (latest_adc_val_A2 / 1024.0) * SupplyV;

    NRF_LOG_INFO("Sensor Voltage A: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(SenVWM1));
    NRF_LOG_INFO("Sensor Voltage B: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(SenVWM2));

    double WM_ResistanceA = (Rx * (SupplyV - SenVWM1) / SenVWM1);
    double WM_ResistanceB = Rx * SenVWM2 / (SupplyV - SenVWM2);

    return (WM_ResistanceA + WM_ResistanceB) / 2.0;
}

void saadc_callback(nrfx_saadc_evt_t const * p_event) {
    // Handle SAADC events here if needed.
}

int main(void) {

    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    // Explicit SAADC configuration
    nrfx_saadc_config_t saadc_config = {
        .resolution = NRF_SAADC_RESOLUTION_10BIT,
        .oversample = NRF_SAADC_OVERSAMPLE_DISABLED,
        .interrupt_priority = APP_IRQ_PRIORITY_LOW,
        .low_power_mode = false
    };
    APP_ERROR_CHECK(nrfx_saadc_init(&saadc_config, saadc_callback));

    // Explicit Channel configuration
    nrf_saadc_channel_config_t channel_config = {
        .resistor_p = NRF_SAADC_RESISTOR_DISABLED,
        .resistor_n = NRF_SAADC_RESISTOR_DISABLED,
        .gain = NRF_SAADC_GAIN1_4,
        .reference = NRF_SAADC_REFERENCE_VDD4,
        .acq_time = NRF_SAADC_ACQTIME_10US,
        .mode = NRF_SAADC_MODE_SINGLE_ENDED,
        .pin_p = NRF_SAADC_INPUT_AIN5,
        .pin_n = NRF_SAADC_INPUT_DISABLED,
        .burst = NRF_SAADC_BURST_DISABLED
    };
    APP_ERROR_CHECK(nrfx_saadc_channel_init(0, &channel_config));

    nrf_gpio_cfg_output(26);
    nrf_gpio_cfg_output(27);
    nrf_gpio_pin_write(26, 0);
    nrf_gpio_pin_write(27, 0);
    nrf_delay_ms(100);

    NRF_LOG_INFO("Application started!!");

    while (1) {
        latest_adc_val_A1 = analog_read_on_nrf(0, 27);
        NRF_LOG_INFO("Reading A1: %d", latest_adc_val_A1);

        latest_adc_val_A2 = analog_read_on_nrf(0, 26);
        NRF_LOG_INFO("Reading A2: %d", latest_adc_val_A2);

        float WM_Resistance = readWMsensor();
        NRF_LOG_INFO("WM Resistance(Ohms): %d", (int)WM_Resistance);
        
        int WM1_CB = myCBvalue((int)WM_Resistance, default_TempC, cFactor);
        NRF_LOG_INFO("WM1(cb/kPa): %d", abs(WM1_CB));

        nrf_delay_ms(1000);
    }
    return 0;
}
