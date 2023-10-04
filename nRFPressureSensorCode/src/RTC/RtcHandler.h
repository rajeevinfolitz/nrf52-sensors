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
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/rtc/maxim_ds3231.h>

/***************************************MACROS**********************************/



/***************************************TYPEDEFS*********************************/


/***************************************FUNCTION DECLARTAION*********************/
bool RtcInit();
bool GetCurrentTime(char *cCurrTime);

#endif