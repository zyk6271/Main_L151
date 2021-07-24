/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-07-14     Rick       the first version
 */
#include "rtthread.h"
#include "radio_encoder.h"
#include "radio_decoder.h"
#include "gateway.h"
#include "flashwork.h"

#define DBG_TAG "GATEWAY"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

extern Device_Info Global_Device;

uint32_t Gateway_ID = 0;
void Gateway_Sync(void)
{
    for(uint8_t i = 1;i<Global_Device.Num;i++)
    {
        if(Global_Device.ID[i]<40000000)
        {
            GatewaySyncEnqueue(3,Global_Device.ID[i],Global_Device.Rssi[i],Global_Device.Bat[i]);
        }
    }
}
void Gateway_Init(void)
{
    Gateway_ID = Global_Device.ID[Global_Device.GatewayNum];
    if(Gateway_ID==0)
    {
        LOG_W("Gateway_ID is 0\r\n");
    }
    else
    {
        LOG_I("Gateway_ID is %ld\r\n",Gateway_ID);
    }
}
void WarUpload_GW(uint32_t device_id,uint8_t warn_id,uint8_t value)
{
    if(GetGatewayID())
    {
        GatewayWarningEnqueue(device_id,Flash_GetRssi(device_id),warn_id,value);
    }
}
void ControlUpload_GW(uint32_t device_id,uint8_t control_id,uint8_t value)
{
    if(GetGatewayID())
    {
        GatewayControlEnqueue(device_id,Flash_GetRssi(device_id),control_id,value);
    }
}
