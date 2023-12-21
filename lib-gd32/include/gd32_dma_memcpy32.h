/*
 * gd32_dma_memcpy32.h
 */

#ifndef GD32_DMA_MEMCPY32_H_
#define GD32_DMA_MEMCPY32_H_

#include <cstdint>
#include <cassert>

#include "gd32.h"

namespace dma {
void memcpy32_init() ;

inline void memcpy32(const void *pDestination, const void *pSource, const uint32_t nLength) {
	assert((reinterpret_cast<uint32_t>(pSource) & 0x3) == 0);
	assert((reinterpret_cast<uint32_t>(pDestination) & 0x3) == 0);

#if !defined (GD32F4XX)
	uint32_t dmaCHCTL = DMA_CHCTL(DMA0, DMA_CH3);
	dmaCHCTL &= ~DMA_CHXCTL_CHEN;
	DMA_CHCTL(DMA0, DMA_CH3) = dmaCHCTL;

    DMA_CHPADDR(DMA0, DMA_CH3) = reinterpret_cast<uint32_t>(pSource);
    DMA_CHMADDR(DMA0, DMA_CH3) = reinterpret_cast<uint32_t>(pDestination);
    DMA_CHCNT(DMA0, DMA_CH3) = (nLength & DMA_CHXCNT_CNT);

    dmaCHCTL |= DMA_CHXCTL_CHEN;
    DMA_CHCTL(DMA0, DMA_CH3) = dmaCHCTL;
#else
	uint32_t dmaCHCTL = DMA_CHCTL(DMA1, DMA_CH0);
	dmaCHCTL &= ~DMA_CHXCTL_CHEN;
	DMA_CHCTL(DMA1, DMA_CH0) = dmaCHCTL;

	DMA_INTC0(DMA1) |= DMA_FLAG_ADD(DMA_CHINTF_RESET_VALUE, DMA_CH0);

    DMA_CHM0ADDR(DMA1, DMA_CH0) = reinterpret_cast<uint32_t>(pDestination);
    DMA_CHPADDR(DMA1, DMA_CH0) = reinterpret_cast<uint32_t>(pSource);
    DMA_CHCNT(DMA1, DMA_CH0) = nLength;

	dmaCHCTL |= DMA_CHXCTL_CHEN;
	DMA_CHCTL(DMA1, DMA_CH0) = dmaCHCTL;
#endif
}

inline bool memcpy32_is_active() {
#if !defined (GD32F4XX)
	return DMA_CHCNT(DMA0, DMA_CH3) != 0;
#else
	return DMA_CHCNT(DMA1, DMA_CH0) != 0;
#endif
}
}  // namespace dma

#endif /* GD32_DMA_MEMCPY32_H_ */
