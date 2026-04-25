/**
 * @file emac_counters.cpp
 *
 */
/* Copyright (C) 2025-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include "network_iface.h"
#include "emac_counters.h"
#include "gd32.h" // IWYU pragma: keep

namespace network::iface {
static uint64_t s_rx_fifo_drop_total = 0;
static uint64_t s_rx_dma_missed_total = 0;

static void PollHardwareCounters() {
    uint32_t rxfifo_drop;  ///< The number of frames dropped by RxFIFO
    uint32_t rxdma_missed; ///< The number of frames missed by the RxDMA controller

#if defined(GD32H7XX)
    enet_missed_frame_counter_get(ENETx, &rxfifo_drop, &rxdma_missed);
#else
    enet_missed_frame_counter_get(&rxfifo_drop, &rxdma_missed);
#endif

    // ENET_DMA_MFBOCNT is read-to-clear, so accumulate immediately.
    s_rx_fifo_drop_total += rxfifo_drop;
    s_rx_dma_missed_total += rxdma_missed;
}

void GetCounters(Counters& counters) {
    PollHardwareCounters();

    // Receive
    counters.rx.ok = emac::eth::globals::counter.received;
    // Linux /proc/net/dev "drop" includes rx_missed_errors.
    // For your embedded view, I would count both as dropped RX frames.
    counters.rx.drp = s_rx_fifo_drop_total + s_rx_dma_missed_total;
    // Linux "fifo" / rx_over_errors / rx_fifo_errors semantic:
    // FIFO overflow / FIFO unable to accept/store frame.
    counters.rx.ovr = s_rx_fifo_drop_total;
    // Optional: track CRC, length, descriptor errors, keep 0.
    counters.rx.err = 0;

    // Transmit
    counters.tx.ok = emac::eth::globals::counter.sent;
    // send_busy is probably better as tx.drp than tx.err.
    // It means software could not queue a packet because DMA still owns desc.
    counters.tx.drp = emac::eth::globals::counter.send_busy;
    counters.tx.err = 0;
    counters.tx.ovr = 0;
}
} // namespace network::iface
