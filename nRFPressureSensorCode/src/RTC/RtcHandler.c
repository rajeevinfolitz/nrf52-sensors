/**
 * @file    : Rtchandler.c
 * @brief   : File containing RTc related functions
 * @author  : Adhil
 * @date    : 04-10-2023
 * @note    :
*/

/***************************************INCLUDES*********************************/
#include "RtcHandler.h"
#include <string.h>

/***************************************GLOBALS*********************************/

const struct device *const ds3231 = DEVICE_DT_GET_ONE(maxim_ds3231);
static long long llLastUpdatedTime = 1696407837;


/***************************************FUNCTION DEFINITIONS********************/

/**
 * @brief Format times as: YYYY-MM-DD HH:MM:SS
*/
static const char *FormatTime(time_t time,long nsec)
{
	static char buf[64];
	char *bp = buf;
	char *const bpe = bp + sizeof(buf);
	struct tm tv;
	struct tm *tp = gmtime_r(&time, &tv);

	bp += strftime(bp, bpe - bp, "%Y-%m-%d %H:%M:%S", tp);
	if (nsec >= 0) {
		bp += snprintf(bp, bpe - bp, ".%09lu", nsec);
	}
	bp += strftime(bp, bpe - bp, " %a %j", tp);
	return buf;
}


/**
 * @brief Print current time value
*/
static void PrintCurrentTime(const struct device *ds3231)
{
	uint32_t now = 0;

	printk("\nCounter at %p\n", ds3231);
	printk("\tMax top value: %u (%08x)\n",
	       counter_get_max_top_value(ds3231),
	       counter_get_max_top_value(ds3231));
	printk("\t%u channels\n", counter_get_num_of_channels(ds3231));
	printk("\t%u Hz\n", counter_get_frequency(ds3231));

	printk("Top counter value: %u (%08x)\n",
	       counter_get_top_value(ds3231),
	       counter_get_top_value(ds3231));

	(void)counter_get_value(ds3231, &now);

	printk("Now %u: %s\n", now, FormatTime(now, -1));
}

/**
 * @brief SetTime and Date
*/
static bool SetTimeAndDate(const struct device *ds3231, long long llUpdateTime)
{
    bool bRetVal = false;
	struct sys_notify notify;
	uint32_t ulRetCode = 0;
	uint32_t syncclock_Hz = maxim_ds3231_syncclock_frequency(ds3231);
	uint32_t syncclock = maxim_ds3231_read_syncclock(ds3231);
	uint32_t now = 0;
	uint32_t align_hour = now + llUpdateTime;

	struct maxim_ds3231_syncpoint sp = 
	{
		.rtc = {
			.tv_sec = align_hour,
			.tv_nsec = (uint64_t)NSEC_PER_SEC * syncclock / syncclock_Hz,
		},
		.syncclock = syncclock,
	};

	ulRetCode = maxim_ds3231_set(ds3231, &sp, &notify);

	if (ulRetCode > 0)
    {
	    printk("\nSet %s at %u ms\n", FormatTime(sp.rtc.tv_sec, sp.rtc.tv_nsec),
	       syncclock);
        bRetVal = true;
    }

    return bRetVal;
}

/**
 * @brief GetCurrentTime()
*/
bool GetCurrentTime(char *cCurrTime)
{
    bool bRetVal = false;
    uint32_t now = 0;
    int nRetCode = 0;

    nRetCode = counter_get_value(ds3231, &now);

    if (!nRetCode)
    {
        strcpy(cCurrTime, FormatTime(now, -1));
        printk("Time %s\n\r", cCurrTime);
        bRetVal = true;
    }
    
    return bRetVal;
}

/**
 * @brief Init RTC 
 * @param None
 * @return 
*/
bool RtcInit()
{
    bool bRetVal = false;

    do
    {
        if (!device_is_ready(ds3231)) 
        {
            printk("%s: device not ready.\n", ds3231->name);
            break;
        }

        uint32_t syncclock_Hz = maxim_ds3231_syncclock_frequency(ds3231);

        printk("DS3231 on %s syncclock %u Hz\n\n", CONFIG_BOARD, syncclock_Hz);

        int rc = maxim_ds3231_stat_update(ds3231, 0, MAXIM_DS3231_REG_STAT_OSF);

        if (rc >= 0) 
        {
            printk("DS3231 has%s experienced an oscillator fault\n",
                (rc & MAXIM_DS3231_REG_STAT_OSF) ? "" : " not");
        } 
        else 
        {
            printk("DS3231 stat fetch failed: %d\n", rc);
            break;
        }
    } while (0);
    


	/* Show the DS3231 counter properties */
	//PrintCurrentTime(ds3231);

	/* Show the DS3231 ctrl and ctrl_stat register values */
	printk("\nDS3231 ctrl %02x ; ctrl_stat %02x\n",
	       maxim_ds3231_ctrl_update(ds3231, 0, 0),
	       maxim_ds3231_stat_update(ds3231, 0, 0));

   bRetVal = SetTimeAndDate(ds3231, llLastUpdatedTime);

   return bRetVal;
}