/**
 * @file emac_eth.cpp
 * @brief Ethernet MAC driver implementation for GD32 devices.
 *
 * This file provides functions for managing Ethernet communication, including
 * packet transmission and reception. It also supports Precision Time Protocol
 * (PTP) functionality if enabled.
 */
/* Copyright (C) 2022-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#if defined(DEBUG_EMAC)
#undef NDEBUG
#endif

#if !defined(CONFIG_REMOTECONFIG_MINIMUM)
#pragma GCC push_options
#pragma GCC optimize("O2")
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#include <cstdint>
#include <cassert>

#include "gd32_enet.h"
#include "../src/core/network_memcpy.h"
#include "emac_counters.h"
#include "firmware/debug/debug_dump.h"
#include "firmware/debug/debug_debug.h"
#include "gd32.h" // IWYU pragma: keep

#if defined(CONFIG_NET_ENABLE_PTP)
#include "gd32_ptp.h"

/// Current PTP receive descriptor
extern enet_descriptors_struct* dma_current_ptp_rxdesc;
/// Current PTP transmit descriptor
extern enet_descriptors_struct* dma_current_ptp_txdesc;

namespace net::globals::ptp {
extern uint32_t timestamp[2];
} // namespace net::globals::ptp
#endif

/// Current receive descriptor
extern enet_descriptors_struct* dma_current_rxdesc;
/// Current transmit descriptor
extern enet_descriptors_struct* dma_current_txdesc;

namespace emac::eth {
namespace globals {
struct Counters counter;
}

// Receives an Ethernet packet.
uint32_t Recv(uint8_t** packet) {
    const auto kLength = gd32::enet::DescInformationGet<RXDESC_FRAME_LENGTH>(dma_current_rxdesc);

    if (kLength > 0) {
#if defined(CONFIG_NET_ENABLE_PTP)
        *packet = reinterpret_cast<uint8_t*>(dma_current_ptp_rxdesc->buffer1_addr);
#else
        *packet = reinterpret_cast<uint8_t*>(dma_current_rxdesc->buffer1_addr);
#endif
        emac::eth::globals::counter.received++;
        return kLength;
    }

    return 0;
}

#if defined(CONFIG_NET_ENABLE_PTP)
// Handles reception of a PTP frame in normal mode.
static void PtpFrameReceiveNormalMode() {
    net::globals::ptp::timestamp[0] = dma_current_rxdesc->buffer1_addr;
    net::globals::ptp::timestamp[1] = dma_current_rxdesc->buffer2_next_desc_addr;

    dma_current_rxdesc->buffer1_addr = dma_current_ptp_rxdesc->buffer1_addr;
    dma_current_rxdesc->buffer2_next_desc_addr = dma_current_ptp_rxdesc->buffer2_next_desc_addr;
    dma_current_rxdesc->status = ENET_RDES0_DAV;

#if defined(GD32H7XX)
    __DMB();
#endif

    gd32::enet::HandleRxBufferUnavailable();

    assert(0 != (dma_current_rxdesc->control_buffer_size & ENET_RDES1_RCHM)); /// chained mode

    /// update the current RxDMA descriptor pointer to the next descriptor in RxDMA descriptor table
    dma_current_rxdesc = reinterpret_cast<enet_descriptors_struct*>(dma_current_ptp_rxdesc->buffer2_next_desc_addr);
    /// if it is the last ptp descriptor */
    if (0 != dma_current_ptp_rxdesc->status) {
        /// pointer back to the first ptp descriptor address in the desc_ptptab list address
        dma_current_ptp_rxdesc = reinterpret_cast<enet_descriptors_struct*>(dma_current_ptp_rxdesc->status);
    } else {
        /// Set pointer to the next ptp descriptor
        dma_current_ptp_rxdesc++;
    }
}
#else
/**
 * @brief Handles reception of a standard Ethernet frame.
 */
static void FrameReceive() {
    dma_current_rxdesc->status = ENET_RDES0_DAV;

    gd32::enet::HandleRxBufferUnavailable();

    assert(0 != (dma_current_rxdesc->control_buffer_size & ENET_RDES1_RCHM));

    /// Update Rx descriptor pointer
    dma_current_rxdesc = reinterpret_cast<enet_descriptors_struct*>(dma_current_rxdesc->buffer2_next_desc_addr);
}
#endif

/**
 * @brief Frees the current packet from the DMA buffer.
 */
void FreePkt() {
    while (0 != (dma_current_rxdesc->status & ENET_RDES0_DAV)) {
        __DMB();
    }

#if defined(CONFIG_NET_ENABLE_PTP)
    PtpFrameReceiveNormalMode();
#else
    FrameReceive();
#endif
}

#if defined(CONFIG_NET_ENABLE_PTP)
/**
 * @brief Retrieves the DMA buffer for Ethernet transmission with PTP.
 *
 * @return Pointer to the DMA buffer for transmission.
 */
uint8_t* SendGetDmaBuffer() {
    while (0 != (dma_current_txdesc->status & ENET_TDES0_DAV)) {
        __DMB(); ///< Wait until descriptor is available
    }

    return reinterpret_cast<uint8_t*>(dma_current_ptp_txdesc->buffer1_addr);
}

/**
 * @brief Transmits a PTP frame.
 *
 * @tparam T Whether timestamping is enabled.
 * @param length Length of the frame to transmit.
 */
template <bool T> static void PtpFrameTransmit(uint32_t length) {
    dma_current_txdesc->control_buffer_size = length;              ///< Set the frame length
    dma_current_txdesc->status |= ENET_TDES0_LSG | ENET_TDES0_FSG; ///< Set the segment of frame, frame is transmitted in one descriptor
    dma_current_txdesc->status |= ENET_TDES0_DAV;                  ///< Enable DMA transmission

#if defined(GD32H7XX)
    __DMB();
#endif

    gd32::enet::ClearDmaTxFlagsAndResume(); ///< Handle transmission flags

    uint32_t timeout = 0;
    uint32_t tdes0_ttmss_flag;

    if constexpr (T) { // Handle timestamping
        do {
            tdes0_ttmss_flag = (dma_current_txdesc->status & ENET_TDES0_TTMSS);
            __DMB();
            timeout++;
        } while ((0 == tdes0_ttmss_flag) && (timeout < UINT32_MAX));

        DEBUG_PRINTF("timeout=%x %d", timeout, (dma_current_txdesc->status & ENET_TDES0_TTMSS));

        dma_current_txdesc->status &= ~ENET_TDES0_TTMSS; ///< Clear timestamp flag

        net::globals::ptp::timestamp[0] = dma_current_txdesc->buffer1_addr;
        net::globals::ptp::timestamp[1] = dma_current_txdesc->buffer2_next_desc_addr;
    }

    dma_current_txdesc->buffer1_addr = dma_current_ptp_txdesc->buffer1_addr;
    dma_current_txdesc->buffer2_next_desc_addr = dma_current_ptp_txdesc->buffer2_next_desc_addr;

    assert(0 != (dma_current_txdesc->status & ENET_TDES0_TCHM)); /// Chained mode

    /// Update the current TxDMA descriptor pointer to the next descriptor in TxDMA descriptor table
    dma_current_txdesc = reinterpret_cast<enet_descriptors_struct*>(dma_current_ptp_txdesc->buffer2_next_desc_addr);
    /// if it is the last ptp descriptor */
    if (0 != dma_current_ptp_txdesc->status) {
        /// pointer back to the first ptp descriptor address in the desc_ptptab list address
        dma_current_ptp_txdesc = reinterpret_cast<enet_descriptors_struct*>(dma_current_ptp_txdesc->status);
    } else {
        /// Set pointer to the next ptp descriptor
        dma_current_ptp_txdesc++;
    }
}

/**
 * @brief Sends a PTP Ethernet frame.
 *
 * @tparam T Whether timestamping is enabled.
 * @param buffer Pointer to the frame buffer.
 * @param length Length of the frame in bytes.
 */
template <bool T> static void PtpFrameTransmit(const void* buffer, uint32_t length) {
    assert(nullptr != buffer);
    assert(length <= ENET_MAX_FRAME_SIZE);

    auto* dst = reinterpret_cast<uint8_t*>(dma_current_ptp_txdesc->buffer1_addr);
    std::memcpy(dst, buffer, length); ///< Copy frame to DMA buffer

    PtpFrameTransmit<T>(length);
}

/**
 * @brief Transmits an Ethernet frame without timestamping.
 *
 * @param length Length of the frame to transmit.
 */
void Send(uint32_t length) {
    assert(length <= ENET_MAX_FRAME_SIZE);

    auto status = dma_current_txdesc->status;
    status &= ~ENET_TDES0_TTSEN;
    dma_current_txdesc->status = status;

#if defined(GD32H7XX)
    __DMB();
#endif

    PtpFrameTransmit<false>(length);
}

/**
 * @brief Transmits an Ethernet frame with data copying.
 *
 * @param buffer Pointer to the frame buffer.
 * @param length Length of the frame in bytes.
 */
void Send(void* buffer, const uint32_t length) {
    assert(nullptr != buffer);
    assert(length <= ENET_MAX_FRAME_SIZE);

    while (0 != (dma_current_txdesc->status & ENET_TDES0_DAV)) {
        __DMB(); // Wait until descriptor is available
    }

    auto status = dma_current_txdesc->status;
    status &= ~ENET_TDES0_TTSEN; // Disable timestamping
    dma_current_txdesc->status = status;

#if defined(GD32H7XX)
    __DMB();
#endif

    PtpFrameTransmit<false>(buffer, length);
}

/**
 * @brief Transmits a timestamped Ethernet frame.
 *
 * @param length Length of the frame in bytes.
 */
void SendTimestamp(uint32_t length) {
    assert(length <= ENET_MAX_FRAME_SIZE);

    auto status = dma_current_txdesc->status;
    status |= ENET_TDES0_TTSEN; ///< Enable timestamping
    dma_current_txdesc->status = status;

#if defined(GD32H7XX)
    __DMB();
#endif

    PtpFrameTransmit<true>(length);
}

/**
 * @brief Transmits a timestamped Ethernet frame with data copying.
 *
 * @param buffer Pointer to the frame buffer.
 * @param length Length of the frame in bytes.
 */
void SendTimestamp(void* buffer, uint32_t length) {
    assert(nullptr != buffer);
    assert(length <= ENET_MAX_FRAME_SIZE);

    while (0 != (dma_current_txdesc->status & ENET_TDES0_DAV)) {
        __DMB();
    }

    auto status = dma_current_txdesc->status;
    status |= ENET_TDES0_TTSEN; // Enable timestamping
    dma_current_txdesc->status = status;

#if defined(GD32H7XX)
    __DMB();
#endif

    PtpFrameTransmit<true>(buffer, length);
}
#else
/**
 * @brief Retrieves the DMA buffer for Ethernet transmission.
 *
 * @return Pointer to the DMA buffer for transmission.
 */
uint8_t* SendGetDmaBuffer() {
    // The descriptor is busy due to own by the DMA
    if (0 != (dma_current_txdesc->status & ENET_TDES0_DAV)) {
        emac::eth::globals::counter.send_busy++;
        while (0 != (dma_current_txdesc->status & ENET_TDES0_DAV)) {
            __DMB(); ///< Wait until descriptor is available
        }
    }

    return reinterpret_cast<uint8_t*>(dma_current_txdesc->buffer1_addr);
}

// Transmits an Ethernet frame.
void Send(uint32_t length) {
    debug::Dump(reinterpret_cast<uint8_t*>(dma_current_txdesc->buffer1_addr), length);

    dma_current_txdesc->control_buffer_size = length;              ///< Set the frame length
    dma_current_txdesc->status |= ENET_TDES0_LSG | ENET_TDES0_FSG; ///< Set the segment of frame, frame is transmitted in one descriptor
    dma_current_txdesc->status |= ENET_TDES0_DAV;                  ///< Enable DMA transmission

#if defined(GD32H7XX)
    __DMB();
#endif

    gd32::enet::ClearDmaTxFlagsAndResume(); ///< Handle transmission flags

    assert(0 != (dma_current_txdesc->status & ENET_TDES0_TCHM)); /// Chained mode

    /// Update the current TxDMA descriptor pointer to the next descriptor in TxDMA descriptor table
    dma_current_txdesc = reinterpret_cast<enet_descriptors_struct*>(dma_current_txdesc->buffer2_next_desc_addr);

    emac::eth::globals::counter.sent++;
}

// Transmits an Ethernet frame with data copying.
void Send(void* buffer, uint32_t length) {
    DEBUG_PRINTF("%p -> %u", buffer, length);

    assert(nullptr != buffer);
    assert(length <= ENET_MAX_FRAME_SIZE);

    auto* dest = SendGetDmaBuffer();
    std::memcpy(dest, buffer, length); ///< Copy frame to DMA buffer

    Send(length);
}
#endif
} // namespace emac::eth