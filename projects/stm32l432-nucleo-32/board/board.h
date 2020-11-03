/**
 ***********************************************************************************************************************
 * Copyright (c) 2020, China Mobile Communications Group Co.,Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with 
 * the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 *
 * @file        board.h
 *
 * @brief       Board resource definition
 *
 * @revision
 * Date         Author          Notes
 * 2020-02-20   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <stm32l4xx.h>
#include "drv_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STM32_FLASH_START_ADRESS ((uint32_t)0x08000000)
#define STM32_FLASH_SIZE         (256 * 1024)
#define STM32_FLASH_END_ADDRESS  ((uint32_t)(STM32_FLASH_START_ADRESS + STM32_FLASH_SIZE))

#define STM32_SRAM1_SIZE  (48)
#define STM32_SRAM1_START (0x20000000)
#define STM32_SRAM1_END   (STM32_SRAM1_START + STM32_SRAM1_SIZE * 1024)

#define HEAP_BEGIN STM32_SRAM1_START
#define HEAP_END   STM32_SRAM1_END

extern const struct push_button key_table[];
extern const int                key_table_size;

extern const led_t led_table[];
extern const int   led_table_size;

void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif

#endif
