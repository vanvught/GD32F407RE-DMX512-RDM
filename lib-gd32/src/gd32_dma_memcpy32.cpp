/*
 * gd32_dma_memcpy32.cpp
 */

#include <cstdint>
#include <cassert>

#include "gd32.h"

namespace dma {
void memcpy32_init() {
#if !defined (GD32F4XX)
	rcu_periph_clock_enable(RCU_DMA0);
    dma_deinit(DMA0, DMA_CH3);

    dma_parameter_struct dma_init_struct;
    dma_struct_para_init(&dma_init_struct);

    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
//  dma_init_struct.memory_addr = destination;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_32BIT;
    dma_init_struct.number = 0; // BUFFER_SIZE;
//  dma_init_struct.periph_addr = source
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_ENABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_32BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;

    dma_init(DMA0, DMA_CH3, &dma_init_struct);
    dma_circulation_disable(DMA0, DMA_CH3);
    dma_memory_to_memory_enable(DMA0, DMA_CH3);
#else
    rcu_periph_clock_enable(RCU_DMA1);
    dma_deinit(DMA1, DMA_CH0);

    dma_multi_data_parameter_struct  dma_init_parameter;

//  dma_init_parameter.periph_addr = source;
    dma_init_parameter.periph_width = DMA_PERIPH_WIDTH_32BIT;
    dma_init_parameter.periph_inc = DMA_PERIPH_INCREASE_ENABLE;
//  dma_init_parameter.memory0_addr = destination;
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
#endif
}
}  // namespace dma
