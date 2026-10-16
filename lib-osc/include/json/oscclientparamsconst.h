/**
 * @file oscclientparamsconst.h
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

#ifndef JSON_OSCCLIENTPARAMSCONST_H_
#define JSON_OSCCLIENTPARAMSCONST_H_

#include "json/json_key.h"

namespace json {
struct OscClientParamsConst {
    static constexpr char kFileName[] = "oscclient.json";

    static constexpr auto kServerIp = json::MakeKey("server_ip");
    static constexpr auto kPingDisable = json::MakeKey("ping_disable");
    static constexpr auto kPingDelay = json::MakeKey("ping_delay");

    static constexpr auto kCmd0 = json::MakeKey("cmd0");
    static constexpr auto kCmd1 = json::MakeKey("cmd1");
    static constexpr auto kCmd2 = json::MakeKey("cmd2");
    static constexpr auto kCmd3 = json::MakeKey("cmd3");
    static constexpr auto kCmd4 = json::MakeKey("cmd4");
    static constexpr auto kCmd5 = json::MakeKey("cmd5");
    static constexpr auto kCmd6 = json::MakeKey("cmd6");
    static constexpr auto kCmd7 = json::MakeKey("cmd7");

    static constexpr json::KeyDefinition kCmd[] = {
        kCmd0, kCmd1, kCmd2, kCmd3, kCmd4, kCmd5, kCmd6, kCmd7,
    };

    static constexpr auto kLed0 = json::MakeKey("led0");
    static constexpr auto kLed1 = json::MakeKey("led1");
    static constexpr auto kLed2 = json::MakeKey("led2");
    static constexpr auto kLed3 = json::MakeKey("led3");
    static constexpr auto kLed4 = json::MakeKey("led4");
    static constexpr auto kLed5 = json::MakeKey("led5");
    static constexpr auto kLed6 = json::MakeKey("led6");
    static constexpr auto kLed7 = json::MakeKey("led7");

    static constexpr json::KeyDefinition kLed[] = {
        kLed0, kLed1, kLed2, kLed3, kLed4, kLed5, kLed6, kLed7,
    };
};
} // namespace json

#endif // JSON_OSCCLIENTPARAMSCONST_H_
