/**
 * @file gd32h759i_eval.h
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

#ifndef BOARD_GD32H759I_EVAL_H_
#define BOARD_GD32H759I_EVAL_H_

#include <stdint.h>

#if !defined(BOARD_GD32H759I_EVAL)
# error This file should not be included
#endif

#if defined (MCU_GD32H759_MCU_H_)
# error This file should be included later
#endif

#define USE_ENET0

/**
 * LEDs
 */

#define LED1_GPIO_PINx		GPIO_PIN_10
#define LED1_GPIOx			GPIOF
#define LED1_RCU_GPIOx		RCU_GPIOF

#define LED2_GPIO_PINx		GPIO_PIN_6
#define LED2_GPIOx			GPIOA
#define LED2_RCU_GPIOx		RCU_GPIOA

#define LED_BLINK_PIN       LED1_GPIO_PINx
#define LED_BLINK_GPIO_PORT LED1_GPIOx
#define LED_BLINK_GPIO_CLK	LED1_RCU_GPIOx

/**
 * KEYs
 */

#define KEY1_PINx						GPIO_PIN_0
#define KEY1_GPIOx						GPIOA
#define KEY1_RCU_GPIOx					RCU_GPIOA

#define KEY2_PINx						GPIO_PIN_13
#define KEY2_GPIOx						GPIOC
#define KEY2_RCU_GPIOx					RCU_GPIOC

#define KEY3_PINx						GPIO_PIN_8
#define KEY3_GPIOx						GPIOF
#define KEY3_RCU_GPIOx					RCU_GPIOF

#define KEY_BOOTLOADER_TFTP_GPIO_PINx	KEY2_PINx
#define KEY_BOOTLOADER_TFTP_GPIOx		KEY2_GPIOx
#define KEY_BOOTLOADER_TFTP_RCU_GPIOx	KEY2_RCU_GPIOx

/**
 * I2C
 */

#define I2C_PERIPH			I2C1_PERIPH
#define I2C_RCU_CLK			I2C1_RCU_CLK
#define I2C_RCU_IDX			IDX_I2C1
#define I2C_GPIO_SCL_PORT	I2C1_SCL_GPIOx
#define I2C_GPIO_SCL_CLK	I2C1_SCL_RCU_GPIOx
#define I2C_GPIO_SDA_PORT	I2C1_SDA_GPIOx
#define I2C_GPIO_SDA_CLK	I2C1_SDA_RCU_GPIOx
#define I2C_SCL_PIN			I2C1_SCL_GPIO_PINx
#define I2C_SDA_PIN			I2C1_SDA_GPIO_PINx
#define I2C_GPIO_AF			I2C1_GPIO_AFx
#define I2CX                I2C1

/**
 * SPI
 */

#define SPI_PERIPH			SPI4_PERIPH
#define SPI_NSS_GPIOx		SPI4_NSS_GPIOx
#define SPI_NSS_RCU_GPIOx	SPI4_NSS_RCU_GPIOx
#define SPI_NSS_GPIO_PINx	SPI4_NSS_GPIO_PINx
#define SPI_RCU_CLK			SPI4_RCU_CLK
#define SPI_GPIOx			SPI4_GPIOx
#define SPI_RCU_GPIOx		SPI4_RCU_GPIOx
#define SPI_SCK_PIN			SPI4_SCK_GPIO_PINx
#define SPI_MISO_PIN		SPI4_MISO_GPIO_PINx
#define SPI_MOSI_PIN		SPI4_MOSI_GPIO_PINx

/**
 * SPI flash
 */

#define SPI_FLASH_CS_GPIOx			SPI_NSS_GPIOx
#define SPI_FLASH_CS_RCU_GPIOx		SPI_NSS_RCU_GPIOx
#define SPI_FLASH_CS_GPIO_PINx		SPI_NSS_GPIO_PINx

//#define SPI_FLASH_WP_GPIO_PINx		SPI_IO2_PIN
//#define SPI_FLASH_HOLD_GPIO_PINx	SPI_IO3_PIN

/**
 * MCU and BOARD name
 */

#define GD32_MCU_NAME			"GD32H759I"
#define GD32_BOARD_NAME			"GD32H759I_EVAL"

#include "mcu/gd32h759_mcu.h"
#include "gd32_gpio.h"

#endif /* BOARD_GD32H759I_EVAL_H_ */
