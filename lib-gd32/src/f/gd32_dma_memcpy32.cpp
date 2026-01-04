/**
 * @file gd32_dma_memcpy32.cpp
 *
 */
/* Copyright (C) 2024-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include "gd32.h"
#include "gd32_dma.h"

namespace dma::memcpy32
{
void Init()
{
#if !defined(GD32F4XX)
    rcu_periph_clock_enable(RCU_DMA0);
    dma_deinit(DMA0, DMA_CH3);

    dma_parameter_struct dma_init_struct;
    dma_struct_para_init(&dma_init_struct);

    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_32BIT;
    dma_init_struct.number = 0; // BUFFER_SIZE;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_ENABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_32BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;

    dma_init(DMA0, DMA_CH3, &dma_init_struct);
    dma_circulation_disable(DMA0, DMA_CH3);
    dma_memory_to_memory_enable(DMA0, DMA_CH3);

    Gd32DmaInterruptFlagClear<DMA0, DMA_CH3, (DMA_INT_FLAG_FTF | DMA_INT_FLAG_G)>();

    NVIC_SetPriority(DMA0_Channel3_IRQn, 0);
    NVIC_EnableIRQ(DMA0_Channel3_IRQn);
#else
    rcu_periph_clock_enable(RCU_DMA1);
    dma_deinit(DMA1, DMA_CH0);

    dma_multi_data_parameter_struct dma_init_parameter;

    dma_init_parameter.periph_width = DMA_PERIPH_WIDTH_32BIT;
    dma_init_parameter.periph_inc = DMA_PERIPH_INCREASE_ENABLE;
    dma_init_parameter.memory_width = DMA_MEMORY_WIDTH_32BIT;
    dma_init_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_parameter.memory_burst_width = DMA_MEMORY_BURST_4_BEAT;
    dma_init_parameter.periph_burst_width = DMA_PERIPH_BURST_4_BEAT;
    dma_init_parameter.critical_value = DMA_FIFO_4_WORD;
    dma_init_parameter.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    dma_init_parameter.direction = DMA_MEMORY_TO_MEMORY;
    dma_init_parameter.number = 0; // BUFFER_SIZE;
    dma_init_parameter.priority = DMA_PRIORITY_ULTRA_HIGH;

    dma_multi_data_mode_init(DMA1, DMA_CH0, &dma_init_parameter);

    Gd32DmaInterruptFlagClear<DMA1, DMA_CH3, (DMA_INT_FLAG_FTF | DMA_INT_FLAG_TAE)>();

    NVIC_SetPriority(DMA1_Channel0_IRQn, 0);
    NVIC_EnableIRQ(DMA1_Channel0_IRQn);
#endif
}
} // namespace dma::memcpy32
