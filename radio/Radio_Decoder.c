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
#include "Radio_Common.h"

#define DBG_TAG "RADIO_DECODER"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

uint8_t Learn_Flag=0;
uint8_t Last_Close_Flag=0;
uint8_t Gw_Flag = 0;
uint8_t Recv_Num;

extern rt_timer_t Learn_Timer;
extern uint8_t ValveStatus ;
extern uint32_t RadioID;

extern enum Device_Status Now_Status;
extern struct ax5043 rf_433;

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
        LOG_W("Start_Learn fail,because in warining mode\r\n");
    }
}

void Stop_Learn(void)
{
    Learn_Flag = 0;
    Warning_Disable();//消警
    beep_start(0, 6);
    LOG_D("Learn timer is stop\r\n");
    if(Gw_Flag)
    {
        Gw_Flag = 0;
        Gateway_Sync();
    }
}

void Device_Learn(Message buf)
{
    if(buf.Command == 3)
    {
        switch(buf.Data)
        {
        case 1:
            if(Check_Valid(buf.From_ID))//如果数据库不存在该值
            {
                LOG_D("Not Include This Device\r\n");
                if(buf.From_ID<30000000)
                {
                    if(Add_Device(buf.From_ID)==RT_EOK)
                    {
                        LOG_D("Slaver Write to Flash With ID %d\r\n",buf.From_ID);
                        RadioEnqueue(buf.From_ID,buf.Counter,3,1);
                    }
                    else
                    {
                        LOG_W("Slaver Write to Flash Fail With ID %d\r\n",buf.From_ID);
                        learn_fail_ring();
                    }
                }
                else if(buf.From_ID>=30000000 && buf.From_ID<40000000)
                {
                    if(Add_DoorDevice(buf.From_ID)==RT_EOK)
                    {
                        LOG_D("Door Write to Flash With ID %d\r\n",buf.From_ID);
                        RadioEnqueue(buf.From_ID,buf.Counter,3,1);
                    }
                    else
                    {
                        LOG_W("Door Write to Flash Fail With ID %d\r\n",buf.From_ID);
                        learn_fail_ring();
                    }
                }
                else if(buf.From_ID>=40000000 && buf.From_ID<50000000)
                {
                    if(Add_GatewayDevice(buf.From_ID)==RT_EOK)
                    {
                        LOG_D("Gateway Write to Flash With ID %d\r\n",buf.From_ID);
                        RadioEnqueue(buf.From_ID,buf.Counter,3,1);
                    }
                    else
                    {
                        LOG_W("Gateway Write to Flash Fail With ID %d\r\n",buf.From_ID);
                        learn_fail_ring();
                    }
                }
            }
            else//存在该值
            {
                RadioEnqueue(buf.From_ID,buf.Counter,3,1);
                LOG_D("Include This Device，Send Ack\r\n");
            }
            break;
        case 2:
            if(Check_Valid(buf.From_ID))//如果数据库不存在该值
            {
                LOG_W("Ack Not Include This Device\r\n");
            }
            else//存在该值
            {
                if(buf.From_ID>=40000000 && buf.From_ID<50000000)
                {
                    Gateway_Reload();
                }
                LOG_D("Include This Device，Send Confirmed\r\n");
                ring_once();    //响一声
                Device_AliveChange(buf.From_ID,1);
                Relearn();
                RadioEnqueue(buf.From_ID,buf.Counter,3,2);
                GatewaySyncEnqueue(0,6,buf.From_ID,Flash_GetRssi(buf.From_ID),0);
            }
            break;
        }
        rt_timer_start(Learn_Timer);
    }
}
void DataSolve(Message buf)
{
    switch(buf.Command)
    {
    case 1://测试模拟报警（RESET）
        RadioEnqueue(buf.From_ID,buf.Counter,1,1);
        LOG_D("Test\r\n");
        break;
    case 2://握手包
        Update_Device_Bat(buf.From_ID,buf.Data);//写入电量
        switch(buf.Data)
        {
        case 0:
            RadioEnqueue(buf.From_ID,buf.Counter,2,0);
            WarUpload_GW(1,buf.From_ID,6,0);//终端低电量报警
            break;
        case 1:
            RadioEnqueue(buf.From_ID,buf.Counter,2,1);
            WarUpload_GW(1,buf.From_ID,6,1);//终端低电量报警
            if(buf.From_ID!=GetDoorID())
            {
                Warning_Enable_Num(7);
            }
            break;
        case 2:
            RadioEnqueue(buf.From_ID,buf.Counter,2,2);
            WarUpload_GW(1,buf.From_ID,6,2);//终端低电量报警
            if(buf.From_ID!=GetDoorID())
            {
                Warning_Enable_Num(1);
            }
            break;
        case 3:
            RadioEnqueue(buf.From_ID,buf.Counter,2,3);
            WarUpload_GW(1,buf.From_ID,6,3);//终端低电量报警
            break;
        case 4:
            RadioEnqueue(buf.From_ID,buf.Counter,2,4);
            WarUpload_GW(1,buf.From_ID,6,4);//终端低电量报警
            break;
        }
        LOG_D("Handshake With %ld\r\n",buf.From_ID);
        break;
    case 3://学习
        if(buf.Data==3)
        {
            if(!Check_Valid(buf.From_ID))//如果数据库不存在该值
            {
                Start_Learn();
                RadioEnqueue(buf.From_ID,buf.Counter,3,3);
            }
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
                RadioEnqueue(buf.From_ID,buf.Counter,4,0);
                WarUpload_GW(1,buf.From_ID,5,0);//终端消除水警
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
                RadioEnqueue(buf.From_ID,buf.Counter,4,1);
                WarUpload_GW(1,buf.From_ID,5,1);//终端水警
                Flash_Set_SlaveAlarmFlag(buf.From_ID,1);
                if(Now_Status!=SlaverWaterAlarmActive)
                {
                    Warning_Enable_Num(2);
                    LOG_D("SlaverWaterAlarm is Active\r\n");
                }
            }
        }
        break;
    case 5://开机
        if((Now_Status==Open||Now_Status==Close))
        {
            LOG_D("Remote valve open from %ld\r\n",buf.From_ID);
            RadioEnqueue(buf.From_ID,buf.Counter,5,1);
            Moto_Open(OtherOpen);
            Last_Close_Flag=0;
            ring_once();
        }
        else
        {
            LOG_D("Remote valve open from %ld fail because device is warning\r\n",buf.From_ID);
            RadioEnqueue(buf.From_ID,buf.Counter,5,2);
        }
        if(buf.From_ID == GetDoorID())
        {
            ControlUpload_GW(1,buf.From_ID,6,ValveStatus);
        }
        else
        {
            ControlUpload_GW(1,buf.From_ID,2,ValveStatus);
        }
        break;
    case 6://关机
        RadioEnqueue(buf.From_ID,buf.Counter,6,0);
        if(Flash_Get_SlaveAlarmFlag())
        {
            Flash_Set_SlaveAlarmFlag(buf.From_ID,0);
            if(Flash_Get_SlaveAlarmFlag()==0)
            {
                Warning_Disable();
            }
        }
        if(Now_Status==Open||Now_Status==Close)
        {
            LOG_D("Remote valve close from %ld\r\n",buf.From_ID);
            Warning_Disable();
            Last_Close_Flag=1;
            Moto_Close(OtherOff);
            ring_once();
        }
        if(buf.From_ID == GetDoorID())
        {
            ControlUpload_GW(1,buf.From_ID,6,ValveStatus);
        }
        else
        {
            ControlUpload_GW(1,buf.From_ID,2,ValveStatus);
        }
        break;
    case 8://延迟
        LOG_I("Valve delay open %d from %ld\r\n",buf.Data,buf.From_ID);
        RadioEnqueue(buf.From_ID,buf.Counter,8,buf.Data);
        if(buf.Data)
        {
            Delay_Timer_CloseDoor(buf.From_ID);
        }
        else
        {
            if(Now_Status==Open)
            {
                Delay_Timer_OpenDoor(buf.From_ID);
            }
        }
        break;
    case 9://终端测水线掉落
        LOG_D("Slave Lost %d From %ld\r\n",buf.Data,buf.From_ID);
        RadioEnqueue(buf.From_ID,buf.Counter,9,buf.Data);
        WarUpload_GW(1,buf.From_ID,9,buf.Data);
        break;
    }
    if(buf.Counter==0)
    {
        ChangeMaxPower(&rf_433);
    }
    else
    {
        BackNormalPower(&rf_433);
    }
    Clear_Device_Time(buf.From_ID);
    Offline_React(buf.From_ID);
}
void GatewayDataSolve(int rssi,uint8_t *rx_buffer,uint8_t rx_len)
{
    Message Rx_message;
    if(rx_buffer[rx_len]=='G')
    {
        rt_sscanf((const char *)&rx_buffer[1],"G{%d,%d,%d,%d,%d,%d}G",&Rx_message.Target_ID,&Rx_message.From_ID,&Rx_message.Device_ID,&Rx_message.Counter,&Rx_message.Command,&Rx_message.Data);
        if(Rx_message.Target_ID == RadioID && Check_Valid(Rx_message.From_ID) == RT_EOK)
        {
            LOG_D("GatewayDataSolve from:%d,command:%d,data:%d,rssi:%d\r\n",Rx_message.From_ID,Rx_message.Command,Rx_message.Data,rssi);
            Heart_Refresh(Rx_message.From_ID);
            switch(Rx_message.Command)
            {
            case 1://延迟
                break;
            case 2://网关开
                ring_once();
                if(GetNowStatus()==Close || GetNowStatus()==Open)
                {
                    if(Rx_message.Data)
                    {
                        Remote_Open();
                        ControlUpload_GW(1,RadioID,2,ValveStatus);
                    }
                    else
                    {
                        Remote_Close();
                        ControlUpload_GW(1,RadioID,2,ValveStatus);
                    }
                }
                else
                {
                    ControlUpload_GW(0,RadioID,2,3);
                }
                break;
            case 3://心跳应答
                LOG_D("Heart Reponse\r\n");
                ControlUpload_GW(1,0,4,ValveStatus);
                break;
            case 4://请求同步
                LOG_D("Request Sync\r\n");
                Gateway_Sync();
                break;
            case 5://请求同步
                LOG_D("Ack Reponse\r\n");
                break;
            case 6://删除指定设备
                LOG_D("Delete Device %ld\r\n",Rx_message.Device_ID);
                Delete_Device(Rx_message.Device_ID);
                break;
            case 7://应答
                break;
            }
        }
    }
}
void NormalSolve(int rssi,uint8_t *rx_buffer,uint8_t rx_len)
{
    Message Rx_message;
    if(rx_buffer[rx_len]==0x0A&&rx_buffer[rx_len-1]==0x0D)
    {
        rt_sscanf((const char *)&rx_buffer[1],"{%d,%d,%d,%d,%d}",&Rx_message.Target_ID,&Rx_message.From_ID,&Rx_message.Counter,&Rx_message.Command,&Rx_message.Data);
        if(Rx_message.Target_ID == RadioID ||Rx_message.Target_ID == 99999999)
        {
            LOG_D("NormalSolve from:%d,command:%d,data:%d,rssi:%d\r\n",Rx_message.From_ID,Rx_message.Command,Rx_message.Data,rssi);
            if(Learn_Flag)
            {
                Update_Device_Rssi(Rx_message.From_ID,rssi);
                Device_AliveChange(Rx_message.From_ID,1);
                Device_Learn(Rx_message);
            }
            else
            {
                if(Rx_message.From_ID==GetGatewayID())
                {
                    wifi_communication_blink();
                }
                if(Flash_Get_Key_Valid(Rx_message.From_ID)==RT_EOK)
                {
                    Update_Device_Rssi(Rx_message.From_ID,rssi);
                    Device_AliveChange(Rx_message.From_ID,1);
                    DataSolve(Rx_message);
                }
                else if(Rx_message.From_ID == 98989898)
                {
                    LOG_W("Factory Get Rssi is %d\r\n",rssi);
                    if(rssi<-85)
                    {
                        Factory_WarningRing();
                    }
                    else
                    {
                        Factory_NormalRing();
                    }
                }
                else
                {
                    LOG_W("Device_ID %ld is not include\r\n",Rx_message.From_ID);
                }
            }

        }
    }
}
void rf433_rx_callback(int rssi,uint8_t *buffer,uint8_t len)
{
    switch(buffer[1])
    {
    case '{':
        NormalSolve(rssi,buffer,len);
        break;
    case 'G':
        GatewayDataSolve(rssi,buffer,len);
        break;
    }
}
