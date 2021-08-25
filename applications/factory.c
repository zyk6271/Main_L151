/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-08-25     Rick       the first version
 */
#include "rtthread.h"
#include "rtdevice.h"
#include "pin_config.h"
#include "factory.h"

#define DBG_TAG "Factory"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

uint8_t Factory_Flag;
rt_timer_t Factory_Cycle = RT_NULL;
void Factory_Cycle_Callback(void *parameter)
{
    RadioEnqueue(0,1,98989898,1,9,0);
}
void Factory_Init(void)
{
    Factory_Cycle = rt_timer_create("Factory_Cycle",Factory_Cycle_Callback,RT_NULL,2000,RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);
    Start_Factory_Cycle();
}
void DetectFactory(void)
{
    rt_pin_mode(TEST,PIN_MODE_INPUT_PULLUP);
    Factory_Flag = !rt_pin_read(TEST);
    if(Factory_Flag)
    {
        Factory_Init();
    }
    LOG_I("DetectFactory Flag is %d\r\n",Factory_Flag);
}

void Stop_Factory_Cycle(void)
{
    if(Factory_Cycle!=RT_NULL)
    {
        rt_timer_stop(Factory_Cycle);
    }
}
void Start_Factory_Cycle(void)
{
    if(Factory_Cycle!=RT_NULL)
    {
        rt_timer_start(Factory_Cycle);
        LOG_I("Start Factory_Cycle\r\n");
    }
}

