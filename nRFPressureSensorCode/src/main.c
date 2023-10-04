/**
 * @file main.c
 * @brief Main function
 * @date 2023-21-09
 * @author Jeslin
 * @note  This is a test code for pressure sensor.
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
#include "BleHandler.h"
#include "BleService.h"
#include "JsonHandler.h"


/*******************************MACROS****************************************/
#define INFOLITZ_EDIT //Comment this line to disable our changes
//#define SLEEP_ENABLE  //Uncomment this line to enable sleep functionality
#define ADC_MAX_VALUE 1023
#define PRESSURE_SENSOR 0x01

/*******************************TYPEDEFS****************************************/

/*******************************GLOBAL VARIABLES********************************/

#ifndef INFOLITZ_EDIT
    const int pressureMax = 929; //analog reading of pressure transducer at 100psi
    const int pressureZero = 110; //analog reading of pressure transducer at 0psi
#else
    static uint32_t pressureZero = 150; //analog reading of pressure transducer at 0psi
                              //PressureZero = 0.5/3.3V*1024~150(supply voltage - 3.3v) taken from Arduino code refernce from visense 
    static uint32_t pressureMax = 775; //analog reading of pressure transducer at 100psi
                             //PressureMax = 2.5/3.3V*1024~775  taken from Arduino code refernce from visense       
#endif

const int pressuretransducermaxPSI = 70; //psi value of transducer being used
cJSON *pcData = NULL;
const struct device *pAdc = NULL;
const struct gpio_dt_spec sSleepStatusLED = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
void setPressureZero(uint32_t ucbuff);
void setPressureMax(uint32_t ucbuff);
/*******************************FUNCTION DEFINITIONS********************************/

/**
 * @brief  This function is to read raw adc value
 * @param  None 
 * @return uint16_t - ADC result
*/
uint16_t AnalogRead(void)
{
    nrfx_err_t status;
    uint16_t sample_value;

    status = nrfx_saadc_buffer_set(&sample_value, 1);
    NRFX_ASSERT(status == NRFX_SUCCESS);
    status = nrfx_saadc_mode_trigger();
    NRFX_ASSERT(status == NRFX_SUCCESS);

    return sample_value;
}
/**
 * @brief Setting pressureZero
 * @param ucbuffer - buffer for setting pressureZero
 * @return void
*/
void SetPressureZero(uint32_t ucbuffer)
{
    pressureZero = ucbuffer;
}
/**
 * @brief Setting pressureMax
 * @param ucbuffer - buffer for setting pressureMax
 * @return void
*/
void SetPressureMax(uint8_t ucbuffer)
{
    pressureMax = ucbuffer;
}

/*
 * @brief Setting Power management policy
 * @param void
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

/**
 * @brief function for entering sleep mode
 * @param nDuration - Duration for sleep
 * @return none
*/
static void EnterSleepMode(int nDuration)
{
    pm_device_action_run(pAdc, PM_DEVICE_ACTION_SUSPEND);
    BleStopAdvertise();
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
    gpio_pin_set(sSleepStatusLED.port, sSleepStatusLED.pin, 0);
}

/**
 * @brief function to initialize adc
 * @return void
*/
static void InitADC()
{
    nrfx_err_t status;
    status = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_saadc_channel_t saadc_channel = 
    {
        .channel_config = 
        {
            .resistor_p        = NRF_SAADC_RESISTOR_DISABLED,
            .resistor_n        = NRF_SAADC_RESISTOR_DISABLED,
            .gain              = NRF_SAADC_GAIN1_6,
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
}

/**
 * @brief  Main function
 * @return int
*/
int main(void)
{
    int nError;
    uint16_t unPressureResult =0;
    uint16_t unPressureRaw = 0;
    char cbuffer[30] = {0};
    char cJsonBuffer[100] = {0};
    uint8_t *pucAdvertisingdata = NULL;
    cJSON *pMainObject = NULL;

    SetPMState();
    pucAdvertisingdata = GetAdvertisingBuffer();
    InitADC();
    k_sleep(K_MSEC(100));
    pAdc = device_get_binding("arduino_adc");

    if (!EnableBLE())
    {
        printk("Bluetooth init failed (err %d)\n", nError);
        return 0;
    }

    nError = InitExtendedAdv();
	if (nError) 
    {
		printk("Advertising failed to create (err %d)\n", nError);
		return 0;
	}
    sprintf(cbuffer,"%dpsi", unPressureResult);
    StartAdvertising();
    gpio_pin_configure_dt(&sSleepStatusLED, GPIO_ACTIVE_LOW);

    while (1) 
    {
        unPressureRaw = AnalogRead();
        if (unPressureRaw > ADC_MAX_VALUE)
        {
            unPressureRaw = 0;
        }
        printk("ADCRaw: %d\n", unPressureRaw);
        pMainObject = cJSON_CreateObject();
        AddItemtoJsonObject(&pMainObject, NUMBER, "ADCValue", &unPressureRaw, sizeof(uint16_t));
        if (unPressureRaw > pressureZero && unPressureRaw < ADC_MAX_VALUE)
        {
            memset(cbuffer, '\0',sizeof(cbuffer));
            unPressureResult = ((unPressureRaw-pressureZero)*pressuretransducermaxPSI)/(pressureMax-pressureZero);
            
            sprintf(cbuffer,"%dpsi", unPressureResult);
            printk("Data:%s\n", cbuffer);
            AddItemtoJsonObject(&pMainObject, STRING, "CurrPressure", (uint8_t*)cbuffer, (uint8_t)strlen(cbuffer));

        }
        else
        {
             AddItemtoJsonObject(&pMainObject, STRING, "PrevPressure", (uint8_t*)cbuffer, (uint8_t)strlen(cbuffer));   
        }

        strcpy(cJsonBuffer, (char *)cJSON_Print(pMainObject));
        pucAdvertisingdata[2] = PRESSURE_SENSOR;
        pucAdvertisingdata[3] = (uint8_t)strlen(cJsonBuffer);
        memcpy(&pucAdvertisingdata[4], cJsonBuffer, strlen(cJsonBuffer));
        printk("JSON:\n%s\n", cJsonBuffer);

        if(IsNotificationenabled())
        {
           VisenseSensordataNotify(pucAdvertisingdata+2, ADV_BUFF_SIZE);
        }
        else if (!IsNotificationenabled() && !IsConnected())
        {
            UpdateAdvertiseData();
            StartAdvertising();
        }
        else
        {
            //NO OP
        }
        
        memset(pucAdvertisingdata, 0, ADV_BUFF_SIZE);

        cJSON_Delete(pMainObject);
        k_sleep(K_MSEC(1000));
        printk("PressureZero: %d\n", pressureZero);
        printk("PressureMax: %d\n", pressureMax);
        #ifdef SLEEP_ENABLE
         EnterSleepMode(180);
         ExitSleepMode();
        #endif
    }
}