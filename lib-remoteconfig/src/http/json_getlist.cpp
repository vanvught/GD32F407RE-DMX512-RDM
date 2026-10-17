/**
 * @file json_getlist.cpp
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
#include <cstring>
#include <cstdio>

#include "common/utils/utils_string.h"
#include "configstore.h"
#include "configurationstore.h"
#include "dmxnode_nodetype.h"
#include "dmxnode_outputtype.h"
#include "ip4/ip4_address.h"
#include "json/json_helpers.h"
#include "network_config.h"

namespace json {
uint32_t GetList(char* out_buffer, uint32_t out_buffer_size) {
    assert(out_buffer != nullptr);
    assert(out_buffer_size != 0);

    uint8_t display_name[common::store::remoteconfig::kDisplayNameLength];

    ConfigStore::Instance().RemoteConfigCopyArray(display_name, &common::store::RemoteConfig::display_name);

    display_name[common::store::remoteconfig::kDisplayNameLength - 1U] = '\0';

#ifdef DMXNODE_NODETYPE_DEFINED
    if (display_name[0] == '\0') {
        const char* const kLongName = DmxNodeNodeType::Get()->GetLongName();

        if (kLongName != nullptr) {
            strncpy(reinterpret_cast<char*>(display_name), kLongName, common::store::remoteconfig::kDisplayNameLength - 1U);

            display_name[common::store::remoteconfig::kDisplayNameLength - 1U] = '\0';
        }
    }
#endif // DMXNODE_NODETYPE_DEFINED

#ifdef DMXNODE_PORTS
    constexpr uint32_t kPortCount = DMXNODE_PORTS;
#else
    constexpr uint32_t kPortCount = 0U;
#endif // DMXNODE_PORTS

    const char* node_type = dmxnode::GetNodeType(dmxnode::kNodeType);
    const char* output_type = dmxnode::GetOutputType(dmxnode::kOutputType);

    if (node_type == nullptr) {
        node_type = common::kUndefined;
    }

    if (output_type == nullptr) {
        output_type = common::kWarning;
    }

    char ip_text[16];
    const auto kIpLength = common::IpToString(network::GetPrimaryIp(), ip_text);

    if (kIpLength >= sizeof(ip_text)) {
        return 0U;
    }

    return json::helpers::Serialize(out_buffer, out_buffer_size, [&](JsonDoc& doc) {
        doc.BeginObject("list");

        doc["ip"] = ip_text;
        doc["name"] = reinterpret_cast<const char*>(display_name);

        doc.BeginObject("node");
        doc["type"] = node_type;

        doc.BeginObject("output");
        doc["type"] = output_type;
        doc["count"] = kPortCount;
        doc.EndObject();

        doc.EndObject();
        doc.EndObject();
    });
}
} // namespace json