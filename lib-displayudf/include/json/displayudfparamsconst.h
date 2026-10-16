/**
 * @file displayudfparamsconst.h
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

#ifndef JSON_DISPLAYUDFPARAMSCONST_H_
#define JSON_DISPLAYUDFPARAMSCONST_H_

#include "json/json_key.h"
#include "dmxnode_outputtype.h" // IWYU pragma: keep
#include "dmxnode_nodetype.h"
#ifdef DMXNODE_TYPE_ARTNET
#include "json/artnetparamsconst.h"
#endif
#if defined(DMXNODE_PORTS) && (DMXNODE_PORTS > 0)
#include "json/dmxnodeparamsconst.h"
#endif

namespace json {
struct DisplayUdfParamsConst {
    static constexpr char kFileName[] = "display.json";

    static constexpr auto kIntensity = json::MakeKey("intensity");
    static constexpr auto kSleepTimeout = json::MakeKey("sleep_timeout");
    static constexpr auto kFlipVertically = json::MakeKey("flip_vertically");
    static constexpr auto kTitle = json::MakeKey("title");
    static constexpr auto kBoardName = json::MakeKey("board_name");
    static constexpr auto kVersion = json::MakeKey("version");
    static constexpr auto kActivePorts = json::MakeKey("active_ports");
    static constexpr auto kHostname = json::MakeKey("hostname");
    static constexpr auto kIpAddress = json::MakeKey("ip_address");
    static constexpr auto kNetMask = json::MakeKey("net_mask");
    static constexpr auto kDefaultGateway = json::MakeKey("default_gateway");
    static constexpr auto kDmxStartAddress = json::MakeKey("dmx_start_address");

    static constexpr json::KeyDefinition kLabels[] = {kTitle,
                                                      kBoardName,
                                                      kVersion,
                                                      kHostname,
                                                      kIpAddress,
                                                      kNetMask,
                                                      kDefaultGateway,
                                                      kActivePorts,
                                                      kDmxStartAddress,
#ifdef DMX_MAX_PORTS
#if (DMX_MAX_PORTS == 1)
                                                      DmxNodeParamsConst::kUniversePortA,
#ifdef DMXNODE_TYPE_ARTNET
                                                      ArtNetParamsConst::kDestinationIpPortA
#endif
#endif
#if (DMX_MAX_PORTS == 2)
                                                      DmxNodeParamsConst::kUniversePortA,
                                                      DmxNodeParamsConst::kUniversePortB,
#ifdef DMXNODE_TYPE_ARTNET
                                                      ArtNetParamsConst::kDestinationIpPortA,
                                                      ArtNetParamsConst::kDestinationIpPortB
#endif
#endif
#if (DMX_MAX_PORTS == 3)
                                                      DmxNodeParamsConst::kUniversePortA,
                                                      DmxNodeParamsConst::kUniversePortB,
                                                      DmxNodeParamsConst::kUniversePortC,
#ifdef DMXNODE_TYPE_ARTNET
                                                      ArtNetParamsConst::kDestinationIpPortA,
                                                      ArtNetParamsConst::kDestinationIpPortB,
                                                      ArtNetParamsConst::kDestinationIpPortC
#endif
#endif
#if (DMX_MAX_PORTS == 4)
                                                      DmxNodeParamsConst::kUniversePortA,
                                                      DmxNodeParamsConst::kUniversePortB,
                                                      DmxNodeParamsConst::kUniversePortC,
                                                      DmxNodeParamsConst::kUniversePortD,
#ifdef DMXNODE_TYPE_ARTNET
                                                      ArtNetParamsConst::kDestinationIpPortA,
                                                      ArtNetParamsConst::kDestinationIpPortB,
                                                      ArtNetParamsConst::kDestinationIpPortC,
                                                      ArtNetParamsConst::kDestinationIpPortD
#endif
#endif
#endif
    };
};
} // namespace json

#endif // JSON_DISPLAYUDFPARAMSCONST_H_
