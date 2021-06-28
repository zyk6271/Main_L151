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
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

rt_thread_t Radio_QueueTask = RT_NULL;
rt_timer_t FreqRefresh = RT_NULL;

uint32_t Self_Id = 0;
uint32_t Self_Default_Id = 10000001;
uint32_t Self_Counter = 0;

typedef struct
{
    uint32_t TargetID;
    uint8_t Command;
    uint8_t Data;
    uint8_t CorrectFlag;
    uint8_t SendFlag;
    uint8_t WarningFlag;
    uint8_t WaitCheckFlag;
}Reponse;

typedef struct
{
    uint8_t NowNum;
    uint8_t TargetNum;
    uint8_t wor_flag[30];
    uint32_t Taget_Id[30];
    uint8_t counter[30];
    uint8_t Command[30];
    uint8_t Data[30];
}Radio_Queue;

Radio_Queue Main_Queue={0};

void Tx_Done_Callback(uint8_t *rx_buffer,uint8_t rx_len)
{
    //LOG_D("Send ok\r\n");
}
void RadioSend(uint32_t Taget_Id,uint8_t counter,uint8_t Command,uint8_t Data)
{
    uint8_t check = 0;
    uint8_t buf[35];
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
void RadioEnqueue(uint32_t wor_flag,uint32_t Taget_Id,uint8_t counter,uint8_t Command,uint8_t Data)
{
    uint8_t NumTemp = Main_Queue.TargetNum;
    if(NumTemp<20)
    {
        NumTemp ++;
        LOG_D("Queue Num Increase,Value is %d\r\n",NumTemp);
    }
    else
    {
        LOG_I("Queue is Full,Value is %d\r\n",NumTemp);
        return;
    }
    Main_Queue.wor_flag[NumTemp] = wor_flag;
    Main_Queue.Taget_Id[NumTemp] = Taget_Id;
    Main_Queue.counter[NumTemp] = counter;
    Main_Queue.Command[NumTemp] = Command;
    Main_Queue.Data[NumTemp] = Data;
    Main_Queue.TargetNum++;
    LOG_D("Enqueue Success\r\n");
}
void RadioDequeue(void *paramaeter)
{
    LOG_D("Queue Init Success\r\n");
    while(1)
    {
        if(Main_Queue.NowNum == Main_Queue.TargetNum)
        {
            //LOG_I("Dequeue Clear Success\r\n");
            Main_Queue.NowNum = 0;
            Main_Queue.TargetNum = 0;
        }
        else if(Main_Queue.TargetNum>0 && Main_Queue.TargetNum>Main_Queue.NowNum)
        {
            Main_Queue.NowNum++;
            switch(Main_Queue.wor_flag[Main_Queue.NowNum])
            {
            case 0:
                RadioSend(Main_Queue.Taget_Id[Main_Queue.NowNum],Main_Queue.counter[Main_Queue.NowNum],Main_Queue.Command[Main_Queue.NowNum],Main_Queue.Data[Main_Queue.NowNum]);
                LOG_D("Normal Send With Now Num %d,Target Num is %d,Target_Id %ld,counter %d,command %d,data %d\r\n",Main_Queue.NowNum,Main_Queue.TargetNum,Main_Queue.Taget_Id[Main_Queue.NowNum],Main_Queue.counter[Main_Queue.NowNum],Main_Queue.Command[Main_Queue.NowNum],Main_Queue.Data[Main_Queue.NowNum]);
                rt_thread_mdelay(300);
                break;
            default:break;
            }
            LOG_D("Dequeue Success\r\n");
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