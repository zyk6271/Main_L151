/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-22     Rick       the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include "drv_spi.h"
#include <string.h>
#include "AX5043.h"
#include "Radio_Config.h"
#include "Radio.h"
#include "Radio_Decoder.h"
#include "Radio_Encoder.h"
#include "stdio.h"
#include "work.h"
#include "Flashwork.h"
#include "status.h"
#include "moto.h"
#include "led.h"
#include "key.h"
#include "pin_config.h"
#include "gateway.h"

#define DBG_TAG "radio_decoder"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

typedef struct
{
    long Target_ID;
    long From_ID;
    int Counter;
    int Command ;
    int Data;
    int Rssi;
}Message;

uint8_t Learn_Flag=0;
uint8_t Last_Close_Flag=0;

extern rt_timer_t Learn_Timer;
extern uint8_t ValveStatus ;
extern uint32_t Self_Id;
extern enum Device_Status Now_Status;

uint8_t Recv_Num;
rt_timer_t Factory_Test_Timer = RT_NULL;
void Factory_Test_Timer_Callback(void *parameter)
{
    if(Recv_Num>7)
    {
        led_on(1);
        LOG_D("Test is Success,Reselt is %d\r\n",Recv_Num);
    }
    else
    {
        led_on(0);
        LOG_D("Test is Failed,Reselt is %d\r\n",Recv_Num);
    }
}
uint8_t Factory_Detect(void)
{
    rt_pin_mode(TEST, PIN_MODE_INPUT_PULLUP);
    if(rt_pin_read(TEST)==0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
void Factory_Test(void)
{
    Recv_Num = 0;
    Factory_Test_Timer = rt_timer_create("Factory_Test", Factory_Test_Timer_Callback, RT_NULL, 1000, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);
    rt_timer_start(Factory_Test_Timer);
    LOG_D("Start Test\r\n");
}
uint8_t Check_Valid(uint32_t From_id)
{
    return Flash_Get_Key_Valid(From_id);
}
void Start_Learn_Key(void)
{
    Now_Status = Learn;
    Learn_Flag = 1;
    beep_start(0, 4);
    rt_timer_start(Learn_Timer);
    LOG_D("Learn timer is start\r\n");
}
void Start_Learn(void)
{
    if(Now_Status==Close||Now_Status==Open)
    {
        Now_Status = Learn;
        Learn_Flag = 1;
        beep_start(0, 4);
        rt_timer_start(Learn_Timer);
        LOG_D("Learn timer is start\r\n");
    }
    else
    {
        LOG_D("Now in Warining Mode\r\n");
    }
}
void Stop_Learn(void)
{
    Learn_Flag = 0;
    rt_timer_stop(Learn_Timer);
    Warning_Disable();//消警
    beep_start(0, 6);
    //if(ValveStatus)Moto_Open(NormalOpen);else Moto_Close(NormalOff);
    LOG_D("Learn timer is stop\r\n");
}
void Device_Learn(Message buf)
{
    switch(buf.Data)
    {
    case 1:
        if(Learn_Flag)
        {
            RadioEnqueue(0,buf.From_ID,buf.Counter,3,1);
            if(Check_Valid(buf.From_ID))//如果数据库不存在该值
            {
                LOG_D("Not Include This Device\r\n");
                if(buf.From_ID<30000000)
                {
                    Add_Device(buf.From_ID);//向数据库写入
                    LOG_I("Slaver Write to Flash With ID %d\r\n",buf.From_ID);
                }
                else if(buf.From_ID>=30000000 && buf.From_ID<40000000)
                {
                    Add_DoorDevice(buf.From_ID);//向数据库写入
                    LOG_I("Door Write to Flash With ID %d\r\n",buf.From_ID);
                }
                else if(buf.From_ID>=40000000 && buf.From_ID<50000000)
                {
                    Add_GatewayDevice(buf.From_ID);//向数据库写入
                    Gateway_Init();
                    LOG_I("Gateway Write to Flash With ID %d\r\n",buf.From_ID);
                }
            }
            else//存在该值
            {
                LOG_I("Include This Device，Send Ack\r\n");
            }
        }
        else LOG_W("Not in Learn Mode\r\n");
        break;
    case 2:
        if(Learn_Flag)
        {
            if(Check_Valid(buf.From_ID))//如果数据库不存在该值
            {
                LOG_W("Ack Not Include This Device\r\n");
            }
            else//存在该值
            {
                LOG_D("Include This Device，Send Confirmed\r\n");
                just_ring();    //响一声
                Relearn();
                RadioEnqueue(0,buf.From_ID,buf.Counter,3,2);
                GatewaySyncEnqueue(3,buf.From_ID,0,0);
            }
        }
        else LOG_W("Not in Learn Mode\r\n");
        break;
    case 3:
        if(!Check_Valid(buf.From_ID))//如果数据库不存在该值
        {
            Start_Learn();
            RadioEnqueue(0,buf.From_ID,buf.Counter,3,3);
        }
        else LOG_W("Unknown Device Want to Learn\r\n");
        break;
    }
    rt_timer_start(Learn_Timer);
}

void DataSolve(Message buf)
{
    switch(buf.Command)
    {
    case 1://测试模拟报警（RESET）
        RadioEnqueue(0,buf.From_ID,buf.Counter,1,1);
        LOG_D("Test\r\n");
        break;
    case 2://握手包
        LOG_D("HandShake\r\n");
        if(buf.Data==2)
        {
            if(buf.From_ID!=GetDoorID())
            {
                RadioEnqueue(0,buf.From_ID,buf.Counter,2,2);
                Warning_Enable_Num(1);
                WarUpload_GW(buf.From_ID,6,2);//终端低电量报警
            }
            else
            {
                RadioEnqueue(0,buf.From_ID,buf.Counter,2,2);
            }
        }
        else if(buf.Data==1)
        {
            RadioEnqueue(0,buf.From_ID,buf.Counter,2,1);
            Warning_Enable_Num(7);
            WarUpload_GW(buf.From_ID,6,1);//终端低电量报警
        }
        else
        {
            Update_Device_Bat(buf.From_ID,buf.Data);//写入电量
            RadioEnqueue(0,buf.From_ID,buf.Counter,2,0);
        }
        LOG_D("Handshake With %ld\r\n",buf.From_ID);
        break;
    case 3://学习
        if(Learn_Flag||buf.Data==3)
        {
            LOG_D("Learn\r\n");
            Device_Learn(buf);
        }
        else
        {
            LOG_D("LearnFlag is Zero\r\n");
        }
        break;
    case 4://报警
        LOG_D("Warning From %ld\r\n",buf.From_ID);
        if(buf.Data==0)
        {
            LOG_D("Warning With Command 4 Data 0\r\n");
            if(GetDoorID()==buf.From_ID)//是否为主控的回包
            {
                LOG_D("RECV 40 From Door\r\n");
            }
            else//是否为来自终端的数据
            {
                RadioEnqueue(0,buf.From_ID,buf.Counter,4,0);
                WarUpload_GW(buf.From_ID,5,1);//终端消除水警
            }
        }
        else if(buf.Data==1)
        {
            LOG_D("Warning With Command 4 Data 1\r\n");
            if(GetDoorID()==buf.From_ID)//是否为主控的回包
            {
                LOG_D("recv 41 from door\r\n");
            }
            else//是否为来自终端的报警包
            {
                RadioEnqueue(0,buf.From_ID,buf.Counter,4,1);
                if(Now_Status!=SlaverWaterAlarmActive)
                {
                    Warning_Enable_Num(2);
                    WarUpload_GW(buf.From_ID,5,0);//终端水警
                    LOG_D("SlaverWaterAlarm is Active\r\n");
                }
            }
        }
        break;
    case 5://开机
        if((Now_Status==Open||Now_Status==Close))
        {
            LOG_D("Pwr On From %ld\r\n",buf.From_ID);
            RadioEnqueue(0,buf.From_ID,buf.Counter,5,1);
            Moto_Open(OtherOpen);
            Delay_Timer_Close();
            Last_Close_Flag=0;
            just_ring();
        }
        else
        {
            LOG_D("Pwr On From %ld On Warning\r\n",buf.From_ID);
            RadioEnqueue(0,buf.From_ID,buf.Counter,5,2);
        }
        ControlUpload_GW(buf.From_ID,2,ValveStatus);
        break;
    case 6://关机
        RadioEnqueue(0,buf.From_ID,buf.Counter,6,0);
        if(Now_Status==Open||Now_Status==Close)
        {
            LOG_D("Pwr Off From %ld\r\n",buf.From_ID);
            Warning_Disable();
            Last_Close_Flag=1;
            Moto_Close(OtherOff);
            just_ring();
        }
        else if(Now_Status == SlaverWaterAlarmActive)
        {
            LOG_D("Warning With Command 6\r\n");
            Warning_Disable();
            Moto_Close(OtherOff);
        }
        ControlUpload_GW(buf.From_ID,2,ValveStatus);
        break;
    case 8://延迟
        LOG_I("Delay Open %d From %ld\r\n",buf.Data,buf.From_ID);
        if(buf.From_ID==GetDoorID())
        {
            RadioEnqueue(0,buf.From_ID,buf.Counter,8,buf.Data);
        }
        if(buf.Data)
        {
            Delay_Timer_Close();
            ControlUpload_GW(buf.From_ID,3,0);
        }
        else
        {
            Delay_Timer_Open();
            ControlUpload_GW(buf.From_ID,3,1);
        }
        break;
    case 9://网关开
        if(buf.Data)
        {
            LOG_I("Gateway Open\r\n");
            just_ring();
            Moto_Open(OtherOpen);
            ControlUpload_GW(buf.From_ID,2,ValveStatus);
        }
        else
        {
            LOG_I("Gateway Close\r\n");
            just_ring();
            Moto_Close(OtherOff);
            ControlUpload_GW(buf.From_ID,2,ValveStatus);
        }
        break;
    case 10://请求心跳
        LOG_I("Request Heart\r\n");
        ControlUpload_GW(0,4,0);
        break;
    case 11://请求同步
        LOG_I("Request Sync\r\n");
        Gateway_Sync();
        break;
    }
    if(buf.Counter==0)
    {
        ChangeMaxPower();
    }
    else
    {
        BackNormalPower();
    }
    Offline_React(buf.From_ID);
}
void Rx_Done_Callback(uint8_t *rx_buffer,uint8_t rx_len,int8_t rssi)
{
    Message Rx_message;
    if(rx_buffer[rx_len-1]==0x0A&&rx_buffer[rx_len-2]==0x0D)
    {
        LOG_D("Rx verify ok\r\n");
        rx_buffer[rx_len-1]=0;
        rx_buffer[rx_len-2]=0;
        sscanf((const char *)&rx_buffer[1],"{%ld,%ld,%d,%d,%d}",&Rx_message.Target_ID,&Rx_message.From_ID,&Rx_message.Counter,&Rx_message.Command,&Rx_message.Data);
        if(Check_Valid(Rx_message.From_ID)!=RT_EOK && Learn_Flag != RT_TRUE)
        {
            LOG_D("Device_ID %ld is not include\r\n",Rx_message.From_ID);
            return;
        }
        if(Rx_message.Target_ID==Self_Id ||Rx_message.Target_ID==99999999)
        {
            Update_Device_Rssi(Rx_message.From_ID,rssi);
            DataSolve(Rx_message);
        }
    }
    else
    {
        LOG_D("Rx verify Fail\r\n");
    }
}
