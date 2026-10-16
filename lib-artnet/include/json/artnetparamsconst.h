/**
 * @file artnetparamsconst.h
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

#ifndef JSON_ARTNETPARAMSCONST_H_
#define JSON_ARTNETPARAMSCONST_H_

#include "json/json_key.h"
#include "dmxnode_outputtype.h"

namespace json {
struct ArtNetParamsConst {
    static constexpr char kFileName[] = "artnet.json";
    static constexpr auto kMapUniverse0 = json::MakeKey("map_universe0");

#if defined(RDM_CONTROLLER) || defined(RDM_RESPONDER)
    static constexpr auto kEnableRdm = json::MakeKey("enable_rdm");

    static constexpr auto kRdmEnablePortA = json::MakeKey("rdm_enable_port_a");
#if (DMX_MAX_PORTS > 1)
    static constexpr auto kRdmEnablePortB = json::MakeKey("rdm_enable_port_b");
#endif
#if (DMX_MAX_PORTS > 2)
    static constexpr auto kRdmEnablePortC = json::MakeKey("rdm_enable_port_c");
#endif
#if (DMX_MAX_PORTS == 4)
    static constexpr auto kRdmEnablePortD = json::MakeKey("rdm_enable_port_d");
#endif

    static constexpr json::KeyDefinition kRdmEnablePort[] = {
        kRdmEnablePortA,
#if (DMX_MAX_PORTS > 1)
        kRdmEnablePortB,
#endif
#if (DMX_MAX_PORTS > 2)
        kRdmEnablePortC,
#endif
#if (DMX_MAX_PORTS == 4)
        kRdmEnablePortD,
#endif
    };
#endif

#ifdef DMX_MAX_PORTS
    static constexpr auto kDestinationIpPortA = json::MakeKey("destination_ip_port_a");
#if (DMX_MAX_PORTS > 1)
    static constexpr auto kDestinationIpPortB = json::MakeKey("destination_ip_port_b");
#endif
#if (DMX_MAX_PORTS > 2)
    static constexpr auto kDestinationIpPortC = json::MakeKey("destination_ip_port_c");
#endif
#if (DMX_MAX_PORTS == 4)
    static constexpr auto kDestinationIpPortD = json::MakeKey("destination_ip_port_d");
#endif

    static constexpr json::KeyDefinition kDestinationIpPort[] = {
        kDestinationIpPortA,
#if (DMX_MAX_PORTS > 1)
        kDestinationIpPortB,
#endif
#if (DMX_MAX_PORTS > 2)
        kDestinationIpPortC,
#endif
#if (DMX_MAX_PORTS == 4)
        kDestinationIpPortD,
#endif
    };

    static constexpr auto kProtocolPortA = json::MakeKey("protocol_port_a");
#if (DMX_MAX_PORTS > 1)
    static constexpr auto kProtocolPortB = json::MakeKey("protocol_port_b");
#endif
#if (DMX_MAX_PORTS > 2)
    static constexpr auto kProtocolPortC = json::MakeKey("protocol_port_c");
#endif
#if (DMX_MAX_PORTS == 4)
    static constexpr auto kProtocolPortD = json::MakeKey("protocol_port_d");
#endif

    static constexpr json::KeyDefinition kProtocolPort[] = {
        kProtocolPortA,
#if (DMX_MAX_PORTS > 1)
        kProtocolPortB,
#endif
#if (DMX_MAX_PORTS > 2)
        kProtocolPortC,
#endif
#if (DMX_MAX_PORTS == 4)
        kProtocolPortD,
#endif
    };

    static constexpr auto kBgDiscoveryPortA = json::MakeKey("bg_discovery_port_a");

#if (DMX_MAX_PORTS > 1)
    static constexpr auto kBgDiscoveryPortB = json::MakeKey("bg_discovery_port_b");
#endif
#if (DMX_MAX_PORTS > 2)
    static constexpr auto kBgDiscoveryPortC = json::MakeKey("bg_discovery_port_c");
#endif
#if (DMX_MAX_PORTS == 4)
    static constexpr auto kBgDiscoveryPortD = json::MakeKey("bg_discovery_port_d");
#endif

    static constexpr json::KeyDefinition kBgDiscoveryPort[] = {
        kBgDiscoveryPortA,
#if (DMX_MAX_PORTS > 1)
        kBgDiscoveryPortB,
#endif
#if (DMX_MAX_PORTS > 2)
        kBgDiscoveryPortC,
#endif
#if (DMX_MAX_PORTS == 4)
        kBgDiscoveryPortD,
#endif
    };
#endif
};
} // namespace json

#endif // JSON_ARTNETPARAMSCONST_H_
