#include <rtthread.h>
#include <rtdevice.h>
#include "pin_config.h"
#include "RTCWork.h"
#include "Flashwork.h"
#include "moto.h"
#include "stm32l1xx.h"

#define DBG_TAG "RTC"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

uint8_t RTC_Counter=0;
uint32_t RTC_Hours = 0;

rt_sem_t RTC_IRQ_Sem;
rt_thread_t RTC_Scan = RT_NULL;
RTC_HandleTypeDef RtcHandle;
void RTC_Timer_Entry(void *parameter)
{
    while(1)
    {
        static rt_err_t result;
        result = rt_sem_take(RTC_IRQ_Sem, RT_WAITING_FOREVER);
        if (result == RT_EOK)
        {
            if(RTC_Hours%120==0)
            {
                Moto_Detect();
            }
            if(RTC_Counter<24)
            {
                Update_All_Time();//24小时更新全部时间
                RTC_Counter++;
            }
            else
            {
                Update_All_Time();//24小时更新全部时间
                Detect_All_Time();//25个小时检测计数器
                RTC_Counter=0;
            }
            LOG_D("Device RTC Detect,Hour is %d\r\n",RTC_Counter);
        }
    }
}
void RTC_AlarmConfig(void)
{
    RTC_DateTypeDef  sdatestructure;
    RTC_TimeTypeDef  stimestructure;
    RTC_AlarmTypeDef salarmstructure;

    sdatestructure.Year = 0x14;
    sdatestructure.Month = RTC_MONTH_FEBRUARY;
    sdatestructure.Date = 0x18;
    sdatestructure.WeekDay = RTC_WEEKDAY_TUESDAY;

    if(HAL_RTC_SetDate(&RtcHandle,&sdatestructure,RTC_FORMAT_BIN) != HAL_OK)
    {
      /* Initialization Error */
    }

    stimestructure.Hours = 0;
    stimestructure.Minutes = 0;
    stimestructure.Seconds = 0;
    stimestructure.TimeFormat = RTC_HOURFORMAT12_AM;
    stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE ;
    stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;

    if(HAL_RTC_SetTime(&RtcHandle,&stimestructure,RTC_FORMAT_BIN) != HAL_OK)
    {
      /* Initialization Error */
    }

    salarmstructure.Alarm = RTC_ALARM_A;
    salarmstructure.AlarmDateWeekDay = RTC_WEEKDAY_MONDAY;
    salarmstructure.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY;
    salarmstructure.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;
    salarmstructure.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    salarmstructure.AlarmTime.TimeFormat = RTC_HOURFORMAT12_AM;
    salarmstructure.AlarmTime.Hours = 1;
    salarmstructure.AlarmTime.Minutes = 0;
    salarmstructure.AlarmTime.Seconds = 0;
    salarmstructure.AlarmTime.SubSeconds = 0;

    if(HAL_RTC_SetAlarm_IT(&RtcHandle,&salarmstructure,RTC_FORMAT_BIN) == HAL_OK)
    {
        LOG_D("RTC Alarm Set Ok\r\n");
    }
}
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *RtcHandle)
{
    RTC_TimeTypeDef sTime = {0};
    sTime.Hours = 0;
    sTime.Minutes = 0;
    sTime.Seconds = 0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    HAL_RTC_SetTime(RtcHandle, &sTime, RTC_FORMAT_BIN);

    rt_sem_release(RTC_IRQ_Sem);
    RTC_Hours++;
}
void RTC_Init(void)
{
    __HAL_RCC_RTC_ENABLE();
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

    RTC_IRQ_Sem = rt_sem_create("RTC_IRQ", 0, RT_IPC_FLAG_FIFO);
    RTC_Scan = rt_thread_create("RTC_Scan", RTC_Timer_Entry, RT_NULL, 2048, 5, 5);
    if(RTC_Scan!=RT_NULL)
    {
        rt_thread_startup(RTC_Scan);
    }
    else
    {
        LOG_W("RTC Init Fail\r\n");
    }
    RtcHandle.Instance = RTC;
    RtcHandle.Init.HourFormat = RTC_HOURFORMAT_24;
    RtcHandle.Init.AsynchPrediv = 127;
    RtcHandle.Init.SynchPrediv = 255;
    RtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
    RtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    RtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    if (HAL_RTC_Init(&RtcHandle) != HAL_OK)
    {
    }
    RTC_AlarmConfig();
}
void RTC_Alarm_IRQHandler(void)
{
    HAL_RTC_AlarmIRQHandler(&RtcHandle);
}
