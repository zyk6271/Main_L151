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
void WarUpload_GW(uint32_t device_id,uint8_t warn_id,uint8_t value);
void ControlUpload_GW(uint32_t device_id,uint8_t control_id,uint8_t value);

#endif /* APPLICATIONS_GATEWAY_H_ */
