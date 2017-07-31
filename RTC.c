#include <stdint.h>
#include "stm32f4xx_hal_rtc.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_pwr.h"
#include "cmsis_os.h" 
#include "RTC.h"

extern osMutexId(EnvData_Mutex_Id);

char DateTimeNow[20];

static RTC_HandleTypeDef hrtc;
static RTC_TimeTypeDef time;
static RTC_DateTypeDef date;

static void RTC_Init(void);

/// RTC Config
void RTC_Config(void)
{
	hrtc.Instance = RTC;
	hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
	hrtc.Init.AsynchPrediv = 0x7F;
	hrtc.Init.SynchPrediv = 0xFF;
}

/// Convert "yyyy-MM-ddThh:mm:ss" and set date time 
void  RTC_SetDateTime(char *dtString)
{
	// Extract date and time
	int year = (dtString[2] - '0') * 10 + (dtString[3] - '0'); // Note year is between 0 and 99
	int month = (dtString[5] - '0') * 10 + (dtString[6] - '0');
	int day = (dtString[8] - '0') * 10 + (dtString[9] - '0');
	
	int hour = (dtString[11] - '0') * 10 + (dtString[12] - '0');
	int minute = (dtString[14] - '0') * 10 + (dtString[15] - '0');
	int second = (dtString[17] - '0') * 10 + (dtString[18] - '0');
	
	// Enter init mode
	RTC_Init();
	
	// Set time
	time.Hours = hour;
	time.Minutes = minute;
	time.Seconds = second;
	HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
	
	// Set date
	date.Year = year;
	date.Month = month;
	date.Date = day;	
	HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);

  HAL_RTC_WaitForSynchro(&hrtc);
}
	
/// Get date time as "yyyy-MM-ddThh:mm:ss"
char* RTC_GetDateTime(void)
{
	HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	
	osMutexWait(EnvData_Mutex_Id, osWaitForever);
	sprintf(DateTimeNow, "%04d-%02d-%02dT%02d:%02d:%02d", 
	        2000 + date.Year, date.Month, date.Date,
	        time.Hours, time.Minutes, time.Seconds);	
	osMutexRelease(EnvData_Mutex_Id);
	
	return DateTimeNow;
}

/// Enter RTC calendar init mode
static void RTC_Init(void)
{
		__HAL_RCC_PWR_CLK_ENABLE();
		HAL_PWR_EnableBkUpAccess();
		
		__HAL_RCC_LSE_CONFIG(RCC_LSE_OFF);
		__HAL_RCC_LSE_CONFIG(RCC_LSE_ON);
		while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET);
		
		__HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);
		__HAL_RCC_RTC_ENABLE();

		HAL_RTC_Init(&hrtc);
}
