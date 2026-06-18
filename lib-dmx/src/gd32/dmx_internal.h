/**
 * @file dmx_internal.h
 *
 */
/* Copyright (C) 2021-2026 by Arjan van Vught mailto:info@gd32-dmx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of thnDmxDataDirecte Software, and to permit persons to whom the Software is
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

#ifndef GD32_DMX_INTERNAL_H_
#define GD32_DMX_INTERNAL_H_

#include <cstdint>
#include <cassert>

#include "gd32_gpio.h"

#ifdef ALIGNED
#undef ALIGNED
#endif
#define ALIGNED __attribute__((aligned(4)))

// Needed for older GD32F firmware
#if !defined(USART_TRANSMIT_DMA_ENABLE)
#define USART_TRANSMIT_DMA_ENABLE USART_DENT_ENABLE
#endif

// GD32F10X is different
#if defined(GD32F10X)
#define USART_FLAG_IDLE USART_FLAG_IDLEF
#endif

// https://www.gd32-dmx.org/memory.html
#if defined(GD32F20X) || defined(GD32F4XX) || defined(GD32H7XX)
#define SECTION_DMA_BUFFER __attribute__((section(".dmx")))
#else
#define SECTION_DMA_BUFFER
#endif

inline constexpr uint32_t DmxPortToUart(uint32_t port) {
    switch (port) {
#if defined(DMX_USE_USART0)
        case dmx::config::kUsart0Port:
            return USART0;
            break;
#endif
#if defined(DMX_USE_USART1)
        case dmx::config::kUsart1Port:
            return USART1;
            break;
#endif
#if defined(DMX_USE_USART2)
        case dmx::config::kUsart2Port:
            return USART2;
            break;
#endif
#if defined(DMX_USE_UART3)
        case dmx::config::kUart3Port:
            return UART3;
            break;
#endif
#if defined(DMX_USE_UART4)
        case dmx::config::kUart4Port:
            return UART4;
            break;
#endif
#if defined(DMX_USE_USART5)
        case dmx::config::kUsart5Port:
            return USART5;
            break;
#endif
#if defined(DMX_USE_UART6)
        case dmx::config::kUart6Port:
            return UART6;
            break;
#endif
#if defined(DMX_USE_UART7)
        case dmx::config::kUart7Port:
            return UART7;
            break;
#endif
        default:
            [[unlikely]] assert(0);
            return 0;
    }

    assert(0);
    return 0;
}

#if defined(GD32F4XX) || defined(GD32H7XX)
inline constexpr uint32_t GetUsartAf(uint32_t usart_periph) {
    switch (usart_periph) {
#if defined(DMX_USE_USART0)
        case USART0:
            return USART0_GPIO_AFx;
#endif
#if defined(DMX_USE_USART1)
        case USART1:
            return USART1_GPIO_AFx;
#endif
#if defined(DMX_USE_USART2)
        case USART2:
            return USART2_GPIO_AFx;
#endif
#if defined(DMX_USE_UART3)
        case UART3:
            return UART3_GPIO_AFx;
#endif
#if defined(DMX_USE_UART4)
        case UART4:
            return UART4_GPIO_AFx;
#endif
#if defined(DMX_USE_USART5)
        case USART5:
            return USART5_GPIO_AFx;
#endif
#if defined(DMX_USE_UART6)
        case UART6:
            return UART6_GPIO_AFx;
#endif
#if defined(DMX_USE_UART7)
        case UART7:
            return UART7_GPIO_AFx;
#endif
        default:
            [[unlikely]] assert(0);
			return 0;
    }

	assert(0);
    return 0;
}

template <uint32_t gpio_periph, uint32_t pin>
inline void Gd32GpioModeOutput() {
    Gd32GpioModeSet<gpio_periph, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, pin>();
}

template <uint32_t gpio_periph, uint32_t pin, uint32_t usart_periph>
inline void Gd32GpioModeAf() {
    Gd32GpioModeSet<gpio_periph, GPIO_MODE_AF, GPIO_PUPD_PULLUP, pin>();

    constexpr uint32_t kAf = GetUsartAf(usart_periph);
    static_assert(kAf != 0, "Invalid USART peripheral");

    Gd32GpioAfSet<gpio_periph, kAf, pin>();
}
#else
template <uint32_t gpio_periph, uint32_t pin>
inline void Gd32GpioModeOutput() {
    gd32_gpio_init<gpio_periph, GPIO_MODE_OUT_PP, pin>();
}

template <uint32_t gpio_periph, uint32_t pin, uint32_t usart_periph>
inline void Gd32GpioModeAf() {
    gd32_gpio_init<gpio_periph, GPIO_MODE_AF_PP, pin>();
}
#endif

inline void DisableTimerIrqUart(uint32_t gpio_periph) {
    switch (gpio_periph) {
#if defined(DMX_USE_USART0)
        case USART0:
            timer_interrupt_disable(TIMER1, TIMER_INT_CH0);
            break;
#endif
#if defined(DMX_USE_USART1)
        case USART1:
            timer_interrupt_disable(TIMER1, TIMER_INT_CH1);
            break;
#endif
#if defined(DMX_USE_USART2)
        case USART2:
            timer_interrupt_disable(TIMER1, TIMER_INT_CH2);
            break;
#endif
#if defined(DMX_USE_UART3)
        case UART3:
            timer_interrupt_disable(TIMER1, TIMER_INT_CH3);
            break;
#endif
#if defined(DMX_USE_UART4)
        case UART4:
            timer_interrupt_disable(TIMER4, TIMER_INT_CH0);
            break;
#endif
#if defined(DMX_USE_USART5)
        case USART5:
            timer_interrupt_disable(TIMER4, TIMER_INT_CH1);
            break;
#endif
#if defined(DMX_USE_UART6)
        case UART6:
            timer_interrupt_disable(TIMER4, TIMER_INT_CH2);
            break;
#endif
#if defined(DMX_USE_UART7)
        case UART7:
            timer_interrupt_disable(TIMER4, TIMER_INT_CH3);
            break;
#endif
        default:
            [[unlikely]] assert(0);
            break;
    }
}

#endif // GD32_DMX_INTERNAL_H_
