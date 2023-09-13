/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-07-01     Rick       the first version
 */
#include "ADCwork.h"
#include "rtthread.h"
#include "rtdevice.h"
#include "status.h"
#include "key.h"
#include "moto.h"
#include "flashwork.h"
#include "gateway.h"
#include <stm32l1xx_hal.h>

#define DBG_TAG "adc"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

extern uint8_t ValveStatus;
extern enum Device_Status Now_Status;

ADC_HandleTypeDef adc_handle;

rt_timer_t ntcscan_timer = RT_NULL;

uint8_t NTC_State = 0;
uint32_t adc_value[20];
uint32_t voltage_temp=0;
double adc_voltage;

static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  adc_handle.Instance = ADC1;
  adc_handle.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  adc_handle.Init.Resolution = ADC_RESOLUTION_12B;
  adc_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  adc_handle.Init.ScanConvMode = ADC_SCAN_DISABLE;
  adc_handle.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  adc_handle.Init.LowPowerAutoWait = ADC_AUTOWAIT_DISABLE;
  adc_handle.Init.LowPowerAutoPowerOff = ADC_AUTOPOWEROFF_DISABLE;
  adc_handle.Init.ChannelsBank = ADC_CHANNELS_BANK_A;
  adc_handle.Init.ContinuousConvMode = ENABLE;
  adc_handle.Init.NbrOfConversion = 1;
  adc_handle.Init.DiscontinuousConvMode = DISABLE;
  adc_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  adc_handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  adc_handle.Init.DMAContinuousRequests = ENABLE;
  if (HAL_ADC_Init(&adc_handle) != HAL_OK)
  {
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_4CYCLES;
  if (HAL_ADC_ConfigChannel(&adc_handle, &sConfig) != HAL_OK)
  {
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

}
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);

}
void ADC1_IRQHandler(void)
{
  /* USER CODE BEGIN ADC1_IRQn 0 */

  /* USER CODE END ADC1_IRQn 0 */
  HAL_ADC_IRQHandler(&adc_handle);
  /* USER CODE BEGIN ADC1_IRQn 1 */

  /* USER CODE END ADC1_IRQn 1 */
}
double ADC_Voltage_Calc(void)
{
    voltage_temp = 0;
    for(uint8_t i=0;i<20;i++)
    {
        voltage_temp += adc_value[i];
    }
    adc_voltage = voltage_temp/20*3.3/4096+0.018;
    return adc_voltage;
}
void NTC_State_Save(uint8_t result)
{
    NTC_State = result;
}
uint8_t NTC_State_read(void)
{
    return NTC_State;
}
void ntc_scan_timer_callback(void *parameter)
{
    if(ADC_Voltage_Calc()<1.153 && Now_Status!=NTCWarning)
    {
        NTC_State_Save(ValveStatus);
        Warning_Enable_Num(8);
    }
    if(ADC_Voltage_Calc()>=1.168 && Now_Status==NTCWarning)
    {
        WarUpload_GW(1,0,8,0);//NTC报警
        Warning_Disable();
        if(NTC_State_read())
        {
            Moto_Open(NormalOpen);
        }
        else
        {
            Moto_Close(NormalOff);
        }
    }
}
void NTC_Init(void)
{
    ntcscan_timer = rt_timer_create("ntc_scan", ntc_scan_timer_callback, RT_NULL, 1000, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    rt_timer_start(ntcscan_timer);
}
void ADC_Init(void)
{
    MX_DMA_Init();
    MX_ADC_Init();
    NTC_Init();
    HAL_ADC_Start_DMA(&adc_handle,(uint32_t*) &adc_value, 20);
}
