/**
 * @file gd32_dma.h
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

#ifndef GD32_DMA_H_
#define GD32_DMA_H_

#include <cstdint>
#include <cassert>

#include "gd32.h"

#if defined (GD32F4XX) || defined (GD32H7XX)
# define DMA_PARAMETER_STRUCT				dma_single_data_parameter_struct
# define DMA_CHMADDR						DMA_CHM0ADDR
# define DMA_MEMORY_TO_PERIPHERAL			DMA_MEMORY_TO_PERIPH
# define DMA_PERIPHERAL_WIDTH_8BIT			DMA_PERIPH_WIDTH_8BIT
# define dma_init							dma_single_data_mode_init
# define dma_memory_to_memory_disable(x,y)
#else
# define DMA_PARAMETER_STRUCT				dma_parameter_struct
#endif

#if defined (GD32F10X) || defined (GD32F30X)
template<uint32_t dma_periph, dma_channel_enum channelx, uint32_t flag>
bool gd32_dma_interrupt_flag_get() {
	uint32_t interrupt_enable = 0, interrupt_flag = 0;

	switch (flag) {
	case DMA_INT_FLAG_FTF:
		interrupt_flag = DMA_INTF(dma_periph) & DMA_FLAG_ADD(flag, channelx);
		interrupt_enable = DMA_CHCTL(dma_periph, channelx) & DMA_CHXCTL_FTFIE;
		break;
	case DMA_INT_FLAG_HTF:
		interrupt_flag = DMA_INTF(dma_periph) & DMA_FLAG_ADD(flag, channelx);
		interrupt_enable = DMA_CHCTL(dma_periph, channelx) & DMA_CHXCTL_HTFIE;
		break;
	case DMA_INT_FLAG_ERR:
		interrupt_flag = DMA_INTF(dma_periph) & DMA_FLAG_ADD(flag, channelx);
		interrupt_enable = DMA_CHCTL(dma_periph, channelx) & DMA_CHXCTL_ERRIE;
		break;
	default:
		assert(0);
	}

	return interrupt_flag && interrupt_enable;
}
#elif defined (GD32F20X)
template<uint32_t dma_periph, dma_channel_enum channelx, uint32_t flag>
bool gd32_dma_interrupt_flag_get() {
	uint32_t interrupt_enable = 0U, interrupt_flag = 0U;
	uint32_t gif_check = 0x0FU, gif_enable = 0x0EU;

	switch (flag) {
	case DMA_INT_FLAG_FTF:
		interrupt_flag = DMA_INTF(dma_periph) & DMA_FLAG_ADD(flag, channelx);
		interrupt_flag = interrupt_flag >> ((channelx) * 4U);
		interrupt_enable = DMA_CHCTL(dma_periph, channelx) & DMA_CHXCTL_FTFIE;
		break;
	case DMA_INT_FLAG_HTF:
		interrupt_flag = DMA_INTF(dma_periph) & DMA_FLAG_ADD(flag, channelx);
		interrupt_flag = interrupt_flag >> ((channelx) * 4U);
		interrupt_enable = DMA_CHCTL(dma_periph, channelx) & DMA_CHXCTL_HTFIE;
		break;
	case DMA_INT_FLAG_ERR:
		interrupt_flag = DMA_INTF(dma_periph) & DMA_FLAG_ADD(flag, channelx);
		interrupt_flag = interrupt_flag >> ((channelx) * 4U);
		interrupt_enable = DMA_CHCTL(dma_periph, channelx) & DMA_CHXCTL_ERRIE;
		break;
	case DMA_INT_FLAG_G:
		interrupt_flag = DMA_INTF(dma_periph) & DMA_FLAG_ADD(gif_check, channelx);
		interrupt_flag = interrupt_flag >> ((channelx) * 4U);
		interrupt_enable = DMA_CHCTL(dma_periph, channelx) & gif_enable;
		break;
	default:
		assert(0);
	}

	return (interrupt_flag && interrupt_enable);
}
#elif defined (GD32F4XX) || defined (GD32H7XX)
template<uint32_t dma_periph, dma_channel_enum channelx, uint32_t flag>
bool gd32_dma_interrupt_flag_get() {
	uint32_t interrupt_enable = 0U, interrupt_flag = 0U;

	if constexpr (channelx < DMA_CH4) {
		switch (flag) {
		case DMA_INTF_FEEIF:
			interrupt_flag = DMA_INTF0(dma_periph) & DMA_FLAG_ADD(flag, channelx);
			interrupt_enable = DMA_CHFCTL(dma_periph, channelx) & DMA_CHXFCTL_FEEIE;
			break;
		case DMA_INTF_SDEIF:
			interrupt_flag = DMA_INTF0(dma_periph) & DMA_FLAG_ADD(flag, channelx);
			interrupt_enable = DMA_CHCTL(dma_periph, channelx) & DMA_CHXCTL_SDEIE;
			break;
		case DMA_INTF_TAEIF:
			interrupt_flag = DMA_INTF0( dma_periph) & DMA_FLAG_ADD(flag, channelx);
			interrupt_enable = DMA_CHCTL(dma_periph, channelx) & DMA_CHXCTL_TAEIE;
			break;
		case DMA_INTF_HTFIF:
			interrupt_flag = DMA_INTF0(dma_periph) & DMA_FLAG_ADD(flag, channelx);
			interrupt_enable = DMA_CHCTL(dma_periph, channelx) & DMA_CHXCTL_HTFIE;
			break;
		case DMA_INTF_FTFIF:
			interrupt_flag = (DMA_INTF0(dma_periph) & DMA_FLAG_ADD(flag, channelx));
			interrupt_enable = (DMA_CHCTL(dma_periph, channelx) & DMA_CHXCTL_FTFIE);
			break;
		default:
			assert(0);
			break;
		}
	} else if constexpr (channelx <= DMA_CH7) {
		constexpr uint32_t channel_flag_offset = static_cast<uint32_t>(channelx) - 4;
		switch (flag) {
		case DMA_INTF_FEEIF:
			interrupt_flag = DMA_INTF1(dma_periph) & DMA_FLAG_ADD(flag, channel_flag_offset);
			interrupt_enable = DMA_CHFCTL(dma_periph, channelx) & DMA_CHXFCTL_FEEIE;
			break;
		case DMA_INTF_SDEIF:
			interrupt_flag = DMA_INTF1(dma_periph) & DMA_FLAG_ADD(flag, channel_flag_offset);
			interrupt_enable = DMA_CHCTL(dma_periph, channelx) & DMA_CHXCTL_SDEIE;
			break;
		case DMA_INTF_TAEIF:
			interrupt_flag = DMA_INTF1(dma_periph) & DMA_FLAG_ADD(flag, channel_flag_offset);
			interrupt_enable = DMA_CHCTL(dma_periph, channelx) & DMA_CHXCTL_TAEIE;
			break;
		case DMA_INTF_HTFIF:
			interrupt_flag = DMA_INTF1(dma_periph) & DMA_FLAG_ADD(flag, channel_flag_offset);
			interrupt_enable = DMA_CHCTL(dma_periph, channelx) & DMA_CHXCTL_HTFIE;
			break;
		case DMA_INTF_FTFIF:
			interrupt_flag = DMA_INTF1(dma_periph) & DMA_FLAG_ADD(flag, channel_flag_offset);
			interrupt_enable = DMA_CHCTL(dma_periph, channelx) & DMA_CHXCTL_FTFIE;
			break;
		default:
			assert(0);
			break;
		}
	} else {
		assert(0);
	}

	return (interrupt_flag && interrupt_enable);
}
#else
# error
#endif

#if defined (GD32F10X) || defined (GD32F30X) || defined (GD32F20X)
template<uint32_t peripheral, dma_channel_enum channel, uint32_t nFlag>
void gd32_dma_interrupt_flag_clear() {
    DMA_INTC(peripheral) |= DMA_FLAG_ADD(nFlag, channel);
}
#elif defined (GD32F4XX) || defined (GD32H7XX)
template<uint32_t peripheral, dma_channel_enum channel, uint32_t nFlag>
inline void gd32_dma_interrupt_flag_clear() {
    if constexpr (channel < DMA_CH4) {
        DMA_INTC0(peripheral) |= DMA_FLAG_ADD(nFlag, channel);
    } else {
        DMA_INTC1(peripheral) |= DMA_FLAG_ADD(nFlag, static_cast<uint32_t>(channel) - 4U);
    }
}
#else
# error
#endif

#if defined (GD32F10X) || defined (GD32F30X)
template<uint32_t peripheral, dma_channel_enum channel, uint32_t nSource>
void gd32_dma_interrupt_disable() {
	if constexpr (DMA1 == peripheral) {
		static_assert(channel <= DMA_CH4, "for DMA1, the channel is from DMA_CH0 to DMA_CH4");
	}

	DMA_CHCTL(peripheral, channel) &= static_cast<uint32_t>(~nSource);
}
#elif defined (GD32F20X)
template<uint32_t peripheral, dma_channel_enum channel, uint32_t nSource>
void gd32_dma_interrupt_disable() {
    DMA_CHCTL(peripheral, channel) &= static_cast<uint32_t>(~nSource);
}
#elif defined (GD32F4XX)
template<uint32_t peripheral, dma_channel_enum channel, uint32_t nSource>
void gd32_dma_interrupt_disable() {
	if constexpr (DMA_CHXFCTL_FEEIE != nSource) {
		DMA_CHCTL(peripheral, channel) &= static_cast<uint32_t>(~nSource);
	} else {
		DMA_CHFCTL(peripheral, channel) &= static_cast<uint32_t>(~nSource);
	}
}
#elif defined (GD32H7XX)
template<uint32_t peripheral, dma_channel_enum channel, uint32_t nSource>
void gd32_dma_interrupt_disable() {
	if constexpr (DMA_CHXFCTL_FEEIE != (DMA_CHXFCTL_FEEIE & nSource)) {
		DMA_CHCTL(peripheral, channel) &= static_cast<uint32_t>(~nSource);
	} else {
		DMA_CHFCTL(peripheral, channel) &= static_cast<uint32_t>(~DMA_CHXFCTL_FEEIE);
		DMA_CHCTL(peripheral, channel) &= static_cast<uint32_t>(~(nSource & (~DMA_CHXFCTL_FEEIE)));
	}
}
#else
# error
#endif

#endif /* GD32_DMA_H_ */
