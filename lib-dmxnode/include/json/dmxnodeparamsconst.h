/**
 * @file dmxnodeparamsconst.h
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

#ifndef JSON_DMXNODEPARAMSCONST_H_
#define JSON_DMXNODEPARAMSCONST_H_

#include "json/json_key.h"
#include "dmxnode_outputtype.h"

namespace json {
struct DmxNodeParamsConst {
    static constexpr char kFileName[] = "dmxnode.json";

    static constexpr auto kNodeName = json::MakeKey("node_name");
    static constexpr auto kFailsafe = json::MakeKey("failsafe");
    static constexpr auto kDisableMergeTimeout = json::MakeKey("disable_merge_timeout");
    static constexpr auto kDmxStartAddress = json::MakeKey("dmx_start_address");
    static constexpr auto kDmxSlotInfo = json::MakeKey("dmx_slot_info");

#ifdef DMX_MAX_PORTS
    static constexpr auto kLabelPortA = json::MakeKey("label_port_a");
#if (DMX_MAX_PORTS > 1)
    static constexpr auto kLabelPortB = json::MakeKey("label_port_b");
#endif // (DMX_MAX_PORTS > 1)
#if (DMX_MAX_PORTS > 2)
    static constexpr auto kLabelPortC = json::MakeKey("label_port_c");
#endif // (DMX_MAX_PORTS > 2)
#if (DMX_MAX_PORTS == 4)
    static constexpr auto kLabelPortD = json::MakeKey("label_port_d");
#endif // (DMX_MAX_PORTS == 4)

    static constexpr json::KeyDefinition kLabelPort[] = {
        kLabelPortA,
#if (DMX_MAX_PORTS > 1)
        kLabelPortB,
#endif // (DMX_MAX_PORTS > 1)
#if (DMX_MAX_PORTS > 2)
        kLabelPortC,
#endif // (DMX_MAX_PORTS > 2)
#if (DMX_MAX_PORTS == 4)
        kLabelPortD,
#endif // (DMX_MAX_PORTS == 4)
    };

    static constexpr auto kUniversePortA = json::MakeKey("universe_port_a");
#if (DMX_MAX_PORTS > 1)
    static constexpr auto kUniversePortB = json::MakeKey("universe_port_b");
#endif // (DMX_MAX_PORTS > 1)
#if (DMX_MAX_PORTS > 2)
    static constexpr auto kUniversePortC = json::MakeKey("universe_port_c");
#endif // (DMX_MAX_PORTS > 2)
#if (DMX_MAX_PORTS == 4)
    static constexpr auto kUniversePortD = json::MakeKey("universe_port_d");
#endif // (DMX_MAX_PORTS == 4)

    static constexpr json::KeyDefinition kUniversePort[] = {
        kUniversePortA,
#if (DMX_MAX_PORTS > 1)
        kUniversePortB,
#endif // (DMX_MAX_PORTS > 1)
#if (DMX_MAX_PORTS > 2)
        kUniversePortC,
#endif // (DMX_MAX_PORTS > 2)
#if (DMX_MAX_PORTS == 4)
        kUniversePortD,
#endif // (DMX_MAX_PORTS == 4)
    };

    static constexpr auto kDirectionPortA = json::MakeKey("direction_port_a");
#if (DMX_MAX_PORTS > 1)
    static constexpr auto kDirectionPortB = json::MakeKey("direction_port_b");
#endif // (DMX_MAX_PORTS > 1)
#if (DMX_MAX_PORTS > 2)
    static constexpr auto kDirectionPortC = json::MakeKey("direction_port_c");
#endif // (DMX_MAX_PORTS > 2)
#if (DMX_MAX_PORTS == 4)
    static constexpr auto kDirectionPortD = json::MakeKey("direction_port_d");
#endif // (DMX_MAX_PORTS == 4)

    static constexpr json::KeyDefinition kDirectionPort[] = {
        kDirectionPortA,
#if (DMX_MAX_PORTS > 1)
        kDirectionPortB,
#endif // (DMX_MAX_PORTS > 1)
#if (DMX_MAX_PORTS > 2)
        kDirectionPortC,
#endif // (DMX_MAX_PORTS > 2)
#if (DMX_MAX_PORTS == 4)
        kDirectionPortD,
#endif // (DMX_MAX_PORTS == 4)
    };

    static constexpr auto kMergeModePortA = json::MakeKey("merge_mode_port_a");
#if (DMX_MAX_PORTS > 1)
    static constexpr auto kMergeModePortB = json::MakeKey("merge_mode_port_b");
#endif // (DMX_MAX_PORTS > 1)
#if (DMX_MAX_PORTS > 2)
    static constexpr auto kMergeModePortC = json::MakeKey("merge_mode_port_c");
#endif // (DMX_MAX_PORTS > 2)
#if (DMX_MAX_PORTS == 4)
    static constexpr auto kMergeModePortD = json::MakeKey("merge_mode_port_d");
#endif // (DMX_MAX_PORTS == 4)

    static constexpr json::KeyDefinition kMergeModePort[] = {
        kMergeModePortA,
#if (DMX_MAX_PORTS > 1)
        kMergeModePortB,
#endif // (DMX_MAX_PORTS > 1)
#if (DMX_MAX_PORTS > 2)
        kMergeModePortC,
#endif // (DMX_MAX_PORTS > 2)
#if (DMX_MAX_PORTS == 4)
        kMergeModePortD,
#endif // (DMX_MAX_PORTS == 4)
    };

    static constexpr auto kOutputStylePortA = json::MakeKey("output_style_a");
#if (DMX_MAX_PORTS > 1)
    static constexpr auto kOutputStylePortB = json::MakeKey("output_style_b");
#endif // (DMX_MAX_PORTS > 1)
#if (DMX_MAX_PORTS > 2)
    static constexpr auto kOutputStylePortC = json::MakeKey("output_style_c");
#endif // (DMX_MAX_PORTS > 2)
#if (DMX_MAX_PORTS == 4)
    static constexpr auto kOutputStylePortD = json::MakeKey("output_style_d");
#endif // (DMX_MAX_PORTS == 4)

    static constexpr json::KeyDefinition kOutputStylePort[] = {
        kOutputStylePortA,
#if (DMX_MAX_PORTS > 1)
        kOutputStylePortB,
#endif // (DMX_MAX_PORTS > 1)
#if (DMX_MAX_PORTS > 2)
        kOutputStylePortC,
#endif // (DMX_MAX_PORTS > 2)
#if (DMX_MAX_PORTS == 4)
        kOutputStylePortD,
#endif // (DMX_MAX_PORTS == 4)
    };
#endif // DMX_MAX_PORTS
};
} // namespace json

#endif // JSON_DMXNODEPARAMSCONST_H_
