/**
 * @file emac_counters.cpp
 *
 */
/* Copyright (C) 2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include "network.h"
#include "gd32.h"
#include "firmware/debug/debug_debug.h"

namespace network::iface
{
void GetSoftwareStats(
    /*out*/ uint64_t* rx_ok,
    /*out*/ uint64_t* rx_drp,
    /*out*/ uint64_t* rx_len_err,
    /*out*/ uint64_t* rx_fifo_err,
    /*out*/ uint64_t* tx_ok,
    /*out*/ uint64_t* tx_drp,
    /*out*/ uint64_t* tx_err,
    /*out*/ uint64_t* tx_fifo_err)
{
    if (rx_ok) *rx_ok = 0;
    if (rx_drp) *rx_drp = 0;
    if (rx_len_err) *rx_len_err = 0;
    if (rx_fifo_err) *rx_fifo_err = 0;
    if (tx_ok) *tx_ok = 0;
    if (tx_drp) *tx_drp = 0;
    if (tx_err) *tx_err = 0;
    if (tx_fifo_err) *tx_fifo_err = 0;
}

// ---------- HW fold: accumulate 32-bit MMC into 64-bit totals ----------
struct HwAcc
{
    uint64_t tx_good = 0;       // TGFCNT
    uint64_t rx_crc = 0;        // RFCECNT
    uint64_t rx_align = 0;      // RFAECNT
    uint64_t rx_fifo_ovr = 0;   // MFBOCNT MSFA
    uint64_t rx_dma_missed = 0; // MFBOCNT MSFC

    uint32_t prev_tx_good = 0;
    uint32_t prev_rx_crc = 0;
    uint32_t prev_rx_align = 0;
    uint32_t prev_rx_fifo_ovr = 0;
    uint32_t prev_rx_dma_missed = 0;
};

static HwAcc g_hw;

static inline uint64_t Delta32(uint32_t now, uint32_t& prev)
{
    uint32_t d = now - prev; // wrap-safe unsigned diff
    prev = now;
    return static_cast<uint64_t>(d);
}

static void FoldHw()
{
#if defined(GD32H7XX)
    const auto kTxTgf = enet_msc_counters_get(ENETx, ENET_MSC_TX_TGFCNT);
    const auto kRxRfce = enet_msc_counters_get(ENETx, ENET_MSC_RX_RFCECNT);
    const auto kRxRfae = enet_msc_counters_get(ENETx, ENET_MSC_RX_RFAECNT);
#else
    const auto kTxTgf = enet_msc_counters_get(ENET_MSC_TX_TGFCNT);
    const auto kRxRfce = enet_msc_counters_get(ENET_MSC_RX_RFCECNT);
    const auto kRxRfae = enet_msc_counters_get(ENET_MSC_RX_RFAECNT);
#endif

    DEBUG_PRINTF("%u:%u:%u", kTxTgf, kRxRfce, kRxRfae);

    uint32_t msfa = 0, msfc = 0;
#if defined(GD32H7XX)
    enet_missed_frame_counter_get(ENETx, &msfa, &msfc);
#else
    enet_missed_frame_counter_get(&msfa, &msfc);
#endif
    g_hw.tx_good += Delta32(kTxTgf, g_hw.prev_tx_good);
    g_hw.rx_crc += Delta32(kRxRfce, g_hw.prev_rx_crc);
    g_hw.rx_align += Delta32(kRxRfae, g_hw.prev_rx_align);
    g_hw.rx_fifo_ovr += Delta32(msfa, g_hw.prev_rx_fifo_ovr);
    g_hw.rx_dma_missed += Delta32(msfc, g_hw.prev_rx_dma_missed);

    DEBUG_PRINTF("g_hw.tx_good=%u", static_cast<uint32_t>(g_hw.tx_good));
}

void GetCounters(network::iface::Counters& st)
{
    DEBUG_ENTRY();
    // 1) Fold hw counters into 64-bit totals
    FoldHw();

    // 2) Get software stats (prefer these for rx_ok/tx_ok)
    uint64_t rx_ok_sw = 0, rx_drp_sw = 0, rx_len_err_sw = 0, rx_fifo_err_sw = 0;
    uint64_t tx_ok_sw = 0, tx_drp_sw = 0, tx_err_sw = 0, tx_fifo_err_sw = 0;
    GetSoftwareStats(&rx_ok_sw, &rx_drp_sw, &rx_len_err_sw, &rx_fifo_err_sw, &tx_ok_sw, &tx_drp_sw, &tx_err_sw, &tx_fifo_err_sw);

    // 3) Map to netstat-like fields
    st.rx_ok = rx_ok_sw;                                         // software includes uni/multi/bcast
    st.rx_drp = rx_drp_sw;                                       // software drops
    st.rx_ovr = g_hw.rx_fifo_ovr;                                // FIFO overflow (MSFA)
    st.rx_err = g_hw.rx_crc + g_hw.rx_align + g_hw.rx_dma_missed // MSFC
                + st.rx_ovr + rx_len_err_sw + rx_fifo_err_sw;

    st.tx_ok = (tx_ok_sw ? tx_ok_sw : g_hw.tx_good); // fallback to HW
    st.tx_err = tx_err_sw;
    st.tx_drp = tx_drp_sw;
    st.tx_ovr = tx_fifo_err_sw;

    DEBUG_EXIT();
}
} // namespace network::iface
