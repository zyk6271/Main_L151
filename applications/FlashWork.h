#ifndef __FLASHWORK_H__
#define __FLASHWORK_H__
#endif
#include "stdint.h"

#define MaxSupport 13
typedef struct
{
    uint8_t  LastFlag;
    uint32_t Num;
    uint32_t DoorNum;
    uint32_t GatewayNum;
    uint32_t ID[20];
    uint8_t Reponse[20];
    uint32_t ID_Time[20];
    uint32_t Alive[20];
    uint8_t  Bat[20];
    uint8_t  Rssi[20];
}Device_Info;

#define NormalOff   1
#define OtherOff    2
#define NormalOpen  3
#define OtherOpen   4

int flash_Init(void);
uint32_t Flash_Get(uint32_t id);
uint8_t Flash_Get_Key_Valid(uint32_t key);
uint8_t Flash_GetRssi(uint32_t Device_ID);//查询内存中的RSSI
uint8_t Remote_Delete(uint32_t Device_ID);
void Flash_Key_Change(uint32_t key,uint32_t value);
void Flash_Set(uint8_t id,uint32_t value);
void Flash_Factory(void);
uint8_t Add_Device(uint32_t Device_ID);
void Update_All_Time(void);
uint8_t Clear_Device_Time(uint32_t Device_ID);//更新时间戳为0
void Detect_All_Time(void);
void Boot_Times_Record(void);
void LoadDevice2Memory(void);
uint8_t AckCheck(uint32_t device);
void AckClear(uint32_t device);
void AckSet(uint32_t device);
uint8_t Add_DoorDevice(uint32_t Device_ID);
uint8_t Add_GatewayDevice(uint32_t Device_ID);
uint32_t GetDoorID(void);
uint32_t GetGatewayID(void);
void DeleteAllDevice(void);//数据载入到内存中;
void Flash_Moto_Change(uint8_t value);
uint32_t Flash_Get_Moto_Flag(void);
uint32_t Flash_Get_Door_Nums(void);
uint32_t Flash_Get_Gateway_Nums(void);
uint8_t Update_Device_Bat(uint32_t Device_ID,uint8_t bat);
uint8_t Update_Device_Rssi(uint32_t Device_ID,uint8_t rssi);//更新Rssi;
void Offline_React(uint32_t ID);
uint8_t Device_RssiGet(uint32_t Device_ID);
void Device_RssiChange(uint32_t Device_ID,uint8_t value);
uint8_t Device_AliveGet(uint32_t Device_ID);
uint8_t Device_AliveChange(uint32_t Device_ID,uint8_t value);
uint8_t Flash_AliveGet(uint32_t Device_ID);
void Flash_AliveChange(uint32_t Device_ID,uint8_t value);
uint8_t Device_BatGet(uint32_t Device_ID);
void Device_BatChange(uint32_t Device_ID,uint8_t value);
uint8_t Delete_Device(uint32_t device_id);
void Flash_Moto1Total_Add(void);
void Flash_Moto1Success_Add(void);
void Flash_Moto2Total_Add(void);
void Flash_Moto2Success_Add(void);

