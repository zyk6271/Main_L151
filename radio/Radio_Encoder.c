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
#include "radio_encoder.h"
#include "AX5043.h"

#define DBG_TAG "RADIO_ENCODER"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static rt_mq_t rf_en_mq;
static struct rt_completion rf_ack;
rt_thread_t rf_encode_t = RT_NULL;

uint32_t RadioID = 0;
uint32_t Self_Default_Id = 10000000;

extern uint32_t Gateway_ID;
extern struct ax5043 rf_433;

char radio_send_buf[255];

void RadioEnqueue(uint32_t Taget_Id, uint8_t Counter, uint8_t Command, uint8_t Data)
{
    Radio_Normal_Format Send_Buf = {0};

    Send_Buf.Type = 0;
    Send_Buf.Ack = 0;
    Send_Buf.Taget_ID = Taget_Id;
    Send_Buf.Counter = Counter;
    Send_Buf.Command = Command;
    Send_Buf.Data = Data;

    rt_mq_send(rf_en_mq, &Send_Buf, sizeof(Radio_Normal_Format));
}

void GatewaySyncEnqueue(uint8_t ack,uint8_t command,uint32_t device_id,uint8_t rssi,uint8_t bat)
{
    Radio_Normal_Format Send_Buf = {0};

    if(Gateway_ID != 0)
    {
        Send_Buf.Type = 1;
        Send_Buf.Ack = ack;
        Send_Buf.Command = command;
        Send_Buf.Payload_ID = device_id;
        Send_Buf.Rssi = rssi;
        Send_Buf.Data = bat;

        rt_mq_send(rf_en_mq, &Send_Buf, sizeof(Radio_Normal_Format));
    }
}

void GatewayWarningEnqueue(uint8_t ack,uint32_t device_id,uint8_t rssi,uint8_t warn_id,uint8_t value)
{
    Radio_Normal_Format Send_Buf = {0};
    if(Gateway_ID != 0)
    {
        Send_Buf.Type = 2;
        Send_Buf.Ack = ack;
        Send_Buf.Payload_ID = device_id;
        Send_Buf.Rssi = rssi;
        Send_Buf.Command = warn_id;
        Send_Buf.Data = value;

        rt_mq_send(rf_en_mq, &Send_Buf, sizeof(Radio_Normal_Format));
    }
}

void GatewayControlEnqueue(uint8_t ack,uint32_t device_id,uint8_t rssi,uint8_t control,uint8_t value)
{
    Radio_Normal_Format Send_Buf = {0};
    if(Gateway_ID != 0)
    {
        Send_Buf.Type = 3;
        Send_Buf.Ack = ack;
        Send_Buf.Payload_ID = device_id;
        Send_Buf.Rssi = rssi;
        Send_Buf.Command = control;
        Send_Buf.Data = value;

        rt_mq_send(rf_en_mq, &Send_Buf, sizeof(Radio_Normal_Format));
    }
}

void SendPrepare(Radio_Normal_Format Send)
{
    rt_memset(radio_send_buf, 0, sizeof(radio_send_buf));
    uint8_t check = 0;
    switch(Send.Type)
    {
    case 0:
        Send.Counter++ <= 255 ? Send.Counter : 0;
        rt_sprintf(radio_send_buf, "{%08ld,%08ld,%03d,%02d,%d}", Send.Taget_ID, RadioID, Send.Counter, Send.Command, Send.Data);
        for (uint8_t i = 0; i < 28; i++)
        {
            check += radio_send_buf[i];
        }
        radio_send_buf[28] = ((check >> 4) < 10) ? (check >> 4) + '0' : (check >> 4) - 10 + 'A';
        radio_send_buf[29] = ((check & 0xf) < 10) ? (check & 0xf) + '0' : (check & 0xf) - 10 + 'A';
        radio_send_buf[30] = '\r';
        radio_send_buf[31] = '\n';
        break;
    case 1://Sync
        rt_sprintf(radio_send_buf,"A{%02d,%02d,%08ld,%08ld,%08ld,%03d,%02d}A",Send.Ack,Send.Command,Gateway_ID,RadioID,Send.Payload_ID,Send.Rssi,Send.Data);
        wifi_communication_blink();
        break;
    case 2://Warn
        rt_sprintf(radio_send_buf,"B{%02d,%08ld,%08ld,%08ld,%03d,%03d,%02d}B",Send.Ack,Gateway_ID,RadioID,Send.Payload_ID,Send.Rssi,Send.Command,Send.Data);
        wifi_communication_blink();
        break;
    case 3://Control
        rt_sprintf(radio_send_buf,"C{%02d,%08ld,%08ld,%08ld,%03d,%03d,%02d}C",Send.Ack,Gateway_ID,RadioID,Send.Payload_ID,Send.Rssi,Send.Command,Send.Data);
        wifi_communication_blink();
        break;
    }
}

void ack_refresh(void)
{
    LOG_I("ack_refresh\r\n");
    rt_completion_done(&rf_ack);
}

void rf_encode_entry(void *paramaeter)
{
    Radio_Normal_Format Send_Data;
    while (1)
    {
        if (rt_mq_recv(rf_en_mq,&Send_Data, sizeof(Radio_Normal_Format), RT_WAITING_FOREVER) == RT_EOK)
        {
            rt_completion_init(&rf_ack);
            SendPrepare(Send_Data);
            LOG_D("RF_Send to:%d,command:%d,data:%d\r\n",Send_Data.Taget_ID,Send_Data.Command,Send_Data.Data);
            RF_Send(&rf_433,radio_send_buf, rt_strlen(radio_send_buf));
            if(Send_Data.Ack)
            {
                for(uint8_t i = 0; i < 3; i++)
                {
                    LOG_D("RF_Send retry num %d\r\n",i);
                    if(rt_completion_wait(&rf_ack, 500) != RT_EOK)
                    {
                        RF_Send(&rf_433,radio_send_buf, rt_strlen(radio_send_buf));
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                rt_thread_mdelay(500);
            }
        }
    }
}

void Print_RadioID(void)
{
    LOG_I("Radio ID is %d\r\n", RadioID);
}
MSH_CMD_EXPORT(Print_RadioID,Print_RadioID);

void RadioQueue_Init(void)
{
    RadioID = *((int *)(0x0801FFF0));//这就是已知的地址，要强制类型转换
    if (RadioID == 0xFFFFFFFF || RadioID == 0)
    {
        RadioID = Self_Default_Id;
    }
    Print_RadioID();

    rf_en_mq = rt_mq_create("rf_en_mq", sizeof(Radio_Normal_Format), 10, RT_IPC_FLAG_PRIO);
    rf_encode_t = rt_thread_create("radio_send", rf_encode_entry, RT_NULL, 1024, 9, 10);
    if (rf_encode_t)
    {
        rt_thread_startup(rf_encode_t);
    }
}
