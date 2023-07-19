/**
 * @file dmx_internal.h
 *
 */
/* Copyright (C) 2021-2022 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include "gd32.h"
#include "gd32/dmx_config.h"

static uint32_t _port_to_uart(const uint32_t nPort) {
	switch (nPort) {
#if defined (DMX_USE_USART0)
	case dmx::config::USART0_PORT:
		return USART0;
		break;
#endif
#if defined (DMX_USE_USART1)
	case dmx::config::USART1_PORT:
		return USART1;
		break;
#endif
#if defined (DMX_USE_USART2)
	case dmx::config::USART2_PORT:
		return USART2;
		break;
#endif
#if defined (DMX_USE_UART3)
	case dmx::config::UART3_PORT:
		return UART3;
		break;
#endif
#if defined (DMX_USE_UART4)
	case dmx::config::UART4_PORT:
		return UART4;
		break;
#endif
#if defined (DMX_USE_USART5)
	case dmx::config::USART5_PORT:
		return USART5;
		break;
#endif
#if defined (DMX_USE_UART6)
	case dmx::config::UART6_PORT:
		return UART6;
		break;
#endif
#if defined (DMX_USE_UART7)
	case dmx::config::UART7_PORT:
		return UART7;
		break;
#endif
	default:
		assert(0);
		__builtin_unreachable();
		break;
	}

	assert(0);
	__builtin_unreachable();
	return 0;
}

#if defined (GD32F4XX)
// GPIO
static void _gpio_mode_output(uint32_t gpio_periph, uint32_t pin) {
	gpio_af_set(gpio_periph, GPIO_AF_0, pin); //TODO This needs some research. Is it really needed?
	gpio_mode_set(gpio_periph, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, pin);
}
static void _gpio_mode_af(const uint32_t gpio_periph, uint32_t pin, const uint32_t usart_periph) {
	gpio_mode_set(gpio_periph, GPIO_MODE_AF, GPIO_PUPD_PULLUP, pin);

	switch (usart_periph) {
#if defined (DMX_USE_USART0)
	case USART0:
#endif
#if defined (DMX_USE_USART1)
	case USART1:
#endif
#if defined (DMX_USE_USART2)
	case USART2:
#endif
#if defined (DMX_USE_USART0) || defined (DMX_USE_USART1) || defined(DMX_USE_USART2)
		gpio_af_set(gpio_periph, GPIO_AF_7, pin);
		break;
#endif
#if defined (DMX_USE_UART3)
	case UART3:
#endif
#if defined (DMX_USE_UART4)
	case UART4:
#endif
#if defined (DMX_USE_USART5)
	case USART5:
#endif
#if defined (DMX_USE_UART6)
	case UART6:
#endif
#if defined (DMX_USE_UART7)
	case UART7:
#endif
#if defined (DMX_USE_UART3) || defined (DMX_USE_UART4) || defined(DMX_USE_USART5) || defined (DMX_USE_UART6) || defined (DMX_USE_UART7)
		gpio_af_set(gpio_periph, GPIO_AF_8, pin);
		break;
#endif
	default:
		assert(0);
		__builtin_unreachable();
		break;
	}
}
// DMA
# define DMA_PARAMETER_STRUCT				dma_single_data_parameter_struct
# define DMA_CHMADDR						DMA_CHM0ADDR
# define DMA_MEMORY_TO_PERIPHERAL			DMA_MEMORY_TO_PERIPH
# define DMA_PERIPHERAL_WIDTH_8BIT			DMA_PERIPH_WIDTH_8BIT
# define dma_init							dma_single_data_mode_init
# define dma_memory_to_memory_disable(x,y)
# define DMA_INTERRUPT_ENABLE				(DMA_CHXCTL_FTFIE)
# define DMA_INTERRUPT_DISABLE				(DMA_CHXCTL_FTFIE | DMA_CHXCTL_HTFIE | DMA_CHXFCTL_FEEIE)
# define DMA_INTERRUPT_FLAG_GET				(DMA_INT_FLAG_FTF)
# define DMA_INTERRUPT_FLAG_CLEAR			(DMA_INT_FLAG_FTF | DMA_INT_FLAG_TAE)
#else
// GPIO
static void _gpio_mode_output(uint32_t gpio_periph, uint32_t pin) {
	gpio_init(gpio_periph, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, pin);
}
static void _gpio_mode_af(uint32_t gpio_periph, uint32_t pin, __attribute__((unused)) const uint32_t usart_periph ) {
	gpio_init(gpio_periph, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, pin);
}
// DMA
# define DMA_PARAMETER_STRUCT				dma_parameter_struct
# define DMA_INTERRUPT_ENABLE				(DMA_INT_FTF)
# define DMA_INTERRUPT_DISABLE				(DMA_INT_FTF | DMA_INT_HTF | DMA_INT_ERR)
# define DMA_INTERRUPT_FLAG_GET				(DMA_INT_FLAG_FTF)
# define DMA_INTERRUPT_FLAG_CLEAR			(DMA_INT_FLAG_FTF | DMA_INT_FLAG_G)
#endif

#endif /* GD32_DMX_INTERNAL_H_ */
