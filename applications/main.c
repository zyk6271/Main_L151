/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-09-01     RT-Thread    first version
 */

#include <rtthread.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#include "drv_common.h"
#include "board.h"

#define MCU_VER "1.2.7"

int main(void)
{
    LOG_I("System Version is %s\r\n",MCU_VER);
    led_Init();
    Key_Reponse();
    flash_Init();
    WarningInit();
    RTC_Init();
    RF_Init();
    Moto_Init();
    ADC_Init();
    Button_Init();
    WaterScan_Init();
    DetectFactory();
    Gateway_Init();
    while (1)
    {
        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}
