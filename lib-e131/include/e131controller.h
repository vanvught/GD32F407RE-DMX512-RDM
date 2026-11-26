/**
 * @file e131controller.h
 *
 */
/* Copyright (C) 2020-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef E131CONTROLLER_H_
#define E131CONTROLLER_H_

#include <cstdint>

#include "e131.h"
#include "dmxnode.h"
#include "softwaretimers.h"

inline constexpr uint16_t kDefaultSynchronizationAddress = 5000;

struct TE131ControllerState {
    uint16_t active_universes;
    uint32_t discovery_time;
    uint8_t priority;
    struct TSynchronizationPacket {
        uint16_t universe;
        uint32_t ip_address;
        uint8_t sequence_number;
    } synchronization_packet;
};

class E131Controller {
   public:
    E131Controller();
    ~E131Controller();

    void Start();
    void Stop();

    void Print();

    void HandleDmxOut(uint16_t universe, const uint8_t* data, uint32_t length);
    void HandleSync();
    void HandleBlackout();

    void SetSynchronizationAddress(uint16_t synchronization_address = kDefaultSynchronizationAddress) {
        state_.synchronization_packet.universe = synchronization_address;
        state_.synchronization_packet.ip_address = e131::UniverseToMulticastIp(synchronization_address);
    }

    [[nodiscard]] uint16_t GetSynchronizationAddress() const { return state_.synchronization_packet.universe; }

    void SetMaster(uint32_t master = dmxnode::kDmxMaxValue) {
        if (master < dmxnode::kDmxMaxValue) {
            master_ = master;
        } else {
            master_ = dmxnode::kDmxMaxValue;
        }
    }

    [[nodiscard]] uint32_t GetMaster() const { return master_; }

    const uint8_t* GetSoftwareVersion();

    void SetSourceName(const char* source_name);
    void SetPriority(uint8_t priority);

    static E131Controller* Get() { return s_this; }

   private:
    void FillDataPacket();
    void FillDiscoveryPacket();
    void FillSynchronizationPacket();
    uint8_t GetSequenceNumber(uint16_t universe, uint32_t& multicast_ip_address);

    void SendDiscoveryPacket();

    void static StaticCallbackFunctionSendDiscoveryPacket([[maybe_unused]] TimerHandle_t timer_handle) { s_this->SendDiscoveryPacket(); }

    int32_t handle_{-1};
    struct TE131ControllerState state_;
    e131::DataPacket* data_packet_{nullptr};
    e131::DiscoveryPacket* discovery_packet_{nullptr};
    e131::SynchronizationPacket* synchronization_packet_{nullptr};
    uint32_t discovery_ip_address_{0};
    uint8_t cid_[e117::kCidLength];
    char source_name_[e131::kSourceNameLength];
    uint32_t master_{dmxnode::kDmxMaxValue};
    TimerHandle_t timer_handle_send_discovery_packet_{-1};

    static inline E131Controller* s_this;
};

#endif // E131CONTROLLER_H_
