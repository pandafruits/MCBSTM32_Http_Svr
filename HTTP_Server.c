/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::Network
 * Copyright (c) 2004-2015 ARM Germany GmbH. All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    HTTP_Server.c
 * Purpose: HTTP Server example
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "rl_net.h"                     // Keil.MDK-Pro::Network:CORE
#include "stm32f4xx_hal.h"              // Keil::Device:STM32Cube HAL:Common
#include "Board_ADC.h"                  // ::Board Support:A/D Converter
#include "Board_Buttons.h"              // ::Board Support:Buttons
#include "Board_LED.h"                  // ::Board Support:LED
#include "Board_GLCD.h"                 // ::Board Support:Graphic LCD
#include "GLCD_Config.h"                // Keil.MCBSTM32F400::Board Support:Graphic LCD
#include "Core_Temp.h"
#include "RTC.h"
#include "LCD_Sleep.h"
#include "Board_Touch.h"
#include "AM2302.h"

#define LCD_SLEEP_TIMEOUT_SEC 60

extern GLCD_FONT GLCD_Font_16x24;

bool LEDrun;
char lcd_text[3][20+1] = { "LCD line 1",
                           "LCD line 2",
                           "LCD line 3"};
													 
// Thread IDs
osThreadId TID_Display;
osThreadId TID_Led;
osThreadId TID_Environment;

// Thread definitions
static void BlinkLed (void const *arg);
static void Display  (void const *arg);
static void Environment(void const *arg);
													 		 
osThreadDef(BlinkLed, osPriorityNormal, 1, 0);
osThreadDef(Display,  osPriorityNormal, 1, 1024);
osThreadDef(Environment, osPriorityNormal, 1, 2048);

// Mutex for LCD access													 
osMutexDef(LCD_Mutex);
osMutexId(LCD_Mutex_Id);
// Mutex for LCD text access													 
osMutexDef(LCDText_Mutex);
osMutexId(LCDText_Mutex_Id);
// Mutex for envrionmental data access													 
osMutexDef(EnvData_Mutex);
osMutexId(EnvData_Mutex_Id);
			
#ifdef __RTX
extern uint32_t os_time;

uint32_t HAL_GetTick(void) {
  return os_time; 
}
#endif

/// System Clock Configuration
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the
     device is clocked below the maximum system frequency (see datasheet). */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 |
                                RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

/// Read analog inputs
uint16_t AD_in (uint32_t ch) {
  int32_t val = 0;

  if (ch == 0) {
    ADC_StartConversion();
    while (ADC_ConversionDone () < 0);
    val = ADC_GetValue();
  }
  return (val);
}

/// Read digital inputs
uint8_t get_button (void) {
  return (Buttons_GetState ());
}

/// IP address change notification
void netDHCP_Notify (uint32_t if_num, uint8_t option, const uint8_t *val, uint32_t len) {

  if (option == NET_DHCP_OPTION_IP_ADDRESS) {
    /* IP address change, trigger LCD update */
    osSignalSet (TID_Display, 0x01);
  }
}

/*----------------------------------------------------------------------------
  Thread 'Display': LCD display handler
 *---------------------------------------------------------------------------*/
static void Display (void const *arg) {
  static uint8_t ip_addr[NET_ADDR_IP6_LEN];
  static char    ip_ascii[40];
  static char    buf[24];

  GLCD_Initialize         ();
  GLCD_SetBackgroundColor (GLCD_COLOR_BLACK);
  GLCD_SetForegroundColor (GLCD_COLOR_GREEN);
  GLCD_ClearScreen        ();
  GLCD_SetFont            (&GLCD_Font_16x24);
  GLCD_DrawString         (0, 1*24, "  MDK HTTP Server   ");

  GLCD_DrawString (0, 3*24, "IP4:Waiting for DHCP");

  // Print Link-local IPv6 address
  netIF_GetOption (NET_IF_CLASS_ETH,
                   netIF_OptionIP6_LinkLocalAddress, ip_addr, sizeof(ip_addr));

  netIP_ntoa(NET_ADDR_IP6, ip_addr, ip_ascii, sizeof(ip_ascii));

  sprintf (buf, "IP6:%.16s", ip_ascii);
  GLCD_DrawString (0, 4*24, buf);
  sprintf (buf, "%s", ip_ascii+16);
  GLCD_DrawString (10*16, 5*24, buf);
	
  while(1) {
    /* Wait for signal from DHCP */
    osSignalWait (0x01, osWaitForever);
    /* Retrieve and print IPv4 address */
    netIF_GetOption (NET_IF_CLASS_ETH,
                     netIF_OptionIP4_Address, ip_addr, sizeof(ip_addr));

    netIP_ntoa (NET_ADDR_IP4, ip_addr, ip_ascii, sizeof(ip_ascii));

		osMutexWait(LCD_Mutex_Id, osWaitForever);
		
    sprintf (buf, "IP4:%-16s",ip_ascii);
    GLCD_DrawString (0, 3*24, buf);
		
		osMutexWait(LCDText_Mutex_Id, osWaitForever);
    /* Display user text lines */
    sprintf (buf, "%-20s", lcd_text[0]);
    GLCD_DrawString (0, 7*24, buf);
    sprintf (buf, "%-20s", lcd_text[1]);
    GLCD_DrawString (0, 8*24, buf);
		sprintf (buf, "%-20s", lcd_text[2]);
    GLCD_DrawString (0, 9*24, buf);
		osMutexRelease(LCDText_Mutex_Id);
		
		osMutexRelease(LCD_Mutex_Id);
  }
}

/*----------------------------------------------------------------------------
  Thread 'BlinkLed': Blink the LEDs on an eval board
 *---------------------------------------------------------------------------*/
static void BlinkLed (void const *arg) {
  const uint8_t led_val[16] = { 0x48,0x88,0x84,0x44,0x42,0x22,0x21,0x11,
                                0x12,0x0A,0x0C,0x14,0x18,0x28,0x30,0x50 };
  int cnt = 0;

  LEDrun = true;
  while(1) {
    // Every 100 ms
    if (LEDrun == true) {
      LED_SetOut (led_val[cnt]);
      if (++cnt >= sizeof(led_val)) {
        cnt = 0;
      }
    }
    osDelay (100);
  }
}

/*----------------------------------------------------------------------------
  Thread 'Environment': read enivronmental data
 *---------------------------------------------------------------------------*/
static void Environment (void const *arg) {
  char buf[24];
  while(1) {
		RTC_GetDateTime();
		CoreTemp_Read();
		AM2302_Read();
		
		osMutexWait(LCDText_Mutex_Id, osWaitForever);
		// Date time
		strcpy (lcd_text[0], DateTimeNow);
		// Core temperature
		sprintf(buf, "Core: %.1fdegC", CoreTemperature);
		strcpy (lcd_text[1], buf);
		// Temperature and humidity
		sprintf(buf, "%.1fdegC  %.1f%%", Ambient.Temp, Ambient.Hum);
		strcpy (lcd_text[2], buf);
		osMutexRelease(LCDText_Mutex_Id);
		
		osSignalSet (TID_Display, 0x01);
    osDelay(1000);
  }
}

/*----------------------------------------------------------------------------
  Main Thread 'main': Run Network
 *---------------------------------------------------------------------------*/
int main (void) {
	TOUCH_STATE touchState;
	uint8_t isSysReset = true;
	uint8_t isLcdSleep = false;
	int16_t lcdSleepCntNum10Ms;
	
  SystemClock_Config ();
  LED_Initialize     ();
  Buttons_Initialize ();
  ADC_Initialize     ();
	CoreTemp_Init();
	RTC_Config();
	Touch_Initialize();
	AM2302_Init();

  netInitialize      ();
	
	LCD_Mutex_Id = osMutexCreate(osMutex(LCD_Mutex));
	LCDText_Mutex_Id = osMutexCreate(osMutex(LCDText_Mutex));
	EnvData_Mutex_Id = osMutexCreate(osMutex(EnvData_Mutex));
	
  TID_Led     = osThreadCreate (osThread(BlinkLed), NULL);
  TID_Display = osThreadCreate (osThread(Display),  NULL);
	TID_Environment = osThreadCreate(osThread(Environment),  NULL);

  while(1) {
    //osSignalWait (0, osWaitForever);
		
		lcdSleepCntNum10Ms = LCD_SLEEP_TIMEOUT_SEC * 100;

		Touch_GetState(&touchState);
		if(touchState.pressed || isSysReset)
		{
			if(!isSysReset)
			{
				LCD_ExitSleep();
				osMutexRelease(LCD_Mutex_Id);
			}
			
			while(lcdSleepCntNum10Ms-- > 0)
			{
				osDelay(10);
				Touch_GetState(&touchState);
				if(touchState.pressed) 
				{
					lcdSleepCntNum10Ms = LCD_SLEEP_TIMEOUT_SEC * 100;
				}
			}
			
		  isSysReset = false;
			isLcdSleep = false;
		}
		
		if(!isLcdSleep)
		{
			osMutexWait(LCD_Mutex_Id, osWaitForever);
			LCD_EnterSleep();
			isLcdSleep = true;
		}
	}
}
