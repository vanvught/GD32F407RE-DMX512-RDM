/**
 * @file json_config_showfile.cpp
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

#include <cstdint>
#include <string_view>

#include "showfile.h"
#include "json/showfileparams.h"
#include "json/showfileparamsconst.h"
#include "json/json_helpers.h"
#include "json/oscparamsconst.h"

namespace json::config {
uint32_t GetShowFile(char* buffer, uint32_t length) {
    auto& show_file = ShowFile::Instance();

    return json::helpers::Serialize(buffer, length, [&](JsonDoc& doc) {
        doc[json::ShowFileParamsConst::kShow.name] = static_cast<uint32_t>(show_file.GetShowFileCurrent());
        doc[json::ShowFileParamsConst::kOptionAutoPlay.name] = static_cast<uint32_t>(show_file.IsAutoStart());
        doc[json::ShowFileParamsConst::kOptionLoop.name] = static_cast<uint32_t>(show_file.GetDoLoop());
#ifdef CONFIG_SHOWFILE_ENABLE_OSC
        doc[OscParamsConst::kIncomingPort.name] = static_cast<uint32_t>(show_file.GetOscPortIncoming());
        doc[OscParamsConst::kOutgoingPort.name] = static_cast<uint32_t>(show_file.GetOscPortOutgoing());
#endif
    });
}

void SetShowFile(const char* buffer, uint32_t size) {
    ::json::ShowFileParams showfile_params;
    showfile_params.Store(std::string_view{buffer, size});
    showfile_params.Set();
}
} // namespace json::config
