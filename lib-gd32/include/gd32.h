/**
 * @file gd32.h
 *
 */
/* Copyright (C) 2021-2025 by Arjan van Vught mailto:info@gd32-dmx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
 #ifndef GD32_H_
 #define GD32_H_

#include <stdint.h>

#ifdef __cplusplus
#if !defined(UDELAY)
#define UDELAY
void udelay(uint32_t us, uint32_t offset = 0);
#endif
#endif

struct HwTimersSeconds
{
#if !defined(CONFIG_NET_ENABLE_PTP)
    volatile uint32_t nTimeval;
#endif
    volatile uint32_t nUptime;
};

/*
 * Needed for GD32 Firmware and CMSIS
 */
#ifdef __cplusplus
#if !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#if __cplusplus > 201402
// error: compound assignment with 'volatile'-qualified left operand is deprecated
#pragma GCC diagnostic ignored "-Wvolatile"
#endif
#endif
extern "C"
{
#endif

#if defined(GD32F10X_HD) || defined(GD32F10X_CL)
#include "gd32f10x.h"
#elif defined(GD32F20X_CL)
#include "gd32f20x.h"
#elif defined(GD32F30X_HD)
#include "gd32f30x.h"
#elif defined(GD32F407) || defined(GD32F450) || defined(GD32F470)
#include "gd32f4xx.h"
#elif defined(GD32H757) || defined(GD32H759)
#include "gd32h7xx.h"
#else
#error MCU is not supported
#endif

#ifdef __cplusplus
}
#endif

#if defined(GD32F30X)
#define bkp_data_write bkp_write_data
#define bkp_data_read bkp_read_data
#endif

#if (defined(GD32F4XX) || defined(GD32H7XX)) && defined(__cplusplus)
typedef enum
{
    BKP_DATA_0,
    BKP_DATA_1
} bkp_data_register_enum;
void bkp_data_write(bkp_data_register_enum register_number, uint16_t data);
uint16_t bkp_data_read(bkp_data_register_enum register_number);
#endif

#if !(defined(GD32F4XX) || defined(GD32H7XX))
#define GPIO_INIT
#endif

#if defined(GD32H7XX)
#define GPIO_OSPEED GPIO_OSPEED_60MHZ
#else
#define GPIO_OSPEED GPIO_OSPEED_50MHZ
#endif

#define GD32_PORT_TO_GPIO(p, n) ((p * 16) + n)
#define GD32_GPIO_TO_PORT(g) (uint8_t)(g / 16)
#define GD32_GPIO_TO_NUMBER(g) (uint8_t)(g - (16 * GD32_GPIO_TO_PORT(g)))

typedef enum T_GD32_Port
{
    GD32_GPIO_PORTA = 0,
    GD32_GPIO_PORTB,
    GD32_GPIO_PORTC,
    GD32_GPIO_PORTD,
    GD32_GPIO_PORTE,
    GD32_GPIO_PORTF,
    GD32_GPIO_PORTG,
    GD32_GPIO_PORTH,
    GD32_GPIO_PORTI,
    GD32_GPIO_PORTJ,
    GD32_GPIO_PORTK
} GD32_Port_TypeDef;

#include "gd32_board.h"

#endif  // GD32_H_
