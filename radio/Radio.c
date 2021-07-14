/*********************************************************************                                                                   **
**  Device:     AX5043
**  File:  Radio.c
**  Author:  Strong
**  Target:  R8C/1B
**  Tools:  ICE
**  Version:    V00
**  Date:     2014-01-20
**  Update :    2014/01/20
**  Description:
**        This file is a sample code for your reference.
**  Copyright (C) 2014 Karv Technology Co.,Ltd
*********************************************************************/
#include <rtthread.h>
#include <rtdevice.h>
#include "drv_spi.h"
#include <string.h>
#include "AX5043.h"
#include "Radio_Config.h"
#include "Radio.h"
#include "pin_config.h"
#include "Radio_Decoder.h"
#include "Radio_Encoder.h"
#include "Radio_Drv.h"
#include "led.h"
#include "status.h"

#define DBG_TAG "radio"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>



uint8_t InitFlag = 0;
uint8_t AxRadioPHYChanPllRng_TX[1];   //频率补偿
uint8_t AxRadioPHYChanPllRng_RX[1];

uint8_t ChannelPtr ;
volatile uint8_t ubRFState;

uint8_t TXBuff[64];
uint8_t TxLen;
uint8_t RXBuff[64];
uint8_t RxLen;

uint8_t wor_flag=0;

rt_sem_t Radio_IRQ_Sem=RT_NULL;
rt_thread_t Radio_Task=RT_NULL;
rt_timer_t Init_Timer = RT_NULL;

#define axradio_phy_preamble_wor_len  400
#define axradio_phy_preamble_wor_longlen 1600
#define axradio_phy_preamble_len  32
#define axradio_framing_synclen   32

#define axradio_phy_preamble_flags 0x38
#define axradio_phy_preamble_byte 0x55

uint8_t ubReceiveFlag;
int ubRssi;;
uint8_t axradio_freq_select = 1;
uint8_t axradio_freq_now = 1;
uint8_t axradio_power_now = 0;
uint32_t axradio_txbuffer_cnt = 0;
uint32_t axradio_txbuffer_len = 0;

//uint32_t axradio_phy_chanfreq[2] = {0x10b62763,0x10b81f83};//434.5,434.7
uint32_t axradio_phy_chanfreq[2] = {0X10AE46E4,0x10b81f83};//433.7,434.7
extern uint32_t Self_Id;

const uint16_t RegisterVaule[][2]=
{
    { REG_AX5043_MODULATION,          0x08},
    { REG_AX5043_ENCODING,            0x00},
    { REG_AX5043_FRAMING,             0x26},
    { REG_AX5043_PINFUNCSYSCLK,       0x01},
    { REG_AX5043_PINFUNCDCLK,         0x01},
    { REG_AX5043_PINFUNCDATA,         0x01},
    { REG_AX5043_PINFUNCANTSEL,       0x04},
    { REG_AX5043_PINFUNCPWRAMP,       0x00},
    { REG_AX5043_WAKEUPXOEARLY,       0x01},
    { REG_AX5043_IFFREQ1,             0x01},
    { REG_AX5043_IFFREQ0,             0xE4},
    { REG_AX5043_DECIMATION,          0x16},
    { REG_AX5043_RXDATARATE2,         0x00},
    { REG_AX5043_RXDATARATE1,         0x3D},
    { REG_AX5043_RXDATARATE0,         0x8D},
    { REG_AX5043_MAXDROFFSET2,        0x00},
    { REG_AX5043_MAXDROFFSET1,        0x00},
    { REG_AX5043_MAXDROFFSET0,        0x00},
    { REG_AX5043_MAXRFOFFSET2,        0x80},
    { REG_AX5043_MAXRFOFFSET1,        0x02},
    { REG_AX5043_MAXRFOFFSET0,        0x31},
    { REG_AX5043_FSKDMAX1,            0x00},
    { REG_AX5043_FSKDMAX0,            0xA6},
    { REG_AX5043_FSKDMIN1,            0xFF},
    { REG_AX5043_FSKDMIN0,            0x5A},
    { REG_AX5043_AMPLFILTER,          0x00},
    { REG_AX5043_RXPARAMSETS,         0xF4},
    { REG_AX5043_AGCGAIN0,            0xC5},
    { REG_AX5043_AGCTARGET0,          0x84},
    { REG_AX5043_TIMEGAIN0,           0xF8},
    { REG_AX5043_DRGAIN0,             0xF2},
    { REG_AX5043_PHASEGAIN0,          0xC3},
    { REG_AX5043_FREQUENCYGAINA0,     0x0F},
    { REG_AX5043_FREQUENCYGAINB0,     0x1F},
    { REG_AX5043_FREQUENCYGAINC0,     0x08},
    { REG_AX5043_FREQUENCYGAIND0,     0x08},
    { REG_AX5043_AMPLITUDEGAIN0,      0x06},
    { REG_AX5043_FREQDEV10,           0x00},
    { REG_AX5043_FREQDEV00,           0x00},
    { REG_AX5043_BBOFFSRES0,          0x00},
    { REG_AX5043_AGCGAIN1,            0xC5},
    { REG_AX5043_AGCTARGET1,          0x84},
    { REG_AX5043_AGCAHYST1,           0x00},
    { REG_AX5043_AGCMINMAX1,          0x00},
    { REG_AX5043_TIMEGAIN1,           0xF6},
    { REG_AX5043_DRGAIN1,             0xF1},
    { REG_AX5043_PHASEGAIN1,          0xC3},
    { REG_AX5043_FREQUENCYGAINA1,     0x0F},
    { REG_AX5043_FREQUENCYGAINB1,     0x1F},
    { REG_AX5043_FREQUENCYGAINC1,     0x08},
    { REG_AX5043_FREQUENCYGAIND1,     0x08},
    { REG_AX5043_AMPLITUDEGAIN1,      0x06},
    { REG_AX5043_FREQDEV11,           0x00},
    { REG_AX5043_FREQDEV01,           0x3E},
    { REG_AX5043_FOURFSK1,            0x16},
    { REG_AX5043_BBOFFSRES1,          0x00},
    { REG_AX5043_AGCGAIN3,            0xFF},
    { REG_AX5043_AGCTARGET3,          0x84},
    { REG_AX5043_AGCAHYST3,           0x00},
    { REG_AX5043_AGCMINMAX3,          0x00},
    { REG_AX5043_TIMEGAIN3,           0xF5},
    { REG_AX5043_DRGAIN3,             0xF0},
    { REG_AX5043_PHASEGAIN3,          0xC3},
    { REG_AX5043_FREQUENCYGAINA3,     0x0F},
    { REG_AX5043_FREQUENCYGAINB3,     0x1F},
    { REG_AX5043_FREQUENCYGAINC3,     0x0C},
    { REG_AX5043_FREQUENCYGAIND3,     0x0C},
    { REG_AX5043_AMPLITUDEGAIN3,      0x06},
    { REG_AX5043_FREQDEV13,           0x00},
    { REG_AX5043_FREQDEV03,           0x3E},
    { REG_AX5043_FOURFSK3,            0x16},
    { REG_AX5043_BBOFFSRES3,          0x00},
    { REG_AX5043_MODCFGF,             0x00},
    { REG_AX5043_FSKDEV2,             0x00},
    { REG_AX5043_FSKDEV1,             0x04},
    { REG_AX5043_FSKDEV0,             0x2F},
    { REG_AX5043_MODCFGA,             0x05},
    { REG_AX5043_TXRATE2,             0x00},
    { REG_AX5043_TXRATE1,             0x0C},
    { REG_AX5043_TXRATE0,             0x19},
    { REG_AX5043_TXPWRCOEFFB1,        0x07},
    { REG_AX5043_TXPWRCOEFFB0,        0x00},
    { REG_AX5043_PLLVCOI,             0x99},
    { REG_AX5043_PLLRNGCLK,           0x04},
    { REG_AX5043_BBTUNE,              0x0F},
    { REG_AX5043_BBOFFSCAP,           0x77},
    { REG_AX5043_PKTADDRCFG,          0x00},
    { REG_AX5043_PKTLENCFG,           0x00},
    { REG_AX5043_PKTLENOFFSET,        0x21},
    { REG_AX5043_PKTMAXLEN,           0xC8},
    { REG_AX5043_MATCH0PAT3,          0xAA},
    { REG_AX5043_MATCH0PAT2,          0xCC},
    { REG_AX5043_MATCH0PAT1,          0xAA},
    { REG_AX5043_MATCH0PAT0,          0xCC},
    { REG_AX5043_MATCH0LEN,           0x9F},
    { REG_AX5043_MATCH0MAX,           0x1F},
    { REG_AX5043_MATCH1PAT1,          0x55},
    { REG_AX5043_MATCH1PAT0,          0x55},
    { REG_AX5043_MATCH1LEN,           0x8A},
    { REG_AX5043_MATCH1MAX,           0x0A},
    { REG_AX5043_TMGTXBOOST,          0x3E},
    { REG_AX5043_TMGTXSETTLE,         0x31},
    { REG_AX5043_TMGRXBOOST,          0x3E},
    { REG_AX5043_TMGRXSETTLE,         0x31},
    { REG_AX5043_TMGRXOFFSACQ,        0x00},
    { REG_AX5043_TMGRXCOARSEAGC,      0x7F},
    { REG_AX5043_TMGRXRSSI,           0x03},
    { REG_AX5043_TMGRXPREAMBLE2,      0x35},
    { REG_AX5043_RSSIABSTHR,          0xE0},
    { REG_AX5043_BGNDRSSITHR,         0x00},
    { REG_AX5043_PKTCHUNKSIZE,        0x0D},
    { REG_AX5043_PKTACCEPTFLAGS,      0x20},
    { REG_AX5043_DACVALUE1,           0x00},
    { REG_AX5043_DACVALUE0,           0x00},
    { REG_AX5043_DACCONFIG,           0x00},
    { REG_AX5043_REF,                 0x03},
    { REG_AX5043_0xF1C,               0x07},
    { REG_AX5043_0xF21,               0x68},
    { REG_AX5043_0xF22,               0xFF},
    { REG_AX5043_0xF23,               0x84},
    { REG_AX5043_0xF26,               0x98},
    { REG_AX5043_0xF34,               0x28},
    { REG_AX5043_0xF35,               0x11},
    { REG_AX5043_0xF44,               0x25},
    {AX5043_Addr_END    , AX5043_Vule_END}
};

const uint16_t TXMODE_REG[][2]=
{
#if(XTAL_FREQ == 26000000)
    {
        REG_AX5043_PLLLOOP  , 0x0b
    },
    { REG_AX5043_PLLCPI   , 0x10    },
    { REG_AX5043_PLLVCODIV  , 0x24   },
    { REG_AX5043_PLLVCOI  , 0x99   },
    { REG_AX5043_XTALCAP  , 0x00   },
    { REG_AX5043_0xF00   , 0x0f    },
    { REG_AX5043_REF   , 0x03    },
    { REG_AX5043_0xF18   , 0x06    }
#endif
};

const uint16_t RXMODE_REG[][2]=
{
#if(XTAL_FREQ == 26000000)
    {
        REG_AX5043_PLLLOOP  , 0x0b
    },
    { REG_AX5043_PLLCPI   , 0x10  },
    { REG_AX5043_PLLVCODIV  , 0x24 },
    { REG_AX5043_PLLVCOI  , 0x99  },
    { REG_AX5043_XTALCAP  , 0x0c  },
    { REG_AX5043_0xF00   , 0x0f  },
    { REG_AX5043_REF   , 0x03   },
    { REG_AX5043_0xF18   , 0x02  }
#endif
};

void AX5043_Reset(void)//复位
{
    uint8_t ubAddres;
    Ax5043_Spi_Reset();

    SpiWriteSingleAddressRegister(REG_AX5043_PWRMODE, 0x00);        //RADIO_PWRMODE = PWRMODE_PWRDOWN=0x00
    SpiWriteSingleAddressRegister(REG_AX5043_SCRATCH, 0x55);
    do
    {
        ubAddres = SpiReadSingleAddressRegister(REG_AX5043_SCRATCH);
    }
    while (ubAddres != 0x55);

    SpiWriteSingleAddressRegister(REG_AX5043_PINFUNCIRQ, 0x01);  // IRQ Line 1   001 IRQ Output ’1’
    SpiWriteSingleAddressRegister(REG_AX5043_PINFUNCIRQ, 0x00);  //IRQ Line 0  000 IRQ Output ’0’
    SpiWriteSingleAddressRegister(REG_AX5043_PINFUNCIRQ, 0x03);  //011 IRQ Output Interrupt Request
}

char SetChannel(uint8_t ubNum )
{
    uint8_t ubRang;
    uint8_t ubTemp;          //ubData;
    unsigned long RadioChannel;

    ubRang = AxRadioPHYChanPllRng_RX[ubNum];

    //RadioChannel  = ChannelTable[ubNum];
    RadioChannel  = WirelessFreqConfigure(0x06,0xA2,0x0c,ubNum);

    //RadioChannel += axradio_curfreqoffset;  //����Ƶ�ʲ���
    ubTemp = SpiReadSingleAddressRegister(REG_AX5043_PLLLOOP);  //��PLLLOOP
    if (ubTemp & 0x80)
    {
        SpiWriteSingleAddressRegister(REG_AX5043_PLLRANGINGA, ubRang&0x0F);
        SpiWriteSingleAddressRegister(REG_AX5043_FREQA0, RadioChannel&0xFF);       //AX5043_FREQA0 = f;
        SpiWriteSingleAddressRegister(REG_AX5043_FREQA1, (RadioChannel>>8)&0xFF);  //AX5043_FREQA1 = f >> 8;
        SpiWriteSingleAddressRegister(REG_AX5043_FREQA2, (RadioChannel>>16)&0xFF); //AX5043_FREQA2 = f >> 16;
        SpiWriteSingleAddressRegister(REG_AX5043_FREQA3, (RadioChannel>>24)&0xFF); //AX5043_FREQA3 = f >> 24;
    }
    else
    {
        SpiWriteSingleAddressRegister(REG_AX5043_PLLRANGINGB, ubRang&0x0F);     //AX5043_PLLRANGINGB = rng & 0x0F;
        SpiWriteSingleAddressRegister(REG_AX5043_FREQB0, RadioChannel&0xFF);       //AX5043_FREQB0 = f;
        SpiWriteSingleAddressRegister(REG_AX5043_FREQB1, (RadioChannel>>8)&0xFF);  //AX5043_FREQB1 = f >> 8;
        SpiWriteSingleAddressRegister(REG_AX5043_FREQB2, (RadioChannel>>16)&0xFF); //AX5043_FREQB2 = f >> 16;
        SpiWriteSingleAddressRegister(REG_AX5043_FREQB3, (RadioChannel>>24)&0xFF); //AX5043_FREQB3 = f >> 24;
    }
    ubTemp^=0x80 ;
    SpiWriteSingleAddressRegister(REG_AX5043_PLLLOOP, ubTemp);                        //AX5043_PLLLOOP ^= 0x80;

    return AXRADIO_ERR_NOERROR;
}
void InitAx5043REG(void)
{
    uint8_t ubi;

    for (ubi = 0; ((RegisterVaule[ubi][0] != 0xFF)&&(RegisterVaule[ubi][1] != 0xDD));ubi++)
    {
        SpiWriteLongAddressRegister(RegisterVaule[ubi][0] , RegisterVaule[ubi][1] );
    }

    WirelessBitRateConfigure(0x02);
    WirelessTxPowerConfigure(0x03);

    SpiWriteSingleAddressRegister(REG_AX5043_PINFUNCIRQ, 0x03);    // AX5043_PINFUNCIRQ = 0x03;  use as IRQ pin

}
void Ax5043SetRegisters_TX(void)
{
    uint8_t ubi;

    for (ubi = 0x00;ubi < 8;ubi++)
    {
        SpiWriteLongAddressRegister(TXMODE_REG[ubi][0] , TXMODE_REG[ubi][1] );
    }
#if (XTAL_FREQ == 26000000)
    switch (0x02)//configData[DATARATE_LOCAT]
    {
        case 0:
        case 1:
        case 2:
            SpiWriteLongAddressRegister(REG_AX5043_PLLLOOP,0x0b);
            SpiWriteLongAddressRegister(REG_AX5043_PLLCPI,0x10);
            break;
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            SpiWriteLongAddressRegister(REG_AX5043_PLLLOOP,0x09);
            SpiWriteLongAddressRegister(REG_AX5043_PLLCPI,0x02);
            break;
    }
#endif

}
void Ax5043SetRegisters_RX(void)
{
    uint8_t ubi;

    for (ubi = 0x00;ubi < 8;ubi++)
    {
        SpiWriteLongAddressRegister(RXMODE_REG[ubi][0] , RXMODE_REG[ubi][1] );
    }
#if (XTAL_FREQ == 26000000)
    switch (0x02)//configData[DATARATE_LOCAT]
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
            SpiWriteLongAddressRegister(REG_AX5043_PLLLOOP,0x0b);
            SpiWriteLongAddressRegister(REG_AX5043_PLLCPI,0x10);
            break;
        case 5:
        case 6:
        case 7:
        case 8:
            SpiWriteLongAddressRegister(REG_AX5043_PLLLOOP,0x09);
            SpiWriteLongAddressRegister(REG_AX5043_PLLCPI,0x01);
            break;
    }
#endif

}
void calibrate_lposc(void)
{
        uint16_t lposckfiltmax = 0x142a;
        SpiWriteLongAddressRegister(AX5043_LPOSCFREQ1,0x00);
        SpiWriteLongAddressRegister(AX5043_LPOSCFREQ0,0x00);

        SpiWriteLongAddressRegister(AX5043_LPOSCREF1, ((XTAL_FREQ/640)>>8) & 0xFF);
        SpiWriteLongAddressRegister(AX5043_LPOSCREF0,((XTAL_FREQ/640)>>0) & 0xFF);
        SpiWriteSingleAddressRegister(REG_AX5043_PWRMODE, AX5043_PWRSTATE_SYNTH_TX);
        SpiWriteLongAddressRegister(AX5043_LPOSCKFILT1,(lposckfiltmax >> (8 + 1)) & 0xFF);
        SpiWriteLongAddressRegister(AX5043_LPOSCKFILT0,(lposckfiltmax >> 1) & 0xFF);

        SpiWriteLongAddressRegister(AX5043_LPOSCCONFIG,0x25);
        rt_thread_mdelay(6);

        SpiWriteLongAddressRegister(AX5043_LPOSCKFILT1,(lposckfiltmax >> (8 + 2)) & 0xFF);
        SpiWriteLongAddressRegister(AX5043_LPOSCKFILT0,(lposckfiltmax >> 2) & 0xFF);
        rt_thread_mdelay(5);
        {
            uint8_t x = SpiReadLongAddressRegister(AX5043_LPOSCFREQ1);
            if( x == 0x7f || x == 0x80 )
            {
                SpiWriteLongAddressRegister(AX5043_LPOSCFREQ1,0);
                SpiWriteLongAddressRegister(AX5043_LPOSCFREQ0,0);
            }
        }
}
void ax5043_set_registers_rxwor(void)
{
    SpiWriteLongAddressRegister(AX5043_REG_TMGRXAGC,0x06);
    SpiWriteLongAddressRegister(AX5043_REG_TMGRXPREAMBLE1,0x19);
    SpiWriteLongAddressRegister(AX5043_REG_PKTMISCFLAGS,0x03);
}
MSH_CMD_EXPORT(ax5043_set_registers_rxwor,ax5043_set_registers_rxwor);
void read(void)
{
    LOG_D("1 is %x\r\n",SpiReadLongAddressRegister(REG_AX5043_WAKEUP1));
    LOG_D("0 is %x\r\n",SpiReadLongAddressRegister(REG_AX5043_WAKEUP0));
    LOG_D("time is %d\r\n",SpiReadLongAddressRegister(REG_AX5043_WAKEUPTIMER1));
}

void RdioXtalON(void)
{
    uint8_t ubTemp;

    ubRFState = trxstate_wait_xtal;
    ubTemp = SpiReadSingleAddressRegister(REG_AX5043_IRQMASK1);
    SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK1, ubTemp|0x01);            // enable xtal ready interrupt
    SpiWriteSingleAddressRegister(REG_AX5043_PWRMODE, AX5043_PWRSTATE_XTAL_ON); //AX5043_PWRSTATE_XTAL_ON=0x5
    while (ubRFState == trxstate_wait_xtal);     //wait for ubRFState=trxstate_xtal_ready
}
void ChangeMaxPower(void)
{
    if(axradio_power_now==0)
    {
        axradio_power_now = 1;
        SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x0F);
        SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0xFF);
        LOG_D("ChangeMaxPower Now\r\n");
    }
    else
    {
        LOG_D("Power Already Max\r\n");
    }
}
void BackNormalPower(void)
{
    if(axradio_power_now)
    {
        axradio_power_now = 0;
        SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x02);
        SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0xEB);
        LOG_D("BackNormalPower Now\r\n");
    }
    else
    {
        LOG_D("Power Already Normal\r\n");
    }
}
uint8_t InitAX5043(void)
{
    uint8_t ubi,ubTemp;
    uint8_t pllloop_save, pllcpi_save;

    Ax5043_Spi_Init();
    IRQ_Bounding();
    AX5043_OFF();
    AX5043_Reset();
    InitAx5043REG();
    RdioXtalON();

    Ax5043SetRegisters_RX();
    pllloop_save = SpiReadSingleAddressRegister(REG_AX5043_PLLLOOP);
    pllcpi_save = SpiReadSingleAddressRegister(REG_AX5043_PLLCPI);

    SpiWriteSingleAddressRegister(REG_AX5043_PLLLOOP, 0x09);
    SpiWriteSingleAddressRegister(REG_AX5043_PLLCPI, 0x08);

    for (ubi = 0x00; ubi < CHANNEL_NUM ;ubi++)
    {
        SpiWriteSingleAddressRegister(REG_AX5043_FREQA0, axradio_phy_chanfreq[axradio_freq_select]&0xFF);
        SpiWriteSingleAddressRegister(REG_AX5043_FREQA1, (axradio_phy_chanfreq[axradio_freq_select]>>8)&0xFF);
        SpiWriteSingleAddressRegister(REG_AX5043_FREQA2, (axradio_phy_chanfreq[axradio_freq_select]>>16)&0xFF);
        SpiWriteSingleAddressRegister(REG_AX5043_FREQA3, (axradio_phy_chanfreq[axradio_freq_select]>>24)&0xFF);
        if ( !(0x0a & 0xF0) )
        {
            ubTemp = 0x0a | 0x10;
        }
        else
        {
            ubTemp = 0x18;
            if (ubi)
            {
                ubTemp = AxRadioPHYChanPllRng_RX[ubi - 1];
                if ( ubTemp & 0x20)
                    ubTemp = 0x08;
                ubTemp &= 0x0F;
                ubTemp |= 0x10;
            }
        }
        ubRFState = trxstate_pll_ranging;
        SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK1, 0x10);     // enable pll autoranging done interrupt
        SpiWriteSingleAddressRegister(REG_AX5043_PLLRANGINGA, ubTemp); //init ranging process starting from "range"
        do
        {
            asm("NOP");
            asm("NOP");
        }
        while (ubRFState == trxstate_pll_ranging); //while(ubRFState != trxstate_pll_ranging_done); wait for trxstate_pll_ranging_done

        ubRFState = trxstate_off;
        SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK0, 0x00);
        SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK1, 0x00);     //AX5043_IRQMASK1 = 0x00;
        ubTemp = SpiReadSingleAddressRegister(REG_AX5043_PLLRANGINGA);
        AxRadioPHYChanPllRng_RX[ubi] = ubTemp;                        //AX5043_PLLRANGINGA;
    }

    Ax5043SetRegisters_TX();
    SpiWriteSingleAddressRegister(REG_AX5043_PLLLOOP, 0x09); // AX5043_PLLLOOP = 0x09;default 100kHz loop BW for ranging
    SpiWriteSingleAddressRegister(REG_AX5043_PLLCPI, 0x08);  //AX5043_PLLCPI = 0x08;

    for (ubi = 0x00;ubi < CHANNEL_NUM ;ubi++)
    {
        if ( !(0x0a & 0xF0) )   // start values for ranging available
        {
            ubTemp = 0x0a | 0x10;
        }
        else
        {
            ubTemp = AxRadioPHYChanPllRng_RX[ubi];
            if ( ubTemp & 0x20)
                ubTemp = 0x08;
            ubTemp &= 0x0F;
            ubTemp |= 0x10;
        }

        ubRFState = trxstate_pll_ranging;
        SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK1, 0x10);      // enable pll autoranging done interrupt
        SpiWriteSingleAddressRegister(REG_AX5043_PLLRANGINGA, ubTemp); //  init ranging process starting from "range"

        do
        {
            asm("NOP");
            asm("NOP");//delay(1);
        }
        while (ubRFState == trxstate_pll_ranging);//while(ubRFState != trxstate_pll_ranging_done);

        ubRFState = trxstate_off;
        SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK0, 0x00);     //disable low byte interrupt
        SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK1, 0x00);     //disable high byte interrupt
        ubTemp=SpiReadSingleAddressRegister(REG_AX5043_PLLRANGINGA);
        AxRadioPHYChanPllRng_TX[ubi] = ubTemp;                        //AX5043_PLLRANGINGA;
    }

    SpiWriteSingleAddressRegister(REG_AX5043_PWRMODE, AX5043_PWRSTATE_POWERDOWN); // AX5043_PWRMODE = AX5043_PWRSTATE_POWERDOWN=0x00;
    ubTemp = AxRadioPHYChanPllRng_RX[0]&0x0F;                                     //AX5043_PLLRANGINGA = axradio_phy_chanpllrng_rx[0] & 0x0F;
    SpiWriteSingleAddressRegister(REG_AX5043_PLLRANGINGA,ubTemp );

    SpiWriteSingleAddressRegister(REG_AX5043_FREQA0, axradio_phy_chanfreq[axradio_freq_select]&0xFF);
    SpiWriteSingleAddressRegister(REG_AX5043_FREQA1, (axradio_phy_chanfreq[axradio_freq_select]>>8)&0xFF);
    SpiWriteSingleAddressRegister(REG_AX5043_FREQA2, (axradio_phy_chanfreq[axradio_freq_select]>>16)&0xFF);
    SpiWriteSingleAddressRegister(REG_AX5043_FREQA3, (axradio_phy_chanfreq[axradio_freq_select]>>24)&0xFF);
    AxRadioPHYChanPllRng_TX[0] = AxRadioPHYChanPllRng_RX[0];

    SpiWriteSingleAddressRegister(REG_AX5043_PLLLOOP, pllloop_save);
    SpiWriteSingleAddressRegister(REG_AX5043_PLLCPI,pllcpi_save);

	SpiWriteLongAddressRegister(REG_AX5043_POWCTRL1, 0x07);

    for (ubi = 0x00;ubi < CHANNEL_NUM ;ubi++)
    {
        if ((AxRadioPHYChanPllRng_TX[ubi] |AxRadioPHYChanPllRng_RX[ubi] ) & 0x20)
        {
            return AXRADIO_ERR_RANGING;
        }
    }
    InitFlag = RT_TRUE;
    return AXRADIO_ERR_NOERROR;
}
void SetTransmitMode(void)
{
    uint8_t Rng;
    uint8_t ubData;

    Ax5043SetRegisters_TX();
    Rng = AxRadioPHYChanPllRng_TX[0];//configData[CHANNEL_LOCAT]
    ubData=SpiReadSingleAddressRegister(REG_AX5043_PLLLOOP);
    if (ubData & 0x80)
    {
        SpiWriteSingleAddressRegister(REG_AX5043_PLLRANGINGB, Rng&0x0F);   //AX5043_PLLRANGINGB = rng & 0x0F;
    }
    else
    {
        SpiWriteSingleAddressRegister(REG_AX5043_PLLRANGINGA , Rng&0x0F);   //  AX5043_PLLRANGINGA = rng & 0x0F;
    }
}
void AX5043ReceiverON(void)
{
    SpiWriteLongAddressRegister(REG_AX5043_RSSIREFERENCE, 0x19);
    SpiWriteLongAddressRegister(REG_AX5043_TMGRXPREAMBLE1, 0x00); //   AX5043_TMGRXPREAMBLE1(0x229) = 0x00;
    SpiWriteLongAddressRegister(REG_AX5043_PKTSTOREFLAGS , 0x50 ); // AX5043_PKTSTOREFLAGS &= (uint8_t)~0x40;

    //���FIFO
    SpiWriteSingleAddressRegister(REG_AX5043_FIFOSTAT,0x03);  // AX5043_FIFOSTAT(0x28) = 3;  bit[5:0]=0b00011 Clear FIFO Data and Flags  ���FIFO page 69
    SpiWriteSingleAddressRegister(REG_AX5043_PWRMODE,AX5043_PWRSTATE_FULL_RX); //AX5043_PWRSTATE_FULL_RX =0x09; bit[3:0]=1001 Receiver Running page55 ��������
    ubRFState = trxstate_rx;
    SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK0,0x01);//AX5043_IRQMASK0��0x07�� = 0x01;  enable FIFO not empty
    SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK1,0x00);
}
void AX5043Receiver_Continuous(void)
{
    uint32_t ubTemp=0;

    SpiWriteSingleAddressRegister(REG_AX5043_RADIOEVENTMASK0, 0x04);

    SpiWriteLongAddressRegister(REG_AX5043_RSSIREFERENCE, 0);
    SpiWriteLongAddressRegister(REG_AX5043_TMGRXPREAMBLE1, 0); //   AX5043_TMGRXPREAMBLE1(0x229) = 0x00;
    SpiWriteLongAddressRegister(REG_AX5043_PKTMISCFLAGS , 0 ); // AX5043_PKTSTOREFLAGS &= (uint8_t)~0x40;

    ubTemp = SpiReadLongAddressRegister(REG_AX5043_PKTSTOREFLAGS);
    ubTemp &= ~0x40;
    SpiWriteLongAddressRegister(REG_AX5043_PKTSTOREFLAGS, ubTemp);            // enable xtal ready interrupt

    SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK0,0x01);//AX5043_IRQMASK0��0x07�� = 0x01;  enable FIFO not empty
    SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK1,0x00);

    SpiWriteSingleAddressRegister(REG_AX5043_FIFOSTAT,0x03);  // AX5043_FIFOSTAT(0x28) = 3;  bit[5:0]=0b00011 Clear FIFO Data and Flags  ���FIFO page 69
    SpiWriteSingleAddressRegister(REG_AX5043_PWRMODE,AX5043_PWRSTATE_FULL_RX); //AX5043_PWRSTATE_FULL_RX =0x09; bit[3:0]=1001 Receiver Running page55 ��������
    ubRFState = trxstate_rx;

}
void SetReceiveMode(void)
{
    uint8_t ubRng;
    uint8_t ubData;

    Ax5043SetRegisters_RX();

    ubRng = AxRadioPHYChanPllRng_RX[0]; //configData[CHANNEL_LOCAT]
    ubData=SpiReadSingleAddressRegister(REG_AX5043_PLLLOOP);
    if (ubData & 0x80)
    {
        SpiWriteSingleAddressRegister(REG_AX5043_PLLRANGINGB, ubRng&0x0F);   //AX5043_PLLRANGINGB = rng & 0x0F;
    }
    else
    {
        SpiWriteSingleAddressRegister(REG_AX5043_PLLRANGINGA , ubRng&0x0F);   //  AX5043_PLLRANGINGA = rng & 0x0F;
    }
}
void ReceiveData(void)
{
    uint8_t ubDataLen,ubTepm;
    uint8_t ubTemCom;
    uint8_t ubDataFlag;
    uint8_t ubOffSet;
    ubRssi=0;

    unsigned long uwFreqOffSet;

    ubDataLen = SpiReadSingleAddressRegister(REG_AX5043_RADIOEVENTREQ0);  //RADIOEVENTREQ0(0x0F); bit0=1  FIFO not empty interrupt pending

    while (SpiReadSingleAddressRegister(REG_AX5043_IRQREQUEST0) & 0x01)   // bit0=1  FIFO not empty interrupt pending
    {
        ubTemCom = SpiReadSingleAddressRegister(REG_AX5043_FIFODATA);   // ��1���ֽ� read command
        ubDataLen = ubTemCom >>5 ;
        if (ubDataLen == 0x07)             //�����bit[7:5]=111  ��ʾ�ɱ�����ݳ���
        {
            ubDataLen = SpiReadSingleAddressRegister(REG_AX5043_FIFODATA); // ��2���ֽ� 7 means variable length, -> get length byte  ��� ���ݳ���
        }
        ubTemCom &=0x1F;
        switch (ubTemCom)
        {
            case AX5043_FIFOCMD_DATA: //0x0001 1111  ���bit[4:0]������  ubTemCom =0x01  ��ʾ����������,
                if (!ubDataLen)
                    break;
                ubDataFlag = SpiReadSingleAddressRegister(REG_AX5043_FIFODATA); //��3���ֽ�  ��־:0 /0/ UNENC/ RAW/ NOCRC/ RESIDUE /PKTEND /PKTSTART
                ubDataLen--;
                SpiReadData(RXBuff,ubDataLen);         //ax5043_readfifo(axradio_rxbuffer, len);  //��FIFO������
                RxLen = ubDataLen;
                rt_kprintf("Receive is");
                for( int i = 1; i < ubDataLen; i++ )
                {
                    LOG_RAW( " %02X", RXBuff[i] );
                }
                ubTepm = SpiReadSingleAddressRegister(REG_AX5043_IRQMASK0);
                SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK0, ubTepm&0xFE);   // disable FIFO not empty irq
                break;

            case AX5043_FIFOCMD_RFFREQOFFS:
                if (ubDataLen != 3)
                    goto dropchunk;
                ubOffSet = SpiReadSingleAddressRegister(REG_AX5043_FIFODATA);
                ubOffSet &= 0x0F;
                ubOffSet |= 1+(~(ubOffSet&0x08));
                uwFreqOffSet=((signed char)ubOffSet)>>8 ;
                uwFreqOffSet <<=8;
                uwFreqOffSet |= ubOffSet;
                uwFreqOffSet <<=8;
                ubOffSet = SpiReadSingleAddressRegister(REG_AX5043_FIFODATA);
                uwFreqOffSet |= ubOffSet;
                uwFreqOffSet <<=8;
                ubOffSet =SpiReadSingleAddressRegister(REG_AX5043_FIFODATA);
                uwFreqOffSet |= ubOffSet;
                break;

            case AX5043_FIFOCMD_RSSI:
                if (ubDataLen != 1)
                    goto dropchunk;
                ubRssi = SpiReadUnderSingleAddressRegister(REG_AX5043_FIFODATA)+64;
                LOG_D("Got Rssi is %d\r\n",ubRssi);
                break;

            case AX5043_FIFOCMD_TIMER:
                if (ubDataLen != 3)
                    goto dropchunk;
                SpiReadSingleAddressRegister(REG_AX5043_FIFODATA);
                SpiReadSingleAddressRegister(REG_AX5043_FIFODATA);
                SpiReadSingleAddressRegister(REG_AX5043_FIFODATA);
                break;

            default:
            dropchunk:
                if (!ubDataLen)
                    break;
                ubTepm = ubDataLen;
                do
                {
                    SpiReadSingleAddressRegister(REG_AX5043_FIFODATA);  // purge FIFO
                }
                while (--ubTepm);
                break;
        }
    }
    ubDataFlag = ubDataFlag;
}
void AX5043_OFF(void)
{
    SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK0, 0x00);      // IRQ off
    SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK1, 0x00);
    SpiWriteSingleAddressRegister(REG_AX5043_PWRMODE, AX5043_PWRSTATE_XTAL_ON);    //AX5043_PWRMODE = AX5043_PWRSTATE_XTAL_ON=0x00;
    SpiWriteLongAddressRegister(REG_AX5043_LPOSCCONFIG, 0x00);                    //AX5043_LPOSCCONFIG(0x310) = 0x00; // LPOSC off
    ubRFState = trxstate_off;
    SpiWriteSingleAddressRegister(REG_AX5043_PWRMODE, AX5043_PWRSTATE_POWERDOWN);//AX5043_PWRMODE = AX5043_PWRSTATE_POWERDOWN=0x00;
}

void transmit_packet_task(uint8_t *Buf, uint8_t u8Len)
{
   // SetChannel(0);
    SpiWriteSingleAddressRegister(REG_AX5043_PWRMODE, AX5043_PWRSTATE_XTAL_ON); //AX5043_PWRMODE = AX5043_PWRSTATE_XTAL_ON;    Crystal Oscillator enabled
    SpiWriteSingleAddressRegister(REG_AX5043_PWRMODE, AX5043_PWRSTATE_FIFO_ON); //AX5043_PWRMODE = AX5043_PWRSTATE_FIFO_ON;    FIFO enabled
    SetTransmitMode();//   ����TX�Ĵ�����������TX����Ƶ��
    TxLen = u8Len;
    TxLen++;
    TXBuff[0] = TxLen;
    memcpy(TXBuff+1, Buf, u8Len);
    SpiWriteSingleAddressRegister(REG_AX5043_FIFOTHRESH1, 0x00);  //AX5043_FIFOTHRESH1(0x2E) = 0;
    SpiWriteSingleAddressRegister(REG_AX5043_FIFOTHRESH0, 0x80);  //AX5043_FIFOTHRESH0(0x2F) = 0x80;
    ubRFState = trxstate_tx_xtalwait;
    SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK0, 0x00);     //AX5043_IRQMASK0 = 0x00;
    SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK1, 0x01);     //AX5043_IRQMASK1 = 0x01; // enable xtal ready interrupt
}
void Normal_send(uint8_t *Buf, uint8_t u8Len)
{
    axradio_txbuffer_cnt = 0;
    transmit_packet_task(Buf,u8Len);
}
static void TransmitData(void)
{
    uint8_t ubFreeCnt;
    uint8_t ubi;
    for (;;)
    {
        ubFreeCnt = SpiReadSingleAddressRegister(REG_AX5043_FIFOFREE0); //uint8_t cnt = AX5043_FIFOFREE0;
        if (SpiReadSingleAddressRegister(REG_AX5043_FIFOFREE1))
            ubFreeCnt = 0xff;
        switch (ubRFState)
        {
            case trxstate_tx_longpreamble: //A
                if (!axradio_txbuffer_cnt)
                {
                    ubRFState = trxstate_tx_shortpreamble;
                    axradio_txbuffer_cnt = axradio_phy_preamble_len;
                    goto shortpreamble;
                }
                if (ubFreeCnt < 4)
                    goto fifocommit;
                ubFreeCnt = 7;
                if (axradio_txbuffer_cnt < 7)
                {
                    ubFreeCnt = axradio_txbuffer_cnt;
                }
                axradio_txbuffer_cnt -= ubFreeCnt;
                ubFreeCnt <<= 5;
                SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA, 0x02 | (3 << 5));     //AX5043_FIFODATA = AX5043_FIFOCMD_REPEATDATA | (3 << 5); //0x62  bit[7:5]= 011 Three byte payload;#define AX5043_FIFOCMD_REPEATDATA 0x02
                SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA, 0x38);     //AX5043_FIFODATA = axradio_phy_preamble_flags;  //axradio_phy_preamble_flags = 0x38; 0b0011 1000  bit5=UNENC bit4=RAW  bit3=NOCRC
                SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA, ubFreeCnt);//AX5043_FIFODATA = cnt;  //�ط�����
                SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA, 0x55);      //AX5043_FIFODATA = axradio_phy_preamble_byte; //axradio_phy_preamble_byte = 0x55;  �ط�������
                //LOG_D("A Send\r\n");
                break;

            case trxstate_tx_shortpreamble:   //B
            shortpreamble:
                if (!axradio_txbuffer_cnt)
                {
                    ubi = SpiReadSingleAddressRegister(REG_AX5043_FRAMING);
                    if ((ubi & 0x0E) == 0x06 && axradio_framing_synclen)                                  //axradio_framing_synclen=32
                    {
                        uint8_t len_byte = axradio_framing_synclen;
                        uint8_t i = (len_byte & 0x07) ? 0x04 : 0;
                        len_byte += 7;
                        len_byte >>= 3;
                        SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA,0x01 | ((len_byte + 1) << 5));//AX5043_FIFODATA =   0xA1; //0x01 | ((len_byte + 1) << 5);  //0xA1
                        SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA,0x18|i);//AX5043_FIFODATA = 0x18;//axradio_framing_syncflags | i;  //0x18   �������ʾʲô?
                        SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA,0xCC);//AX5043_FIFODATA = 0xcc;
                        SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA,0xAA);//AX5043_FIFODATA = 0xaa;
                        SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA,0xCC);//AX5043_FIFODATA = 0xcc;
                        SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA,0xAA);//AX5043_FIFODATA = 0xaa;

                        ubRFState = trxstate_tx_packet;
                        break;
                    }
                }

                ubFreeCnt = 255;
                if (axradio_txbuffer_cnt < (255*8))
                    ubFreeCnt = axradio_txbuffer_cnt >> 3;
                if (ubFreeCnt)
                {
                    axradio_txbuffer_cnt -= ((uint16_t)ubFreeCnt) << 3;
                    SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA , AX5043_FIFOCMD_REPEATDATA | (3 << 5));
                    SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA ,axradio_phy_preamble_flags);
                    SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA , ubFreeCnt);
                    SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA ,axradio_phy_preamble_byte);
                    break;
                }
                {
                    uint8_t byte = axradio_phy_preamble_byte;
                    ubFreeCnt = axradio_txbuffer_cnt;
                    axradio_txbuffer_cnt = 0;
                    SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA ,AX5043_FIFOCMD_DATA | (2 << 5));
                    SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA, 0x1C);
                    if (SpiReadLongAddressRegister(REG_AX5043_PKTADDRCFG) & 0x80)
                    {
                        // msb first -> stop bit below
                        byte &= 0xFF << (8-ubFreeCnt);
                        byte |= 0x80 >> ubFreeCnt;
                    }
                    else
                    {
                        // lsb first -> stop bit above
                        byte &= 0xFF >> (8-ubFreeCnt);
                        byte |= 0x01 << ubFreeCnt;
                    }
                    SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA, byte);
                }
                // ubRFState = trxstate_tx_packet;
                LOG_D("B Send\r\n");
                break;
            case trxstate_tx_packet:  //C
                SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA, AX5043_FIFOCMD_DATA | (7 << 5));//AX5043_FIFODATA = AX5043_FIFOCMD_DATA | (7 << 5);
                SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA,TxLen+1);//write FIFO chunk length byte (data length+MAC_len + Flag byte=64+3+1)
                SpiWriteSingleAddressRegister(REG_AX5043_FIFODATA,0x03);//AX5043_FIFODATA = flags=0x03;
                SpiWriteData( TXBuff,TxLen);          //ax5043_writefifo(&axradio_txbuffer[axradio_txbuffer_cnt], cnt);
                SpiWriteSingleAddressRegister(REG_AX5043_FIFOSTAT,0x04);        //AX5043_FIFOSTAT = 4; // commit
                ubRFState = trxstate_tx_waitdone;
                SpiWriteSingleAddressRegister(REG_AX5043_RADIOEVENTMASK0,0x01);//AX5043_RADIOEVENTMASK0 = 0x01; // enable REVRDONE event
                SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK0,0x40);        //AX5043_IRQMASK0 = 0x40; // enable radio controller irq
                LOG_D("C Send,Len is %d\r\n",TxLen);
                break;
            default:
                return;
        }
    }
    fifocommit:
        SpiWriteSingleAddressRegister(REG_AX5043_FIFOSTAT, 4); // commit
}
void Receive_ISR(void *parameter)
{
    rt_sem_release(Radio_IRQ_Sem);
    //LOG_D("IRQ Inturrpet\r\n");
}
uint8_t reg_list[]={REG_AX5043_IRQINVERSION0,REG_AX5043_IRQINVERSION1,REG_AX5043_IRQMASK0,REG_AX5043_IRQMASK1,REG_AX5043_IRQREQUEST0,REG_AX5043_IRQREQUEST1};
void readirq(void)
{
    uint16_t temp;
    for(uint8_t i=0;i<sizeof(reg_list);i++)
    {
        temp = SpiReadSingleAddressRegister(reg_list[i]);
        LOG_D("The %d Value is %x",i,temp);
    }
}
MSH_CMD_EXPORT(readirq,readirq);
void Radio_Task_Callback(void *parameter)
{
    while(1)
    {
        static rt_err_t result;
        result = rt_sem_take(Radio_IRQ_Sem, RT_WAITING_FOREVER);
        if (result == RT_EOK)
        {
        switch (ubRFState)
        {
        case trxstate_rx: //0x01
            ReceiveData();
            AX5043Receiver_Continuous();
            if (RxLen != 0)
            {
                Rx_Done_Callback(RXBuff,RxLen,ubRssi);
            }
            break;

        case trxstate_wait_xtal:     //3
            SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK0, 0x00);
            SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK1, 0x00);//AX5043_IRQMASK1 = 0x00 otherwise crystal ready will fire all over again
            ubRFState = trxstate_xtal_ready;
            break;

        case trxstate_pll_ranging:     //5
            SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK0, 0x00);
            SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK1, 0x00);//AX5043_IRQMASK1 = 0x00 otherwise autoranging done will fire all over again
            ubRFState = trxstate_pll_ranging_done;
            break;

        case trxstate_pll_settling:     //7
            SpiWriteSingleAddressRegister(REG_AX5043_RADIOEVENTMASK0, 0x00);// AX5043_RADIOEVENTMASK0 = 0x00;
            ubRFState = trxstate_pll_settled;
            break;

        case trxstate_tx_xtalwait:    //9
            SpiReadSingleAddressRegister(REG_AX5043_RADIOEVENTREQ0); //make sure REVRDONE bit is cleared, so it is a reliable indicator that the packet is out
            SpiWriteSingleAddressRegister(REG_AX5043_FIFOSTAT, 0x03);
            SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK1, 0x00);
            SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK0, 0x08);// enable fifo free threshold
            ubRFState = trxstate_tx_longpreamble;
            TransmitData();
            SpiWriteSingleAddressRegister(REG_AX5043_PWRMODE, AX5043_PWRSTATE_FULL_TX); //AX5043_PWRMODE = AX5043_PWRSTATE_FULL_TX;
            break;
        case trxstate_tx_waitdone:                 //D
            SpiReadSingleAddressRegister(REG_AX5043_RADIOEVENTREQ0);        //clear Interrupt flag
            if (SpiReadSingleAddressRegister(REG_AX5043_RADIOSTATE) != 0)
            {
                break;
            }
            Tx_Done_Callback(TXBuff,TxLen);
            SpiWriteSingleAddressRegister(REG_AX5043_RADIOEVENTMASK0, 0x00);
            SetReceiveMode();           //接收模式
            AX5043Receiver_Continuous();
            break;
        default:
            SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK1, 0x00);
            SpiWriteSingleAddressRegister(REG_AX5043_IRQMASK0, 0x00);
            break;
        }
    }
    }
}
void Init_Timer_Callback(void *parameter)
{
    if(!InitFlag)
    {
        LOG_D("Radio Init Fail\r\n");
        RadioInitFail();
    }
}
void Radio_Task_Init(void)
{
    Radio_IRQ_Sem = rt_sem_create("Radio_IRQ", 0, RT_IPC_FLAG_FIFO);

    Radio_Task = rt_thread_create("Radio_Task", Radio_Task_Callback, RT_NULL, 2048, 10, 10);
    rt_thread_startup(Radio_Task);

    Init_Timer = rt_timer_create("Init_Timer", Init_Timer_Callback, RT_NULL, 3000, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);
    rt_timer_start(Init_Timer);

    InitAX5043();
    SetReceiveMode();           //接收模式
    AX5043ReceiverON();         //接收开启
    RadioDequeueTaskInit();
    LOG_D("Radio Init success,Self ID is %ld\r\n",Self_Id);
    just_ring();
}
/********************************the end of file***********************/
