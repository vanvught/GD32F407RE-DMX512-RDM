/**
 * @file bw_opidmx4.h
 *
 */
/* Copyright (C) 2022 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef BOARD_BW_OPIDMX4_H_
#define BOARD_BW_OPIDMX4_H_

#if !defined(BOARD_BW_OPIDMX4)
# error This file should not be included
#endif

#if defined (MCU_GD32F20X_MCU_H_) || defined (MCU_GD32F407_MCU_H_)
# error This file should be included later
#endif

#if !defined(CONSOLE_I2C)
# error CONSOLE_I2C is not defined
#endif

/**
 * LEDs
 */

#define LED1_GPIO_PINx		GPIO_PIN_0
#define LED1_GPIOx			GPIOC
#define LED1_RCU_GPIOx		RCU_GPIOC

#define LED2_GPIO_PINx		GPIO_PIN_2
#define LED2_GPIOx			GPIOC
#define LED2_RCU_GPIOx		RCU_GPIOC

#define LED3_GPIO_PINx		GPIO_PIN_3
#define LED3_GPIOx			GPIOC
#define LED3_RCU_GPIOx		RCU_GPIOC

#define LED_BLINK_PIN       LED1_GPIO_PINx
#define LED_BLINK_GPIO_PORT LED1_GPIOx
#define LED_BLINK_GPIO_CLK	LED1_RCU_GPIOx

/**
 * KEYs
 */

#define KEY2_PINx			GPIO_PIN_14
#define KEY2_GPIOx			GPIOB
#define KEY2_RCU_GPIOx		RCU_GPIOB

/**
 * I2C
 */

#define I2C0_REMAP
#define I2C_REMAP			GPIO_I2C0_REMAP
#define I2C_PERIPH			I2C0_PERIPH
#define I2C_RCU_CLK			I2C0_RCU_CLK
#define I2C_GPIO_SCL_PORT	I2C0_SCL_GPIOx
#define I2C_GPIO_SCL_CLK	I2C0_SCL_RCU_GPIOx
#define I2C_GPIO_SDA_PORT	I2C0_SDA_GPIOx
#define I2C_GPIO_SDA_CLK	I2C0_SDA_RCU_GPIOx
#define I2C_SCL_PIN			I2C0_SCL_GPIO_PINx
#define I2C_SDA_PIN			I2C0_SDA_GPIO_PINx

/**
 * SPI
 */

#define SPI_PERIPH			SPI2_PERIPH
#define SPI_NSS_GPIOx		SPI2_NSS_GPIOx
#define SPI_NSS_RCU_GPIOx	SPI2_NSS_RCU_GPIOx
#define SPI_NSS_GPIO_PINx	SPI2_NSS_GPIO_PINx
#define SPI_RCU_CLK			SPI2_RCU_CLK
#define SPI_GPIOx			SPI2_GPIOx
#define SPI_RCU_GPIOx		SPI2_RCU_GPIOx
#define SPI_SCK_PIN			SPI2_SCK_GPIO_PINx
#define SPI_MISO_PIN		SPI2_MISO_GPIO_PINx
#define SPI_MOSI_PIN		SPI2_MOSI_GPIO_PINx
#define SPI_DMAx			SPI2_DMAx
#define SPI_DMA_CHx			SPI2_TX_DMA_CHx
#define SPI_DMA_SUBPERIx	SPI2_TX_DMA_SUBPERIx

/**
 * U(S)ART
 */

#define USART2_PARTIAL_REMAP

#if defined (GD32F20X_CL)
# include "mcu/gd32f20x_mcu.h"
# define GD32_MCU_NAME	"GD32F207R"
#elif defined (GD32F407)
# include "mcu/gd32f407_mcu.h"
# define GD32_MCU_NAME	"GD32F407R"
#else
# error MCU is not supported
#endif

#include "gd32_gpio.h"

#define GD32_BOARD_NAME			"BW_OPIDMX4"
#define GD32_BOARD_LED1			GD32_PORT_TO_GPIO(GD32_GPIO_PORTC, 0)
#define GD32_BOARD_LED2			GD32_PORT_TO_GPIO(GD32_GPIO_PORTC, 2)
#define GD32_BOARD_LED3			GD32_PORT_TO_GPIO(GD32_GPIO_PORTC, 3)
#define GD32_BOARD_STATUS_LED	GD32_BOARD_LED1

#include "gpio_header.h"

#endif /* BOARD_BW_OPIDMX4_H_ */
