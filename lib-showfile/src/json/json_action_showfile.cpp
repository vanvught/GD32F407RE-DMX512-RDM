/**
 * @file json_action_showfile.cpp
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
#include <string_view>

#include "json/json_key.h"
#include "json/json_parser.h"
#include "common/utils/utils_string.h"
#include "showfile.h"

namespace {
void SetPlayer(std::string_view value) {
    if (value == "play") {
        ShowFile::Instance().Play();
        return;
    }

    if (value == "stop") {
        ShowFile::Instance().Stop();
        return;
    }

    if (value == "resume") {
        ShowFile::Instance().Resume();
        return;
    }

#ifndef CONFIG_SHOWFILE_DISABLE_RECORD
    if (value == "record") {
        ShowFile::Instance().Record();
        return;
    }
#endif
}

void SetLoop(std::string_view value) {
    if (value.size() != 1) {
        return;
    }

    ShowFile::Instance().DoLoop(value.front() != '0');
}

void SetShow(std::string_view value) {
    if (value.size() > 2) {
        return;
    }

    ShowFile::Instance().SetPlayerShowFileCurrent(common::Atoi(value));
}

constexpr auto kPlayer = json::MakeKey("player");
constexpr auto kLoop = json::MakeKey("loop");
constexpr auto kShow = json::MakeKey("show");

constexpr json::Key kActionKeys[] = {
    json::MakeKey(SetPlayer, kPlayer), //
    json::MakeKey(SetLoop, kLoop),     //
    json::MakeKey(SetShow, kShow),     //
};
} // namespace

namespace json::action {
void SetShowFile(const char* buffer, uint32_t buffer_size) {
    ParseJsonWithTable(buffer, buffer_size, kActionKeys);
}
} // namespace json::action
