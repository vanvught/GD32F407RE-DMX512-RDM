/**
 * @file oscclientparams.cpp
 *
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

#include "json/oscclientparams.h"
#include "json/oscclientparamsconst.h"
#include "json/oscparamsconst.h"
#include "json/json_parser.h"
#include "json/json_parsehelper.h"
#include "ip4/ip4_helpers.h"
#include "configstore.h"
#include "configurationstore.h"
#include "common/utils/utils_flags.h"
#include "ip4/ip4_address.h"
#include "osc_debug.h"
#include "oscclient.h"

using common::store::osc::client::Flags;

namespace json {
OscClientParams::OscClientParams() {
    ConfigStore::Instance().Copy(&store_oscclient, &ConfigurationStore::osc_client);
}

void OscClientParams::SetIncomingPort(std::string_view value) {
    uint16_t port;

    if (ParseInRange<uint32_t, uint16_t>(value, 1U, UINT16_MAX, &port)) {
        store_oscclient.incoming_port = port;
    }
}

void OscClientParams::SetOutgoingPort(std::string_view value) {
    uint16_t port;

    if (ParseInRange<uint32_t, uint16_t>(value, 1U, UINT16_MAX, &port)) {
        store_oscclient.outgoing_port = port;
    }
}

void OscClientParams::SetServerIp(std::string_view value) {
    store_oscclient.server_ip = net::ParseIpString(value);
}

void OscClientParams::SetPingDisable(std::string_view value) {
    if (value.size() == 1) {
        store_oscclient.flags = common::SetFlagValue(store_oscclient.flags, Flags::Flag::kPingDisable, value.front() != '0');
    }
}

void OscClientParams::SetPingDelay(std::string_view value) {
    uint8_t ping_delay;

    if (ParseInRange<uint16_t, uint8_t>(value, 2U, 60U, &ping_delay)) {
        store_oscclient.ping_delay = ping_delay;
    }
}

void OscClientParams::SetCmd(std::string_view key, std::string_view value) {
    const auto kSize = value.size();

    if (kSize > common::store::osc::client::kCmdPathLength - 1) {
        return;
    }

    const auto kSuffix = key.back();
    const auto kIndex = static_cast<uint8_t>(kSuffix - '0');
    auto* dst = store_oscclient.cmd[kIndex];

    if (kSize >= 1) {
        if (value.front() != '/') {
            dst[0] = '\0';
            return;
        }
    }

    memcpy(dst, value.data(), kSize);
    dst[kSize] = '\0';
}

void OscClientParams::SetLed(std::string_view key, std::string_view value) {
    const auto kSize = value.size();

    if (kSize > common::store::osc::client::kLedPathLength - 1) {
        return;
    }

    const auto kSuffix = key.back();
    const auto kIndex = static_cast<uint8_t>(kSuffix - '0');
    auto* dst = store_oscclient.led[kIndex];

    if (kSize >= 1) {
        if (value.front() != '/') {
            dst[0] = '\0';
            return;
        }
    }

    memcpy(dst, value.data(), kSize);
    dst[kSize] = '\0';
}

void OscClientParams::Store(const char* buffer, uint32_t buffer_size) {
    ParseJsonWithTable(buffer, buffer_size, kOscClientKeys);
    ConfigStore::Instance().Store(&store_oscclient, &ConfigurationStore::osc_client);

    Dump();
}

void OscClientParams::Set() {
    auto& osc_client = OscClient::Instance();

    osc_client.SetServerIP(store_oscclient.server_ip);
    osc_client.SetPortOutgoing(store_oscclient.outgoing_port);
    osc_client.SetPortIncoming(store_oscclient.incoming_port);
    osc_client.SetPingDisable(common::IsFlagSet(store_oscclient.flags, Flags::Flag::kPingDisable));
    osc_client.SetPingDelaySeconds(store_oscclient.ping_delay);
    osc_client.CopyCmds(reinterpret_cast<const char*>(&store_oscclient.cmd));
    osc_client.CopyLeds(reinterpret_cast<const char*>(&store_oscclient.led));

    Dump();
}

void OscClientParams::Dump() {
    if constexpr (kDebugOscClientEnabled) {
        printf("%s::%s \'%s\':\n", __FILE__, __FUNCTION__, json::OscClientParamsConst::kFileName);
        printf(" %s=" IPSTR "\n", OscClientParamsConst::kServerIp.name, IP2STR(store_oscclient.server_ip));
        printf(" %s=%u\n", OscParamsConst::kOutgoingPort.name, static_cast<unsigned>(store_oscclient.outgoing_port));
        printf(" %s=%u\n", OscParamsConst::kIncomingPort.name, static_cast<unsigned>(store_oscclient.incoming_port));
        printf(" %s=%u\n", OscClientParamsConst::kPingDisable.name, static_cast<unsigned>(common::IsFlagSet(store_oscclient.flags, Flags::Flag::kPingDisable)));
        printf(" %s=%u\n", OscClientParamsConst::kPingDelay.name, static_cast<unsigned>(store_oscclient.ping_delay));

        for (auto& cmd : store_oscclient.cmd) {
            printf(" %s=[%s]\n", OscClientParamsConst::kCmd[0].name, reinterpret_cast<char*>(&cmd));
        }

        for (auto& led : store_oscclient.led) {
            printf(" %s=[%s]\n", OscClientParamsConst::kLed[0].name, reinterpret_cast<char*>(&led));
        }
    }
}
} // namespace json