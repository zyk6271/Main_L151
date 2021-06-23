/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-06-16     Rick       the first version
 */
#ifndef RADIO_RADIO_DRV_H_
#define RADIO_RADIO_DRV_H_
#include <stdint.h>

void Ax5043_Spi_Init(void);
void SpiWriteByte(uint8_t ubByte);
void SpiWriteWord(uint16_t ubByte);
void Ax5043_Spi_Reset(void);
void SpiWriteSingleAddressRegister(uint8_t Addr, uint8_t Data);
void SpiWriteLongAddressRegister(uint16_t Addr, uint8_t Data);
void SpiLongWriteLongAddressRegister(uint16_t Addr, uint16_t Data);
void SpiWriteData(uint8_t *pBuf,uint8_t Length);
void SpiReadData(uint8_t *pBuf,uint8_t Length);

uint8_t SpiReadSingleAddressRegister(uint8_t Addr);
uint8_t SpiReadByte( void );
uint8_t SpiReadLongAddressRegister(uint16_t Addr);
int8_t SpiReadUnderSingleAddressRegister(uint8_t Addr);

#endif /* RADIO_RADIO_DRV_H_ */
