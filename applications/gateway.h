/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-07-14     Rick       the first version
 */
#ifndef APPLICATIONS_GATEWAY_H_
#define APPLICATIONS_GATEWAY_H_

#include "rtthread.h"

void Gateway_Init(void);
void Gateway_Reload(void);
void Heart_Test_Start(void);
void Gateway_Sync(void);
void Gateway_RemoteDelete(void);
void WarUpload_GW(uint8_t ack,uint32_t device_id,uint8_t warn_id,uint8_t value);
void ControlUpload_GW(uint8_t ack,uint32_t device_id,uint8_t control_id,uint8_t value);
void Replace_Door(uint32_t old);
void Heart_Refresh(void);

#endif /* APPLICATIONS_GATEWAY_H_ */
