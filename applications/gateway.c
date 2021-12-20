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
#include "led.h"
#include "status.h"
#include "moto.h"

#define DBG_TAG "GATEWAY"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

extern Device_Info Global_Device;
extern uint8_t ValveStatus;

rt_timer_t Heart_Check_t = RT_NULL;
rt_timer_t Heart_Test_t = RT_NULL;
rt_timer_t Gateway_Sync_t = RT_NULL;
uint8_t Heart_Flag = 0;
uint8_t Heart_Check_Count = 0;
uint8_t Gateway_Sync_Num = 1;
uint32_t Gateway_ID = 0;

void Gateway_Sync_Callback(void *parameter)
{
    if(Gateway_Sync_Num <= Global_Device.Num)
    {
        if(Global_Device.ID[Gateway_Sync_Num]<40000000)
        {
            if(Global_Device.Alive[Gateway_Sync_Num])
            {
                GatewaySyncEnqueue(1,3,Global_Device.ID[Gateway_Sync_Num],Global_Device.Rssi[Gateway_Sync_Num],Global_Device.Bat[Gateway_Sync_Num]);
            }
            else
            {
                GatewaySyncEnqueue(1,5,Global_Device.ID[Gateway_Sync_Num],Global_Device.Rssi[Gateway_Sync_Num],Global_Device.Bat[Gateway_Sync_Num]);
            }
        }
        Gateway_Sync_Num++;
    }
    else
    {
        Gateway_Sync_Num = 1;
        rt_timer_stop(Gateway_Sync_t);
    }
}
void PowerOn_Upload(void)
{
    switch(GetNowStatus())
    {
    case Open:
        WarUpload_GW(1,0,7,0);
        break;
    case Close:
        WarUpload_GW(1,0,7,0);
        break;
    case MasterLostPeak:
        WarUpload_GW(1,0,3,1);//掉落报警
        break;
    case MasterWaterAlarmActive:
        WarUpload_GW(1,0,1,1);//主控水警
        break;
    case MasterWaterAlarmDeActive:
        WarUpload_GW(1,0,1,1);//主控水警
        break;
    case NTCWarning:
        WarUpload_GW(1,0,8,1);//NTC报警
        break;
    case MotoFail:
        if(Get_Moto1_Fail_FLag())
        {
            WarUpload_GW(1,0,2,2);//MOTO1报警
        }
        if(Get_Moto2_Fail_FLag())
        {
            WarUpload_GW(1,0,2,3);//MOTO2报警
        }
        break;
    default:
        break;
    }
}
void Gateway_Sync(void)
{
    ControlUpload_GW(1,0,5,ValveStatus);
    PowerOn_Upload();
    rt_timer_start(Gateway_Sync_t);
}
void Gateway_RemoteDelete(void)
{
    GatewaySyncEnqueue(1,4,0,0,0);
}
void Heart_Refresh(uint32_t ID)
{
    if(ID == Gateway_ID)
    {
        Heart_Flag = 1;
        LOG_D("Gateway Heart_Refresh\r\n");
    }
}
void Heart_Check(void *parameter)
{
    if(Heart_Flag)
    {
        Heart_Flag = 0;
        wifi_led(1);
        LOG_I("Gateway Heart Check Success\r\n");
    }
    else
    {
        wifi_led(2);
        LOG_W("Gateway Heart Check Fail\r\n");
    }
}
void Heart_Test(void *parameter)
{
    extern uint8_t ValveStatus;
    if(Heart_Flag)
    {
        rt_timer_stop(Heart_Test_t);
        wifi_led(1);
        LOG_I("Gateway Test Check Success\r\n");
        PowerOn_Upload();
    }
    else
    {
        if(Heart_Check_Count<3)
        {
            Heart_Check_Count++;
            wifi_led(0);
            ControlUpload_GW(0,0,5,ValveStatus);
            LOG_W("Gateway Test Check Again %d\r\n",Heart_Check_Count);
        }
        else
        {
            rt_timer_stop(Heart_Test_t);
            wifi_led(2);
            LOG_W("Gateway Test Check Fail\r\n");
        }
    }
}
void Heart_Test_Start(void)
{
    Heart_Check_Count = 0;
    rt_timer_start(Heart_Test_t);
}
void Gateway_Reload(void)
{
    extern uint8_t Gw_Flag;
    Gw_Flag = 1;
    wifi_led(1);
    Gateway_ID = Global_Device.ID[Global_Device.GatewayNum];
    Heart_Refresh(Gateway_ID);
    LOG_I("Gateway_ID is %ld\r\n",Gateway_ID);
}
void Gateway_Init(void)
{
    extern uint8_t ValveStatus;
    Gateway_ID = Global_Device.ID[Global_Device.GatewayNum];
    if(Heart_Test_t == RT_NULL)
    {
        Heart_Test_t = rt_timer_create("Heart_Test", Heart_Test,RT_NULL,5000,RT_TIMER_FLAG_SOFT_TIMER|RT_TIMER_FLAG_PERIODIC);
    }
    if(Heart_Check_t == RT_NULL)
    {
        Heart_Check_t = rt_timer_create("Heart_Check", Heart_Check,RT_NULL,1000*60*20,RT_TIMER_FLAG_SOFT_TIMER|RT_TIMER_FLAG_PERIODIC);
    }
    if(Gateway_Sync_t == RT_NULL)
    {
        Gateway_Sync_t = rt_timer_create("Gateway_Sync", Gateway_Sync_Callback,RT_NULL,2000,RT_TIMER_FLAG_SOFT_TIMER|RT_TIMER_FLAG_PERIODIC);
    }
    if(Gateway_ID==0)
    {
        LOG_W("Gateway_ID is 0\r\n");
        wifi_led(0);
    }
    else
    {
        wifi_led(0);
        LOG_I("Gateway_ID is %ld\r\n",Gateway_ID);
        Heart_Test_Start();
        rt_timer_start(Heart_Check_t);
    }
}
void WarUpload_GW(uint8_t ack,uint32_t device_id,uint8_t warn_id,uint8_t value)
{
    if(GetGatewayID())
    {
        GatewayWarningEnqueue(ack,device_id,Flash_GetRssi(device_id),warn_id,value);
    }
}
void ControlUpload_GW(uint8_t ack,uint32_t device_id,uint8_t control_id,uint8_t value)
{
    if(GetGatewayID())
    {
        GatewayControlEnqueue(ack,device_id,Flash_GetRssi(device_id),control_id,value);
    }
}
void Replace_Door(uint32_t old)
{
    GatewayWarningEnqueue(1,old,Flash_GetRssi(old),4,0);
}
