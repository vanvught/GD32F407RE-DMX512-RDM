/**
 * @file gd32h7xx_mcu.h
 *
 */
/* Copyright (C) 2024 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef MCU_GD32H7XX_MCU_H_
#define MCU_GD32H7XX_MCU_H_

#if !defined(MCU_GD32H759_MCU_H_)
# error This file should not be included
#endif

#include <gd32h7xx_libopt.h>

/**
 * U(S)ART
 */

#define USART0_RCU_CLK		RCU_USART0
#if defined (USART0_REMAP)
# define USART0_GPIO_PORT	GPIOB
# define USART0_GPIO_CLK	RCU_GPIOB
# define USART0_TX_PIN		GPIO_PIN_6
# define USART0_RX_PIN		GPIO_PIN_7
#else
# define USART0_GPIO_PORT	GPIOA
# define USART0_GPIO_CLK	RCU_GPIOA
# define USART0_TX_PIN		GPIO_PIN_9
# define USART0_RX_PIN		GPIO_PIN_10
#endif

#define USART1_RCU_CLK		RCU_USART1
#if defined (USART1_REMAP)
# define USART1_GPIO_PORT	GPIOD
# define USART1_GPIO_CLK	RCU_GPIOD
# define USART1_TX_PIN		GPIO_PIN_5
# define USART1_RX_PIN		GPIO_PIN_6
#else
# define USART1_GPIO_PORT	GPIOA
# define USART1_GPIO_CLK	RCU_GPIOA
# define USART1_TX_PIN		GPIO_PIN_2
# define USART1_RX_PIN		GPIO_PIN_3
#endif

#define USART2_RCU_CLK		RCU_USART2
#if defined (USART2_FULL_REMAP)
# define USART2_GPIO_PORT	GPIOD
# define USART2_GPIO_CLK	RCU_GPIOD
# define USART2_TX_PIN		GPIO_PIN_8
# define USART2_RX_PIN		GPIO_PIN_9
#elif defined (USART2_PARTIAL_REMAP)
# define USART2_GPIO_PORT	GPIOC
# define USART2_GPIO_CLK	RCU_GPIOC
# define USART2_TX_PIN		GPIO_PIN_10
# define USART2_RX_PIN		GPIO_PIN_11
#else
# define USART2_GPIO_PORT	GPIOB
# define USART2_GPIO_CLK	RCU_GPIOB
# define USART2_TX_PIN		GPIO_PIN_10
# define USART2_RX_PIN		GPIO_PIN_11
#endif

#define UART3_RCU_CLK		RCU_UART3
#if defined (UART3_REMAP)
# define UART3_GPIO_PORT	GPIOA
# define UART3_GPIO_CLK		RCU_GPIOA
# define UART3_TX_PIN		GPIO_PIN_0
# define UART3_RX_PIN		GPIO_PIN_1
#else
# define UART3_GPIO_PORT	GPIOC
# define UART3_GPIO_CLK		RCU_GPIOC
# define UART3_TX_PIN		GPIO_PIN_10
# define UART3_RX_PIN		GPIO_PIN_11
#endif

#define UART4_RCU_CLK		RCU_UART4
#define UART4_GPIO_TX_PORT	GPIOC
#define UART4_GPIO_TX_CLK	RCU_GPIOC
#define UART4_GPIO_RX_PORT	GPIOD
#define UART4_GPIO_RX_CLK	RCU_GPIOD
#define UART4_TX_PIN		GPIO_PIN_12
#define UART4_RX_PIN		GPIO_PIN_2

#define USART5_RCU_CLK		RCU_USART5
#if defined (USART5_REMAP)
# define USART5_GPIO_PORT	GPIOG
# define USART5_GPIO_CLK	RCU_GPIOG
# define USART5_TX_PIN		GPIO_PIN_14
# define USART5_RX_PIN		GPIO_PIN_9
#else
# define USART5_GPIO_PORT	GPIOC
# define USART5_GPIO_CLK	RCU_GPIOC
# define USART5_TX_PIN		GPIO_PIN_6
# define USART5_RX_PIN		GPIO_PIN_7
#endif

#define UART6_RCU_CLK		RCU_UART6
#if defined (UART6_REMAP)
# define UART6_GPIO_PORT	GPIOF
# define UART6_GPIO_CLK		RCU_GPIOF
# define UART6_TX_PIN		GPIO_PIN_7
# define UART6_RX_PIN		GPIO_PIN_6
#else
# define UART6_GPIO_PORT	GPIOE
# define UART6_GPIO_CLK		RCU_GPIOE
# define UART6_TX_PIN		GPIO_PIN_8
# define UART6_RX_PIN		GPIO_PIN_7
#endif

#define UART7_RCU_CLK		RCU_UART7
#define UART7_GPIO_PORT		GPIOE
#define UART7_GPIO_CLK		RCU_GPIOE
#define UART7_TX_PIN		GPIO_PIN_1
#define UART7_RX_PIN		GPIO_PIN_0

/**
 * I2C
 */

#define I2C1_PERIPH			I2C1
#define I2C1_RCU_CLK		RCU_I2C1
#define I2C1_SCL_GPIOx		GPIOH
#define I2C1_SCL_RCU_GPIOx	RCU_GPIOH
#define I2C1_SCL_GPIO_PINx	GPIO_PIN_4
#define I2C1_SDA_GPIOx		GPIOB
#define I2C1_SDA_RCU_GPIOx	RCU_GPIOB
#define I2C1_SDA_GPIO_PINx	GPIO_PIN_11
#define I2C1_GPIO_AFx		GPIO_AF_4

#define SPI4_PERIPH				SPI4
#define SPI4_RCU_CLK			RCU_SPI4
#define SPI4_NSS_GPIOx			GPIOF
#define SPI4_NSS_RCU_GPIOx		RCU_GPIOF
#define SPI4_NSS_GPIO_PINx		GPIO_PIN_6
#define SPI4_GPIOx				GPIOG
#define SPI4_RCU_GPIOx			RCU_GPIOG
#define SPI4_SCK_GPIO_PINx		GPIO_PIN_13
#define SPI4_MISO_GPIO_PINx		GPIO_PIN_15
#define SPI4_MOSI_GPIO_PINx		GPIO_PIN_14
#define SPI4_IO2_GPIO_PINx		GPIO_PIN_10
#define SPI4_IO3_GPIO_PINx		GPIO_PIN_11

#endif /* MCU_GD32H7XX_MCU_H_ */
