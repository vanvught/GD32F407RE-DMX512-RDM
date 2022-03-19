/**
 * @file gd32_spi.cpp
 *
 */
/* Copyright (C) 2021-2022 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include "gd32_spi.h"
#include "gd32.h"

static uint8_t s_chip_select = GD32_SPI_CS0;

static inline void _cs_high(void) {
	if (s_chip_select == GD32_SPI_CS0) {
		GPIO_BOP(SPI_NSS_GPIOx) = (uint32_t)SPI_NSS_GPIO_PINx;
	}
}

static inline void _cs_low(void) {
	if (s_chip_select == GD32_SPI_CS0) {
		GPIO_BC(SPI_NSS_GPIOx) = (uint32_t)SPI_NSS_GPIO_PINx;
	}
}

static uint8_t _send_byte(uint8_t byte) {
	while (RESET == (SPI_STAT(SPI_PERIPH) & SPI_FLAG_TBE))
		;
	SPI_DATA(SPI_PERIPH) = (uint32_t) byte;
	while (RESET == (SPI_STAT(SPI_PERIPH) & SPI_FLAG_RBNE))
		;
	return (uint8_t) ((uint16_t) SPI_DATA(SPI_PERIPH));
}

void gd32_spi_begin(void)  {
	spi_parameter_struct spi_init_struct;

	rcu_periph_clock_enable(SPI_RCU_GPIOx);
	rcu_periph_clock_enable(SPI_NSS_RCU_GPIOx);
	rcu_periph_clock_enable(SPI_RCU_CLK);

#if !defined (GD32F4XX)
	rcu_periph_clock_enable(RCU_AF);

	gpio_init(SPI_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN);
	gpio_init(SPI_NSS_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, SPI_NSS_GPIO_PINx);

# if defined (SPI_REMAP)
	gpio_pin_remap_config(SPI_REMAP, ENABLE);
# endif
#else
    gpio_af_set(SPI_GPIOx, GPIO_AF_5, SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN);
    gpio_mode_set(SPI_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN);
    gpio_output_options_set(SPI_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN);

    gpio_mode_set(SPI_NSS_GPIOx, GPIO_MODE_OUTPUT,GPIO_PUPD_NONE, SPI_NSS_GPIO_PINx);
    gpio_output_options_set(SPI_NSS_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SPI_NSS_GPIO_PINx);
#endif

	_cs_high();

	spi_disable(SPI_PERIPH);

	spi_i2s_deinit(SPI_PERIPH);
	spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
	spi_init_struct.device_mode = SPI_MASTER;
	spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
	spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
	spi_init_struct.nss = SPI_NSS_SOFT;
	spi_init_struct.prescale = SPI_PSC_64;
	spi_init_struct.endian = SPI_ENDIAN_MSB;
	spi_init(SPI_PERIPH, &spi_init_struct);

	spi_enable(SPI_PERIPH);
}

void gd32_spi_end(void) {
	spi_i2s_deinit(SPI_PERIPH);
#if !defined (GD32F4XX)
	gpio_init(SPI_GPIOx, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ, SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN);
	gpio_init(SPI_NSS_GPIOx, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ, SPI_NSS_GPIO_PINx);
#else
#endif
}

void gd32_spi_set_speed_hz(uint32_t speed_hz) {
	uint32_t div;
	uint32_t reg;

	if (SPI_PERIPH == SPI0) {
		div = APB2_CLOCK_FREQ / speed_hz;	/* PCLK2 when using SPI0  */
	} else {
		div = APB1_CLOCK_FREQ / speed_hz;	/* PCLK1 when using SPI1 and SPI2 */
	}

	reg = SPI_CTL0(SPI_PERIPH);
	reg &= ~CTL0_PSC(7);

	if (div <= 2) {
		reg |= SPI_PSC_2;
	} else if (div <= 4) {
		reg |= SPI_PSC_4;
	} else if (div <= 8) {
		reg |= SPI_PSC_8;
	} else if (div <= 16) {
		reg |= SPI_PSC_16;
	} else if (div <= 32) {
		reg |= SPI_PSC_32;
	} else if (div <= 64) {
		reg |= SPI_PSC_64;
	} else if (div <= 128) {
		reg |= SPI_PSC_128;
	} else {
		reg |= SPI_PSC_256;
	}

	SPI_CTL0(SPI_PERIPH) = reg;
}

void gd32_spi_setDataMode(uint8_t mode) {
    uint32_t reg = SPI_CTL0(SPI_PERIPH);
    reg &= ~0x3;
    reg |= (mode & 0x3);
    SPI_CTL0(SPI_PERIPH) = reg;
}

void gd32_spi_chipSelect(uint8_t chip_select) {
	s_chip_select = chip_select;
}

void gd32_spi_write(uint16_t data) {
	_cs_low();

	(void) _send_byte((uint8_t) (data >> 8));
	(void) _send_byte((uint8_t) ((uint8_t) data & 0xFF));

	_cs_high();
}

void gd32_spi_writenb(const char *tx_buffer, uint32_t data_length) {
	_cs_low();

	while (data_length-- > 0) {
		(void) _send_byte((uint8_t) *tx_buffer);
		tx_buffer++;
	}

	_cs_high();
}
