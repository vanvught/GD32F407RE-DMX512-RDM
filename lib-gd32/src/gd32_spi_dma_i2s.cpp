/**
 * @file gd32_spi_dma_i2s.cpp
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
#include <cassert>

#include "gd32.h"

#if defined (I2S_PERIPH)

# if defined (GD32H7XX)
	static_assert(I2S_PERIPH != SPI3);
	static_assert(I2S_PERIPH != SPI4);
# else
	static_assert(I2S_PERIPH != SPI0);
# endif

# if defined (GD32F4XX) || defined (GD32H7XX)
#  define DMA_PARAMETER_STRUCT				dma_single_data_parameter_struct
#  define DMA_CHMADDR						DMA_CHM0ADDR
#  define DMA_MEMORY_TO_PERIPHERAL			DMA_MEMORY_TO_PERIPH
#  define DMA_PERIPHERAL_WIDTH_16BIT		DMA_PERIPH_WIDTH_16BIT
#  define dma_init							dma_single_data_mode_init
#  define dma_struct_para_init				dma_single_data_para_struct_init
#  define dma_memory_to_memory_disable(x,y)
# else
#  define DMA_PARAMETER_STRUCT				dma_parameter_struct
# endif

# ifndef NDEBUG
#  if !defined (GD32H7XX)
	void i2s_psc_config_dump(uint32_t spi_periph, uint32_t audiosample, uint32_t frameformat, uint32_t mckout);
#  endif
# endif

# if !defined(SPI_BUFFER_SIZE)
#  define SPI_BUFFER_SIZE ((24 * 1024) / 2)
# endif

static uint16_t s_TxBuffer[SPI_BUFFER_SIZE] __attribute__ ((aligned (4)));

static void rcu_config() {
	rcu_periph_clock_enable(I2S_WS_RCU_GPIOx);

#if defined (GPIO_INIT)
	rcu_periph_clock_enable(RCU_AF);
	rcu_periph_clock_enable(I2S_RCU_SPIx);
	rcu_periph_clock_enable(I2S_RCU_GPIOx);
#else
	rcu_periph_clock_enable(I2S_CK_RCU_GPIOx);
	rcu_periph_clock_enable(I2S_SD_RCU_GPIOx);
#endif

	if (I2S_DMAx == DMA0) {
		rcu_periph_clock_enable(RCU_DMA0);
	} else {
		rcu_periph_clock_enable(RCU_DMA1);
	}

#if defined (GD32H7XX)
	rcu_periph_clock_enable(RCU_DMAMUX);
#endif
}

static void gpio_config() {
#if defined (GPIO_INIT)
# if defined (I2S_REMAP_GPIO)
	gpio_pin_remap_config(I2S_REMAP_GPIO, ENABLE);
	if (I2S_PERIPH == SPI0) {
		gpio_pin_remap_config(GPIO_SWJ_DISABLE_REMAP, ENABLE);
	}
# else
	if (I2S_PERIPH == SPI2) {
		gpio_pin_remap_config(GPIO_SWJ_DISABLE_REMAP, ENABLE);
	}
# endif
	gpio_init(I2S_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, I2S_CK_GPIO_PINx | I2S_SD_GPIO_PINx);
	gpio_init(I2S_WS_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, I2S_WS_GPIO_PINx);
#else
	gpio_af_set(I2S_CK_GPIOx, I2S_GPIO_AFx, I2S_CK_GPIO_PINx);
    gpio_mode_set(I2S_CK_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, I2S_CK_GPIO_PINx);
    gpio_output_options_set(I2S_CK_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, I2S_CK_GPIO_PINx);

	gpio_af_set(I2S_SD_GPIOx, I2S_GPIO_AFx, I2S_SD_GPIO_PINx);
    gpio_mode_set(I2S_SD_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, I2S_SD_GPIO_PINx);
    gpio_output_options_set(I2S_SD_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, I2S_SD_GPIO_PINx);

	gpio_af_set(I2S_WS_GPIOx, I2S_GPIO_AFx, I2S_WS_GPIO_PINx);
    gpio_mode_set(I2S_WS_GPIOx, GPIO_MODE_OUTPUT,GPIO_PUPD_NONE, I2S_WS_GPIO_PINx);
    gpio_output_options_set(I2S_WS_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, I2S_WS_GPIO_PINx);
#endif
}

static void spi_i2s_dma_config() {
	dma_deinit(I2S_DMAx, I2S_DMA_CHx);

	DMA_PARAMETER_STRUCT dma_init_struct;
	dma_struct_para_init(&dma_init_struct);

#if defined (GD32H7XX)
	dma_init_struct.request = I2S_DMA_REQUEST_I2S_TX;
#endif
	dma_init_struct.direction			= DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc			= DMA_MEMORY_INCREASE_ENABLE;
# if defined (GD32F4XX) || defined (GD32H7XX)
# else
	dma_init_struct.memory_width		= DMA_MEMORY_WIDTH_16BIT;
#endif
#if defined (GD32H7XX)
	dma_init_struct.periph_addr         = (uint32_t)&SPI_TDATA(I2S_PERIPH);
#else
	dma_init_struct.periph_addr			= I2S_PERIPH + 0x0CU;
#endif
	dma_init_struct.periph_inc			= DMA_PERIPH_INCREASE_DISABLE;
#if defined (GD32F4XX) || defined (GD32H7XX)
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_16BIT;
#else
	dma_init_struct.periph_width		= DMA_PERIPHERAL_WIDTH_16BIT;
#endif
	dma_init_struct.priority			= DMA_PRIORITY_HIGH;
	dma_init(I2S_DMAx, I2S_DMA_CHx, &dma_init_struct);

	dma_circulation_disable(I2S_DMAx, I2S_DMA_CHx);
	dma_memory_to_memory_disable(I2S_DMAx, I2S_DMA_CHx);
#if defined (GD32F4XX)
	dma_channel_subperipheral_select(I2S_DMAx, I2S_DMA_CHx, I2S_DMA_SUBPERIx);
#endif

	DMA_CHCNT(I2S_DMAx, I2S_DMA_CHx) = 0;
}
namespace i2s {
void gd32_spi_dma_begin() {
	rcu_config();
	gpio_config();

	i2s_disable(I2S_PERIPH);
	i2s_psc_config(I2S_PERIPH, 200000, I2S_FRAMEFORMAT_DT16B_CH16B,  I2S_MCKOUT_DISABLE);
	i2s_init(I2S_PERIPH, I2S_MODE_MASTERTX, I2S_STD_MSB, I2S_CKPL_LOW);
	i2s_enable(I2S_PERIPH);

	spi_i2s_dma_config();

#ifndef NDEBUG
	i2s_psc_config_dump(I2S_PERIPH, 200000, I2S_FRAMEFORMAT_DT16B_CH16B,  I2S_MCKOUT_DISABLE);
#endif
}

void gd32_spi_dma_set_speed_hz(uint32_t nSpeedHz) {
	const auto audiosample = nSpeedHz / 16U / 2U ;

	i2s_disable(I2S_PERIPH);
	i2s_psc_config(I2S_PERIPH, audiosample, I2S_FRAMEFORMAT_DT16B_CH16B,  I2S_MCKOUT_DISABLE);
	i2s_enable(I2S_PERIPH);
}

const uint8_t *gd32_spi_dma_tx_prepare(uint32_t *nLength) {
	*nLength = (sizeof(s_TxBuffer) / sizeof(s_TxBuffer[0])) * 2;
	return (const uint8_t *) s_TxBuffer;
}

void gd32_spi_dma_tx_start(const uint8_t *pTxBuffer, uint32_t nLength) {
	assert(((uint32_t )pTxBuffer & 0x1) != 0x1);
	assert((uint32_t )pTxBuffer >= (uint32_t )s_TxBuffer);
	assert(nLength != 0);

#if defined (GD32F4XX) || defined (GD32H7XX)
	dma_flag_clear(I2S_DMAx, I2S_DMA_CHx, DMA_FLAG_FTF);
#endif

	const auto dma_chcnt = (((nLength + 1) / 2) & DMA_CHXCNT_CNT);

	auto nDmaChCTL = DMA_CHCTL(I2S_DMAx, I2S_DMA_CHx);
	nDmaChCTL &= ~DMA_CHXCTL_CHEN;
	DMA_CHCTL(I2S_DMAx, I2S_DMA_CHx) = nDmaChCTL;

	DMA_CHMADDR(I2S_DMAx, I2S_DMA_CHx) = (uint32_t)pTxBuffer;
	DMA_CHCNT(I2S_DMAx, I2S_DMA_CHx) = dma_chcnt;

	nDmaChCTL |= DMA_CHXCTL_CHEN;
	DMA_CHCTL(I2S_DMAx, I2S_DMA_CHx) = nDmaChCTL;

	spi_dma_enable(I2S_PERIPH, SPI_DMA_TRANSMIT);
}

bool gd32_spi_dma_tx_is_active() {
	return DMA_CHCNT(I2S_DMAx, I2S_DMA_CHx) != 0;
}
}  // namespace i2s
#endif
