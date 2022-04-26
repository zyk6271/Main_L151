/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-22     Rick       the first version
 */
#ifndef RADIO_RADIO_DECODER_H_
#define RADIO_RADIO_DECODER_H_

typedef struct
{
    long Target_ID;
    long From_ID;
    long Device_ID;
    int Counter;
    int Command ;
    int Data;
    int Rssi;
}Message;

void Start_Learn(void);
void Stop_Learn(void);
void Start_Learn_Key(void);
uint8_t Factory_Detect(void);
void Factory_Test(void);
void rf433_rx_callback(int rssi,uint8_t *buffer,uint8_t len);

#endif /* RADIO_RADIO_DECODER_H_ */
