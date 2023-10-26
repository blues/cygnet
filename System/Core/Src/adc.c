// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "main.h"
#include "adc.h"

ADC_HandleTypeDef hadc1;
uint16_t adcGpioPin;
GPIO_TypeDef *adcGpioPort;

// ADC1 function to read a channel (supply NULL for gpioPort for internal channels such as VREFINT)
uint32_t MX_ADC1_ReadChannel(uint32_t adcChannel, uint16_t gpioPin, GPIO_TypeDef *gpioPort)
{

	// Peripheral active
	peripherals |= PERIPHERAL_ADC1;

	// Save for msp init/deinit
	adcGpioPin = gpioPin;
	adcGpioPort = gpioPort;

    // Common config
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = ENABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    hadc1.Init.OversamplingMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    // Start Calibration
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) {
        Error_Handler();
    }

    // Configure Regular Channel
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = adcChannel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_12CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    // Wait for end of conversion
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);

    // Stop conversion, which calls ADC_Disable()
    HAL_ADC_Stop(&hadc1);

    // Read the data
    uint32_t convertedValue = HAL_ADC_GetValue(&hadc1);

    // Deinit
    HAL_ADC_DeInit(&hadc1);

	// Peripheral inactive
	peripherals &= ~PERIPHERAL_ADC1;

	// Done
	return convertedValue;

}

// ADC MSP init function
void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(adcHandle->Instance==ADC1) {

        // Initializes the peripherals clock
        RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
        PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_SYSCLK;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // ADC1 clock enable
        __HAL_RCC_ADC_CLK_ENABLE();

        // ADC1 GPIO Configuration
		if (adcGpioPort != NULL) {
	        GPIO_InitStruct.Pin = adcGpioPin;
	        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	        GPIO_InitStruct.Pull = GPIO_NOPULL;
	        HAL_GPIO_Init(adcGpioPort, &GPIO_InitStruct);
		}

    }
}

// ADC MSP de-init function
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

    if(adcHandle->Instance==ADC1) {

        // Peripheral clock disable
        __HAL_RCC_ADC_CLK_DISABLE();

        // Deinit pins
		if (adcGpioPort != NULL) {
	        HAL_GPIO_DeInit(adcGpioPort, adcGpioPin);
		}

    }
}

// Get the mcu's voltage level in millivolts
uint16_t MX_ADC_GetMcuVoltageMv(void)
{

    uint16_t batteryLevelmV = 0;
    uint32_t measuredLevel = 0;

    measuredLevel = MX_ADC1_ReadChannel(ADC_CHANNEL_VREFINT, 0, NULL);

    if (measuredLevel == 0) {
        batteryLevelmV = 0;
    } else {
        if ((uint32_t)*VREFINT_CAL_ADDR != (uint32_t)0xFFFFU) {
            // Device with Reference voltage calibrated in production:
            // use device optimized parameters
            batteryLevelmV = __LL_ADC_CALC_VREFANALOG_VOLTAGE(measuredLevel, ADC_RESOLUTION_12B);
        } else {
            // Device with Reference voltage not calibrated in production:
            // use generic parameters
            batteryLevelmV = (VREFINT_CAL_VREF * 1510) / measuredLevel;
        }
    }

    return batteryLevelmV;

}

// Get the mcu's temp in degrees centigrade
int16_t MX_ADC_GetMcuTemperatureC(void)
{

    __IO int16_t temperatureDegreeC = 0;
    uint32_t measuredLevel = 0;
    uint16_t batteryLevelmV = MX_ADC_GetMcuVoltageMv();

    measuredLevel = MX_ADC1_ReadChannel(ADC_CHANNEL_TEMPSENSOR, 0, NULL);

    // Convert ADC level to temperature
    // check whether device has temperature sensor calibrated in production
    if (((int32_t)*TEMPSENSOR_CAL2_ADDR - (int32_t)*TEMPSENSOR_CAL1_ADDR) != 0) {
        // Device with temperature sensor calibrated in production:
        // use device optimized parameters
        temperatureDegreeC = __LL_ADC_CALC_TEMPERATURE(batteryLevelmV, measuredLevel, LL_ADC_RESOLUTION_12B);
    } else {
        // Device with temperature sensor not calibrated in production:
        // use generic parameters
        temperatureDegreeC = __LL_ADC_CALC_TEMPERATURE_TYP_PARAMS(TEMPSENSOR_TYP_AVGSLOPE,
                             TEMPSENSOR_TYP_CAL1_V,
                             TEMPSENSOR_CAL1_TEMP,
                             batteryLevelmV,
                             measuredLevel,
                             LL_ADC_RESOLUTION_12B);
    }

    return (int16_t) temperatureDegreeC;

}
