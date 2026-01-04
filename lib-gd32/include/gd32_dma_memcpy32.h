/**
 * @file gd32_dma_memcpy32.h
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

#ifndef GD32_DMA_MEMCPY32_H_
#define GD32_DMA_MEMCPY32_H_

#include <cstdint>
#include <cassert>

#include "gd32.h"

namespace dma::memcpy32
{
void Init();

inline void StartDma(const void* destination, const void* source, uint32_t length)
{
    assert((reinterpret_cast<uint32_t>(source) & 0x3) == 0);
    assert((reinterpret_cast<uint32_t>(destination) & 0x3) == 0);

#if !defined(GD32F4XX)
    uint32_t dma_chctl = DMA_CHCTL(DMA0, DMA_CH3);
    dma_chctl &= ~DMA_CHXCTL_CHEN;
    DMA_CHCTL(DMA0, DMA_CH3) = dma_chctl;

    DMA_CHPADDR(DMA0, DMA_CH3) = reinterpret_cast<uint32_t>(source);
    DMA_CHMADDR(DMA0, DMA_CH3) = reinterpret_cast<uint32_t>(destination);
    DMA_CHCNT(DMA0, DMA_CH3) = (length & DMA_CHXCNT_CNT);

    dma_chctl |= (DMA_CHXCTL_CHEN | DMA_INT_FTF);
    DMA_CHCTL(DMA0, DMA_CH3) = dma_chctl;
#else
    uint32_t dma_chctl = DMA_CHCTL(DMA1, DMA_CH0);
    dma_chctl &= ~DMA_CHXCTL_CHEN;
    DMA_CHCTL(DMA1, DMA_CH0) = dma_chctl;

    DMA_INTC0(DMA1) |= DMA_FLAG_ADD(DMA_CHINTF_RESET_VALUE, DMA_CH0);

    DMA_CHM0ADDR(DMA1, DMA_CH0) = reinterpret_cast<uint32_t>(destination);
    DMA_CHPADDR(DMA1, DMA_CH0) = reinterpret_cast<uint32_t>(source);
    DMA_CHCNT(DMA1, DMA_CH0) = length;

    dma_chctl |= (DMA_CHXCTL_CHEN | DMA_CHXCTL_FTFIE);
    DMA_CHCTL(DMA1, DMA_CH0) = dma_chctl;
#endif
}

inline bool IsActive()
{
#if !defined(GD32F4XX)
    return DMA_CHCNT(DMA0, DMA_CH3) != 0;
#else
    return DMA_CHCNT(DMA1, DMA_CH0) != 0;
#endif
}
} // namespace dma::memcpy32

#endif // GD32_DMA_MEMCPY32_H_
