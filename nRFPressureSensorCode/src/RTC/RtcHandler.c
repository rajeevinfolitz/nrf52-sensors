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
static long long llLastUpdatedTime = 1696585221;


/***************************************FUNCTION DEFINITIONS********************/

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