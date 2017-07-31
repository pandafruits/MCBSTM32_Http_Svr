#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_tim.h"
#include "cmsis_os.h" 
#include "DWT_Delay.h"
#include "AM2302.h"

#define AM2302_PORT				     GPIOE
#define AM2302_PIN				     GPIO_PIN_2

#define AM2302_PIN_LOW			   HAL_GPIO_WritePin(AM2302_PORT, AM2302_PIN, GPIO_PIN_RESET)
#define AM2302_PIN_HIGH			   HAL_GPIO_WritePin(AM2302_PORT, AM2302_PIN, GPIO_PIN_SET)
#define AM2302_PIN_IN			     GPIO_SetPinAsInput(AM2302_PORT, AM2302_PIN)
#define AM2302_PIN_OUT			   GPIO_SetPinAsOutput(AM2302_PORT, AM2302_PIN)
#define AM2302_PIN_READ			   HAL_GPIO_ReadPin(AM2302_PORT, AM2302_PIN)

extern osMutexId(EnvData_Mutex_Id);

static void GPIO_SetPinAsInput(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
static void GPIO_SetPinAsOutput(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

AM2302_Data_t Ambient;

// Initialize AM2302
void AM2302_Init(void) 
{
	uint8_t dwt = DWT_Delay_Init();
	__HAL_RCC_GPIOE_CLK_ENABLE();
}

void AM2302_Read(void)
{
	uint8_t retry = 0;
	uint8_t rawData[5];
	do
	{
		for(int i = 0; i < 5; ++i)
		{
			rawData[i] = 0x00;
		}
		uint8_t bitNum = 0;
		uint8_t currBit = 7;
		uint8_t currByte = 0;
		volatile uint8_t numUs;
	
		/* MCU pulls the data line low to signal a start for 20 ms */
		AM2302_PIN_OUT;
		AM2302_PIN_LOW;
		HAL_Delay(20);
		
		// Then pulls the data line high for 40 us
		AM2302_PIN_HIGH;
		Delay_us(40);

		/* AM2302 responds with a low and a high pulse */
		AM2302_PIN_IN;
		numUs = 0;
		while(!AM2302_PIN_READ)
		{
			if(numUs > 185) 
				return;
			Delay_us(1);
			++numUs;
		}
		numUs = 0;
		while(AM2302_PIN_READ)
		{
			if(numUs > 185) 
				return;
			Delay_us(1);
			++numUs;
		}

		/* Receive 40 bits of raw data */
		for (bitNum = 0; bitNum < 40; ++bitNum) 
		{
			numUs = 0;
			while(!AM2302_PIN_READ)
			{
				if(numUs > 155) 
					return;
				Delay_us(1);
				++numUs;
			}
			
			numUs = 0;
			while(AM2302_PIN_READ)
			{
				if(numUs > 175) 
					return;
				Delay_us(1);
				++numUs;
			}			

			// 1 received if pulse long, otherwise 0 received
			if(numUs > 40) 
			{
				rawData[currByte] |= (1 << currBit);
			}
			
			if (currBit == 0) 
			{
				currBit = 7;
				currByte++;
			} 
			else 
			{
				currBit--;
			}
		}
	}
	while(((rawData[0] + rawData[1] + rawData[2] + rawData[3]) & 0xFF) != rawData[4] && ++retry < 5);
		
	// Give up after 5 attemps to get a valid data
	if(((rawData[0] + rawData[1] + rawData[2] + rawData[3]) & 0xFF) != rawData[4])
		return;
	
	// Convert raw data
	uint16_t temp, hum;
	temp = rawData[2] & 0x7F;
  temp *= 256;
  temp += rawData[3];
	
	hum = rawData[0];
  hum *= 256;
  hum += rawData[1];
	
	// Drop invalid values
	if(hum > 1000)
		return;
	
	osMutexWait(EnvData_Mutex_Id, osWaitForever);
	Ambient.Temp = temp/10.0;
	if (rawData[2] & 0x80)
	{
    Ambient.Temp *= -1;
	}
	Ambient.Hum = hum/10.0;
	osMutexRelease(EnvData_Mutex_Id);
}

static void GPIO_SetPinAsInput(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) 
{
	GPIO_InitTypeDef gpioInit;
	gpioInit.Pin = AM2302_PIN;
	gpioInit.Mode =	GPIO_MODE_INPUT;
	gpioInit.Pull =	GPIO_PULLUP;
	gpioInit.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(AM2302_PORT, &gpioInit);
}

static void GPIO_SetPinAsOutput(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) 
{
	GPIO_InitTypeDef gpioInit;
	gpioInit.Pin = AM2302_PIN;
	gpioInit.Mode =	GPIO_MODE_OUTPUT_PP;
	gpioInit.Pull =	GPIO_PULLUP;
	gpioInit.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(AM2302_PORT, &gpioInit);
}
