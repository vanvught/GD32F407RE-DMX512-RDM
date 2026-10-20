/**
 * @file dmxsendparams.cpp
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
#include <cstdio>

#include "json/dmxsendparams.h"
#include "dmx_debug.h"
#include "json/dmxsendparamsconst.h"
#include "json/json_parser.h"
#include "json/json_parsehelper.h"
#include "dmxconst.h"
#include "configstore.h"
#include "dmx.h" // IWYU pragma: keep

namespace json {
using dmx::kChannelsMax;

static constexpr uint8_t RounddownSlots(uint16_t n) {
    return static_cast<uint8_t>((n / 2U) - 1);
}

static constexpr uint16_t RoundupSlots(uint8_t n) {
    return static_cast<uint16_t>((n + 1U) * 2U);
}

DmxSendParams::DmxSendParams() {
    ConfigStore::Instance().Copy(&store_dmx_send, &ConfigurationStore::dmx_send);
}

void DmxSendParams::SetBreakTime(std::string_view value) {
    const auto kValue = ParseValue<uint16_t>(value);
    store_dmx_send.break_time = kValue > dmx::transmit::kBreakTimeMin ? kValue : dmx::transmit::kBreakTimeMin;
}

void DmxSendParams::SetMabTime(std::string_view value) {
    const auto kValue = ParseValue<uint16_t>(value);
    store_dmx_send.mab_time = kValue > dmx::transmit::kMabTimeMin ? kValue : dmx::transmit::kMabTimeMin;
}

void DmxSendParams::SetRefreshRate(std::string_view value) {
    uint8_t v;

    if (ParseInRange<uint16_t, uint8_t>(value, 1U, 255U, &v)) {
        store_dmx_send.refresh_rate = v;
    }
}

void DmxSendParams::SetSlotsCount(std::string_view value) {
    const auto kValue = ParseValue<uint16_t>(value);

    if (kValue >= 2U && kValue <= dmx::kChannelsMax) {
        store_dmx_send.slots_count = RounddownSlots(kValue);
    } else {
        store_dmx_send.slots_count = RounddownSlots(dmx::kChannelsMax);
    }
}

void DmxSendParams::Store(std::string_view buffer) {
    ParseJsonWithTable(buffer,kDmxSendKeys);
    ConfigStore::Instance().Store(&store_dmx_send, &ConfigurationStore::dmx_send);
}

void DmxSendParams::Set() {
    auto& dmx = *Dmx::Get();

    dmx.SetTransmitBreakTime(store_dmx_send.break_time);
    dmx.SetTransmitMabTime(store_dmx_send.mab_time);
    dmx.SetTransmitSlots(RoundupSlots(store_dmx_send.slots_count));

    uint32_t period = 0;
    if (store_dmx_send.refresh_rate != 0) {
        period = 1000000U / store_dmx_send.refresh_rate;
    }

    dmx.SetTransmitPeriodTime(period);

    Dump();
}

void DmxSendParams::Dump() {
    if constexpr (kDebugDmxEnabled) {
        printf("%s::%s \'%s\':\n", __FILE__, __FUNCTION__, json::DmxSendParamsConst::kFileName);
        printf(" %s=%u\n", DmxSendParamsConst::kBreakTime.name, store_dmx_send.break_time);
        printf(" %s=%u\n", DmxSendParamsConst::kMabTime.name, store_dmx_send.mab_time);
        printf(" %s=%u\n", DmxSendParamsConst::kRefreshRate.name, store_dmx_send.refresh_rate);
        printf(" %s=%u [%u]\n", DmxSendParamsConst::kSlotsCount.name, RoundupSlots(store_dmx_send.slots_count), store_dmx_send.slots_count);
    }
}
} // namespace json