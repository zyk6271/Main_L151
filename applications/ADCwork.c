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
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

extern uint8_t ValveStatus;
extern enum Device_Status Now_Status;

ADC_HandleTypeDef hadc;
rt_thread_t ntc_work = RT_NULL;
rt_sem_t ADC_Convert_Done = RT_NULL;

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
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc.Init.LowPowerAutoWait = ADC_AUTOWAIT_DISABLE;
  hadc.Init.LowPowerAutoPowerOff = ADC_AUTOPOWEROFF_DISABLE;
  hadc.Init.ChannelsBank = ADC_CHANNELS_BANK_A;
  hadc.Init.ContinuousConvMode = ENABLE;
  hadc.Init.NbrOfConversion = 1;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = ENABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_4CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
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
  HAL_ADC_IRQHandler(&hadc);
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
    //printf("voltage is %1.3f\r\n",adc_voltage);
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
void NTC_Work_Callback(void *parameter)
{
    LOG_D("NTC With ADC is Init Success\r\n");
    while(1)
    {
        if(ADC_Voltage_Calc()<1.1 && Now_Status!=NTCWarning)
        {
            NTC_State_Save(ValveStatus);
            Warning_Enable_Num(8);
            WarUpload_GW(0,8,1);//NTC报警
        }
        if(ADC_Voltage_Calc()>=1.2 && Now_Status==NTCWarning)
        {
            WarUpload_GW(0,8,0);//NTC报警
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
        rt_thread_mdelay(100);
    }
}
void NTC_Init(void)
{
    ADC_Convert_Done = rt_sem_create("ADC_Convert_Done",0, RT_IPC_FLAG_FIFO);
    ntc_work = rt_thread_create("ntc_work", NTC_Work_Callback, RT_NULL, 2048, 15, 10);
    if(ntc_work != RT_NULL);rt_thread_startup(ntc_work);
}
void ADC_Init(void)
{
    MX_DMA_Init();
    MX_ADC_Init();
    NTC_Init();
    HAL_ADC_Start_DMA(&hadc,(uint32_t*) &adc_value, 20);
}
