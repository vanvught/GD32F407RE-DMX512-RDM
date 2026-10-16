/**
 * @file e131paramsconst.h
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

#ifndef JSON_E131PARAMSCONST_H_
#define JSON_E131PARAMSCONST_H_

#include "json/json_key.h"
#include "dmxnode_outputtype.h"

namespace json {
struct E131ParamsConst {
    static constexpr char kFileName[] = "e131.json";

#ifdef DMX_MAX_PORTS
    static constexpr auto kPriorityPortA = json::MakeKey("priority_port_a");
#if (DMX_MAX_PORTS > 1)
    static constexpr auto kPriorityPortB = json::MakeKey("priority_port_b");
#endif
#if (DMX_MAX_PORTS > 2)
    static constexpr auto kPriorityPortC = json::MakeKey("priority_port_c");
#endif
#if (DMX_MAX_PORTS == 4)
    static constexpr auto kPriorityPortD = json::MakeKey("priority_port_d");
#endif

    static constexpr json::KeyDefinition kPriorityPort[] = {
        kPriorityPortA,
#if (DMX_MAX_PORTS > 1)
        kPriorityPortB,
#endif
#if (DMX_MAX_PORTS > 2)
        kPriorityPortC,
#endif
#if (DMX_MAX_PORTS == 4)
        kPriorityPortD,
#endif
    };
#endif
};
} // namespace json

#endif // JSON_E131PARAMSCONST_H_
