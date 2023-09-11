/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-27     Rick       the first version
 */
#include "rtthread.h"
#include "rtdevice.h"
#include "pin_config.h"
#include "led.h"
#include "key.h"
#include "moto.h"
#include "flashwork.h"
#include "status.h"
#include "gateway.h"
#include "radio_encoder.h"
#include "device.h"
#include "work.h"

#define DBG_TAG "moto"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

rt_timer_t Moto1_Timer_Act,Moto2_Timer_Act = RT_NULL;
rt_timer_t Moto1_Timer_Detect,Moto2_Timer_Detect = RT_NULL;
rt_timer_t Moto_Detect_Timer = RT_NULL;

uint8_t Turn1_Flag;
uint8_t Turn2_Flag;
uint8_t Moto1_Fail_FLag;
uint8_t Moto2_Fail_FLag;
uint8_t Valve_Alarm_Flag;

extern uint8_t ValveStatus;
extern enum Device_Status Now_Status;
extern Device_Info Global_Device;

void Moto_InitOpen(uint8_t ActFlag)
{
    LOG_I("Moto Open Init Now is is %d , act is %d\r\n",Global_Device.LastFlag,ActFlag);
    if((Global_Device.LastFlag == OtherOff && ActFlag == OtherOpen)||(Global_Device.LastFlag != OtherOff))
    {
        LOG_D("Moto is Open\r\n");
        Now_Status = Open;
        led_Long_Start(1);//绿灯
        ValveStatus=1;
        Global_Device.LastFlag = ActFlag;
        Flash_Moto_Change(ActFlag);
        rt_pin_write(Turn1,1);
        rt_pin_write(Turn2,1);
    }
    else if(Global_Device.LastFlag == OtherOff && ActFlag == NormalOpen)
    {
        beep_start(0,7);//蜂鸣器三下
        LOG_D("No permissions to Open\r\n");
    }
    else
    {
      beep_start(0,6);//蜂鸣器一下
      LOG_D("No permissions to Open\r\n");
    }
}
void Moto_Open(uint8_t ActFlag)
{
    LOG_I("Moto Open Now is is %d , act is %d\r\n",Global_Device.LastFlag,ActFlag);
    if((Global_Device.LastFlag == OtherOff && ActFlag == OtherOpen)||(Global_Device.LastFlag != OtherOff))
    {
        LOG_D("Moto is Open\r\n");
        Now_Status = Open;
        led_Long_Start(1);//绿灯
        ValveStatus=1;
        Global_Device.LastFlag = ActFlag;
        Flash_Moto_Change(ActFlag);
        rt_pin_write(Turn1,1);
        rt_pin_write(Turn2,1);
        if(ActFlag==NormalOpen)
        {
            ControlUpload_GW(1,0,1,1);
            rt_timer_start(Moto_Detect_Timer);
        }
        ring_once();
        Delay_Timer_Stop();
    }
    else if(Global_Device.LastFlag == OtherOff && ActFlag == NormalOpen)
    {
        beep_start(0,7);//蜂鸣器三下
        LOG_D("No permissions to Open\r\n");
    }
}
void Moto_Close(uint8_t ActFlag)
{
    LOG_I("Moto Close Now is is %d , act is %d\r\n",Global_Device.LastFlag,ActFlag);
    if(Global_Device.LastFlag != OtherOff )
    {
        LOG_D("Moto is Close\r\n");
        Now_Status = Close;
        led_Stop(1);//绿灯
        ValveStatus=0;
        Global_Device.LastFlag = ActFlag;
        Flash_Moto_Change(ActFlag);
        if(ActFlag==NormalOff)
        {
            ControlUpload_GW(1,0,1,0);
        }
        rt_pin_irq_enable(Senor1, PIN_IRQ_DISABLE);
        rt_pin_write(Turn1,0);
        rt_pin_irq_enable(Senor2, PIN_IRQ_DISABLE);
        rt_pin_write(Turn2,0);
        ring_once();
        Delay_Timer_Stop();
        rt_timer_stop(Moto1_Timer_Act);
        rt_timer_stop(Moto2_Timer_Act);
        rt_timer_stop(Moto1_Timer_Detect);
        rt_timer_stop(Moto2_Timer_Detect);
        Key_IO_Init();
        WaterScan_IO_Init();
    }
    else if(Global_Device.LastFlag == OtherOff && ActFlag == OtherOff)
    {
        Now_Status = Close;
        ValveStatus=0;
        ring_once();
        Delay_Timer_Stop();
        LOG_D("Moto is alreay otheroff\r\n");
    }
    else
    {
        beep_start(0,7);//蜂鸣器三下
        LOG_D("No permissions to Off\r\n");
    }
}
void Turn1_Edge_Callback(void *parameter)
{
    LOG_D("Turn1_Edge_Callback\r\n");
    Turn1_Flag ++;
}
void Turn2_Edge_Callback(void *parameter)
{
    LOG_D("Turn2_Edge_Callback\r\n");
    Turn2_Flag ++;
}
uint8_t Get_Moto1_Fail_FLag(void)
{
    return Moto1_Fail_FLag;
}
uint8_t Get_Moto2_Fail_FLag(void)
{
    return Moto2_Fail_FLag;
}
void Turn1_Timer_Callback(void *parameter)
{
    Key_IO_Init();
    WaterScan_IO_Init();
    rt_pin_irq_enable(Senor1, PIN_IRQ_DISABLE);
    rt_pin_write(Turn1,1);
    if(Turn1_Flag<2)
    {
        if(!Moto2_Fail_FLag)
        {
            Warning_Enable_Num(6);
            Valve_Alarm_Flag = 1;
        }
        Moto1_Fail_FLag = 1;
        LOG_E("Moto1 is Fail\r\n");
    }
    else
    {
        if(!Moto2_Fail_FLag)
        {
            WarUpload_GW(1,0,2,0);//MOTO1解除报警
            Valve_Alarm_Flag = 0;
        }
        Moto1_Fail_FLag = 0;
        LOG_D("Moto1 is Good\r\n");
    }
}
void Turn2_Timer_Callback(void *parameter)
{
    Key_IO_Init();
    WaterScan_IO_Init();
    rt_pin_irq_enable(Senor2, PIN_IRQ_DISABLE);
    rt_pin_write(Turn2,1);
    if(Turn2_Flag<2)
    {
        Warning_Enable_Num(9);
        Moto2_Fail_FLag = 1;
        Valve_Alarm_Flag = 1;
        LOG_E("Moto2 is Fail\r\n");
    }
    else
    {
        WarUpload_GW(1,0,2,1);//MOTO2解除报警
        Moto2_Fail_FLag = 0;
        Valve_Alarm_Flag = 0;
        LOG_D("Moto2 is Good\r\n");
    }
}
void Moto1_Timer_Act_Callback(void *parameter)
{
    rt_pin_write(Turn1,1);
    rt_timer_start(Moto1_Timer_Detect);
}
void Moto2_Timer_Act_Callback(void *parameter)
{
    rt_pin_write(Turn2,1);
    rt_timer_start(Moto2_Timer_Detect);
}
void Moto_Detect_Timer_Callback(void *parameter)
{
    LOG_D("Moto_Detect_Timer_Callback\r\n");
    Moto_Detect();
}
void Moto_Init(void)
{
    rt_pin_mode(Senor1,PIN_MODE_INPUT);
    rt_pin_mode(Senor2,PIN_MODE_INPUT);
    rt_pin_mode(Turn1,PIN_MODE_OUTPUT);
    rt_pin_mode(Turn2,PIN_MODE_OUTPUT);
    rt_pin_attach_irq(Senor1, PIN_IRQ_MODE_RISING_FALLING, Turn1_Edge_Callback, RT_NULL);
    rt_pin_attach_irq(Senor2, PIN_IRQ_MODE_RISING_FALLING, Turn2_Edge_Callback, RT_NULL);
    Moto1_Timer_Act = rt_timer_create("Moto1_Timer_Act", Moto1_Timer_Act_Callback, RT_NULL, 5100, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);
    Moto2_Timer_Act = rt_timer_create("Moto2_Timer_Act", Moto2_Timer_Act_Callback, RT_NULL, 5000, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);
    Moto1_Timer_Detect = rt_timer_create("Moto1_Timer_Detect", Turn1_Timer_Callback, RT_NULL, 3000, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);
    Moto2_Timer_Detect = rt_timer_create("Moto2_Timer_Detect", Turn2_Timer_Callback, RT_NULL, 3000, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);
    Moto_Detect_Timer = rt_timer_create("Moto_Detect", Moto_Detect_Timer_Callback, RT_NULL, 60000*5, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);
    if(Flash_Get_SlaveAlarmFlag())
    {
        Warning_Enable_Num(2);
        LOG_I("Moto is Init Fail,Last is Slaver Alarm\r\n");
        return;
    }
    if(Global_Device.LastFlag != OtherOff)
    {
        Moto_InitOpen(NormalOpen);
    }
    LOG_D("Moto is Init Success,Flag is %d\r\n",Global_Device.LastFlag);
}
void Moto_Detect(void)
{
    if(ValveStatus == 1)
    {
        Turn1_Flag = 0;
        Turn2_Flag = 0;
        Moto1_Fail_FLag = 0;
        Moto2_Fail_FLag = 0;
        if(rt_pin_read(Senor1))
        {
            Key_IO_DeInit();
            WaterScan_IO_DeInit();
            rt_pin_irq_enable(Senor1, PIN_IRQ_ENABLE);
            rt_pin_write(Turn1,0);
            rt_timer_start(Moto1_Timer_Act);
        }
        if(rt_pin_read(Senor2))
        {
            Key_IO_DeInit();
            WaterScan_IO_DeInit();
            rt_pin_irq_enable(Senor2, PIN_IRQ_ENABLE);
            rt_pin_write(Turn2,0);
            rt_timer_start(Moto2_Timer_Act);
        }
    }
}
