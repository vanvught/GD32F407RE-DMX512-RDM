/**
 * @file gd32_spi.cpp
 *
 */
/* Copyright (C) 2021-2024 by Arjan van Vught mailto:info@gd32-dmx.org
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
#include <cstdio>
#include <cassert>

#include "gd32_spi.h"
#include "gd32.h"

static uint8_t s_nChipSelect = GD32_SPI_CS0;

static void cs_high() {
	if (s_nChipSelect == GD32_SPI_CS0) {
		GPIO_BOP(SPI_NSS_GPIOx) = SPI_NSS_GPIO_PINx;
	}
}

static void cs_low() {
	if (s_nChipSelect == GD32_SPI_CS0) {
		GPIO_BC(SPI_NSS_GPIOx) = SPI_NSS_GPIO_PINx;
	}
}

static uint8_t send_byte(uint8_t byte) {
	while (RESET == (SPI_STAT(SPI_PERIPH) & SPI_FLAG_TBE))
		;

	SPI_DATA(SPI_PERIPH) = static_cast<uint32_t>(byte);

	while (RESET == (SPI_STAT(SPI_PERIPH) & SPI_FLAG_RBNE))
		;

	return static_cast<uint8_t>(static_cast<uint16_t>(SPI_DATA(SPI_PERIPH)));
}

static void rcu_config() {
	rcu_periph_clock_enable(SPI_RCU_SPIx);
	rcu_periph_clock_enable(SPI_RCU_GPIOx);
	rcu_periph_clock_enable(SPI_NSS_RCU_GPIOx);

#if defined (GPIO_INIT)
	rcu_periph_clock_enable(RCU_AF);
#endif
}

static void gpio_config() {
#if defined (GPIO_INIT)
# if defined (SPI_REMAP_GPIO)
	gpio_pin_remap_config(SPI_REMAP_GPIO, ENABLE);
	if (SPI_PERIPH == SPI0) {
		gpio_pin_remap_config(GPIO_SWJ_DISABLE_REMAP, ENABLE);
	}
# else
	if (SPI_PERIPH == SPI2) {
		gpio_pin_remap_config(GPIO_SWJ_DISABLE_REMAP, ENABLE);
	}
# endif
	gpio_init(SPI_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, SPI_SCK_GPIO_PINx | SPI_MOSI_GPIO_PINx);
	gpio_init(SPI_GPIOx, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, SPI_MISO_GPIO_PINx);
	gpio_init(SPI_NSS_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, SPI_NSS_GPIO_PINx);
#else
	gpio_af_set(SPI_GPIOx, SPI_GPIO_AFx, SPI_SCK_GPIO_PINx | SPI_MISO_GPIO_PINx | SPI_MOSI_GPIO_PINx);
	gpio_mode_set(SPI_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, SPI_SCK_GPIO_PINx | SPI_MISO_GPIO_PINx | SPI_MOSI_GPIO_PINx);
    gpio_output_options_set(SPI_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SPI_SCK_GPIO_PINx | SPI_MOSI_GPIO_PINx);

    gpio_mode_set(SPI_NSS_GPIOx, GPIO_MODE_OUTPUT,GPIO_PUPD_NONE, SPI_NSS_GPIO_PINx);
    gpio_output_options_set(SPI_NSS_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, SPI_NSS_GPIO_PINx);
#endif

	cs_high();
}

static void spi_config() {
	spi_disable(SPI_PERIPH);
	spi_i2s_deinit(SPI_PERIPH);

	spi_parameter_struct spi_init_struct;
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

/*
 * Public API's
 */

void gd32_spi_begin()  {
	rcu_config();
	gpio_config();
	spi_config();
}

void gd32_spi_end() {
	spi_disable(SPI_PERIPH);
#if defined (GPIO_INIT)
	gpio_init(SPI_GPIOx, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ, SPI_SCK_GPIO_PINx | SPI_MISO_GPIO_PINx | SPI_MOSI_GPIO_PINx);
	gpio_init(SPI_NSS_GPIOx, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ, SPI_NSS_GPIO_PINx);
#else
	gpio_mode_set(SPI_GPIOx, GPIO_MODE_INPUT, GPIO_PUPD_NONE, SPI_SCK_GPIO_PINx | SPI_MISO_GPIO_PINx | SPI_MOSI_GPIO_PINx);
    gpio_mode_set(SPI_NSS_GPIOx, GPIO_MODE_INPUT, GPIO_PUPD_NONE, SPI_NSS_GPIO_PINx);
#endif
}

void gd32_spi_set_speed_hz(uint32_t nSpeedHz) {
	assert(nSpeedHz != 0);

	uint32_t nDiv;

	if (SPI_PERIPH == SPI0) {
		nDiv = APB2_CLOCK_FREQ / nSpeedHz;	/* PCLK2 when using SPI0  */
	} else {
		nDiv = APB1_CLOCK_FREQ / nSpeedHz;	/* PCLK1 when using SPI1 and SPI2 */
	}

	uint32_t nCTL0 = SPI_CTL0(SPI_PERIPH);
	nCTL0 &= ~CTL0_PSC(7);

	if (nDiv <= 2) {
		nCTL0 |= SPI_PSC_2;
	} else if (nDiv <= 4) {
		nCTL0 |= SPI_PSC_4;
	} else if (nDiv <= 8) {
		nCTL0 |= SPI_PSC_8;
	} else if (nDiv <= 16) {
		nCTL0 |= SPI_PSC_16;
	} else if (nDiv <= 32) {
		nCTL0 |= SPI_PSC_32;
	} else if (nDiv <= 64) {
		nCTL0 |= SPI_PSC_64;
	} else if (nDiv <= 128) {
		nCTL0 |= SPI_PSC_128;
	} else {
		nCTL0 |= SPI_PSC_256;
	}

	spi_disable(SPI_PERIPH);
	SPI_CTL0(SPI_PERIPH) = nCTL0;
	spi_enable(SPI_PERIPH);
}

void gd32_spi_setDataMode(uint8_t nMode) {
    uint32_t nCTL0 = SPI_CTL0(SPI_PERIPH);
    nCTL0 &= ~0x3;
    nCTL0 |= (nMode & 0x3);

    spi_disable(SPI_PERIPH);
    SPI_CTL0(SPI_PERIPH) = nCTL0;
    spi_enable(SPI_PERIPH);
}

void gd32_spi_chipSelect(uint8_t nChipSelect) {
	s_nChipSelect = nChipSelect;

	if (nChipSelect == GD32_SPI_CS0) {
		spi_nss_output_enable(SPI_PERIPH);
	} else {
		spi_nss_output_disable(SPI_PERIPH);
	}
}

void gd32_spi_transfernb(const char *pTxBuffer, char *pRxBuffer, uint32_t nDataLength) {
	assert(pTxBuffer != nullptr);
	assert(pRxBuffer != nullptr);

	cs_low();

	while (nDataLength-- > 0) {
		*pRxBuffer = send_byte(static_cast<uint8_t>(*pTxBuffer));
		pRxBuffer++;
		pTxBuffer++;
	}

	cs_high();
}

void gd32_spi_transfern(char *pTxBuffer, uint32_t nDataLength) {
	gd32_spi_transfernb(pTxBuffer, pTxBuffer, nDataLength);
}

void gd32_spi_write(const uint16_t nData) {
	cs_low();

	send_byte(static_cast<uint8_t>(nData >> 8));
	send_byte(static_cast<uint8_t>(nData & 0xFF));

	cs_high();
}

void gd32_spi_writenb(const char *pTxBuffer, uint32_t nDataLength) {
	assert(pTxBuffer != nullptr);

	cs_low();

	while (nDataLength-- > 0) {
		send_byte(static_cast<uint8_t>(*pTxBuffer));
		pTxBuffer++;
	}

	cs_high();
}
