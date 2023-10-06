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

/***************************************MACROS*********************************/
#define MAX_TIME_BUFF_SIZE 80
#define RTC_DEV_ADDR 0x68
#define RTC_CTRL_REG 0x0E

/***************************************GLOBALS*********************************/

static const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
//const struct device *const ds3231 = DEVICE_DT_GET_ONE(maxim_ds3231);
static long long llLastUpdatedTime = 1696585221;


/***************************************FUNCTION DEFINITIONS********************/

#if 0
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
	uint32_t now = llUpdateTime;
	uint32_t ulTimeSet = now + 3600 - (now % 3600);
	

	struct maxim_ds3231_syncpoint sp = 
	{
		.rtc = {
			.tv_sec = ulTimeSet,
			.tv_nsec = (uint64_t)NSEC_PER_SEC * syncclock / syncclock_Hz,
		},
		.syncclock = 0,
	};
	//  struct k_poll_signal ss;
	// //  //struct sys_notify notify;
	//   struct k_poll_event sevt = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_IGNORE,
	//   						    K_POLL_MODE_NOTIFY_ONLY,
	// 						    &ss);

	// k_poll_signal_init(&ss);
	//  sys_notify_init_signal(&notify, &ss);
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
 * @brief 
*/
bool GetCurrenTimeInEpoch(long long *pllCurrEpoch)
{
	
	int nRetCode = 0;
	bool bRetVal = false;

	if (pllCurrEpoch)
	{

    nRetCode = counter_get_value(ds3231, pllCurrEpoch);

    if (!nRetCode)
    {
        bRetVal = true;
    }
    
    return bRetVal;
	}
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

      //  printk("DS3231 on %s syncclock %u Hz\n\n", CONFIG_BOARD, syncclock_Hz);

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
#endif

static int ConvertBCDToDecimal(int to_convert)
{
   return (to_convert >> 4) * 10 + (to_convert & 0x0F);
}

static int ConvertDecimalToBCD(int to_convert)
{
   return ((to_convert / 10) << 4) + (to_convert % 10);
}


static bool ConvertEpochToTime(long long llTimeStamp, struct tm *psTimeStruct, char *pcBuffer)
{
   //  time_t rawtime = 1696585221;
     //1696585221
    struct tm  ts = {0};
    char       buf[MAX_TIME_BUFF_SIZE] = {0};


	if (psTimeStruct && pcBuffer)
	{
		ts = *localtime(&llTimeStamp);
		printk("sec: %d\n\r", ts.tm_sec);
		printk("min: %d\n\r", ts.tm_min);    
		printk("hr: %d\n\r", ts.tm_hour);
		printk("day: %d\n\r", ts.tm_wday);
		printk("date: %d\n\r", ts.tm_mday);
		printk("mon: %d\n\r", ts.tm_mon);   
		printk("yr: %d\n\r", ts.tm_year);                     
		strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
		strcpy(pcBuffer, buf);
		memcpy(psTimeStruct, &ts, sizeof(ts));
	}
}

static bool SetTimeDate()
{
	bool bRetVal = false;
	struct tm sTimeDate;
	char cTimeBuffer[MAX_TIME_BUFF_SIZE];
	uint8_t ucReg = 0x00;

	ConvertEpochToTime(llLastUpdatedTime, &sTimeDate, cTimeBuffer);

	do
    {
        if (i2c_reg_write_byte(i2c_dev, RTC_DEV_ADDR, 
								ucReg, ConvertDecimalToBCD(sTimeDate.tm_sec)) != 0)
        {
            printk("writing seconds failed\n\r");
            break;
        }

        ucReg++;

        if (i2c_reg_write_byte(i2c_dev, RTC_DEV_ADDR, 
								ucReg, ConvertDecimalToBCD(sTimeDate.tm_min)) != 0)
        {
            printk("writing minute failed\n\r");
            break;
        }

        ucReg++;

        if (i2c_reg_write_byte(i2c_dev, RTC_DEV_ADDR, 
								ucReg, ConvertDecimalToBCD(sTimeDate.tm_hour)) != 0)
        {
            printk("writing hour failed\n\r");
            break;
        }

        ucReg++;

        if (i2c_reg_write_byte(i2c_dev, RTC_DEV_ADDR, 
									ucReg, ConvertDecimalToBCD(sTimeDate.tm_wday)) != 0)
        {
            printk("writing Day failed\n\r");
            break;
        }

        ucReg++;

        if (i2c_reg_write_byte(i2c_dev, RTC_DEV_ADDR, 
									ucReg, ConvertDecimalToBCD(sTimeDate.tm_mday)) != 0)
        {
            printk("writing Day failed\n\r");
            break;
        }

        ucReg++;
		printk("Month set: %d\n\r", sTimeDate.tm_mon);
		k_sleep(K_MSEC(1000));
        if (i2c_reg_write_byte(i2c_dev, RTC_DEV_ADDR, 
									ucReg, ConvertDecimalToBCD(sTimeDate.tm_mon)) != 0)
        {
            printk("writing Day failed\n\r");
            break;
        }

        ucReg++;

         if (i2c_reg_write_byte(i2c_dev, RTC_DEV_ADDR, 
		 							ucReg, ConvertDecimalToBCD((sTimeDate.tm_year+1900)-2000)) != 0)
        {
            printk("writing Day failed\n\r");
            break;
        }

    } while (0);

	return bRetVal;
}

bool InitRtc()
{
    bool bRetVal = false;

    if (i2c_reg_write_byte(i2c_dev, RTC_DEV_ADDR, RTC_CTRL_REG, 0x00) != 0)
    {
        printk("Configuring RTC failed\n\r");
    }
    else
    {
        printk("Configuring RTC success\n\r");
        bRetVal = true;
    }

	bRetVal = SetTimeDate();

    return bRetVal;
}


bool GetCurrenTimeInEpoch(long long *pllCurrEpoch)
{
	struct tm sTimeStamp = {0};
	uint16_t ucData = 0x00;
	uint8_t ucReg = 0x00;
	char cTimeBuffer[MAX_TIME_BUFF_SIZE] = {0};
	bool bRetval = false;

	if (pllCurrEpoch)
	{
		do
		{
			if (0 != i2c_reg_read_byte(i2c_dev, RTC_DEV_ADDR, ucReg, &ucData))
			{
				printk("Reading seconds failed\n\r");
				break;
			}

			sTimeStamp.tm_sec = ConvertBCDToDecimal(ucData);
			//printk("Seconds: %d\n\r",sTimeStamp.tm_sec);
			ucReg++;

			if (0 != i2c_reg_read_byte(i2c_dev, RTC_DEV_ADDR, ucReg, &ucData))
			{
				printk("Reading seconds failed\n\r");
				break;
			}
			
			sTimeStamp.tm_min = ConvertBCDToDecimal(ucData);
			//printk("Min: %d\n\r",sTimeStamp.tm_min);
			ucReg++;

			if (0 != i2c_reg_read_byte(i2c_dev, RTC_DEV_ADDR, ucReg, &ucData))
			{
				printk("Reading seconds failed\n\r");
				break;
			}
			
			sTimeStamp.tm_hour = ConvertBCDToDecimal(ucData);
			//printk("Hours: %d\n\r",sTimeStamp.tm_hour);
			ucReg++;

			if (0 != i2c_reg_read_byte(i2c_dev, RTC_DEV_ADDR, ucReg, &ucData))
			{
				printk("Reading seconds failed\n\r");
				break;
			}
			
			sTimeStamp.tm_wday = ConvertBCDToDecimal(ucData);
			//printk("Day: %d\n\r",sTimeStamp.tm_wday);
			ucReg++;

			if (0 != i2c_reg_read_byte(i2c_dev, RTC_DEV_ADDR, ucReg, &ucData))
			{
				printk("Reading seconds failed\n\r");
				break;
			}
			
			sTimeStamp.tm_mday = ConvertBCDToDecimal(ucData);
			//printk("Date: %d\n\r",sTimeStamp.tm_mday);
			ucReg++;	

			if (0 != i2c_reg_read_byte(i2c_dev, RTC_DEV_ADDR, ucReg, &ucData))
			{
				printk("Reading seconds failed\n\r");
				break;
			}
			
			sTimeStamp.tm_mon = ConvertBCDToDecimal(ucData);
			//printk("Month: %d\n\r",sTimeStamp.tm_mon);
			ucReg++;		

			if (0 != i2c_reg_read_byte(i2c_dev, RTC_DEV_ADDR, ucReg, &ucData))
			{
				printk("Reading seconds failed\n\r");
				break;
			}
			
			sTimeStamp.tm_year = ((ConvertBCDToDecimal(ucData)+2000) - 1900);
			//printk("Year: %d\n\r",sTimeStamp.tm_year+1900);
			strftime(cTimeBuffer, sizeof(cTimeBuffer), "%a %Y-%m-%d %H:%M:%S %Z", &sTimeStamp);
			printk("Current Time: %s\n\r", cTimeBuffer);

		} while (0);
		
		sTimeStamp.tm_isdst = -1;
		*pllCurrEpoch = mktime(&sTimeStamp);

		bRetval = true;
	}
}