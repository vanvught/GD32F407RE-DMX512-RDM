/**
 * @file gd32.h
 *
 */
/* Copyright (C) 2021-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

struct HwTimersSeconds {
#if !defined(CONFIG_NET_ENABLE_PTP)
    volatile uint32_t timeval;
#endif
    volatile uint32_t uptime;
};

#include "gd32xxxx.h" // IWYU pragma: keep

#if defined(GD32F30X)
#define bkp_data_write bkp_write_data
#define bkp_data_read bkp_read_data
#endif

#if (defined(GD32F4XX) || defined(GD32H7XX)) && defined(__cplusplus)
typedef enum { BKP_DATA_0, BKP_DATA_1 } bkp_data_register_enum;
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

#ifdef __cplusplus
constexpr uint32_t Gd32PortToGpio(uint32_t port, uint32_t pin) {
    return (port * 16U) + pin;
}

constexpr uint8_t Gd32GpioToPort(uint32_t gpio) {
    return static_cast<uint8_t>(gpio / 16U);
}

constexpr uint8_t Gd32GpioToNumber(uint32_t gpio) {
    return static_cast<uint8_t>(gpio % 16U);
}

#define GD32_PORT_TO_GPIO(p, n) Gd32PortToGpio((p), (n))
#define GD32_GPIO_TO_PORT(g)    Gd32GpioToPort((g))
#define GD32_GPIO_TO_NUMBER(g)  Gd32GpioToNumber((g))
#endif

typedef enum T_GD32_Port { 
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

#include "gd32_board.h" // IWYU pragma: keep

#endif // GD32_H_
