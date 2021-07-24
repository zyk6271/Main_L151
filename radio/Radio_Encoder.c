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
#include "Radio_Encoder.h"
#include "led.h"
#include <stdio.h>

#define DBG_TAG "radio_encoder"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

typedef struct
{
    uint8_t NowNum;
    uint8_t TargetNum;
    uint8_t type[30];
    uint32_t Taget_Id[30];
    uint8_t counter[30];
    uint8_t Command[30];
    uint8_t Data[30];
}Radio_Queue;

rt_thread_t Radio_QueueTask = RT_NULL;
rt_timer_t FreqRefresh = RT_NULL;
Radio_Queue Main_Queue={0};

extern uint32_t Gateway_ID;
uint32_t Self_Id = 0;
uint32_t Self_Default_Id = 10000088;
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

void GatewaySyncEnqueue(uint8_t type,uint32_t device_id,uint8_t rssi,uint8_t bat)
{
    RadioEnqueue(1,device_id,type,rssi,bat);
}
void GatewaySyncSend(uint8_t type,uint32_t device_id,uint8_t rssi,uint8_t bat)
{
    uint8_t buf[50]={0};
    sprintf((char *)(&buf),"A{%02d,%08ld,%08ld,%08ld,%03d,%02d}A",\
                                            type,\
                                            Gateway_ID,\
                                            Self_Id,\
                                            device_id,\
                                            rssi,\
                                            bat);
    Normal_send(buf,40);
}
void GatewayWarningEnqueue(uint32_t device_id,uint8_t rssi,uint8_t warn_id,uint8_t value)
{
    RadioEnqueue(2,device_id,rssi,warn_id,value);
}
void GatewayWarningSend(uint32_t device_id,uint8_t rssi,uint8_t warn_id,uint8_t value)
{
    uint8_t buf[50]={0};
    sprintf((char *)(&buf),"B{%08ld,%08ld,%08ld,%03d,%03d,%02d}B",\
                                            Gateway_ID,\
                                            Self_Id,\
                                            device_id,\
                                            rssi,\
                                            warn_id,\
                                            value);
    Normal_send(buf,41);
}
void GatewayControlEnqueue(uint32_t device_id,uint8_t rssi,uint8_t control,uint8_t value)
{
    RadioEnqueue(3,device_id,rssi,control,value);
}
void GatewayControlSend(uint32_t device_id,uint8_t rssi,uint8_t control,uint8_t value)
{
    uint8_t buf[50]={0};
    sprintf((char *)(&buf),"C{%08ld,%08ld,%08ld,%03d,%03d,%02d}C",\
                                            Gateway_ID,\
                                            Self_Id,\
                                            device_id,\
                                            rssi,\
                                            control,\
                                            value);
    Normal_send(buf,41);
}
void RadioEnqueue(uint32_t type,uint32_t Taget_Id,uint8_t counter,uint8_t Command,uint8_t Data)
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
        }
        else if(Main_Queue.TargetNum>0 && Main_Queue.TargetNum>Main_Queue.NowNum)
        {
            Main_Queue.NowNum++;
            switch(Main_Queue.type[Main_Queue.NowNum])
            {
            case 0:
                RadioSend(Main_Queue.Taget_Id[Main_Queue.NowNum],Main_Queue.counter[Main_Queue.NowNum],Main_Queue.Command[Main_Queue.NowNum],Main_Queue.Data[Main_Queue.NowNum]);
                LOG_I("Normal Send With Now Num %d,Target Num is %d,Target_Id %ld,counter %d,command %d,data %d\r\n",Main_Queue.NowNum,Main_Queue.TargetNum,Main_Queue.Taget_Id[Main_Queue.NowNum],Main_Queue.counter[Main_Queue.NowNum],Main_Queue.Command[Main_Queue.NowNum],Main_Queue.Data[Main_Queue.NowNum]);
                rt_thread_mdelay(150);
                break;
            case 1:
                GatewaySyncSend(Main_Queue.counter[Main_Queue.NowNum],Main_Queue.Taget_Id[Main_Queue.NowNum],Main_Queue.Command[Main_Queue.NowNum],Main_Queue.Data[Main_Queue.NowNum]);
                LOG_I("GatewaySync With Now Num %d,Type is %d,Target Num is %d,Target_Id %ld,rssi %d,bat %d\r\n",Main_Queue.NowNum,Main_Queue.TargetNum,Main_Queue.counter[Main_Queue.NowNum],Main_Queue.Taget_Id[Main_Queue.NowNum],Main_Queue.Command[Main_Queue.NowNum],Main_Queue.Data[Main_Queue.NowNum]);
                rt_thread_mdelay(150);
                break;
            case 2:
                GatewayWarningSend(Main_Queue.Taget_Id[Main_Queue.NowNum],Main_Queue.counter[Main_Queue.NowNum],Main_Queue.Command[Main_Queue.NowNum],Main_Queue.Data[Main_Queue.NowNum]);
                LOG_I("GatewayWarningSend With Now Num %d,Target Num is %d,Target_Id %ld,Rssi is %d,warn_id %d,value %d\r\n",Main_Queue.NowNum,Main_Queue.TargetNum,Main_Queue.Taget_Id[Main_Queue.NowNum],Main_Queue.counter[Main_Queue.NowNum],Main_Queue.Command[Main_Queue.NowNum],Main_Queue.Data[Main_Queue.NowNum]);
                rt_thread_mdelay(150);
                break;
            case 3:
                GatewayControlSend(Main_Queue.Taget_Id[Main_Queue.NowNum],Main_Queue.counter[Main_Queue.NowNum],Main_Queue.Command[Main_Queue.NowNum],Main_Queue.Data[Main_Queue.NowNum]);
                LOG_I("GatewayControl With Now Num %d,Target Num is %d,Target_Id %ld,Rssi is %d,control %d,value %d\r\n",Main_Queue.NowNum,Main_Queue.TargetNum,Main_Queue.Taget_Id[Main_Queue.NowNum],Main_Queue.counter[Main_Queue.NowNum],Main_Queue.Command[Main_Queue.NowNum],Main_Queue.Data[Main_Queue.NowNum]);
                rt_thread_mdelay(150);
                break;
            default:break;
            }
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
