/**
 * @file gd32_spi_dma_i2s.cpp
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
#include <cassert>

#include "gd32.h"

#if defined (GD32F4XX)
# define DMA_PARAMETER_STRUCT				dma_single_data_parameter_struct
# define DMA_CHMADDR						DMA_CHM0ADDR
# define DMA_MEMORY_TO_PERIPHERAL			DMA_MEMORY_TO_PERIPH
# define DMA_PERIPHERAL_WIDTH_16BIT			DMA_PERIPH_WIDTH_16BIT
# define dma_init							dma_single_data_mode_init
# define dma_struct_para_init				dma_single_data_para_struct_init
# define dma_memory_to_memory_disable(x,y)
#else
# define DMA_PARAMETER_STRUCT				dma_parameter_struct
#endif

#ifndef NDEBUG
void i2s_psc_config_dump(uint32_t spi_periph, uint32_t audiosample, uint32_t frameformat, uint32_t mckout);
#endif

#if !defined(SPI_BUFFER_SIZE)
# define SPI_BUFFER_SIZE ((24 * 1024) / 2)
#endif

static uint16_t s_TxBuffer[SPI_BUFFER_SIZE] __attribute__ ((aligned (4)));

static void _spi_i2s_dma_config() {
	rcu_periph_clock_enable(RCU_DMA1);

	dma_deinit(SPI_DMAx, SPI_DMA_CHx);

	DMA_PARAMETER_STRUCT dma_init_struct;
	dma_struct_para_init(&dma_init_struct);
	dma_init_struct.direction			= DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc			= DMA_MEMORY_INCREASE_ENABLE;
#if !defined (GD32F4XX)
	dma_init_struct.memory_width		= DMA_MEMORY_WIDTH_16BIT;
#endif
	dma_init_struct.periph_addr			= SPI_PERIPH + 0x0CU;
	dma_init_struct.periph_inc			= DMA_PERIPH_INCREASE_DISABLE;
#if !defined (GD32F4XX)
	dma_init_struct.periph_width		= DMA_PERIPHERAL_WIDTH_16BIT;
#else
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_16BIT;
#endif
	dma_init_struct.priority			= DMA_PRIORITY_HIGH;
	dma_init(SPI_DMAx, SPI_DMA_CHx, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(SPI_DMAx, SPI_DMA_CHx);
	dma_memory_to_memory_disable(SPI_DMAx, SPI_DMA_CHx);
#if defined (GD32F4XX)
	dma_channel_subperipheral_select(SPI_DMAx, SPI_DMA_CHx, SPI_DMA_SUBPERIx);
#endif

	DMA_CHCNT(SPI_DMAx, SPI_DMA_CHx) = 0;
}

void gd32_spi_dma_begin() {
	assert(SPI_PERIPH != SPI0);

	rcu_periph_clock_enable(SPI_NSS_RCU_GPIOx);
	rcu_periph_clock_enable(SPI_RCU_GPIOx);
	rcu_periph_clock_enable(SPI_RCU_CLK);

#if !defined (GD32F4XX)
	rcu_periph_clock_enable(RCU_AF);

	gpio_init(SPI_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN);
	gpio_init(SPI_NSS_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, SPI_NSS_GPIO_PINx);

# if defined (SPI2_REMAP)
	gpio_pin_remap_config(GPIO_SPI2_REMAP, ENABLE);
# else
	if (SPI_PERIPH == SPI2) {
		gpio_pin_remap_config(GPIO_SWJ_DISABLE_REMAP, ENABLE);
	}
# endif
#else
    gpio_af_set(SPI_GPIOx, GPIO_AF_5, SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN);
    gpio_mode_set(SPI_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN);
    gpio_output_options_set(SPI_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN);

    gpio_mode_set(SPI_NSS_GPIOx, GPIO_MODE_OUTPUT,GPIO_PUPD_NONE, SPI_NSS_GPIO_PINx);
    gpio_output_options_set(SPI_NSS_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SPI_NSS_GPIO_PINx);
#endif

#if defined (GD32F20X_CL)
	/**
	 * Setup PLL2
	 *
	 * clks=14
	 * i2sclock=160000000
	 * i2sdiv=12, i2sof=256
	 */
	rcu_pll2_config(RCU_PLL2_MUL16);
	RCU_CTL |= RCU_CTL_PLL2EN;
	while ((RCU_CTL & RCU_CTL_PLL2STB) == 0U) {
	}
	if (SPI_PERIPH == SPI2) {
		rcu_i2s2_clock_config(RCU_I2S2SRC_CKPLL2_MUL2);
	} else {
		rcu_i2s1_clock_config(RCU_I2S1SRC_CKPLL2_MUL2);
	}
#endif

	i2s_disable(SPI_PERIPH);
	i2s_psc_config(SPI_PERIPH, 200000, I2S_FRAMEFORMAT_DT16B_CH16B,  I2S_MCKOUT_DISABLE);
	i2s_init(SPI_PERIPH, I2S_MODE_MASTERTX, I2S_STD_MSB, I2S_CKPL_LOW);
	i2s_enable(SPI_PERIPH);

	_spi_i2s_dma_config();

#ifndef NDEBUG
	i2s_psc_config_dump(SPI_PERIPH, 200000, I2S_FRAMEFORMAT_DT16B_CH16B,  I2S_MCKOUT_DISABLE);
#endif
}

void gd32_spi_dma_set_speed_hz(uint32_t nSpeedHz) {
	const auto audiosample = nSpeedHz / 16 / 2 ;

	i2s_disable(SPI_PERIPH);
	i2s_psc_config(SPI2, audiosample, I2S_FRAMEFORMAT_DT16B_CH16B,  I2S_MCKOUT_DISABLE);
	i2s_enable(SPI_PERIPH);
}

/**
 * DMA
 */

const uint8_t *gd32_spi_dma_tx_prepare(uint32_t *nLength) {
	*nLength = (sizeof(s_TxBuffer) / sizeof(s_TxBuffer[0])) * 2;
	return (const uint8_t*) s_TxBuffer;
}

void gd32_spi_dma_tx_start(const uint8_t *pTxBuffer, uint32_t nLength) {
	assert(((uint32_t )pTxBuffer & 0x1) != 0x1);
	assert((uint32_t )pTxBuffer >= (uint32_t )s_TxBuffer);
	assert(nLength != 0);

	const auto dma_chcnt = (((nLength + 1) / 2) & DMA_CHXCNT_CNT);

	auto nDmaChCTL = DMA_CHCTL(SPI_DMAx, SPI_DMA_CHx);
	nDmaChCTL &= ~DMA_CHXCTL_CHEN;
	DMA_CHCTL(SPI_DMAx, SPI_DMA_CHx) = nDmaChCTL;
	DMA_CHMADDR(SPI_DMAx, SPI_DMA_CHx) = (uint32_t)pTxBuffer;
	DMA_CHCNT(SPI_DMAx, SPI_DMA_CHx) = dma_chcnt;
	nDmaChCTL |= DMA_CHXCTL_CHEN;
	DMA_CHCTL(SPI_DMAx, SPI_DMA_CHx) = nDmaChCTL;
	spi_dma_enable(SPI_PERIPH, SPI_DMA_TRANSMIT);
}

bool gd32_spi_dma_tx_is_active(void) {
	return (uint32_t) DMA_CHCNT(SPI_DMAx, SPI_DMA_CHx) != 0;
}

/**
 * /CS
 */

//TODO Implement /CS
