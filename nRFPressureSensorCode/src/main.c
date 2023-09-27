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
#include "Json/cJSON.h"


/*******************************MACROS****************************************/
#define INFOLITZ_EDIT //Comment this line to disable our changes
//#define SLEEP_ENABLE  //Uncomment this line to enable sleep functionality
#define ADC_MAX_VALUE 1023

/*******************************GLOBAL VARIABLES********************************/

#ifndef INFOLITZ_EDIT
    const int pressureMax = 929; //analog reading of pressure transducer at 100psi
    const int pressureZero = 110; //analog reading of pressure transducer at 0psi
#else
    const int pressureZero = 150; //analog reading of pressure transducer at 0psi
                              //PressureZero = 0.5/3.3V*1024~150(supply voltage - 3.3v) taken from Arduino code refernce from visense 
    const int pressureMax = 775; //analog reading of pressure transducer at 100psi
                             //PressureMax = 2.5/3.3V*1024~775  taken from Arduino code refernce from visense       
#endif

const int pressuretransducermaxPSI = 70; //psi value of transducer being used
cJSON *pcData = NULL;
const struct device *pAdc = NULL;
const struct gpio_dt_spec sSleepStatusLED = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

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
 * @brief function to add json object to json
 * @param pcJsonHandle - Json object handle
 * @param pcKey - Key name
 * @param pcValue - value
 * @param ucLen - value length
 * @return true or false
*/
bool AddItemtoJsonObject(cJSON **pcJsonHandle, const char *pcKey, 
                    uint8_t *pcValue, uint8_t ucLen)
{
    uint8_t ucIndex = 0;
    bool bRetVal = false;

    if (*pcJsonHandle && pcKey && pcValue)
    {
        if (strcmp(pcKey,"data") == 0)
        {
            pcData = cJSON_AddArrayToObject(*pcJsonHandle, "data");

            if (pcData)
            {
                for (ucIndex = 0; ucIndex < ucLen; ucIndex++)
                {
                    cJSON_AddItemToArray(pcData, cJSON_CreateNumber(pcValue[ucIndex]));
                }
            }
            else
            {
                return false;
            }
        }
        else
        {
            cJSON_AddNumberToObject(*pcJsonHandle, pcKey, *pcValue);
        }
        bRetVal = true;
    }

    return bRetVal;
}

/**
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
            .gain              = NRF_SAADC_GAIN1_5,
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

    StartAdvertising();
    gpio_pin_configure_dt(&sSleepStatusLED, GPIO_ACTIVE_LOW);

    while (1) 
    {
        unPressureRaw = AnalogRead();
        printk("ADCRaw: %d\n", unPressureRaw);
        pMainObject = cJSON_CreateObject();

        if (unPressureRaw > pressureZero && unPressureRaw < ADC_MAX_VALUE)
        {
            unPressureResult = ((unPressureRaw-pressureZero)*pressuretransducermaxPSI)/(pressureMax-pressureZero);
            sprintf(cbuffer,"pr=%dpsi", unPressureResult);
            printk("Data:%s\n", cbuffer);
            pucAdvertisingdata[2] = 0x01;
            pucAdvertisingdata[3] = (uint8_t)strlen(cbuffer);
            AddItemtoJsonObject(&pMainObject, "data", (uint8_t*)cbuffer, (uint8_t)strlen(cbuffer));
            strcpy(cJsonBuffer, (char *)cJSON_Print(pMainObject));
            memcpy(&pucAdvertisingdata[4], cJsonBuffer, strlen(cJsonBuffer));
            printk("JSON:\n%s\n", cJsonBuffer);
        } 

        if(IsNotificationenabled())
        {
            VisenseSensordataNotify(pucAdvertisingdata+2, 15);
        }
        else
        {
            UpdateAdvertiseData();
            StartAdvertising();
        }
        
        memset(pucAdvertisingdata, 0, ADV_BUFF_SIZE);
        cJSON_Delete(pcData);
        cJSON_Delete(pMainObject);
        k_sleep(K_MSEC(1000));
        
        #ifdef SLEEP_ENABLE
         EnterSleepMode(180);
         ExitSleepMode();
        #endif
    }
}