/**
 * @file artnetrdmcontroller.h
 *
 */
/* Copyright (C) 2017-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef ARTNETRDMCONTROLLER_H_
#define ARTNETRDMCONTROLLER_H_

#include <cstdint>

#include "rdm_discovery.h"

namespace artnet::rdm::controller {
inline static constexpr uint32_t kBackgroundIntervalMinutes = 15;

void DiscoveryStart(uint32_t port_index);
void DiscoveryDone(uint32_t port_index);
} // namespace artnet::rdm::controller

class ArtNetRdmController final : rdm::Discovery {
   public:
    ArtNetRdmController() {
#ifndef NODE_RDMNET_LLRP_ONLY
        rdm::device::Base::Instance().Print();
#endif // NODE_RDMNET_LLRP_ONLY
    }

    ~ArtNetRdmController() = default;

    ArtNetRdmController(const ArtNetRdmController&) = delete;
    ArtNetRdmController& operator=(const ArtNetRdmController&) = delete;

    // Discovery process BEGIN

    void Enable(uint32_t port_index) { rdm::Discovery::Enable(port_index); }

    void Disable(uint32_t port_index) { rdm::Discovery::Disable(port_index); }

    [[nodiscard]] bool IsEnabled(uint32_t port_index) const { return rdm::Discovery::IsEnabled(port_index); }

    void EnableBackground(uint32_t port_index) { rdm::Discovery::EnableBackground(port_index); }

    void DisableBackground(uint32_t port_index) { rdm::Discovery::DisableBackground(port_index); }

    [[nodiscard]] bool IsEnabledBackground(uint32_t port_index) const { return rdm::Discovery::IsEnabledBackground(port_index); }

    void Full(uint32_t port_index) { rdm::Discovery::Full(port_index); }

    void Incremental(uint32_t port_index) { rdm::Discovery::Incremental(port_index); }

    void Stop(uint32_t port_index) { rdm::Discovery::Stop(port_index); }

    bool IsRunning(uint32_t port_index) { return rdm::Discovery::IsRunning(port_index); }

    void Run() { rdm::Discovery::Run(); }

    // Discovery process END

    uint32_t TodUidCount(uint32_t port_index) { return rdm::Discovery::TodUidCount(port_index); }

    void TodCopy(uint32_t port_index, uint8_t* tod) { rdm::Discovery::TodCopy(port_index, tod); }

    // Gateway

    bool RdmReceive(uint32_t port_index, const uint8_t* rdm_data);

    void TodReset(uint32_t port_index) { rdm::Discovery::TodReset(port_index); }

    bool TodAddUid(uint32_t port_index, const uint8_t* uid) { return rdm::Discovery::TodAddUid(port_index, uid); }

    // Generic

    bool CopyTodEntry(uint32_t port_index, uint32_t index, uint8_t uid[rdm::kUidSize]) { return rdm::Discovery::TodCopyUidEntry(port_index, index, uid); }

    void TodDump(uint32_t port_index) { rdm::Discovery::TodDump(port_index); }
};

#endif // ARTNETRDMCONTROLLER_H_
