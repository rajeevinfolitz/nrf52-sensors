#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <nrfx_example.h>
#include <saadc_examples_common.h>
#include <nrfx_saadc.h>
#include <nrfx_log.h>

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
volatile nrf_saadc_value_t  latest_adc_val_A1 = 0;
volatile nrf_saadc_value_t  latest_adc_val_A2 = 0;

const struct gpio_dt_spec sensor_power_spec = GPIO_DT_SPEC_GET(DT_ALIAS(testpin0), gpios);
const struct gpio_dt_spec sensor_power_spec2 = GPIO_DT_SPEC_GET(DT_ALIAS(testpin1), gpios);


float AnalogRead(int channel)
{
    nrfx_err_t status;
    nrf_saadc_value_t sample_value;

    status = nrfx_saadc_buffer_set(&sample_value, 1);
    NRFX_ASSERT(status == NRFX_SUCCESS);
    
    status = nrfx_saadc_mode_trigger();
    NRFX_ASSERT(status == NRFX_SUCCESS);

    return (float)sample_value / 1023.0 * SupplyV;
}

int analog_read_on_nrf(int channel, const struct gpio_dt_spec *excite_pin_spec)
{
    const struct device *gpio_dev;

    if (!device_is_ready(excite_pin_spec->port)) {
        printk("Error: %s device not ready\n", excite_pin_spec->port->name);
        return -1;
    }

    gpio_dev = excite_pin_spec->port;
    gpio_pin_set(gpio_dev, excite_pin_spec->pin, 1);
    k_sleep(K_USEC(90)); 

    float voltage = AnalogRead(channel);

    gpio_pin_set(gpio_dev, excite_pin_spec->pin, 0);
    k_sleep(K_MSEC(100)); 

    return (voltage / SupplyV) * 1023.0;
}

int myCBvalue(int res, float TC)
{
    int WM_CB;
    res = res * cFactor;
    
    if (res > 550) {
        if (res > 8000) {
            WM_CB = (-2.246 - 5.239 * (res / 1000.0) * (1 + .018 * (TC - 24.0)) 
                    - .06756 * (res / 1000.0) * (res / 1000.0) * ((1.0 + 0.018 * (TC - 24.0)) 
                    * (1.0 + 0.018 * (TC - 24.0))));
        } else if (res > 1000) {
            WM_CB = (-3.213 * (res / 1000.0) - 4.093) / (1 - 0.009733 * (res / 1000.0) 
                    - 0.01205 * TC);
        } else {
            WM_CB = ((res / 1000.0) * 23.156 - 12.736) * (1.0 + 0.018 * (TC - 24.0));
        }
    } else {
        if (res > 300) {
            WM_CB = 0;
        } else if (res < 300 && res >= short_resistance) {
            WM_CB = short_CB;
            printk("Sensor Short WM\n");
        }
    }

    if (res >= open_resistance || res == 0) {
        WM_CB = open_CB;
        printk("Open or Fault for WM\n");
    }
    
    return WM_CB;
}

float readWMsensor(void)
{
    float SenVWM1 = (latest_adc_val_A1 / 1024.0) * SupplyV;
    float SenVWM2 = (latest_adc_val_A2 / 1024.0) * SupplyV;

    printk("Sensor Voltage A: %.3f V\n", SenVWM1);
    printk("Sensor Voltage B: %.3f V\n", SenVWM2);

    double WM_ResistanceA = (Rx * (SupplyV - SenVWM1) / SenVWM1);
    double WM_ResistanceB = Rx * SenVWM2 / (SupplyV - SenVWM2);

    return (WM_ResistanceA + WM_ResistanceB) / 2.0;
}

void saadc_callback(nrfx_saadc_evt_t const * p_event) {
    // Handle SAADC events here if needed.
}

void main(void)
{
    nrfx_err_t status;

    status = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_saadc_channel_t saadc_channel = {
        .channel_config = {
            .resistor_p        = NRF_SAADC_RESISTOR_DISABLED,
            .resistor_n        = NRF_SAADC_RESISTOR_DISABLED,
            .gain              = NRF_SAADC_GAIN1_4,
            .reference         = NRF_SAADC_REFERENCE_VDD4,
            .acq_time          = NRF_SAADC_ACQTIME_10US,
            .mode              = NRF_SAADC_MODE_SINGLE_ENDED,
            .burst             = NRF_SAADC_BURST_DISABLED,
        },
        .pin_p             = NRF_SAADC_INPUT_AIN5,
        .pin_n             = NRF_SAADC_INPUT_DISABLED,
        .channel_index     = 0
    };

    status = nrfx_saadc_channel_config(&saadc_channel);
    NRFX_ASSERT(status == NRFX_SUCCESS);
    
    uint32_t channels_mask = nrfx_saadc_channels_configured_get();
    status = nrfx_saadc_simple_mode_set(channels_mask,
                                        NRF_SAADC_RESOLUTION_10BIT,
                                        NRF_SAADC_OVERSAMPLE_DISABLED,
                                        NULL);
    NRFX_ASSERT(status == NRFX_SUCCESS);


    gpio_pin_configure_dt(&sensor_power_spec, GPIO_ACTIVE_LOW);
    gpio_pin_configure_dt(&sensor_power_spec2, GPIO_ACTIVE_LOW);
    k_sleep(K_MSEC(100)); 

    printk("Application started!!\n");

     while (1) {
        latest_adc_val_A1 = analog_read_on_nrf(0, &sensor_power_spec);
        printk("Reading A1: %d\n", latest_adc_val_A1);

        latest_adc_val_A2 = analog_read_on_nrf(0, &sensor_power_spec2);
        printk("Reading A2: %d\n", latest_adc_val_A2);
        // float WM_Resistance = readWMsensor();
        // printk("WM Resistance(Ohms): %d\n", (int)WM_Resistance);

        // int WM1_CB = myCBvalue((int)WM_Resistance, default_TempC, cFactor);
        // printk("WM1(cb/kPa): %d\n", abs(WM1_CB));

        k_sleep(K_MSEC(1000));
    }
}