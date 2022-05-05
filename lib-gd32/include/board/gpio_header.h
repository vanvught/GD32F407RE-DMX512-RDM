/**
 * @file gpio_header.h
 *
 */
/* Copyright (C) 2021 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef BOARD_GPIO_HEADER_H_
#define BOARD_GPIO_HEADER_H_

/**
 * Below is for (backwards) compatibility with Orange Pi Zero board.
 */

#define	EXT_UART_NUMBER 	2
#define EXT_UART_BASE		USART2
#define EXT_MIDI_UART_BASE	USART5

typedef enum GD32_BOARD_GPIO_HEADER {
	// 1 3V3
	GPIO_EXT_3  = GD32_PORT_TO_GPIO(GD32_GPIO_PORTB, 7),	///< I2C0 SCA, PB7
	GPIO_EXT_5  = GD32_PORT_TO_GPIO(GD32_GPIO_PORTB, 6),	///< I2C0 SDL, PB6
	GPIO_EXT_7  = GD32_PORT_TO_GPIO(GD32_GPIO_PORTA, 6),	///< PA6
	GPIO_EXT_11 = GD32_PORT_TO_GPIO(GD32_GPIO_PORTC, 7),	///< USART5 RX, PC7
	GPIO_EXT_13 = GD32_PORT_TO_GPIO(GD32_GPIO_PORTC, 6),	///< USART5 TX, PC6
	GPIO_EXT_15 = GD32_PORT_TO_GPIO(GD32_GPIO_PORTB, 14),	///< PB14
	// 17 3V3
	GPIO_EXT_19 = GD32_PORT_TO_GPIO(GD32_GPIO_PORTB, 5),	///< SPI2 MOSI, PB5
	GPIO_EXT_21 = GD32_PORT_TO_GPIO(GD32_GPIO_PORTB, 4),	///< SPI2 MISO, PB4
	GPIO_EXT_23 = GD32_PORT_TO_GPIO(GD32_GPIO_PORTB, 3),	///< SPI2 SCLK, PB3
	// 2, 4 5V
	// 6 GND
	GPIO_EXT_8  = GD32_PORT_TO_GPIO(GD32_GPIO_PORTC, 10),	///< USART2 TX, PC10
	GPIO_EXT_10 = GD32_PORT_TO_GPIO(GD32_GPIO_PORTC, 11),	///< USART2 RX, PC11
	GPIO_EXT_12 = GD32_PORT_TO_GPIO(GD32_GPIO_PORTB, 10),	///< PB10
	// 14 GND
	GPIO_EXT_16 = GD32_PORT_TO_GPIO(GD32_GPIO_PORTB, 15),	///< PB15
	GPIO_EXT_18 = GD32_PORT_TO_GPIO(GD32_GPIO_PORTA, 13),	///< PA13
	GPIO_EXT_22 = GD32_PORT_TO_GPIO(GD32_GPIO_PORTA, 11),	///< PA11
	GPIO_EXT_24 = GD32_PORT_TO_GPIO(GD32_GPIO_PORTA, 15),	///< SPI2 NSS, PA15
	GPIO_EXT_26 = GD32_PORT_TO_GPIO(GD32_GPIO_PORTA, 14)	///< PA14
} _gpio_pin;

#endif /* BOARD_GPIO_HEADER_H_ */
