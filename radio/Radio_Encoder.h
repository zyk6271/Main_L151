/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-22     Rick       the first version
 */
#ifndef RADIO_RADIO_ENCODER_H_
#define RADIO_RADIO_ENCODER_H_

typedef struct
{
    uint8_t NowNum;
    uint8_t TargetNum;
    uint8_t SendNum;
    uint8_t ack[30];
    uint8_t trials[30];
    uint8_t type[30];
    uint32_t Taget_Id[30];
    uint8_t counter[30];
    uint8_t Command[30];
    uint8_t Data[30];
}Radio_Queue;

void Check_Wor_Recv(uint32_t From_ID,uint8_t Command,uint8_t Data);
void Tx_Done_Callback(uint8_t *rx_buffer,uint8_t rx_len);
void RadioSend(uint32_t Taget_Id,uint8_t counter,uint8_t Command,uint8_t Data);
void WorSend(uint32_t Taget_Id,uint8_t counter,uint8_t Command,uint8_t Data);
void RadioEnqueue(uint8_t ack,uint32_t type,uint32_t Taget_Id,uint8_t counter,uint8_t Command,uint8_t Data);
void GatewaySyncEnqueue(uint8_t ack,uint8_t type,uint32_t device_id,uint8_t rssi,uint8_t bat);
void GatewaySyncSend(uint8_t ack,uint8_t type,uint32_t device_id,uint8_t rssi,uint8_t bat);
void GatewayWarningEnqueue(uint8_t ack,uint32_t device_id,uint8_t rssi,uint8_t warn_id,uint8_t value);
void GatewayWarningSend(uint8_t ack,uint32_t device_id,uint8_t rssi,uint8_t warn_id,uint8_t value);
void GatewayControlEnqueue(uint8_t ack,uint32_t device_id,uint8_t rssi,uint8_t control,uint8_t value);
void GatewayControlSend(uint8_t ack,uint32_t device_id,uint8_t rssi,uint8_t control,uint8_t value);
void RadioDequeueTaskInit(void);
void SendWithOldBuff(void);
void FreqRefresh_Init(void);

#endif /* RADIO_RADIO_ENCODER_H_ */
