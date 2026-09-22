/**
 * @file gd32_dma_memcpy16.cpp
 *
 */
/* Copyright (C) 2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include "gd32.h" // IWYU pragma: keep

namespace ggd32::dma::memcpy16 {
void Init() {
#ifndef GD32F4XX
#error
#else
    rcu_periph_clock_enable(RCU_DMA1);
    dma_deinit(DMA1, DMA_CH0);

    dma_multi_data_parameter_struct dma_init_parameter;
    dma_multi_data_para_struct_init(&dma_init_parameter);

    dma_init_parameter.periph_width = DMA_PERIPH_WIDTH_16BIT;
    dma_init_parameter.periph_inc = DMA_PERIPH_INCREASE_ENABLE;
    dma_init_parameter.memory_width = DMA_MEMORY_WIDTH_16BIT;
    dma_init_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_parameter.memory_burst_width = DMA_PERIPH_BURST_SINGLE;
    dma_init_parameter.periph_burst_width = DMA_PERIPH_BURST_SINGLE;
    dma_init_parameter.critical_value = DMA_FIFO_1_WORD;
    dma_init_parameter.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    dma_init_parameter.direction = DMA_MEMORY_TO_MEMORY;
    dma_init_parameter.number = 0; // BUFFER_SIZE;
    dma_init_parameter.priority = DMA_PRIORITY_LOW;

    dma_multi_data_mode_init(DMA1, DMA_CH0, &dma_init_parameter);
#endif
}
} // namespace ggd32::dma::memcpy16