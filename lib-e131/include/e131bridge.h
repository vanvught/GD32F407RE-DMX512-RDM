/**
 * @file e131bridge.h
 */
/* Copyright (C) 2016-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef E131BRIDGE_H_
#define E131BRIDGE_H_

#include <cstdint>

#include "dmxnode.h"
#include "dmxnode_data.h"
#include "e131.h"
#include "e131sync.h"
#include "dmxnode_outputtype.h"
#include "softwaretimers.h"
#ifdef NODE_RDMNET_LLRP_ONLY
#include "llrp/llrpdevice.h"
#endif // NODE_RDMNET_LLRP_ONLY
#include "board_statusled.h"
#include "timing.h"
#if defined(NODE_SHOWFILE) && defined(CONFIG_SHOWFILE_PROTOCOL_NODE_E131)
#include "network_config.h"
#endif // defined(NODE_SHOWFILE) && defined(CONFIG_SHOWFILE_PROTOCOL_NODE_E131)

#ifndef ALIGNED
#define ALIGNED __attribute__((aligned(4)))
#endif // ALIGNED

namespace e131bridge {
enum class Status : uint8_t {
    kOff,     //
    kStandby, //
    kOn,      //
};

enum StateFlags : uint8_t {
    kNetworkDataLoss = (1 << 0),
    kMergeMode = (1 << 1),
    kSynchronized = (1 << 2),
    kForcedSynchronized = (1 << 3),
    kChanged = (1 << 4),
    kDisableMergeTimeout = (1 << 5),
    kDisableSynchronize = (1 << 6), //
};

struct State {
    uint8_t enabled_input_ports;
    uint8_t enabled_output_ports;
    uint8_t priority;
    uint8_t receiving_dmx;
    dmxnode::FailSafe failsafe;
    e131bridge::Status status;
    uint16_t discovery_packet_length;
    uint16_t synchronization_address_source_a;
    uint16_t synchronization_address_source_b;
    uint32_t synchronization_time;
    bool is_network_data_loss;
    bool is_merge_mode;
    bool is_synchronized;
    bool is_forced_synchronized;
    bool is_changed;
    bool disable_merge_timeout;
    bool disable_synchronize;
};

struct Bridge {
    struct Port {
        uint16_t universe;
        dmxnode::Direction direction;
        bool local_merge;
    } port[dmxnode::kMaxPorts] ALIGNED;
};

struct Source {
    uint32_t millis;
    uint32_t ip;
    uint8_t cid[e117::kCidLength];
    uint8_t sequence_number_data;
};

struct OutputPort {
    Source source_a ALIGNED;
    Source source_b ALIGNED;
    dmxnode::MergeMode merge_mode;
    dmxnode::OutputStyle output_style;
    bool is_merging;
    bool is_transmitting;
    bool is_data_pending;
};

struct InputPort {
    uint32_t multicast_ip;
    uint32_t millis;
    uint8_t sequence_number;
    uint8_t priority;
    bool is_disabled;
};
} // namespace e131bridge

class E131Bridge
#ifdef NODE_RDMNET_LLRP_ONLY
    : LLRPDevice
#endif // NODE_RDMNET_LLRP_ONLY
{
   public:
    E131Bridge();
    E131Bridge(const E131Bridge&) = delete;
    E131Bridge(E131Bridge&&) = delete;
    E131Bridge& operator=(const E131Bridge&) = delete;
    E131Bridge& operator=(E131Bridge&&) = delete;
    ~E131Bridge() = default;

    void SetOutput(DmxNodeOutputType* dmx_node_output_type);
    [[nodiscard]] DmxNodeOutputType* GetOutput() const;

    void SetLongName(const char* name);
    const char* GetLongName();
    void GetLongNameDefault(char* name);

    void SetShortName(uint32_t port_index, const char* name);
    [[nodiscard]] const char* GetShortName(uint32_t port_index) const;

    void SetDisableMergeTimeout(bool disable) { state_.disable_merge_timeout = disable; }
    [[nodiscard]] bool GetDisableMergeTimeout() const;

    void SetFailSafe(dmxnode::FailSafe failsafe) { state_.failsafe = failsafe; }
    [[nodiscard]] dmxnode::FailSafe GetFailSafe() const;

    void SetUniverse(uint32_t port_index, uint16_t universe);
    [[nodiscard]] uint16_t GetUniverse(uint32_t port_index) const;

    void SetDirection(uint32_t port_index, dmxnode::Direction port_direction);
    [[nodiscard]] dmxnode::Direction GetDirection(uint32_t port_index) const;

    void SetUniverse(uint32_t port_index, dmxnode::Direction port_direction, uint16_t universe);
    bool GetUniverse(uint32_t port_index, uint16_t& universe, dmxnode::Direction port_direction) const;

    void SetMergeMode(uint32_t port_index, dmxnode::MergeMode merge_mode);
    [[nodiscard]] dmxnode::MergeMode GetMergeMode(uint32_t port_index) const;

    [[nodiscard]] dmxnode::Direction PortDirection(uint32_t port_index) const;

#ifdef OUTPUT_HAVE_STYLESWITCH
    void SetOutputStyle(uint32_t port_index, dmxnode::OutputStyle output_style);
    [[nodiscard]] dmxnode::OutputStyle GetOutputStyle(uint32_t port_index) const;
#endif // OUTPUT_HAVE_STYLESWITCH

    void SetPriority(uint32_t port_index, uint8_t priority);
    [[nodiscard]] uint8_t GetPriority(uint32_t port_index) const;

    bool GetOutputPort(uint16_t universe, uint32_t& port_index);

    [[nodiscard]] uint32_t GetActiveOutputPorts() const;
    [[nodiscard]] uint32_t GetActiveInputPorts() const;

    [[nodiscard]] bool IsTransmitting(uint32_t port_index) const;
    [[nodiscard]] bool IsMerging(uint32_t port_index) const;
    bool IsStatusChanged();

    void SetEnableDataIndicator(bool enable);
    [[nodiscard]] bool GetEnableDataIndicator() const;

    void SetDisableSynchronize(bool disable_synchronize);
    [[nodiscard]] bool GetDisableSynchronize() const;

    void SetE131Sync(E131SyncCallbackFunctionPtr e131_sync) { sync_callback_function_pointer_ = e131_sync; }

    void SetInputDisabled(uint32_t port_index, bool disable);
    [[nodiscard]] bool GetInputDisabled(uint32_t port_index) const;

    void Clear(uint32_t port_index);

#if defined(E131_HAVE_DMXIN) || defined(NODE_SHOWFILE)
    void SetSourceName(const char* source_name);
    [[nodiscard]] const char* GetSourceName() const;
    [[nodiscard]] const uint8_t* GetCid() const;
#endif // defined(E131_HAVE_DMXIN) || defined(NODE_SHOWFILE)

#if defined(NODE_SHOWFILE) && defined(CONFIG_SHOWFILE_PROTOCOL_NODE_E131)
    void HandleShowFile(const e131::DataPacket* pE131DataPacket);
#endif // defined(NODE_SHOWFILE) && defined(CONFIG_SHOWFILE_PROTOCOL_NODE_E131)

    void Print();

    void Start();
    void Stop();
    void Run();

    void static StaticCallbackFunctionUdp(const uint8_t* buffer, uint32_t size, uint32_t from_ip, uint16_t from_port) { s_this->InputUdp(buffer, size, from_ip, from_port); }

    static E131Bridge* Get() { return s_this; }

   private:
    void InputUdp(const uint8_t* buffer, uint32_t size, uint32_t from_ip, uint16_t from_port);

    void SetNetworkDataLossCondition(bool source_a = true, bool source_b = true);

    void SetSynchronizationAddress(bool source_a, bool source_b, uint16_t synchronization_address);

    void CheckMergeTimeouts(uint32_t port_index);
    [[nodiscard]] bool IsPriorityTimeOut(uint32_t port_index) const;
    bool IsIpCidMatch(const e131bridge::Source* source) const;
    void UpdateMergeStatus(uint32_t port_index);

    void HandleDmx();
    void HandleSynchronization();

    enum class JoinLeave { kJoin, kLeave };

    void JoinUniverse(uint32_t port_index, uint16_t universe);
    void LeaveUniverse(uint32_t port_index, uint16_t universe);

    void HandleDmxIn();
    void SetLocalMerging();
    void FillDataPacket();
    void FillDiscoveryPacket();
    void SendDiscoveryPacket();

    void static StaticCallbackFunctionSendDiscoveryPacket([[maybe_unused]] TimerHandle_t timer_handle) { s_this->SendDiscoveryPacket(); }

    int32_t handle_{-1};
    uint8_t* receive_buffer_{nullptr};
    uint32_t packet_millis_{0};
    uint32_t current_millis_{0};
    uint32_t ip_address_from_{0};
    char node_name_[dmxnode::kNodeNameLength];

    e131bridge::State state_;
    e131bridge::Bridge bridge_;
    e131bridge::OutputPort output_port_[dmxnode::kMaxPorts];
    e131bridge::InputPort input_port_[dmxnode::kMaxPorts];

    bool enable_data_indicator_{true};

    DmxNodeOutputType* dmxnode_output_type_{nullptr};
    E131SyncCallbackFunctionPtr sync_callback_function_pointer_{nullptr};

#if defined(E131_HAVE_DMXIN) || defined(NODE_SHOWFILE)
    char source_name_[e131::kSourceNameLength];
    uint8_t cid_[e117::kCidLength];
#endif // defined(E131_HAVE_DMXIN) || defined(NODE_SHOWFILE)

#ifdef E131_HAVE_DMXIN
    e131::DataPacket e131_data_packet_;
    e131::DiscoveryPacket e131_discovery_packet_;
    uint32_t discovery_ip_address_{0};
    TimerHandle_t timer_handle_send_discovery_packet_{-1};
#endif // E131_HAVE_DMXIN

    static inline E131Bridge* s_this;
};

inline void E131Bridge::SetOutput(DmxNodeOutputType* dmx_node_output_type) {
    dmxnode_output_type_ = dmx_node_output_type;
}

inline DmxNodeOutputType* E131Bridge::GetOutput() const {
    return dmxnode_output_type_;
}

inline void E131Bridge::SetLongName(const char* long_name) {
    if (long_name == nullptr) {
        GetLongNameDefault(node_name_);
    } else {
        strncpy(node_name_, long_name, dmxnode::kNodeNameLength - 1);
    }

    node_name_[dmxnode::kNodeNameLength - 1] = '\0';
}

inline const char* E131Bridge::GetLongName() {
    return node_name_;
}

inline void E131Bridge::SetShortName(uint32_t port_index, const char* name) {
    DmxNode::Instance().SetShortName(port_index, name);
}

inline const char* E131Bridge::GetShortName(uint32_t port_index) const {
    return DmxNode::Instance().GetPortName(port_index);
}

inline bool E131Bridge::GetDisableMergeTimeout() const {
    return state_.disable_merge_timeout;
}

inline dmxnode::FailSafe E131Bridge::GetFailSafe() const {
    return state_.failsafe;
}

inline uint16_t E131Bridge::GetUniverse(uint32_t port_index) const {
    assert(port_index < dmxnode::kMaxPorts);
    return bridge_.port[port_index].universe;
}

inline bool E131Bridge::GetUniverse(uint32_t port_index, uint16_t& universe, dmxnode::Direction port_direction) const {
    assert(port_index < dmxnode::kMaxPorts);
    if (port_direction == dmxnode::Direction::kDisable) {
        return false;
    }
    universe = bridge_.port[port_index].universe;
    return bridge_.port[port_index].direction == port_direction;
}

inline dmxnode::Direction E131Bridge::GetDirection(uint32_t port_index) const {
    assert(port_index < dmxnode::kMaxPorts);
    return bridge_.port[port_index].direction;
}

inline void E131Bridge::SetUniverse(uint32_t port_index, dmxnode::Direction port_direction, uint16_t universe) {
    SetUniverse(port_index, universe);
    SetDirection(port_index, port_direction);
}

inline void E131Bridge::SetMergeMode(uint32_t port_index, dmxnode::MergeMode merge_mode) {
    assert(port_index < dmxnode::kMaxPorts);
    output_port_[port_index].merge_mode = merge_mode;
}

inline dmxnode::MergeMode E131Bridge::GetMergeMode(uint32_t port_index) const {
    assert(port_index < dmxnode::kMaxPorts);
    return output_port_[port_index].merge_mode;
}

inline dmxnode::Direction E131Bridge::PortDirection(uint32_t port_index) const {
    assert(port_index < dmxnode::kMaxPorts);
    return bridge_.port[port_index].direction;
}

inline bool E131Bridge::GetOutputPort(uint16_t universe, uint32_t& port_index) {
    for (port_index = 0; port_index < dmxnode::kMaxPorts; port_index++) {
        if (bridge_.port[port_index].direction != dmxnode::Direction::kOutput) {
            continue;
        }
        if (bridge_.port[port_index].universe == universe) {
            return true;
        }
    }
    return false;
}

#ifdef OUTPUT_HAVE_STYLESWITCH
inline void E131Bridge::SetOutputStyle(uint32_t port_index, dmxnode::OutputStyle output_style) {
    assert(port_index < dmxnode::kMaxPorts);

    if (dmxnode_output_type_ != nullptr) {
        dmxnode_output_type_->SetOutputStyle(port_index, output_style);
        output_style = dmxnode_output_type_->GetOutputStyle(port_index);
    }

    output_port_[port_index].output_style = output_style;
}

inline dmxnode::OutputStyle E131Bridge::GetOutputStyle(uint32_t port_index) const {
    assert(port_index < dmxnode::kMaxPorts);
    return output_port_[port_index].output_style;
}
#endif // OUTPUT_HAVE_STYLESWITCH

inline void E131Bridge::SetPriority(uint32_t port_index, uint8_t priority) {
    assert(port_index < dmxnode::kMaxPorts);
    if ((priority >= e131::priority::kLowest) && (priority <= e131::priority::kHighest)) {
        input_port_[port_index].priority = priority;
    }
}

inline uint8_t E131Bridge::GetPriority(uint32_t port_index) const {
    assert(port_index < dmxnode::kMaxPorts);
    return input_port_[port_index].priority;
}

inline uint32_t E131Bridge::GetActiveOutputPorts() const {
    return state_.enabled_output_ports;
}

inline uint32_t E131Bridge::GetActiveInputPorts() const {
    return state_.enabled_input_ports;
}

inline bool E131Bridge::IsTransmitting(uint32_t port_index) const {
    assert(port_index < dmxnode::kMaxPorts);
    return output_port_[port_index].is_transmitting;
}

inline bool E131Bridge::IsMerging(uint32_t port_index) const {
    assert(port_index < dmxnode::kMaxPorts);
    return output_port_[port_index].is_merging;
}

inline bool E131Bridge::IsStatusChanged() {
    if (state_.is_changed) {
        state_.is_changed = false;
        return true;
    }

    return false;
}

inline void E131Bridge::SetEnableDataIndicator(bool enable) {
    enable_data_indicator_ = enable;
}

inline bool E131Bridge::GetEnableDataIndicator() const {
    return enable_data_indicator_;
}

inline void E131Bridge::SetDisableSynchronize(bool disable_synchronize) {
    state_.disable_synchronize = disable_synchronize;
}

inline bool E131Bridge::GetDisableSynchronize() const {
    return state_.disable_synchronize;
}

inline void E131Bridge::SetInputDisabled(uint32_t port_index, bool disable) {
    assert(port_index < dmxnode::kMaxPorts);
    input_port_[port_index].is_disabled = disable;
}

inline bool E131Bridge::GetInputDisabled(uint32_t port_index) const {
    return input_port_[port_index].is_disabled;
}

inline void E131Bridge::Clear(uint32_t port_index) {
    assert(port_index < dmxnode::kMaxPorts);

    dmxnode::Data::Clear(port_index);
    dmxnode::DataOutput(dmxnode_output_type_, port_index);

    if ((bridge_.port[port_index].direction == dmxnode::Direction::kOutput) && !output_port_[port_index].is_transmitting) {
        dmxnode_output_type_->Start(port_index);
        output_port_[port_index].is_transmitting = true;
    }

    state_.is_network_data_loss = false; // Force timeout
}

#if defined(E131_HAVE_DMXIN) || defined(NODE_SHOWFILE)
inline void E131Bridge::SetSourceName(const char* source_name) {
    assert(source_name != nullptr);
    strncpy(source_name_, source_name, e131::kSourceNameLength - 1);
    source_name_[e131::kSourceNameLength - 1] = '\0';
}

inline const char* E131Bridge::GetSourceName() const {
    return source_name_;
}

inline const uint8_t* E131Bridge::GetCid() const {
    return cid_;
}
#endif // defined(E131_HAVE_DMXIN) || defined(NODE_SHOWFILE)

#if defined(NODE_SHOWFILE) && defined(CONFIG_SHOWFILE_PROTOCOL_NODE_E131)
inline void E131Bridge::HandleShowFile(const e131::DataPacket* pE131DataPacket) {
    packet_millis_ = timing::Millis();
    ip_address_from_ = network::GetPrimaryIp();
    receive_buffer_ = reinterpret_cast<uint8_t*>(const_cast<e131::DataPacket*>(pE131DataPacket));
    HandleDmx();
}

#endif // defined(NODE_SHOWFILE) && defined(CONFIG_SHOWFILE_PROTOCOL_NODE_E131)

inline void E131Bridge::Run() {
#ifdef E131_HAVE_DMXIN
    HandleDmxIn();
#endif // E131_HAVE_DMXIN

    current_millis_ = timing::Millis();
    const auto kDeltaMillis = current_millis_ - packet_millis_;

    if (state_.enabled_output_ports != 0) {
        if (kDeltaMillis >= static_cast<uint32_t>(e131::kNetworkDataLossTimeoutSeconds * 1000)) {
            if ((dmxnode_output_type_ != nullptr) && (!state_.is_network_data_loss)) {
                SetNetworkDataLossCondition();
            }
        }

        if (kDeltaMillis >= 1000U) {
            state_.receiving_dmx &= static_cast<uint8_t>(~(1U << static_cast<uint8_t>(dmxnode::Direction::kOutput)));
        }
    }

    // The board::statusled::Mode::FAST is for RDM Identify (Art-Net 4)
    if (enable_data_indicator_ && (board::statusled::GetMode() != board::statusled::Mode::kFast)) {
        if (state_.receiving_dmx != 0) {
            board::statusled::SetMode(board::statusled::Mode::kData);
        } else {
            board::statusled::SetMode(board::statusled::Mode::kNormal);
        }
    }
}

#endif // E131BRIDGE_H_
