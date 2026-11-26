/**
 * @file e131controller.cpp
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

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cassert>

#include "e131controller.h"
#include "e131.h"
#include "e117.h"
#include "uuid.h"
#include "board.h"
#include "network.h"
#include "softwaretimers.h"
#include "firmware/debug/debug_debug.h"

using namespace e131;

namespace {
constexpr uint8_t kDeviceSoftwareVersion[] = {1, 0};

struct TSequenceNumbers {
    uint16_t universe;
    uint8_t sequence_number;
    uint32_t ip_address;
};

struct TSequenceNumbers s_sequence_numbers[512] __attribute__((aligned(8)));
} // namespace

E131Controller::E131Controller() {
    DEBUG_ENTRY();

    assert(s_this == nullptr);
    s_this = this;

    memset(&state_, 0, sizeof(struct TE131ControllerState));
    state_.priority = 100;

    char aSourceName[e131::kSourceNameLength];
    uint8_t nLength;
    snprintf(aSourceName, e131::kSourceNameLength, "%.48s %s", network::iface::HostName(), board::BoardName(nLength));
    SetSourceName(aSourceName);

    UuidCopy(cid_);

    for (auto& s_sequence_number : s_sequence_numbers) {
        memset(&s_sequence_number, 0, sizeof(s_sequence_numbers[0]));
    }

    SetSynchronizationAddress();

    const auto kIpMulticast = network::ConvertToUint(239, 255, 0, 0);
    discovery_ip_address_ = kIpMulticast | ((universe::kDiscovery & static_cast<uint32_t>(0xFF)) << 24) | ((universe::kDiscovery & 0xFF00) << 8);

    // e131::DataPacket
    data_packet_ = new struct e131::DataPacket;
    assert(data_packet_ != nullptr);

    // e131::DiscoveryPacket
    discovery_packet_ = new struct e131::DiscoveryPacket;
    assert(discovery_packet_ != nullptr);

    // e131::SynchronizationPacket
    synchronization_packet_ = new struct e131::SynchronizationPacket;
    assert(synchronization_packet_ != nullptr);

    handle_ = network::udp::Begin(e131::kUdpPort, nullptr);
    assert(handle_ != -1);

    DEBUG_EXIT();
}

E131Controller::~E131Controller() {
    DEBUG_ENTRY();

    network::udp::End(e131::kUdpPort);

    if (synchronization_packet_ != nullptr) {
        delete synchronization_packet_;
        synchronization_packet_ = nullptr;
    }

    if (discovery_packet_ != nullptr) {
        delete discovery_packet_;
        discovery_packet_ = nullptr;
    }

    if (data_packet_ != nullptr) {
        delete data_packet_;
        data_packet_ = nullptr;
    }

    DEBUG_EXIT();
}

void E131Controller::Start() {
    DEBUG_ENTRY();

    FillDataPacket();
    FillDiscoveryPacket();
    FillSynchronizationPacket();

    timer_handle_send_discovery_packet_ = SoftwareTimerAdd(e131::kUniverseDiscoveryIntervalSeconds * 1000U, StaticCallbackFunctionSendDiscoveryPacket);
    assert(timer_handle_send_discovery_packet_ >= 0);

    DEBUG_EXIT();
}

void E131Controller::Stop() {
    SoftwareTimerDelete(timer_handle_send_discovery_packet_);
}

void E131Controller::FillDataPacket() {
    // Root Layer (See Section 5)
    data_packet_->root_layer.pre_amble_size = __builtin_bswap16(0x0010);
    data_packet_->root_layer.post_amble_size = __builtin_bswap16(0x0000);
    memcpy(data_packet_->root_layer.acn_packet_identifier, e117::kAcnPacketIdentifier, e117::kAcnPacketIdentifierLength);
    data_packet_->root_layer.vector = __builtin_bswap32(vector::root::kData);
    memcpy(data_packet_->root_layer.cid, cid_, e117::kCidLength);

    // E1.31 Framing Layer (See Section 6)
    data_packet_->frame_layer.vector = __builtin_bswap32(vector::data::kPacket);
    memcpy(data_packet_->frame_layer.source_name, source_name_, e131::kSourceNameLength);
    data_packet_->frame_layer.priority = state_.priority;
    data_packet_->frame_layer.synchronization_address = __builtin_bswap16(state_.synchronization_packet.universe);
    data_packet_->frame_layer.options = 0;

    // Data Layer
    data_packet_->dmp_layer.vector = e131::vector::dmp::kSetProperty;
    data_packet_->dmp_layer.type = 0xa1;
    data_packet_->dmp_layer.first_address_property = __builtin_bswap16(0x0000);
    data_packet_->dmp_layer.address_increment = __builtin_bswap16(0x0001);
    data_packet_->dmp_layer.property_values[0] = 0;
}

void E131Controller::FillDiscoveryPacket() {
    memset(discovery_packet_, 0, sizeof(struct e131::DiscoveryPacket));

    // Root Layer (See Section 5)
    discovery_packet_->root_layer.pre_amble_size = __builtin_bswap16(0x10);
    memcpy(discovery_packet_->root_layer.acn_packet_identifier, e117::kAcnPacketIdentifier, e117::kAcnPacketIdentifierLength);
    discovery_packet_->root_layer.vector = __builtin_bswap32(vector::root::kExtended);
    memcpy(discovery_packet_->root_layer.cid, cid_, e117::kCidLength);

    // E1.31 Framing Layer (See Section 6)
    discovery_packet_->frame_layer.vector = __builtin_bswap32(vector::extended::kDiscovery);
    memcpy(discovery_packet_->frame_layer.source_name, source_name_, e131::kSourceNameLength);

    // Universe Discovery Layer (See Section 8)
    discovery_packet_->universe_discovery_layer.vector = __builtin_bswap32(vector::universe::kDiscoveryUniverseList);
}

void E131Controller::FillSynchronizationPacket() {
    memset(synchronization_packet_, 0, sizeof(struct e131::SynchronizationPacket));

    // Root Layer (See Section 4.2)
    synchronization_packet_->root_layer.pre_amble_size = __builtin_bswap16(0x10);
    memcpy(synchronization_packet_->root_layer.acn_packet_identifier, e117::kAcnPacketIdentifier, e117::kAcnPacketIdentifierLength);
    synchronization_packet_->root_layer.flags_length = __builtin_bswap16((0x07 << 12) | (e131::kSynchronizationRootLayerSize));
    synchronization_packet_->root_layer.vector = __builtin_bswap32(vector::root::kExtended);
    memcpy(synchronization_packet_->root_layer.cid, cid_, e117::kCidLength);

    // E1.31 Framing Layer (See Section 6)
    synchronization_packet_->frame_layer.flags_length = __builtin_bswap16((0x07 << 12) | (e131::kSynchronizationFrameLayerSize));
    synchronization_packet_->frame_layer.vector = __builtin_bswap32(vector::extended::kSynchronization);
    synchronization_packet_->frame_layer.universe_number = __builtin_bswap16(state_.synchronization_packet.universe);
}

void E131Controller::HandleDmxOut(uint16_t nUniverse, const uint8_t* pDmxData, uint32_t nLength) {
    uint32_t ip;

    // Root Layer (See Section 5)
    data_packet_->root_layer.flags_length = __builtin_bswap16(static_cast<uint16_t>((0x07 << 12) | (e131::DataRootLayerLength(1U + nLength))));

    // E1.31 Framing Layer (See Section 6)
    data_packet_->frame_layer.flags_length = __builtin_bswap16(static_cast<uint16_t>((0x07 << 12) | (e131::DataFrameLayerLength(1U + nLength))));
    data_packet_->frame_layer.sequence_number = GetSequenceNumber(nUniverse, ip);
    data_packet_->frame_layer.universe = __builtin_bswap16(nUniverse);

    // Data Layer
    data_packet_->dmp_layer.flags_length = __builtin_bswap16(static_cast<uint16_t>((0x07 << 12) | (e131::DataLayerLength(1U + nLength))));

    if (__builtin_expect((master_ == dmxnode::kDmxMaxValue), 1)) {
        memcpy(&data_packet_->dmp_layer.property_values[1], pDmxData, nLength);
    } else if (master_ == 0) {
        memset(&data_packet_->dmp_layer.property_values[1], 0, nLength);
    } else {
        for (uint32_t i = 0; i < nLength; i++) {
            data_packet_->dmp_layer.property_values[1 + i] = static_cast<uint8_t>((master_ * pDmxData[i]) / dmxnode::kDmxMaxValue);
        }
    }

    data_packet_->dmp_layer.property_value_count = __builtin_bswap16(static_cast<uint16_t>(1 + nLength));

    network::udp::Send(handle_, reinterpret_cast<const uint8_t*>(data_packet_), static_cast<uint16_t>(e131::DataPacketSize(1U + nLength)), ip, e131::kUdpPort);
}

void E131Controller::HandleSync() {
    if (state_.synchronization_packet.universe != 0) {
        synchronization_packet_->frame_layer.sequence_number = state_.synchronization_packet.sequence_number++;
        network::udp::Send(handle_, reinterpret_cast<const uint8_t*>(synchronization_packet_), e131::kSynchronizationPacketSize, state_.synchronization_packet.ip_address, e131::kUdpPort);
    }
}

void E131Controller::HandleBlackout() {
    // Root Layer (See Section 5)
    data_packet_->root_layer.flags_length = __builtin_bswap16((0x07 << 12) | (e131::DataRootLayerLength(513)));

    // E1.31 Framing Layer (See Section 6)
    data_packet_->frame_layer.flags_length = __builtin_bswap16((0x07 << 12) | (e131::DataFrameLayerLength(513)));

    // Data Layer
    data_packet_->dmp_layer.flags_length = __builtin_bswap16((0x07 << 12) | (e131::DataLayerLength(513)));
    data_packet_->dmp_layer.property_value_count = __builtin_bswap16(513);
    memset(&data_packet_->dmp_layer.property_values[1], 0, 512);

    for (uint32_t nIndex = 0; nIndex < state_.active_universes; nIndex++) {
        uint32_t ip;
        uint16_t nUniverse = s_sequence_numbers[nIndex].universe;

        data_packet_->frame_layer.sequence_number = GetSequenceNumber(nUniverse, ip);
        data_packet_->frame_layer.universe = __builtin_bswap16(nUniverse);

        network::udp::Send(handle_, reinterpret_cast<const uint8_t*>(data_packet_), e131::DataPacketSize(513), ip, e131::kUdpPort);
    }

    if (state_.synchronization_packet.universe != 0) {
        HandleSync();
    }
}

const uint8_t* E131Controller::GetSoftwareVersion() {
    return kDeviceSoftwareVersion;
}

void E131Controller::SetSourceName(const char* pSourceName) {
    assert(pSourceName != nullptr);
    strncpy(source_name_, pSourceName, e131::kSourceNameLength - 1);
    source_name_[e131::kSourceNameLength - 1] = '\0';
}

void E131Controller::SetPriority(uint8_t priority) { // TODO (a) SetPriority
    state_.priority = priority;
}

void E131Controller::SendDiscoveryPacket() {
    assert(discovery_ip_address_ != 0);

    discovery_packet_->root_layer.flags_length = __builtin_bswap16(static_cast<uint16_t>((0x07 << 12) | (e131::DiscoveryRootLayerLength(state_.active_universes))));
    discovery_packet_->frame_layer.flags_length = __builtin_bswap16(static_cast<uint16_t>((0x07 << 12) | (e131::DiscoveryFrameLayerLength(state_.active_universes))));
    discovery_packet_->universe_discovery_layer.flags_length = __builtin_bswap16(static_cast<uint16_t>((0x07 << 12) | e131::DiscoveryLayerLength(state_.active_universes)));

    for (uint32_t i = 0; i < state_.active_universes; i++) {
        discovery_packet_->universe_discovery_layer.list_of_universes[i] = __builtin_bswap16(s_sequence_numbers[i].universe);
    }

    network::udp::Send(handle_, reinterpret_cast<const uint8_t*>(discovery_packet_), static_cast<uint16_t>(e131::DiscoveryPacketSize(state_.active_universes)), discovery_ip_address_, e131::kUdpPort);

    DEBUG_PUTS("Discovery sent");
}

uint8_t E131Controller::GetSequenceNumber(uint16_t universe, uint32_t& nMulticastIpAddress) {
    assert(sizeof(struct TSequenceNumbers) == sizeof(uint64_t));

    int32_t low = 0;
    int32_t mid = 0;
    int32_t nHigh = state_.active_universes;

    while (low <= nHigh) {
        mid = low + ((nHigh - low) / 2);

        const uint32_t nMidValue = s_sequence_numbers[mid].universe;

        if (nMidValue < universe) {
            low = mid + 1;
        } else if (nMidValue > universe) {
            nHigh = mid - 1;
        } else {
            DEBUG_PRINTF("Found nUniverse=%u", universe);
            nMulticastIpAddress = s_sequence_numbers[mid].ip_address;
            s_sequence_numbers[mid].sequence_number++;
            return s_sequence_numbers[mid].sequence_number;
        }
    }

    DEBUG_PRINTF("nActiveUniverses=%u -> %u : nLow=%d, nMid=%d, nHigh=%d", state_.active_universes, universe, low, mid, nHigh);

    if ((nHigh != -1) && (state_.active_universes != static_cast<uint32_t>(nHigh))) {
        auto p64 = reinterpret_cast<uint64_t*>(s_sequence_numbers);

        for (int32_t i = state_.active_universes - 1; i >= low; i--) {
            p64[i + 1] = p64[i];
        }

        s_sequence_numbers[low].ip_address = UniverseToMulticastIp(universe);
        s_sequence_numbers[low].universe = universe;
        s_sequence_numbers[low].sequence_number = 0;

        nMulticastIpAddress = s_sequence_numbers[low].ip_address;

        DEBUG_PRINTF(">m< nUniverse=%u, nLow=%d", universe, low);
    } else {
        s_sequence_numbers[mid].ip_address = UniverseToMulticastIp(universe);
        s_sequence_numbers[mid].universe = universe;

        nMulticastIpAddress = s_sequence_numbers[mid].ip_address;

        DEBUG_PRINTF(">a< nUniverse=%u, nMid=%d", universe, mid);
    }

    state_.active_universes++;

    return 0;
}

void E131Controller::Print() {
    puts("sACN E1.31 Controller");
    printf(" Max Universes : %d\n", static_cast<int>(sizeof(s_sequence_numbers) / sizeof(s_sequence_numbers[0])));
    if (state_.synchronization_packet.universe != 0) {
        printf(" Synchronization Universe : %u\n", state_.synchronization_packet.universe);
    } else {
        puts(" Synchronization is disabled");
    }
}
