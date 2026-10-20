/**
 * @file dmxnodeparams.cpp
 */
/* Copyright (C) 2025-2026 by Arjan van Vught mailto:infogd32-dmx.org
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
#include <utility>

#include "json/dmxnodeparams.h"
#include "json/dmxnodeparamsconst.h"
#include "common/utils/utils_bitfield.h"
#include "json/json_parser.h"
#include "json/json_parsehelper.h"
#include "dmxnode.h"
#include "dmxnode_nodetype.h"
#include "dmxnode_debug.h"
#include "configstore.h"
#include "configurationstore.h"
#include "common/utils/utils_flags.h"
#include "dmxnode_outputtype.h"
#ifdef DMXNODE_OUTPUT_DMX
#include "dmx.h"
#endif // DMXNODE_OUTPUT_DMX
#include "common/utils/utils_string.h"

using common::store::dmxnode::Flags;

namespace json {
DmxNodeParams::DmxNodeParams() {
    ConfigStore::Instance().Copy(&store_dmxnode, &ConfigurationStore::dmx_node);
}

void DmxNodeParams::SetNodeName(std::string_view value) {
    common::CopyString(store_dmxnode.node_name, value);
}

void DmxNodeParams::SetFailsafe(std::string_view value) {
    store_dmxnode.fail_safe = std::to_underlying(dmxnode::GetFailsafe(value.data()));
}

void DmxNodeParams::SetDisableMergeTimeout(std::string_view value) {
    if (value.empty()) {
        return;
    }

    store_dmxnode.flags = common::SetFlagValue(store_dmxnode.flags, Flags::Flag::kDisableMergeTimeout, value.front() == 't');
}

void DmxNodeParams::SetLabelPort(std::string_view key, std::string_view value) {
    const auto kSuffix = key.back();
    const auto kIndex = static_cast<uint8_t>(kSuffix - 'a');

    common::CopyString(store_dmxnode.port_name[kIndex], value);
}

void DmxNodeParams::SetUniversePort(std::string_view key, std::string_view value) {
    const auto kSuffix = key.back();
    const auto kIndex = static_cast<uint8_t>(kSuffix - 'a');
    store_dmxnode.universe[kIndex] = ParseValue<uint16_t>(value);
}

void DmxNodeParams::SetDirectionPort(std::string_view key, std::string_view value) {
    const auto kSuffix = key.back();
    const auto kIndex = static_cast<uint8_t>(kSuffix - 'a');

    auto direction = store_dmxnode.direction;
    common::Set2BitField<dmxnode::Direction>(kIndex, dmxnode::PortDirection(value.data()), direction);
    store_dmxnode.direction = direction;
}

void DmxNodeParams::SetMergeModePort(std::string_view key, std::string_view value) {
    const auto kSuffix = key.back();
    const auto kIndex = static_cast<uint8_t>(kSuffix - 'a');

    auto merge_mode = store_dmxnode.merge_mode;
    common::Set2BitField<dmxnode::MergeMode>(kIndex, dmxnode::GetMergeMode(value.data()), merge_mode);
    store_dmxnode.merge_mode = merge_mode;
}

void DmxNodeParams::SetOutputStylePort(std::string_view key, std::string_view value) {
    const auto kSuffix = key.back();
    const auto kIndex = static_cast<uint8_t>(kSuffix - 'a');
    const auto kOutputStyle = dmxnode::GetOutputStyle(value.data());

    if (kOutputStyle != dmxnode::OutputStyle::kDelta) {
        store_dmxnode.output_style |= static_cast<uint8_t>(1U << kIndex);
    } else {
        store_dmxnode.output_style &= static_cast<uint8_t>(~(1U << kIndex));
    }
}

void DmxNodeParams::Store(std::string_view buffer) {
    ParseJsonWithTable(buffer,kDmxNodeKeys);
    ConfigStore::Instance().Store(&store_dmxnode, &ConfigurationStore::dmx_node);
}

void DmxNodeParams::Set() {
    DMXNODE_DEBUG_ENTRY();

    auto* dmx_node = DmxNodeNodeType::Get();
    assert(dmx_node != nullptr);

    dmx_node->SetLongName(reinterpret_cast<char*>(store_dmxnode.node_name));
    dmx_node->SetFailSafe(static_cast<dmxnode::FailSafe>(store_dmxnode.fail_safe));
    dmx_node->SetDisableMergeTimeout(common::IsFlagSet(store_dmxnode.flags, Flags::Flag::kDisableMergeTimeout));

    if constexpr (dmxnode::kConfigPortCount != 0) {
        for (uint32_t config_port_index = 0; config_port_index < dmxnode::kConfigPortCount; config_port_index++) {
            const auto kPortIndex = config_port_index + dmxnode::kDmxportOffset;

            if (kPortIndex >= dmxnode::kMaxPorts) {
                break;
            }

            dmx_node->SetShortName(kPortIndex, reinterpret_cast<char*>(store_dmxnode.port_name[config_port_index]));
            dmx_node->SetUniverse(kPortIndex, store_dmxnode.universe[config_port_index]);
            const auto kPortDirection = common::Get2BitField<dmxnode::Direction>(config_port_index, store_dmxnode.direction);
            dmx_node->SetDirection(kPortIndex, kPortDirection);
            const auto kPortMergeMode = common::Get2BitField<dmxnode::MergeMode>(config_port_index, store_dmxnode.merge_mode);
            dmx_node->SetMergeMode(kPortIndex, kPortMergeMode);
#ifdef OUTPUT_HAVE_STYLESWITCH
            const auto kOutputStyle = GetOutputStyleSet(1U << config_port_index);
            dmx_node->SetOutputStyle(kPortIndex, kOutputStyle);
#endif // OUTPUT_HAVE_STYLESWITCH
        }
    }

#ifdef DEBUG_DMXNODE
    Dump();
#endif // DEBUG_DMXNODE
    DMXNODE_DEBUG_EXIT();
}

void DmxNodeParams::Dump() {
    printf("%s::%s \'%s\':\n", __FILE__, __FUNCTION__, json::DmxNodeParamsConst::kFileName);
    printf(" %s=%s\n", json::DmxNodeParamsConst::kNodeName.name, store_dmxnode.node_name);
    printf(" %s=%s [%u]\n", json::DmxNodeParamsConst::kFailsafe.name, dmxnode::GetFailsafe(static_cast<dmxnode::FailSafe>(store_dmxnode.fail_safe)), store_dmxnode.fail_safe);
    printf(" %s=%u\n", json::DmxNodeParamsConst::kDisableMergeTimeout.name, static_cast<unsigned>(common::IsFlagSet(store_dmxnode.flags, Flags::Flag::kDisableMergeTimeout)));

    if constexpr (dmxnode::kConfigPortCount != 0) {
#ifdef DMX_MAX_PORTS
        for (uint32_t port_index = 0; port_index < dmxnode::kConfigPortCount; port_index++) {
            printf(" %s=%s\n", json::DmxNodeParamsConst::kLabelPort[port_index].name, reinterpret_cast<char*>(store_dmxnode.port_name[port_index]));
            printf(" %s=%u\n", json::DmxNodeParamsConst::kUniversePort[port_index].name, store_dmxnode.universe[port_index]);
            const auto kPortDirection = common::Get2BitField<dmxnode::Direction>(port_index, store_dmxnode.direction);
            printf(" %s=%s\n", json::DmxNodeParamsConst::kDirectionPort[port_index].name, dmxnode::PortDirection(kPortDirection));
            const auto kPortMergeMode = common::Get2BitField<dmxnode::MergeMode>(port_index, store_dmxnode.merge_mode);
            printf(" %s=%s\n", json::DmxNodeParamsConst::kMergeModePort[port_index].name, dmxnode::GetMergeMode(kPortMergeMode));
            const auto kOutputStyle = GetOutputStyleSet(1U << port_index);
            printf(" %s=%s\n", DmxNodeParamsConst::kOutputStylePort[port_index].name, dmxnode::GetOutputStyle(kOutputStyle));
        }
#endif // DMX_MAX_PORTS
    }

    auto& dmx_node = *DmxNodeNodeType::Get();
    dmx_node.Print();
}
} // namespace json