/**
 * @file spi_flash.cpp
 *
 */
/* Copyright (C) 2022-2024 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include <cstdint>

#include "./../../spi/spi_flash_internal.h"
#include "gd32_spi.h"
#include "gd32_gpio.h"
#include "gd32.h"
#include "firmware/debug/debug_debug.h"

void SpiInit() {
	Gd32SpiBegin();
	Gd32SpiChipSelect(GD32_SPI_CS_NONE);
	Gd32SpiSetSpeedHz(SPI_XFER_SPEED_HZ);
	Gd32SpiSetDataMode(GD32_SPI_MODE0);

	Gd32GpioFsel(SPI_FLASH_CS_GPIOx, SPI_FLASH_CS_GPIO_PINx, GPIO_FSEL_OUTPUT);
	GPIO_BOP(SPI_FLASH_CS_GPIOx) = SPI_FLASH_CS_GPIO_PINx;

#if defined (SPI_FLASH_WP_GPIO_PINx)
	Gd32GpioFsel(SPI_GPIOx, SPI_FLASH_WP_GPIO_PINx, GPIO_FSEL_OUTPUT);
	GPIO_BOP(SPI_GPIOx) = SPI_FLASH_WP_GPIO_PINx;
#endif

#if defined (SPI_FLASH_HOLD_GPIO_PINx)
	Gd32GpioFsel(SPI_GPIOx, SPI_FLASH_HOLD_GPIO_PINx, GPIO_FSEL_OUTPUT);
	GPIO_BOP(SPI_GPIOx) = SPI_FLASH_HOLD_GPIO_PINx;
#endif
}

inline static void SpiTransfern(char *buffer, uint32_t length) {
	Gd32SpiTransfernb(buffer, buffer, length);
}

void SpiXfer(uint32_t length, const uint8_t *out, uint8_t *in, uint32_t flags) {
	if (flags & SPI_XFER_BEGIN) {
		GPIO_BC(SPI_FLASH_CS_GPIOx) = SPI_FLASH_CS_GPIO_PINx;
	}

	if (length != 0) {
		if (in == nullptr) {
			Gd32SpiWritenb(reinterpret_cast<const char *>(out), length);
		} else if (out == nullptr) {
			SpiTransfern(reinterpret_cast<char *>(in), length);
		} else {
			Gd32SpiTransfernb(reinterpret_cast<const char *>(out), reinterpret_cast<char *>(in), length);
		}
	}

	if (flags & SPI_XFER_END) {
		GPIO_BOP(SPI_FLASH_CS_GPIOx) = SPI_FLASH_CS_GPIO_PINx;
	}
}
