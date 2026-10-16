/**
 * @file json_action.cpp
 */
/* Copyright (C) 2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include <string_view>

#include "display.h"
#include "board_statusled.h"
#include "board.h"
#include "json/json_key.h"
#include "json/json_parser.h"

namespace {
void SetDisplay(std::string_view value) {
    if (value.size() != 1) {
        return;
    }

    Display::Get()->SetSleep(value[0] == '0');
}

void SetIdentify(std::string_view value) {
    if (value.size() != 1) {
        return;
    }

    board::statusled::SetMode(value[0] != '0' ? board::statusled::Mode::kFast : board::statusled::Mode::kNormal);
}

void SetReboot(std::string_view value) {
    if (value.size() != 1) {
        return;
    }

    if (value[0] != '0') {
        board::Reboot();
    }
}

constexpr auto kDisplay = json::MakeKey("display");
constexpr auto kIdentify = json::MakeKey("identify");
constexpr auto kReboot = json::MakeKey("reboot");

constexpr json::Key kActionKeys[] = {
    json::MakeKey(SetDisplay, kDisplay),
    json::MakeKey(SetIdentify, kIdentify),
    json::MakeKey(SetReboot, kReboot),
};
} // namespace

namespace json::action {
void Set(const char* buffer, uint32_t buffer_size) {
    ParseJsonWithTable(buffer, buffer_size, kActionKeys);
}
} // namespace json::action
