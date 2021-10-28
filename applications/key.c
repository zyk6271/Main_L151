/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-25     Rick       the first version
 */
#include <rtthread.h>
#include "pin_config.h"
#include "key.h"
#include "led.h"
#include "moto.h"
#include "Radio_Decoder.h"
#include "Radio_encoder.h"
#include "work.h"
#include "status.h"
#include "flashwork.h"
#include "rthw.h"
#include "status.h"
#include "device.h"
#include "gateway.h"
#include "factory.h"

#define DBG_TAG "key"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

rt_thread_t key_response_t = RT_NULL;
rt_timer_t Learn_Timer = RT_NULL;

uint8_t K0_Status=0;
uint8_t K0_Long_Status=0;
uint8_t K1_Status=0;
uint8_t K1_Long_Status=0;
uint8_t K0_K1_Status=0;
uint8_t ValveStatus = 0;

extern rt_sem_t K0_Sem;
extern rt_sem_t K0_Long_Sem;
extern rt_sem_t K1_Sem;
extern rt_sem_t K1_Long_Sem;
extern rt_sem_t K0_K1_Long_Sem;

extern uint8_t Learn_Flag;
extern uint8_t Last_Close_Flag;

extern enum Device_Status Now_Status;
extern uint8_t Factory_Flag;
void release_k0(void)
{
    if(K0_Sem != RT_NULL)
    {
        rt_sem_release(K0_Sem);
    }
}
void release_k1(void)
{
    if(K1_Sem != RT_NULL)
    {
        rt_sem_release(K1_Sem);
    }
}
void Key_Reponse_Callback(void *parameter)
{
    Key_SemInit();
    LOG_D("Key_Reponse Init Success\r\n");
    while(1)
    {
        K0_Status = rt_sem_take(K0_Sem, 0);
        K0_Long_Status = rt_sem_take(K0_Long_Sem, 0);
        K1_Status = rt_sem_take(K1_Sem, 0);
        K1_Long_Status = rt_sem_take(K1_Long_Sem, 0);
        K0_K1_Status = rt_sem_take(K0_K1_Long_Sem, 0);
        if(K0_Status==RT_EOK)//ON
        {
            switch(Now_Status)
            {
            case Close:
                if(Last_Close_Flag==0)
                {
                    just_ring();
                    Moto_Open(NormalOpen);
                }
                else
                {
                    beep_start(0,7);//蜂鸣器三下
                }
                LOG_D("Valve Open With ON\r\n");
                break;
            case Open:
                just_ring();
                LOG_D("Valve Already Open With ON\r\n");
                break;
            case SlaverLowPower:
                break;
            case SlaverUltraLowPower:
                beep_three_times();
                break;
            case SlaverWaterAlarmActive:
                break;
            case MasterLostPeak:
                key_down();
                Now_Status = Open;
                Moto_Open(NormalOpen);
                LOG_D("MasterLostPeak With ON\r\n");
                break;
            case MasterWaterAlarmActive:
                beep_three_times();
                break;
            case MasterWaterAlarmDeActive:
                beep_three_times();
                LOG_D("MasterWaterAlarmActive With ON\r\n");
                break;
            case MotoFail:
                just_ring();
                break;
            case Learn:
                break;
            case Offline:
                break;
            case NTCWarning:
                break;
            }
        }
        else if(K1_Status==RT_EOK)//OFF
        {
            if(Factory_Flag)
            {
                Stop_Factory_Cycle();
                Warning_Disable();
                Moto_Detect();
            }
            else
            {
                switch(Now_Status)
                {
                case Close:
                    if(Last_Close_Flag==0)
                    {
                        key_down();
                    }
                    else
                    {
                        beep_start(0,7);//蜂鸣器三下
                    }
                    LOG_D("Valve Already Close With OFF\r\n");
                    break;
                case Open:
                    key_down();
                    Last_Close_Flag = 0;
                    Moto_Close(NormalOff);
                    LOG_D("Valve Close With OFF\r\n");
                    break;
                case SlaverLowPower:
                    break;
                case SlaverUltraLowPower:
                    just_ring();
                    break;
                case SlaverWaterAlarmActive:
                    beep_stop();
                    break;
                case MasterLostPeak:
                    key_down();
                    Moto_Close(NormalOff);
                    beep_stop();
                    Now_Status = Close;
                    LOG_D("MasterLostPeak With OFF\r\n");
                    break;
                case MasterWaterAlarmActive:
                    beep_stop();
                    break;
                case MasterWaterAlarmDeActive:
                    key_down();
                    Now_Status = Close;
                    Warning_Disable();
                    WarUpload_GW(1,0,1,0);//主控消除水警
                    LOG_D("MasterWaterAlarmActive With OFF\r\n");
                    break;
                case Learn:
                    break;
                case MotoFail:
                    key_down();
                    LOG_D("MotoFail With OFF\r\n");
                    break;
                case Offline:
                    break;
                case NTCWarning:
                    beep_stop();
                    key_down();
                    break;
                }
            }
        }
        else if(K0_K1_Status==RT_EOK)
        {
            DeleteAllDevice();
            beep_start(0,8);//蜂鸣器5次
            rt_thread_mdelay(3000);
            rt_hw_cpu_reset();
        }
        else if(K0_Long_Status==RT_EOK)//ON
        {
        }
        else if(K1_Long_Status==RT_EOK)//OFF
        {
            if(Now_Status==Close||Now_Status==Open)
            {
                Now_Status = Learn;
                Start_Learn_Key();
            }
            else if(Now_Status==Learn)
            {
                rt_timer_stop(Learn_Timer);
                Stop_Learn();
            }
            else
            {
                LOG_D("Now in Warining Mode\r\n");
            }
        }
        rt_thread_mdelay(10);
    }
}
void Learn_Timer_Callback(void *parameter)
{
    LOG_D("Learn timer is Timeout\r\n");
    Stop_Learn();
}
void Key_Reponse(void)
{
    key_response_t = rt_thread_create("key_response_t", Key_Reponse_Callback, RT_NULL, 2048, 10, 10);
    if(key_response_t!=RT_NULL)rt_thread_startup(key_response_t);
    Learn_Timer = rt_timer_create("Learn_Timer", Learn_Timer_Callback, RT_NULL, 30*1000, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER );
}
