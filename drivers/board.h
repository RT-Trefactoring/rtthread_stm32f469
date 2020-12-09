/*
 * File      : board.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-09-22     Bernard      add board.h to this bsp
 */
// <<< Use Configuration Wizard in Context Menu >>>
#ifndef __BOARD_H__
#define __BOARD_H__
#include <stm32f4xx.h>
#include <stm32f4xx_hal.h>
#include <rtthread.h>

#define IO_D0	186
#define IO_D1	191
#define IO_D2	190
#define IO_D3	49
#define IO_D4	189
#define IO_D5	50
#define IO_D6	61
#define IO_D7	188
#define IO_D8	187
#define IO_D9	62
#define IO_D10	102
#define IO_D11	114
#define IO_D12	113
#define IO_D13	175
#define IO_D14	207
#define IO_D15	206

#define IO_A0	67
#define IO_A1	40
#define IO_A2	41
#define IO_A3	63
#define IO_A4	64
#define IO_A5	59


#define STM32F4xx_PIN_NUMBERS 216
#define STM32_SRAM_SIZE (384-64)
#define STM32_SRAM_END          (0x20000000 + STM32_SRAM_SIZE * 1024)

#if defined(__CC_ARM) || defined(__CLANG_ARM)
    extern int Image$$RW_IRAM1$$ZI$$Limit;
    #define HEAP_BEGIN  ((void *)&Image$$RW_IRAM1$$ZI$$Limit)
#elif __ICCARM__
#pragma section="HEAP"
#define HEAP_BEGIN  (__segment_end("HEAP"))
#else
extern int __bss_end;
#define HEAP_BEGIN  ((void *)&__bss_end)
#endif

#define HEAP_END    STM32_SRAM_END
#define HEAP_SIZE   ((uint32_t)HEAP_END - (uint32_t)HEAP_BEGIN)

#define SDRAM_BEGIN (0xC0000000u)
#define SDRAM_END   (0xC0800000u)

extern void rt_hw_board_init(void);

#endif

