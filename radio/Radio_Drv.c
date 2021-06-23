/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-06-16     Rick       the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include "drv_spi.h"
#include <string.h>
#include "AX5043.h"
#include "Radio_Config.h"
#include "Radio.h"
#include "pin_config.h"
#include "Radio_Drv.h"

#define DBG_TAG "radio_drv"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define RADIO_SPI1_NSS_PORT GPIOA
#define RADIO_SPI1_NSS_PIN GPIO_PIN_4

struct rt_spi_device *ax5043_device;//spi设备

#define RADIO_Device_Name "ax5043"

struct rt_spi_device *ax5043_radio_spi_init(void)
{
    rt_err_t res;
    struct rt_spi_device *radio_spi_device;

    {
        rt_hw_spi_device_attach("spi1", "ax5043", GPIOA, GPIO_PIN_4);//往总线spi2上挂载一个spi20设备，cs脚：PB12

        radio_spi_device = (struct rt_spi_device *)rt_device_find(RADIO_Device_Name);
        if (!radio_spi_device)
        {
            LOG_D("spi sample run failed! cant't find %s device!\n", RADIO_Device_Name);
            return RT_NULL;
        }
    }

    LOG_D("find %s device!\n", RADIO_Device_Name);

    /* config spi */
    {
        struct rt_spi_configuration cfg;
        cfg.data_width = 8;
        cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB; /* SPI Compatible: Mode 0. */
        cfg.max_hz = 8 * 1000000;             /* max 10M */

        res = rt_spi_configure(radio_spi_device, &cfg);

        if (res != RT_EOK)
        {
            LOG_D("rt_spi_configure failed!\r\n");
        }
        res = rt_spi_take_bus(radio_spi_device);
        if (res != RT_EOK)
        {
            LOG_D("rt_spi_take_bus failed!\r\n");
        }

        res = rt_spi_release_bus(radio_spi_device);

        if(res != RT_EOK)
        {
            LOG_D("rt_spi_release_bus failed!\r\n");
        }
    }

    return radio_spi_device;
}

void IRQ_Bounding(void)
{
    rt_pin_mode(Radio_IRQ, PIN_MODE_INPUT_PULLUP);
    rt_pin_attach_irq(Radio_IRQ, PIN_IRQ_MODE_RISING, Receive_ISR, RT_NULL);
    rt_pin_irq_enable(Radio_IRQ, PIN_IRQ_ENABLE);
}

void Ax5043_Spi_Init(void)
{
    ax5043_device = ax5043_radio_spi_init();
}

void Ax5043_Spi_Reset(void)
{
    uint8_t msg[2] = {0};
    msg[0] = REG_AX5043_PWRMODE|0x80 ;
    msg[1] = 0x80 ;
    rt_spi_send(ax5043_device,msg,2);
}
                             //write comm  bit7=1
/***********************************************************************
==单字节写==
***********************************************************************/
void SpiWriteByte(uint8_t ubByte)
{
    rt_spi_send(ax5043_device,&ubByte,1);
}

/***********************************************************************
==双字节写==
***********************************************************************/
void SpiWriteWord(uint16_t ubByte)
{
    uint8_t msg[2] = {0};
    msg[0] = ( ubByte & 0xFF00 ) >> 8;
    msg[1] = ubByte & 0x00FF;
    rt_spi_send(ax5043_device,msg,2);
}
/***********************************************************************
==单字节读==
***********************************************************************/
uint8_t SpiReadByte( void )
{
    uint8_t data;
    rt_spi_recv(ax5043_device,&data,1);
    return data;
}
/***********************************************************************
==写单字节地址==
***********************************************************************/
void SpiWriteSingleAddressRegister(uint8_t Addr, uint8_t Data)
{
    uint8_t ubAddr = Addr|0x80;
    rt_spi_send_then_send(ax5043_device,&ubAddr,1,&Data,1);
}
/***********************************************************************
==写双字节地址==
***********************************************************************/
void SpiWriteLongAddressRegister(uint16_t Addr, uint8_t Data)
{
    uint16_t ubAddr;
    ubAddr = Addr|0xF000;
    uint8_t msg[2] = {0};
    msg[0] = ( ubAddr & 0xFF00 ) >> 8;
    msg[1] = ubAddr & 0x00FF;

    rt_spi_send_then_send(ax5043_device,msg,2,&Data,1);
}
void SpiLongWriteLongAddressRegister(uint16_t Addr, uint16_t Data)
{
    uint16_t ubAddr;
    ubAddr = Addr|0xF000;
    uint8_t msg1[2] = {0};
    uint8_t msg2[2] = {0};
    msg1[0] = ( ubAddr & 0xFF00 ) >> 8;
    msg1[1] = ubAddr & 0x00FF;
    msg2[0] = ( Data & 0xFF00 ) >> 8;
    msg2[1] = Data & 0x00FF;
    rt_spi_send_then_send(ax5043_device,msg1,2,msg2,2);
}
/***********************************************************************
==写数据==
***********************************************************************/
void SpiWriteData(uint8_t *pBuf,uint8_t Length)
{
    uint8_t  data;
    data=REG_AX5043_FIFODATA| 0x80;
    rt_spi_send_then_send(ax5043_device,&data,1,pBuf,Length);
}
/***********************************************************************
==读取指定寄存器==
***********************************************************************/
uint8_t SpiReadSingleAddressRegister(uint8_t Addr)
{
    uint8_t ubAddr ;
    uint8_t RcvAddr ;
    ubAddr = Addr&0x7F ;//read common bit7=0
    rt_spi_send_then_recv(ax5043_device,&ubAddr,1,&RcvAddr,1);
    return RcvAddr ;
}

int8_t SpiReadUnderSingleAddressRegister(uint8_t Addr)//负数
{
    uint8_t ubAddr ;
    uint8_t RcvAddr ;
    ubAddr = Addr&0x7F ;//read common bit7=0
    rt_spi_send_then_recv(ax5043_device,&ubAddr,1,&RcvAddr,1);
    return RcvAddr ;
}
/***********************************************************************
==读取指定长寄存器==
***********************************************************************/
uint8_t SpiReadLongAddressRegister(uint16_t Addr)
{
    uint16_t ubAddr ;
    uint8_t RcvAddr ;
    ubAddr = Addr|0x7000 ;//read common bit7=0
    uint8_t msg[2] = {0};
    msg[0] = ( ubAddr & 0xFF00 ) >> 8;
    msg[1] = ubAddr & 0x00FF;
    rt_spi_send_then_recv(ax5043_device,msg,2,&RcvAddr,1);
    return RcvAddr ;
}

/***********************************************************************
==读取数据==
***********************************************************************/
void SpiReadData(uint8_t *pBuf,uint8_t Length)
{
    uint8_t SendAddr ;
    SendAddr=REG_AX5043_FIFODATA & 0x7F;
    rt_spi_send_then_recv(ax5043_device,&SendAddr,1,pBuf,Length);
}
