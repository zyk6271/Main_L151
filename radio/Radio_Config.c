#include "Radio.h"
#include "AX5043.h"
#include "Radio_Config.h"

#define DBG_TAG "radio"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

void WirelessBitRateConfigure(uint8_t BitRate0)
{
    SpiWriteLongAddressRegister(REG_AX5043_IFFREQ1  , 0x01 );
    SpiWriteLongAddressRegister(REG_AX5043_IFFREQ0  , 0xE4  );
    SpiWriteLongAddressRegister(REG_AX5043_DECIMATION , 0x16  );
    SpiWriteLongAddressRegister(REG_AX5043_RXDATARATE2 , 0x00 );
    SpiWriteLongAddressRegister(REG_AX5043_RXDATARATE1 , 0x3D );
    SpiWriteLongAddressRegister(REG_AX5043_RXDATARATE0 , 0x8D  );
    SpiWriteLongAddressRegister( REG_AX5043_MAXRFOFFSET1 , 0x02);
    SpiWriteLongAddressRegister( REG_AX5043_MAXRFOFFSET0 , 0x31);
    SpiWriteLongAddressRegister(REG_AX5043_FSKDMAX1,0x00);
    SpiWriteLongAddressRegister(REG_AX5043_FSKDMAX0,0xA6);
    SpiWriteLongAddressRegister(REG_AX5043_FSKDMIN1,0xFF);
    SpiWriteLongAddressRegister(REG_AX5043_FSKDMIN0,0x5A);

    SpiWriteLongAddressRegister(REG_AX5043_AGCGAIN0  , 0xC5);
    SpiWriteLongAddressRegister( REG_AX5043_AGCTARGET0 , 0x84);
    SpiWriteLongAddressRegister( REG_AX5043_TIMEGAIN0  , 0xF8);
    SpiWriteLongAddressRegister(REG_AX5043_DRGAIN0  , 0xF2);
    SpiWriteLongAddressRegister(REG_AX5043_PHASEGAIN0  , 0xC3);
    SpiWriteLongAddressRegister( REG_AX5043_FREQUENCYGAINC0, 0x09);
    SpiWriteLongAddressRegister( REG_AX5043_FREQUENCYGAIND0, 0x09);
    SpiWriteLongAddressRegister( REG_AX5043_AGCGAIN1   , 0xC5);
    SpiWriteLongAddressRegister( REG_AX5043_TIMEGAIN1   , 0xF6);
    SpiWriteLongAddressRegister( REG_AX5043_DRGAIN1   , 0xF1 );
    SpiWriteLongAddressRegister(REG_AX5043_PHASEGAIN1  , 0xc3);
    SpiWriteLongAddressRegister( REG_AX5043_FREQUENCYGAINC1, 0x09);
    SpiWriteLongAddressRegister( REG_AX5043_FREQUENCYGAIND1, 0x09);

    SpiWriteLongAddressRegister( REG_AX5043_TIMEGAIN3   , 0xF5);
    SpiWriteLongAddressRegister( REG_AX5043_DRGAIN3,0xF0);
    SpiWriteLongAddressRegister(REG_AX5043_PHASEGAIN3  , 0xC3);

    SpiWriteLongAddressRegister(REG_AX5043_FREQUENCYGAINC3, 0x0C);
    SpiWriteLongAddressRegister(REG_AX5043_FREQUENCYGAIND3, 0x0C);
    SpiWriteLongAddressRegister( REG_AX5043_FSKDEV2   , 0x00  );
    SpiWriteLongAddressRegister(REG_AX5043_FSKDEV1   , 0x04  );
    SpiWriteLongAddressRegister(REG_AX5043_FSKDEV0   , 0x08  );
    SpiWriteLongAddressRegister( REG_AX5043_MODCFGA   , 0x05  );
    SpiWriteLongAddressRegister( REG_AX5043_TXRATE2   , 0x00  );
    SpiWriteLongAddressRegister(REG_AX5043_TXRATE1   , 0x0C );
    SpiWriteLongAddressRegister( REG_AX5043_TXRATE0   , 0x19 );
    SpiWriteLongAddressRegister(REG_AX5043_BBTUNE   , 0x0F  );
    SpiWriteLongAddressRegister(REG_AX5043_RSSIREFERENCE   , 0xFA);
}

uint32_t WirelessFreqConfigure(uint8_t Freq2, uint8_t Freq1, uint8_t Freq0, uint8_t Channel_Num)
{
    uint32_t Freq_centr = 0;
    uint32_t FreqCentr_cal = 0;
    Freq_centr = Freq2;
    Freq_centr = Freq_centr << 8;
    Freq_centr += Freq1;
    Freq_centr = Freq_centr << 8;
    Freq_centr += Freq0;

    if (Freq_centr > 910000 || Freq_centr < 300000)
    {
        Freq_centr = 433000;
    }
    //Freq_centr = Freq_centr * 1000;
    Freq_centr += CHANNEL_BW * Channel_Num;
    FreqCentr_cal = (uint32_t)((double)(Freq_centr/(double)(XTAL_FREQ/1000))*1024*1024*16);
    //FreqCentr_cal = 0x0906eeef;
    return FreqCentr_cal;
}

void WirelessTxPowerConfigure(uint8_t TxPower)
{
    switch (TxPower)
    {
        case 0:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x02);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0x07);
            break;
        case 1:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x02);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0x44);
            break;
        case 2:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x02);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0x90);
            break;
        case 3:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x02);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0xEB);
            break;
        case 4:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x03);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0x5E);
            break;
        case 5:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x03);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0xD6);
            break;
        case 6:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x04);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0x06);
            break;
        case 7:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x04);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0xA9);
            break;
        case 8:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x05);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0x7C);
            break;
        case 9:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x06);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0x00);
            break;
        case 10:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x07);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0x00);
            break;
        case 11:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x08);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0x00);
            break;
        case 12:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x09);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0xD4);
            break;
        case 13:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x0C);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0x00);
            break;
        case 14:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x0F);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0x00);
        case 15:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x0F);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0xFF);
            break;
        default:
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB1, 0x02);
            SpiWriteLongAddressRegister(REG_AX5043_TXPWRCOEFFB0, 0x07);
            break;
    }
}
