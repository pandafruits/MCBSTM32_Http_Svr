#include "stm32f4xx_hal.h"
#include "cmsis_os.h" 
#include "Core_Temp.h"

#define ADC_RESOLUTION        12        /* Number of A/D converter bits       */

float CoreTemperature;

static ADC_HandleTypeDef hadc1;
static volatile uint8_t  AD_done;       /* AD conversion done flag            */

static void ADC_StartConversion (void);
static int8_t ADC_ConversionDone (void);
static int32_t ADC_GetValue (void);

extern osMutexId(EnvData_Mutex_Id);

/// Initialize internal temperature sensor
void CoreTemp_Init (void) {
  ADC_ChannelConfTypeDef sConfig;

  /* Peripheral clock enable */
  __HAL_RCC_ADC1_CLK_ENABLE();

  /* Configure the global features of the ADC
    (Clock, Resolution, Data Alignment and number of conversion) */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.NbrOfDiscConversion = 1;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = EOC_SINGLE_CONV;
  HAL_ADC_Init(&hadc1);

  /* Configure the selected ADC channel */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  AD_done = 0;
}

/// Get temperature as degC
float CoreTemp_Read(void) {
  uint16_t raw_adc_reading;
	float sensor_millivolt;

  ADC_StartConversion();
  while (ADC_ConversionDone () < 0);
  raw_adc_reading = ADC_GetValue();

  /* Convert from raw reading to degC. 
	(Reference Manual pg.411 & Datasheet pg.129). */
	sensor_millivolt = ( (float)raw_adc_reading/4095 ) * 3.3F * 1000;
	osMutexWait(EnvData_Mutex_Id, osWaitForever);
	CoreTemperature = 25 + ( sensor_millivolt - 760 ) / 2.5F;
	osMutexRelease(EnvData_Mutex_Id);
	
	return CoreTemperature;
}

/// Start temp sensor adc conversion
static void ADC_StartConversion (void) {
  __HAL_ADC_CLEAR_FLAG(&hadc1, ADC_FLAG_EOC);
  HAL_ADC_Start(&hadc1);

  AD_done = 0;
}

/// Check if temp sensor adc conversion done
static int8_t ADC_ConversionDone (void) {
  HAL_StatusTypeDef status;

  status = HAL_ADC_PollForConversion(&hadc1, 0);
  if (status == HAL_OK) {
    AD_done = 1;
    return 0;
  } else {
    AD_done = 0;
    return -1;
  }
}

/// Read temp sensor adc value
static int32_t ADC_GetValue (void) {
  HAL_StatusTypeDef status;

  if (AD_done == 0) {
    status = HAL_ADC_PollForConversion(&hadc1, 0);
    if (status != HAL_OK) return -1;
  } else {
    AD_done = 0;
  }

  return HAL_ADC_GetValue(&hadc1);
}
