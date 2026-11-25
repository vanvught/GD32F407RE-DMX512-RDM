/**
 * @file dmxnode.h
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

#ifndef DMXNODE_H_
#define DMXNODE_H_

#include <cstdint>
#include <cstring>
#include <cassert>
#include <span>
#include <string_view>

#include "common/utils/utils_string.h"
#include "configurationstore.h"

namespace dmxnode {
inline constexpr uint16_t kAddressInvalid = 0xFFFF;
inline constexpr uint32_t kStartAddressDefault = 1;
inline constexpr uint32_t kUniverseSize = 512;
inline constexpr uint8_t kDmxMaxValue = 255;
// Art-Net
inline constexpr uint32_t kNodeNameLength = 64; // Long Name
static_assert(common::store::dmxnode::kNodeNameLength == kNodeNameLength);
inline constexpr uint32_t kPortNameLength = 18; // Port Name
static_assert(common::store::dmxnode::kPortNameLength == kPortNameLength);
// sACN E1.31
namespace priority {
inline constexpr uint8_t kLowest = 1;
inline constexpr uint8_t kDefault = 100;
inline constexpr uint8_t kHighest = 200;
} // namespace priority

inline constexpr uint32_t kMaxPorts =
#if !defined(DMXNODE_PORTS) || (DMXNODE_PORTS == 0)
    1; // ISO C++ forbids zero-size array
#else
    DMXNODE_PORTS; // From build config
#endif // !defined(DMXNODE_PORTS) || (DMXNODE_PORTS == 0)

inline constexpr uint32_t kDmxportOffset =
#ifndef CONFIG_DMXNODE_DMX_PORT_OFFSET
    0; // Default if not overridden
#else
    CONFIG_DMXNODE_DMX_PORT_OFFSET; // From build config
#endif // CONFIG_DMXNODE_DMX_PORT_OFFSET

static_assert(kDmxportOffset <= kMaxPorts);
inline constexpr auto kConfigPortCount = std::min<uint32_t>(kMaxPorts - kDmxportOffset, common::store::dmxnode::kParamPorts);

enum class Personality : uint8_t { kArtnet, kSacn, kNode };

namespace personality {
inline constexpr std::string_view kNode{"node"};
inline constexpr std::string_view kSacn{"sacn"};
inline constexpr std::string_view kArtnet{"artnet"};
} // namespace personality

enum class MergeMode : uint8_t { kHtp, kLtp };

namespace mergemode {
inline constexpr std::string_view kHtp{"htp"};
inline constexpr std::string_view kLtp{"ltp"};
inline constexpr std::string_view kHtpUpper{"HTP"};
inline constexpr std::string_view kLtpUpper{"LTP"};
} // namespace mergemode

enum class Direction : uint8_t { kInput, kOutput, kDisable };

namespace direction {
inline constexpr std::string_view kInput{"input"};
inline constexpr std::string_view kOutput{"output"};
inline constexpr std::string_view kDisable{"disable"};
} // namespace direction

enum class FailSafe : uint8_t { kHold, kOff, kOn, kPlayback, kRecord };

namespace failsafe {
inline constexpr std::string_view kHold{"hold"};
inline constexpr std::string_view kOff{"off"};
inline constexpr std::string_view kOn{"on"};
inline constexpr std::string_view kPlayback{"playback"};
inline constexpr std::string_view kRecord{"record"};
} // namespace failsafe

enum class OutputStyle : uint8_t {
    kDelta,    ///< DMX frame is triggered
    kConstant, ///< DMX output is continuous
};

namespace outputstyle {
inline constexpr std::string_view kDelta{"delta"};
inline constexpr std::string_view kConstant{"const"};
inline constexpr std::string_view kDeltaUpper{"DELTA"};
inline constexpr std::string_view kConstantUpper{"CONST"};
} // namespace outputstyle

enum class Rdm : uint8_t { kDisable, kEnable };

[[nodiscard]] constexpr std::string_view GetPersonality(Personality value) {
    switch (value) {
        case Personality::kNode:
            return personality::kNode;
        case Personality::kSacn:
            return personality::kSacn;
        case Personality::kArtnet:
            return personality::kArtnet;
    }

    return personality::kArtnet;
}

[[nodiscard]] constexpr Personality GetPersonality(std::string_view value) {
    if (value == personality::kNode) {
        return Personality::kNode;
    }

    if (value == personality::kSacn) {
        return Personality::kSacn;
    }

    return Personality::kArtnet;
}

static_assert(GetPersonality("node") == Personality::kNode);
static_assert(GetPersonality("sacn") == Personality::kSacn);
static_assert(GetPersonality("artnet") == Personality::kArtnet);

static_assert(GetPersonality("node123") == Personality::kArtnet);
static_assert(GetPersonality("") == Personality::kArtnet);

static_assert(GetPersonality(Personality::kNode) == "node");
static_assert(GetPersonality(Personality::kSacn) == "sacn");

[[nodiscard]] constexpr MergeMode GetMergeMode(std::string_view value) {
    return value == mergemode::kLtp ? MergeMode::kLtp : MergeMode::kHtp;
}

[[nodiscard]] constexpr std::string_view GetMergeMode(MergeMode value, bool to_upper = false) {
    if (to_upper) {
        return value == MergeMode::kLtp ? mergemode::kLtpUpper : mergemode::kHtpUpper;
    }

    return value == MergeMode::kLtp ? mergemode::kLtp : mergemode::kHtp;
}

[[nodiscard]] constexpr Direction PortDirection(std::string_view value) {
    if (value == direction::kInput) {
        return Direction::kInput;
    }

    if (value == direction::kDisable) {
        return Direction::kDisable;
    }

    return Direction::kOutput;
}

[[nodiscard]] constexpr std::string_view PortDirection(Direction value) {
    switch (value) {
        case Direction::kInput:
            return direction::kInput;
        case Direction::kOutput:
            return direction::kOutput;
        case Direction::kDisable:
            return direction::kDisable;
    }

    return direction::kOutput;
}

static_assert(PortDirection("input") == Direction::kInput);
static_assert(PortDirection("output") == Direction::kOutput);
static_assert(PortDirection("disable") == Direction::kDisable);

static_assert(PortDirection(Direction::kInput) == "input");
static_assert(PortDirection(Direction::kOutput) == "output");
static_assert(PortDirection(Direction::kDisable) == "disable");

// Verify fallback behaviour.
static_assert(PortDirection("") == Direction::kOutput);
static_assert(PortDirection("invalid") == Direction::kOutput);

// Verify exact, case-sensitive matching.
static_assert(PortDirection("INPUT") == Direction::kOutput);
static_assert(PortDirection("input123") == Direction::kOutput);

[[nodiscard]] constexpr FailSafe GetFailsafe(std::string_view value) {
    if (value == failsafe::kOff) {
        return FailSafe::kOff;
    }

    if (value == failsafe::kOn) {
        return FailSafe::kOn;
    }

    if (value == failsafe::kPlayback) {
        return FailSafe::kPlayback;
    }

    if (value == failsafe::kRecord) {
        return FailSafe::kRecord;
    }

    return FailSafe::kHold;
}

[[nodiscard]] constexpr std::string_view GetFailsafe(FailSafe value) {
    switch (value) {
        case FailSafe::kHold:
            return failsafe::kHold;
        case FailSafe::kOff:
            return failsafe::kOff;
        case FailSafe::kOn:
            return failsafe::kOn;
        case FailSafe::kPlayback:
            return failsafe::kPlayback;
        case FailSafe::kRecord:
            return failsafe::kRecord;
    }

    return failsafe::kHold;
}

[[nodiscard]] constexpr OutputStyle GetOutputStyle(std::string_view value) {
    return value == outputstyle::kConstant ? OutputStyle::kConstant : OutputStyle::kDelta;
}

[[nodiscard]] constexpr std::string_view GetOutputStyle(OutputStyle value, bool to_upper = false) {
    if (to_upper) {
        return value == OutputStyle::kDelta ? outputstyle::kDeltaUpper : outputstyle::kConstantUpper;
    }

    return value == OutputStyle::kDelta ? outputstyle::kDelta : outputstyle::kConstant;
}

namespace scenes {
inline constexpr auto kBytesNeeded = dmxnode::kMaxPorts * dmxnode::kUniverseSize;

void WriteStart();
void Write(uint32_t port_index, std::span<const uint8_t> data);
void WriteEnd();

void ReadStart();
void Read(uint32_t port_index, std::span<uint8_t> data);
void ReadEnd();
} // namespace scenes
} // namespace dmxnode

class DmxNode {
   public:
    static DmxNode& Instance() {
        static DmxNode instance;
        return instance;
    }

    void SetShortName(uint32_t port_index, std::string_view name = {}) {
        assert(port_index < dmxnode::kMaxPorts);

        if (name.empty()) {
            SetShortNameDefault(port_index);
            return;
        }

        common::CopyString(port_[port_index].label, name);
    }

    void SetShortNameDefault(uint32_t port_index) {
        assert(port_index < dmxnode::kMaxPorts);

        auto& port = port_[port_index];

        constexpr char kPrefix[] = "Port ";
        constexpr auto kPrefixLength = sizeof(kPrefix) - 1;

        static_assert(dmxnode::kPortNameLength >= kPrefixLength + 11);

        memcpy(port.label, kPrefix, kPrefixLength);

        auto* p = common::AppendUnsigned(port.label + kPrefixLength, port_index + 1U);
        *p = '\0';
    }

    [[nodiscard]] const char* GetPortName(uint32_t port_index) const {
        assert(port_index < dmxnode::kMaxPorts);
        const auto& port = port_[port_index];

        return port.label;
    }

    void SceneStore();
    void ScenePlayback();

   private:
    DmxNode() {
        for (uint32_t i = 0; i < dmxnode::kMaxPorts; i++) {
            SetShortNameDefault(i);
        }
    }

    struct {
        dmxnode::Direction port_direction{dmxnode::Direction::kDisable};
        bool is_transmitting{false};
        char label[dmxnode::kPortNameLength];
    } port_[dmxnode::kMaxPorts];
};

#endif // DMXNODE_H_
