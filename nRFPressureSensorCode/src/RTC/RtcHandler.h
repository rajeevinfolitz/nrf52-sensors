/**
 * @file    : RtcHandler.h 
 * @brief   : File containing functions for handling RTC 
 * @author  : Adhil
 * @date    : 04-10-2023
 * @note 
*/

#ifndef _RTC_HANDLER_H
#define _RTC_HANDLER_H

/***************************************INCLUDES*********************************/
#include <time.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
// #include <zephyr/drivers/rtc/maxim_ds3231.h>

/***************************************MACROS**********************************/



/***************************************TYPEDEFS*********************************/


/***************************************FUNCTION DECLARTAION*********************/
bool InitRtc();
bool GetCurrenTimeInEpoch(long long *pllCurrEpoch);

#endif