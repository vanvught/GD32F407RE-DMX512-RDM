/**
 * @file rdm_discovery_statemachine.h
 *
 */
/* Copyright (C) 2023-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef RDM_DISCOVERY_STATEMACHINE_H_
#define RDM_DISCOVERY_STATEMACHINE_H_

#include <cstdint>
#include <algorithm>

#include "rdmmessage.h"
#include "rdm_tod.h"

namespace rdm::discovery {
inline constexpr uint32_t kReceiveTimeOut = 5800;
inline constexpr uint32_t kLateResponseTimeOut = 1000;
inline constexpr uint32_t kUnmuteCounter = 3;
inline constexpr uint32_t kMuteCounter = 10;
inline constexpr uint32_t kDiscoveryStackSize = rdm::Tod::kTableSize;
inline constexpr uint32_t kDiscoveryCounter = 3;
inline constexpr uint32_t kQuikfindCounter = 5;
inline constexpr uint32_t kQuikfindDiscoveryCounter = 5;
inline constexpr uint32_t kUidStringLength = 13;

enum class State { kIdle, kUnmute, kMute, kDiscovery, kDiscoverySingleDevice, kDub, kQuickfind, kQuickfindDiscovery, kLateResponse, kFinished };

struct UidRange {
    uint64_t lower_bound;
    uint64_t upper_bound;
};

inline void FormatUid(uint64_t uid, char (&buffer)[kUidStringLength + 1]) noexcept {
    constexpr char kHex[] = "0123456789abcdef";

    for (uint32_t i = 0; i < 12; ++i) {
        const auto kShift = (11U - i) * 4U;
        const auto kDigit = static_cast<uint32_t>((uid >> kShift) & 0xFU);
        const auto kPos = i < 4U ? i : i + 1U;

        buffer[kPos] = kHex[kDigit];
    }

    buffer[4] = ':';
    buffer[kUidStringLength] = '\0';
}

inline void FormatUid(const uint8_t* uid, char (&buffer)[kUidStringLength + 1]) noexcept {
    constexpr char kHex[] = "0123456789abcdef";

    for (uint32_t i = 0; i < 6; ++i) {
        const auto kPos = 2U * i + (i >= 2U ? 1U : 0U);
        buffer[kPos] = kHex[uid[i] >> 4U];
        buffer[kPos + 1U] = kHex[uid[i] & 0x0FU];
    }

    buffer[4] = ':';
    buffer[kUidStringLength] = '\0';
}

class StateMachine {
   public:
    explicit StateMachine(const uint8_t* uid);

    ~StateMachine() = default;

    bool Full(uint32_t port_index, rdm::Tod* tod);
    bool Incremental(uint32_t port_index, rdm::Tod* tod);

    bool Stop();

    bool IsRunning() { return (state_ != rdm::discovery::State::kIdle); }

    bool IsRunning(uint32_t& port_index, bool& is_incremental) const {
        port_index = port_index_;
        is_incremental = do_incremental_;
        return (state_ != rdm::discovery::State::kIdle);
    }

    bool IsFinished(uint32_t& port_index, bool& is_incremental) {
        port_index = port_index_;
        is_incremental = do_incremental_;

        if (is_finished_) {
            is_finished_ = false;
            return true;
        }

        return false;
    }

    uint32_t CopyWorkingQueue(char* out_buffer, uint32_t out_buffer_size);

    [[nodiscard]] bool GetWorkingQueueEntry(uint32_t index, UidRange& range) const noexcept;

    void Run() {
        if (state_ == rdm::discovery::State::kIdle) [[likely]] {
            return;
        }

        Process();
    }

   private:
    void Process();
    bool Start(uint32_t port_index, rdm::Tod* tod, bool do_incremental);
    bool IsValidDiscoveryResponse(uint8_t* uid);

    void SavedState(uint32_t line);
    void NewState(rdm::discovery::State state, bool do_state_late_response, uint32_t line);

    RdmMessage message_;
    uint8_t* response_{nullptr};
    uint8_t uid_[rdm::kUidSize];
    uint32_t port_index_{0};
    rdm::Tod* tod_{nullptr};

    bool is_finished_{false};
    bool do_incremental_{false};
    rdm::discovery::State state_{rdm::discovery::State::kIdle};
    rdm::discovery::State saved_state_{rdm::discovery::State::kIdle};

    struct {
        uint32_t micros;
    } late_response_;

    struct {
        uint32_t counter;
        uint32_t micros;
        bool is_command_running;
    } unmute_;

    struct {
        uint32_t tod_entries;
        uint32_t counter;
        uint32_t micros;
        uint8_t uid[rdm::kUidSize];
        bool is_command_running;
    } mute_;

    struct {
        struct {
            bool Push(uint64_t lower_bound, uint64_t upper_bound) {
                if (top == rdm::discovery::kDiscoveryStackSize - 1) {
                    assert(0);
                    return false;
                }

                top++;
                items[top].lower_bound = lower_bound;
                items[top].upper_bound = upper_bound;

                debug_stack_top_max = std::max(debug_stack_top_max, top);
                return true;
            }

            bool Pop(uint64_t& lower_bound, uint64_t& upper_bound) {
                if (top == -1) {
                    return false;
                }

                lower_bound = items[top].lower_bound;
                upper_bound = items[top].upper_bound;
                top--;

                return true;
            }

            int32_t top;

            struct {
                uint64_t lower_bound;
                uint64_t upper_bound;
            } items[rdm::discovery::kDiscoveryStackSize];

            int32_t debug_stack_top_max;
        } stack;

        uint64_t lower_bound;
        uint64_t mid_position;
        uint64_t upper_bound;
        uint32_t counter;
        uint32_t micros;
        uint8_t uid[rdm::kUidSize];
        uint8_t pdl[2][rdm::kUidSize];
        bool is_command_running;
    } discovery_;

    struct {
        uint32_t counter;
        uint32_t micros;
        bool is_command_running;
    } discovery_single_device_;

    struct {
        uint32_t counter;
        uint32_t micros;
        bool is_command_running;
        uint8_t uid[rdm::kUidSize];
    } quick_find_;

    struct {
        uint32_t counter;
        uint32_t micros;
        bool is_command_running;
        uint8_t uid[rdm::kUidSize];
    } quick_find_discovery_;
};
} // namespace rdm::discovery
#endif // RDM_DISCOVERY_STATEMACHINE_H_
