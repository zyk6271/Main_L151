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
#include <stdio.h>
#include <string.h>
#include "led.h"
#include "drv_spi.h"
#include "AX5043.h"
#include "Radio.h"
#include "Radio_Config.h"
#include "Radio_Encoder.h"
#include "flashwork.h"

#define DBG_TAG "radio_encoder"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

rt_thread_t Radio_QueueTask = RT_NULL;
rt_timer_t AckCheck_t = RT_NULL;
Radio_Queue Main_Queue={0};

extern uint32_t Gateway_ID;
uint32_t Self_Id = 0;
uint32_t Self_Default_Id = 10000210;
uint32_t Self_Counter = 0;

void RadioSend(uint32_t Taget_Id,uint8_t counter,uint8_t Command,uint8_t Data)
{
    uint8_t check = 0;
    uint8_t buf[35]={0};
    if(counter<255)counter++;
    else counter=0;

    sprintf((char *)(buf),"{%08ld,%08ld,%03d,%02d,%d}",\
                                            Taget_Id,\
                                            Self_Id,\
                                            counter,\
                                            Command,\
                                            Data);

    for(uint8_t i = 0 ; i < 28 ; i ++)
    {
        check += buf[i];
    }
    buf[28] = ((check>>4) < 10)?  (check>>4) + '0' : (check>>4) - 10 + 'A';
    buf[29] = ((check&0xf) < 10)?  (check&0xf) + '0' : (check&0xf) - 10 + 'A';
    buf[30] = '\r';
    buf[31] = '\n';
    Normal_send(buf,32);
}

void GatewaySyncEnqueue(uint8_t ack,uint8_t type,uint32_t device_id,uint8_t rssi,uint8_t bat)
{
    RadioEnqueue(ack,2,device_id,type,rssi,bat);
}
void GatewaySyncSend(uint8_t ack,uint8_t type,uint32_t device_id,uint8_t rssi,uint8_t bat)
{
    uint8_t buf[50]={0};
    sprintf((char *)(&buf),"A{%02d,%02d,%08ld,%08ld,%08ld,%03d,%02d}A",\
                                            ack,\
                                            type,\
                                            Gateway_ID,\
                                            Self_Id,\
                                            device_id,\
                                            rssi,\
                                            bat);
    Normal_send(buf,43);
}
void GatewayWarningEnqueue(uint8_t ack,uint32_t device_id,uint8_t rssi,uint8_t warn_id,uint8_t value)
{
    RadioEnqueue(ack,3,device_id,rssi,warn_id,value);
}
void GatewayWarningSend(uint8_t ack,uint32_t device_id,uint8_t rssi,uint8_t warn_id,uint8_t value)
{
    uint8_t buf[50]={0};
    sprintf((char *)(&buf),"B{%02d,%08ld,%08ld,%08ld,%03d,%03d,%02d}B",\
                                            ack,\
                                            Gateway_ID,\
                                            Self_Id,\
                                            device_id,\
                                            rssi,\
                                            warn_id,\
                                            value);
    Normal_send(buf,44);
}
void GatewayControlEnqueue(uint8_t ack,uint32_t device_id,uint8_t rssi,uint8_t control,uint8_t value)
{
    RadioEnqueue(ack,4,device_id,rssi,control,value);
}
void GatewayControlSend(uint8_t ack,uint32_t device_id,uint8_t rssi,uint8_t control,uint8_t value)
{
    uint8_t buf[50]={0};
    sprintf((char *)(&buf),"C{%02d,%08ld,%08ld,%08ld,%03d,%03d,%02d}C",\
                                            ack,\
                                            Gateway_ID,\
                                            Self_Id,\
                                            device_id,\
                                            rssi,\
                                            control,\
                                            value);
    Normal_send(buf,44);
}
void RadioEnqueue(uint8_t ack,uint32_t type,uint32_t Taget_Id,uint8_t counter,uint8_t Command,uint8_t Data)
{
    uint8_t NumTemp = Main_Queue.TargetNum;
    if(NumTemp<20)
    {
        NumTemp ++;
        LOG_D("Queue Num Increase,Value is %d\r\n",NumTemp);
    }
    else
    {
        LOG_E("Queue is Full,Value is %d\r\n",NumTemp);
        return;
    }
    Main_Queue.ack[NumTemp] = ack;
    Main_Queue.type[NumTemp] = type;
    Main_Queue.Taget_Id[NumTemp] = Taget_Id;
    Main_Queue.counter[NumTemp] = counter;
    Main_Queue.Command[NumTemp] = Command;
    Main_Queue.Data[NumTemp] = Data;
    Main_Queue.TargetNum++;
    LOG_D("Enqueue Success\r\n");
}
void RadioDequeue(void *paramaeter)
{
    rt_thread_mdelay(2000);
    LOG_I("Queue Init Success\r\n");
    while(1)
    {
        if(Main_Queue.NowNum == Main_Queue.TargetNum)
        {
            Main_Queue.NowNum = 0;
            Main_Queue.TargetNum = 0;
            Main_Queue.SendNum = 0;
        }
        if(Main_Queue.TargetNum>Main_Queue.NowNum)
        {
            if(Main_Queue.ack[Main_Queue.NowNum+1]==1)
            {
                if(Main_Queue.trials[Main_Queue.NowNum+1]==0)
                {
                    Main_Queue.SendNum = Main_Queue.NowNum+1;
                    Main_Queue.trials[Main_Queue.NowNum+1]=1;
                    LOG_I("Ack First try\r\n");
                }
                else if(Main_Queue.trials[Main_Queue.NowNum+1]>0 && Main_Queue.trials[Main_Queue.NowNum+1]<=3)
                {
                    if(AckCheck(Gateway_ID)==0)
                    {
                        if(Main_Queue.trials[Main_Queue.NowNum+1]==3)
                        {
                            Main_Queue.trials[Main_Queue.NowNum+1]=0;
                            Main_Queue.NowNum++;
                            Main_Queue.SendNum = Main_Queue.NowNum;
                            LOG_I("Ack Retry Stop\r\n");
                            continue;
                        }
                        else if(Main_Queue.trials[Main_Queue.NowNum+1]<3)
                        {
                            ChangeMaxPower();
                            Main_Queue.SendNum = Main_Queue.NowNum+1;
                            Main_Queue.trials[Main_Queue.NowNum+1]++;
                            LOG_I("Ack Retry again\r\n");
                        }
                    }
                    else
                    {
                        Main_Queue.trials[Main_Queue.NowNum+1]=0;
                        Main_Queue.NowNum++;
                        Main_Queue.SendNum = Main_Queue.NowNum;
                        LOG_I("Ack Ok\r\n");
                        continue;
                    }
                }
            }
            else
            {
                Main_Queue.NowNum++;
                Main_Queue.SendNum = Main_Queue.NowNum;
                LOG_I("No Ack Send\r\n");
            }
            AckClear(Gateway_ID);
            switch(Main_Queue.type[Main_Queue.SendNum])
            {
            case 1:
                RadioSend(Main_Queue.Taget_Id[Main_Queue.SendNum],Main_Queue.counter[Main_Queue.SendNum],Main_Queue.Command[Main_Queue.SendNum],Main_Queue.Data[Main_Queue.SendNum]);
                LOG_D("Normal Send With Now Num %d,Target Num is %d,Target_Id %ld,counter %d,command %d,data %d\r\n",Main_Queue.SendNum,Main_Queue.TargetNum,Main_Queue.Taget_Id[Main_Queue.SendNum],Main_Queue.counter[Main_Queue.SendNum],Main_Queue.Command[Main_Queue.SendNum],Main_Queue.Data[Main_Queue.SendNum]);
                rt_thread_mdelay(500);
                break;
            case 2:
                GatewaySyncSend(Main_Queue.ack[Main_Queue.SendNum],Main_Queue.counter[Main_Queue.SendNum],Main_Queue.Taget_Id[Main_Queue.SendNum],Main_Queue.Command[Main_Queue.SendNum],Main_Queue.Data[Main_Queue.SendNum]);
                LOG_D("GatewaySync With Now Num %d,Target Num is %d,Type is %d,Target_Id %ld,rssi %d,bat %d\r\n",Main_Queue.SendNum,Main_Queue.TargetNum,Main_Queue.counter[Main_Queue.SendNum],Gateway_ID,Main_Queue.Command[Main_Queue.SendNum],Main_Queue.Data[Main_Queue.SendNum]);
                rt_thread_mdelay(800);
                wifi_led(3);
                break;
            case 3:
                GatewayWarningSend(Main_Queue.ack[Main_Queue.SendNum],Main_Queue.Taget_Id[Main_Queue.SendNum],Main_Queue.counter[Main_Queue.SendNum],Main_Queue.Command[Main_Queue.SendNum],Main_Queue.Data[Main_Queue.SendNum]);
                LOG_D("GatewayWarningSend With Now Num %d,Target Num is %d,Target_Id %ld,Rssi is %d,warn_id %d,value %d\r\n",Main_Queue.SendNum,Main_Queue.TargetNum,Gateway_ID,Main_Queue.counter[Main_Queue.SendNum],Main_Queue.Command[Main_Queue.SendNum],Main_Queue.Data[Main_Queue.SendNum]);
                rt_thread_mdelay(800);
                wifi_led(3);
                break;
            case 4:
                GatewayControlSend(Main_Queue.ack[Main_Queue.SendNum],Main_Queue.Taget_Id[Main_Queue.SendNum],Main_Queue.counter[Main_Queue.SendNum],Main_Queue.Command[Main_Queue.SendNum],Main_Queue.Data[Main_Queue.SendNum]);
                LOG_D("GatewayControl With Now Num %d,Target Num is %d,Target_Id %ld,Rssi is %d,control %d,value %d\r\n",Main_Queue.SendNum,Main_Queue.TargetNum,Gateway_ID,Main_Queue.counter[Main_Queue.SendNum],Main_Queue.Command[Main_Queue.SendNum],Main_Queue.Data[Main_Queue.SendNum]);
                rt_thread_mdelay(800);
                wifi_led(3);
                break;
            default:break;
            }
            BackNormalPower();
        }
        rt_thread_mdelay(10);
    }
}

void RadioDequeueTaskInit(void)
{
    int *p;
    p=(int *)(0x0801FFFC);//这就是已知的地址，要强制类型转换
    Self_Id = *p;//从Flash加载ID
    if(Self_Id==0xFFFFFFFF || Self_Id==0)
    {
        Self_Id = Self_Default_Id;
    }
    Radio_QueueTask = rt_thread_create("Radio_QueueTask", RadioDequeue, RT_NULL, 1024, 10, 10);
    if(Radio_QueueTask)rt_thread_startup(Radio_QueueTask);
}
MSH_CMD_EXPORT(RadioDequeueTaskInit,RadioDequeueTaskInit);
