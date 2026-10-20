/**
 * @file oscserverparams.cpp
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

#include "json/oscserverparams.h"
#include "json/oscserverparamsconst.h"
#include "json/json_parser.h"
#include "json/json_parsehelper.h"
#include "configstore.h"
#include "configurationstore.h"
#include "common/utils/utils_flags.h"
#include "osc_debug.h"

using common::store::osc::server::Flags;

namespace json {
OscServerParams::OscServerParams() {
    ConfigStore::Instance().Copy(&store_oscserver, &ConfigurationStore::osc_server);
}

void OscServerParams::SetIncomingPort(std::string_view value) {
    uint16_t port;

    if (ParseInRange<uint32_t, uint16_t>(value, 1U, UINT16_MAX, &port)) {
        store_oscserver.incoming_port = port;
    }
}

void OscServerParams::SetOutgoingPort(std::string_view value) {
    uint16_t port;

    if (ParseInRange<uint32_t, uint16_t>(value, 1U, UINT16_MAX, &port)) {
        store_oscserver.outgoing_port = port;
    }
}

void OscServerParams::SetPath(std::string_view value) {
    const auto kSize = value.size();

    if (kSize > common::store::osc::server::kPathLength - 1) {
        return;
    }

    auto* dst = store_oscserver.path;

    if (kSize >= 1) {
        if (value.front() != '/') {
            dst[0] = '\0';
            return;
        }
    }

    memcpy(dst, value.data(), kSize);
    dst[kSize] = '\0';
}

void OscServerParams::SetPathInfo(std::string_view value) {
    const auto kSize = value.size();

    if (kSize > common::store::osc::server::kPathLength - 1) {
        return;
    }

    auto* dst = store_oscserver.path_info;

    if (kSize >= 1) {
        if (value.front() != '/') {
            dst[0] = '\0';
            return;
        }
    }

    memcpy(dst, value.data(), kSize);
    dst[kSize] = '\0';
}

void OscServerParams::SetPathBlackout(std::string_view value) {
    const auto kSize = value.size();

    if (kSize > common::store::osc::server::kPathLength - 1) {
        return;
    }

    auto* dst = store_oscserver.path_blackout;

    if (kSize >= 1) {
        if (value.front() != '/') {
            dst[0] = '\0';
            return;
        }
    }

    memcpy(dst, value.data(), kSize);
    dst[kSize] = '\0';
}

void OscServerParams::SetTransmission(std::string_view value) {
    if (value.size() != 1) {
        return;
    }

    store_oscserver.flags = common::SetFlagValue(store_oscserver.flags, Flags::Flag::kPartialTransmission, value.front() == 't');
}

void OscServerParams::Store(std::string_view buffer) {
    ParseJsonWithTable(buffer,kOscServerKeys);
    ConfigStore::Instance().Store(&store_oscserver, &ConfigurationStore::osc_server);

    Dump();
}

void OscServerParams::Set() {
    Dump();
}

void OscServerParams::Dump() {
    if constexpr (kDebugOscServerEnabled) {
        printf("%s::%s \'%s\':\n", __FILE__, __FUNCTION__, json::OscServerParamsConst::kFileName);
        printf(" %s=%u\n", OscParamsConst::kIncomingPort.name, static_cast<unsigned>(store_oscserver.incoming_port));
        printf(" %s=%u\n", OscParamsConst::kOutgoingPort.name, static_cast<unsigned>(store_oscserver.outgoing_port));
        printf(" %s=%s\n", OscServerParamsConst::kPath.name, store_oscserver.path);
        printf(" %s=%s\n", OscServerParamsConst::kPathInfo.name, store_oscserver.path_info);
        printf(" %s=%s\n", OscServerParamsConst::kPathBlackout.name, store_oscserver.path_blackout);
        printf(" %s=%u\n", OscServerParamsConst::kTransmission.name, static_cast<unsigned>(common::IsFlagSet(store_oscserver.flags, Flags::Flag::kPartialTransmission)));
    }
}
} // namespace json