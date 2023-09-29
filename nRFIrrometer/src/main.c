/**
 * @file main.c
 * @brief Main function
 * @date 2023-09-27
 * @author Jeslin
 * @note  This is a test code for Irrometer.
*/


/*******************************INCLUDES****************************************/
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <nrfx_example.h>
#include <saadc_examples_common.h>
#include <nrfx_saadc.h>
#include <nrfx_log.h>
#include <stdlib.h>
#include <stdio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include "Json/cJSON.h"
#include "BleHandler.h"
#include "BleService.h"
#include "JsonHandler.h"

/*******************************MACROS****************************************/
//#define SLEEP_ENABLE  //Uncomment this line to enable sleep functionality
/*******************************GLOBAL VARIABLES********************************/
nrf_saadc_value_t  sAdcReadValue1 = 0;
nrf_saadc_value_t  sAdcReadValue2 = 0;
const int Rx = 10000;
const float default_TempC = 24.0;
const long open_resistance = 35000;
const long short_resistance = 200;
const long short_CB = 240, open_CB = 255;
const float SupplyV = 3.3;


cJSON *pcData = NULL;
const struct device *pAdc = NULL;

const struct gpio_dt_spec sSensorPwSpec1 = GPIO_DT_SPEC_GET(DT_ALIAS(testpin0), gpios);
const struct gpio_dt_spec sSensorPwSpec2 = GPIO_DT_SPEC_GET(DT_ALIAS(testpin1), gpios);
const struct gpio_dt_spec sSleepStatusLED = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);



/*******************************FUNCTION DEFINITIONS********************************/

/**
 * @brief  This function is to read raw adc value
 * @param  None 
 * @return float - ADC result
*/
float AnalogRead(void)
{

    nrfx_err_t status;
    nrf_saadc_value_t sample_value;

    status = nrfx_saadc_buffer_set(&sample_value, 1);
    NRFX_ASSERT(status == NRFX_SUCCESS);
    
    status = nrfx_saadc_mode_trigger();
    NRFX_ASSERT(status == NRFX_SUCCESS);
    return sample_value;
}

int GetAdcResult( const struct gpio_dt_spec *excite_pin_spec)
{
    int16_t AdcReadValue ;
    if (!device_is_ready(excite_pin_spec->port)) {
        printk("Error: %s device not ready\n", excite_pin_spec->port->name);
        return -1;
    }
    gpio_pin_set(excite_pin_spec->port, excite_pin_spec->pin, 1);
    k_sleep(K_USEC(90)); 

    AdcReadValue = AnalogRead();

    gpio_pin_set(excite_pin_spec->port, excite_pin_spec->pin, 0);
    k_sleep(K_MSEC(100)); 

    
    return AdcReadValue;
}

int myCBvalue(int res, float TC, float cF)
{  

	int WM_CB;
	float resK = res / 1000.0;
	float tempD = 1.00 + 0.018 * (TC - 24.00);

	if (res > 550.00)
    { 
		if (res > 8000.00)
        { 
			WM_CB = (-2.246 - 5.239 * resK * (1 + .018 * (TC - 24.00)) - .06756 * resK * resK * (tempD * tempD)) * cF;
		}
        else if (res > 1000.00)
        {
			WM_CB = (-3.213 * resK - 4.093) / (1 - 0.009733 * resK - 0.01205 * (TC)) * cF ;
		}
        else
        {
			WM_CB = (resK * 23.156 - 12.736) * tempD;
		}
	} 
    else
    {
		if (res > 300.00)
        {
			WM_CB = 0.00;
		}
		if (res < 300.00 && res >= short_resistance)
        {
			WM_CB = short_CB;
			printk("Sensor Short WM \n");
		}
	}
	if (res >= open_resistance || res==0)
    {
		WM_CB = open_CB;
	}
	
	return WM_CB;
}
float readWMsensor(void)
{
    float SenVWM1 = (sAdcReadValue1 / 1024.0) * SupplyV;
    float SenVWM2 = (sAdcReadValue2 / 1024.0) * SupplyV;

    printk("Sensor Voltage A: %.3f V\n", SenVWM1);
    printk("Sensor Voltage B: %.3f V\n", SenVWM2);

    double WM_ResistanceA = (Rx * (SupplyV - SenVWM1) / SenVWM1);
    double WM_ResistanceB = (Rx * SenVWM2) / (SupplyV - SenVWM2);

    return (WM_ResistanceA + WM_ResistanceB) / 2.0;
}

void saadc_callback(nrfx_saadc_evt_t const * p_event) {
    // Handle SAADC events here if needed.
}
/**
 * @brief function to initialize adc
 * @return void
*/
static void InitAdc(void)
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
        .pin_p             = NRF_SAADC_INPUT_AIN1,
        .pin_n             = NRF_SAADC_INPUT_DISABLED,
        .channel_index     = 1
    };

    status = nrfx_saadc_channel_config(&saadc_channel);
    NRFX_ASSERT(status == NRFX_SUCCESS);
    
    uint32_t channels_mask = nrfx_saadc_channels_configured_get();
    status = nrfx_saadc_simple_mode_set(channels_mask,
                                        NRF_SAADC_RESOLUTION_10BIT,
                                        NRF_SAADC_OVERSAMPLE_DISABLED,
                                        NULL);
    NRFX_ASSERT(status == NRFX_SUCCESS);


    gpio_pin_configure_dt(&sSensorPwSpec1, GPIO_OUTPUT_LOW);
    gpio_pin_configure_dt(&sSensorPwSpec2, GPIO_OUTPUT_LOW);
    k_sleep(K_MSEC(100)); 

}
/**
 * @brief function to add json object to json
 * @param pcJsonHandle - Json object handle
 * @param pcKey - Key name
 * @param pcValue - value
 * @param ucLen - value length
 * @return true or false
*/

static bool SetPMState()
{
    bool bRetVal = false;

    struct pm_state_info info =
    {
        .exit_latency_us  = 0,
        .min_residency_us = 0,
        .state            = PM_STATE_SUSPEND_TO_RAM,
        .substate_id      = 0,
    };

    bRetVal = pm_state_force(0, &info);
    return bRetVal;
}
static void EnterSleepMode(int nDuration)
{
    pm_device_action_run(pAdc, PM_DEVICE_ACTION_SUSPEND);
    pm_device_action_run(&sSensorPwSpec1, PM_DEVICE_ACTION_SUSPEND);
    pm_device_action_run(&sSensorPwSpec2, PM_DEVICE_ACTION_SUSPEND);
    BleStopAdv();
    gpio_pin_set(sSleepStatusLED.port, sSleepStatusLED.pin, 1);
    k_sleep(K_SECONDS(nDuration));
}

/**
 * @brief function for exiting sleep mode
 * @return none
*/
static void ExitSleepMode()
{
    pm_device_action_run(pAdc, PM_DEVICE_ACTION_RESUME);
    pm_device_action_run(&sSensorPwSpec1, PM_DEVICE_ACTION_RESUME);
    pm_device_action_run(&sSensorPwSpec2, PM_DEVICE_ACTION_RESUME);
    gpio_pin_set(sSleepStatusLED.port, sSleepStatusLED.pin, 0);
}


int main(void)
{
    nrfx_err_t status;
    int Ret;
    char cbuffer[60] = {0};
    char cJsonBuffer[100] = {0};
    cJSON *pMainObject = NULL;
    const float cFactor = 1.1;
    uint8_t *pucAdvBuffer = NULL;

    InitAdc();

    pucAdvBuffer = GetAdvBuffer();
    Ret = bt_enable(NULL);
	if (Ret) 
    {
		printk("Bluetooth init failed (err %d)\n", Ret);
		return 0;
	}
    Ret = InitExtAdv();
	if (Ret) 
    {
		printk("Advertising failed to create (err %d)\n", Ret);
		return 0;
	}
    Ret = StartAdv();
    if(Ret)
    {
        printk("Advertising failed to start (err %d)\n", Ret);
        return 0;
    }
    
     while (1) 
     {
        pMainObject = cJSON_CreateObject();
        sAdcReadValue1 = GetAdcResult(&sSensorPwSpec1);
        printk("Reading A1: %d\n", sAdcReadValue1);
        sAdcReadValue2 = GetAdcResult(&sSensorPwSpec2);
        printk("Reading A2: %d\n", sAdcReadValue2);
        
        float WM_Resistance = readWMsensor();
        printk("WM Resistance(Ohms): %d\n", (int)WM_Resistance);
        int WM1_CB = myCBvalue((int)WM_Resistance, default_TempC, cFactor);
        printk("WM1(cb/kPa): %d\n", abs(WM1_CB));

        sprintf(cbuffer,"CB=%d", abs(WM1_CB));
        printk("Data:%s\n", cbuffer);
        
        
        // AddItemtoJsonObject(&pMainObject, "data", (uint8_t*)cbuffer, (uint8_t)strlen(cbuffer));
        AddItemtoJsonObject(&pMainObject, NUMBER, "ADCValue1", &sAdcReadValue1, sizeof(uint16_t));
        AddItemtoJsonObject(&pMainObject, NUMBER, "ADCValue2", &sAdcReadValue2, sizeof(uint16_t));
        AddItemtoJsonObject(&pMainObject, STRING, "CBValue", (uint8_t*)cbuffer, (uint8_t)strlen(cbuffer));
        strcpy(cJsonBuffer, (char *)cJSON_Print(pMainObject));
        
        memset(cbuffer,0 , sizeof(cbuffer));
        pucAdvBuffer[2] = 0x02;
        pucAdvBuffer[3] = (uint8_t)strlen(cJsonBuffer);
        memcpy(pucAdvBuffer+4, cJsonBuffer, strlen(cJsonBuffer));

        printk("JSON:\n%s\n", cJsonBuffer);
        cJSON_Delete(pcData);
        cJSON_Delete(pMainObject);

        if(IsNotificationenabled())
        {
            VisenseSensordataNotify(pucAdvBuffer+2, ADV_BUFF_SIZE);
        }
        else
        {
            UpdateAdvData();
            StartAdv();
        }
        
        memset(pucAdvBuffer, 0, ADV_BUFF_SIZE);

        k_sleep(K_MSEC(1000));
        #ifdef SLEEP_ENABLE
         EnterSleepMode(60);
         ExitSleepMode();
        #endif
     }
}
    
