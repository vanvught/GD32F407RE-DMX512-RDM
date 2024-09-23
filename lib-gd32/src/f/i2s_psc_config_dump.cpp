/**
 * @file i2s_psc_config_dump_cpp
 *
 */
/* Copyright (C) 2022-2023 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef NDEBUG
#include <cstdio>
/* SPI/I2S parameter initialization mask */
#define SPI_INIT_MASK                   ((uint32_t)0x00003040U)  /*!< SPI parameter initialization mask */
#define I2S_INIT_MASK                   ((uint32_t)0x0000F047U)  /*!< I2S parameter initialization mask */

/* I2S clock source selection, multiplication and division mask */
#define I2S1_CLOCK_SEL                  ((uint32_t)0x00020000U)  /* I2S1 clock source selection */
#define I2S2_CLOCK_SEL                  ((uint32_t)0x00040000U)  /* I2S2 clock source selection */
#define I2S_CLOCK_MUL_MASK              ((uint32_t)0x0000F000U)  /* I2S clock multiplication mask */
#define I2S_CLOCK_DIV_MASK              ((uint32_t)0x000000F0U)  /* I2S clock division mask */

/* default value and offset */
#define SPI_I2SPSC_DEFAULT_VALUE        ((uint32_t)0x00000002U)  /* default value of SPI_I2SPSC register */
#define RCU_CFG1_PREDV1_OFFSET          4U                       /* PREDV1 offset in RCU_CFG1 */
#define RCU_CFG1_PLL2MF_OFFSET          12U                      /* PLL2MF offset in RCU_CFG1 */

void i2s_psc_config_dump([[maybe_unused]] uint32_t spi_periph, uint32_t audiosample, uint32_t frameformat, uint32_t mckout) {
	uint32_t i2sdiv = 2U, i2sof = 0U;
	uint32_t clks = 0U;
	uint32_t i2sclock = 0U;

#if defined (GD32F10X_CL) || defined (GD32F20X_CL)
	/* get the I2S clock source */
	if (SPI1 == ((uint32_t) spi_periph)) {
		/* I2S1 clock source selection */
		clks = I2S1_CLOCK_SEL;
	} else {
		/* I2S2 clock source selection */
		clks = I2S2_CLOCK_SEL;
	}

	if (0U != (RCU_CFG1 & clks)) {
		/* get RCU PLL2 clock multiplication factor */
		clks = (uint32_t) ((RCU_CFG1 & I2S_CLOCK_MUL_MASK)
				>> RCU_CFG1_PLL2MF_OFFSET);

		if ((clks > 5U) && (clks < 15U)) {
			/* multiplier is between 8 and 14 */
			clks += 2U;
		} else {
			if (15U == clks) {
				/* multiplier is 20 */
				clks = 20U;
			}
		}

		/* get the PREDV1 value */
		i2sclock = (uint32_t) (((RCU_CFG1 & I2S_CLOCK_DIV_MASK)
				>> RCU_CFG1_PREDV1_OFFSET) + 1U);
		/* calculate I2S clock based on PLL2 and PREDV1 */
		i2sclock = (uint32_t) ((HXTAL_VALUE / i2sclock) * clks * 2U);
	} else
#endif
	{
		/* get system clock */
		i2sclock = rcu_clock_freq_get(CK_SYS);
	}

	/* config the prescaler depending on the mclk output state, the frame format and audio sample rate */
	if (I2S_MCKOUT_ENABLE == mckout) {
		clks = (uint32_t) (((i2sclock / 256U) * 10U) / audiosample);
	} else {
		if (I2S_FRAMEFORMAT_DT16B_CH16B == frameformat) {
			clks = (uint32_t) (((i2sclock / 32U) * 10U) / audiosample);
		} else {
			clks = (uint32_t) (((i2sclock / 64U) * 10U) / audiosample);
		}
	}

	/* remove the floating point */
	clks = (clks + 5U) / 10U;
	i2sof = (clks & 0x00000001U);
	i2sdiv = ((clks - i2sof) / 2U);
	i2sof = (i2sof << 8U);

	/* set the default values */
	if ((i2sdiv < 2U) || (i2sdiv > 255U)) {
		i2sdiv = 2U;
		i2sof = 0U;
	}

	printf("clks=%u, i2sclock=%u, i2sof=%u, i2sdiv=%u\n", clks, i2sclock, i2sof, i2sdiv);
}
#endif
