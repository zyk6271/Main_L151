/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-12-5      SummerGift   first version
 */

#include "board.h"

#ifdef BSP_USING_ON_CHIP_FLASH
#include "drv_config.h"
#include "drv_flash.h"

#if defined(RT_USING_FAL)
#include "fal.h"
#endif

#define DBG_TAG "drv.flash"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/**
  * @brief  Gets the page of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static uint32_t GetPage(uint32_t addr)
{
    uint32_t page = 0;

    page = RT_ALIGN_DOWN(addr - STM32_FLASH_START_ADRESS, FLASH_PAGE_SIZE)/FLASH_PAGE_SIZE;

    return page;
}


/**
 * Read data from flash.
 * @note This operation's units is word.
 *
 * @param addr flash address
 * @param buf buffer to store read data
 * @param size read bytes size
 *
 * @return result
 */
int stm32_flash_read(rt_uint32_t addr, rt_uint8_t *buf, size_t size)
{
    size_t i;

    if ((addr + size) > STM32_FLASH_END_ADDRESS)
    {
        LOG_E("read outrange flash size! addr is (0x%p)", (void*)(addr + size));
        return -RT_EINVAL;
    }

    for (i = 0; i < size; i++, buf++, addr++)
    {
        *buf = *(rt_uint8_t *) addr;
    }

    return size;
}

/**
 * Write data to flash.
 * @note This operation's units is word.
 * @note This operation must after erase. @see flash_erase.
 *
 * @param addr flash address
 * @param buf the write data buffer
 * @param size write bytes size
 *
 * @return result
 */

int stm32_flash_write(rt_uint32_t addr, const uint8_t *buf, size_t size)
{
    rt_err_t result = 0;

    size_t i, j;

    rt_uint32_t write_data = 0, temp_data = 0;

    if ((addr + size) > STM32_FLASH_END_ADDRESS)
    {
        LOG_E("ERROR: write outrange flash size! addr is (0x%p)\n", (void*)(addr + size));
        return -RT_EINVAL;
    }

    if(addr % 4 != 0)
    {
        LOG_E("write addr must be 4-byte alignment,addr is %08X\r\n",addr);
        return -RT_EINVAL;
    }

    L151_Flash_Init(0,0,2);

    if (size < 1)
    {
        LOG_E("size is < 1,DATA is %02X\r\n",*buf);
        return -RT_ERROR;
    }

    for (i = 0; i < size;)
    {
        if ((size - i) < 4)
        {
            for (j = 0; (size - i) > 0; i++, j++)
            {
                temp_data = *buf;
                write_data = (write_data) | (temp_data << 8 * j);
                buf ++;
            }
        }
        else
        {
            for (j = 0; j < 4; j++, i++)
            {
                temp_data = *buf;
                write_data = (write_data) | (temp_data << 8 * j);
                buf ++;
            }
        }

        LOG_D("START write : addr (0x%p), value %08X", (void*)addr, write_data);

        result = L151_Flash_ProgramWord(addr,write_data);
        if (result == HAL_OK)
        {
            /* Check the written value */
            if (*(uint32_t *)addr != write_data)
            {
                LOG_E("ERROR: write data %08X != read data %08X\n",write_data,*(uint32_t *)addr);
                result = -RT_ERROR;
                goto __exit;
            }
            else
            {
                LOG_D("Correct: write data %08X == read data %08X\n",write_data,*(uint32_t *)addr);
            }
        }
        else
        {
            result = -RT_ERROR;
            goto __exit;
        }

        temp_data = 0;
        write_data = 0;

        addr += 4;
    }

__exit:
    L151_Flash_UnInit(2);

    if (result != 0)
    {
        return result;
    }

    return size;
}

/**
 * Erase data on flash.
 * @note This operation is irreversible.
 * @note This operation's units is different which on many chips.
 *
 * @param addr flash address
 * @param size erase bytes size
 *
 * @return result
 */
int stm32_flash_erase(rt_uint32_t addr, size_t size)
{
    rt_err_t result = RT_EOK;
    uint32_t NbPages,PageAddress = 0;

    if ((addr + size) > STM32_FLASH_END_ADDRESS)
    {
        LOG_E("ERROR: erase outrange flash size! addr is (0x%p)\n", (void*)(addr + size));
        return -RT_EINVAL;
    }

    L151_Flash_Init(0,0,1);

    NbPages = (size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
    PageAddress = GetPage(addr);

    for (uint32_t index = PageAddress; index < PageAddress + NbPages; index++)
    {
        result = L151_Flash_EraseSector(index);
    }

    if (result != HAL_OK)
    {
        LOG_D("erase failed: addr (0x%p), size %d", (void*)addr, size);
        result = -RT_ERROR;
        goto __exit;
    }

__exit:
    L151_Flash_UnInit(1);

    if (result != RT_EOK)
    {
        return result;
    }

    LOG_D("erase done: addr (0x%p), size %d", (void*)addr, size);
    return size;
}

#if defined(RT_USING_FAL)

static int fal_flash_read(long offset, rt_uint8_t *buf, size_t size);
static int fal_flash_write(long offset, const rt_uint8_t *buf, size_t size);
static int fal_flash_erase(long offset, size_t size);

const struct fal_flash_dev stm32_onchip_flash = { "onchip_flash", STM32_FLASH_START_ADRESS, STM32_FLASH_SIZE, FLASH_PAGE_SIZE, {NULL, fal_flash_read, fal_flash_write, fal_flash_erase} };

static int fal_flash_read(long offset, rt_uint8_t *buf, size_t size)
{
   return stm32_flash_read(stm32_onchip_flash.addr + offset, buf, size);
}

static int fal_flash_write(long offset, const rt_uint8_t *buf, size_t size)
{
    return stm32_flash_write(stm32_onchip_flash.addr + offset, buf, size);
}

static int fal_flash_erase(long offset, size_t size)
{
    return stm32_flash_erase(stm32_onchip_flash.addr + offset, size);
}

#endif
#endif /* BSP_USING_ON_CHIP_FLASH */
