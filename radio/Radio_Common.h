#ifndef _RADIO_COMMON_H_
#define _RADIO_COMMON_H_

#include "rtthread.h"
#include "radio_drv.h"

#define CHANNEL_NUM     1

typedef union {
    struct {
        uint8_t svio : 1;               //(Sticky) IO Voltage Large Enough (not Brownout)
        uint8_t sbevmodem : 1;          //(Sticky) Modem Domain Voltage Brownout Error (Inverted; 0 = Brownout, 1 = Power OK)
        uint8_t sbevana : 1;            //(Sticky) Analog Domain Voltage Brownout Error (Inverted; 0 = Brownout, 1 = Power OK)
        uint8_t svmodem : 1;            //(Sticky) Modem Domain Voltage Regulator Ready
        uint8_t svana : 1;              //(Sticky) Analog Domain Voltage Regulator Ready
        uint8_t svref : 1;              //(Sticky) Reference Voltage Regulator Ready
        uint8_t sref : 1;               //(Sticky) Reference Ready
        uint8_t ssum : 1;               //(Sticky) Summary Ready Status (one when all unmasked POWIRQMASK power sources are ready)
    };
    uint8_t raw;
} PwrStatus;

void Ax5043_Reset(struct ax5043 *dev);
char SetChannel(struct ax5043 *dev,uint8_t ubNum);
void vcoi_rng_get(struct ax5043 *dev);
void InitAx5043REG(struct ax5043 *dev);
uint8_t Ax5043SetRegisters_TX(struct ax5043 *dev);
uint8_t Ax5043SetRegisters_RX(struct ax5043 *dev);
void RdioXtalON(struct ax5043 *dev);
void ChangeMaxPower(struct ax5043 *dev);
void BackNormalPower(struct ax5043 *dev);
void SetTransmitMode(struct ax5043 *dev);
void AX5043ReceiverON(struct ax5043 *dev);
void AX5043Receiver_Continuous(struct ax5043 *dev);
void SetReceiveMode(struct ax5043 *dev);
void ReceiveData(struct ax5043 *dev);
void Ax5043_OFF(struct ax5043 *dev);
void transmit_packet_task(struct ax5043 *dev,char *buffer, uint8_t len);
uint8_t rf_startup(struct ax5043 *dev);
void RF_Send(struct ax5043 *dev,char *send_buf, uint8_t send_len);
void Radio_Task_Init(void);
void TransmitData(struct ax5043 *dev);
void rf_restart(struct ax5043 *dev);

#endif

