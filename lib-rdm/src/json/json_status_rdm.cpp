#ifdef RDM_CONTROLLER
/**
 * @file json_status_rdm.cpp
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

#include "rdm_discovery.h"
#include "json/json_jsondoc.h"

namespace json::status {
namespace {
bool ToBool(uint32_t port_index, uint8_t data) {
    return (data & (1U << port_index)) != 0U;
}

char ToType(uint32_t port_index, uint8_t data) {
    const auto kIsSet = (data & (1U << port_index)) == (1U << port_index);
    return kIsSet ? 'f' : 'i';
}

bool PortStatus(JsonDoc& doc, uint8_t data[5], uint32_t port_index) {
    doc.BeginObject();

    doc["port"] = static_cast<char>('A' + port_index);
    doc["enabled"] = ToBool(port_index, data[0]);
    doc["waiting"] = ToBool(port_index, data[1]);
    doc["type"] = ToType(port_index, data[2]);
    doc["bg"] = ToBool(port_index, data[3]);
    doc["running"] = ToBool(port_index, data[4]);

    doc.EndObject();

    const auto kHasNoOverflow = !doc.HasOverflow();

    return kHasNoOverflow;
}
} // namespace

uint32_t Rdm(char* out_buffer, uint32_t out_buffer_size) {
    JsonDoc doc(out_buffer, out_buffer_size);

    doc.BeginArray("rdm");

    uint8_t data[5];
    rdm::Discovery::Instance().GetStatus(data);

    for (uint32_t port_index = 0; port_index < rdm::Discovery::kPorts; port_index++) {
        if (!PortStatus(doc, data, port_index)) {
            break;
        }
    }

    doc.EndArray();
    doc.End();

    return doc.Ok() ? doc.Size() : 0U;
}

uint32_t RdmQueue(char* out_buffer, uint32_t out_buffer_size) {
    JsonDoc doc(out_buffer, out_buffer_size);

    doc.BeginArray("uid");

    auto& discovery = rdm::Discovery::Instance();

    rdm::discovery::UidRange range{};
    for (uint32_t index = 0; discovery.GetWorkingQueueEntry(index, range); ++index) {
        char lower[rdm::discovery::kUidStringLength + 1];
        char upper[rdm::discovery::kUidStringLength + 1];
        char text[28];

        rdm::discovery::FormatUid(range.lower_bound, lower);
        rdm::discovery::FormatUid(range.upper_bound, upper);

        memcpy(text, lower, rdm::discovery::kUidStringLength);
        text[rdm::discovery::kUidStringLength] = '-';
        memcpy(text + rdm::discovery::kUidStringLength + 1, upper, rdm::discovery::kUidStringLength + 1);

        doc.Add(std::string_view{text, 27});
    }

    doc.EndArray();
    doc.End();

    return doc.Ok() ? doc.Size() : 0U;
}

uint32_t RdmTod(char* out_buffer, uint32_t out_buffer_size, uint32_t port_index) {
    if (port_index >= rdm::Discovery::kPorts) {
        return 0;
    }

    JsonDoc doc(out_buffer, out_buffer_size);

    doc["port"] = static_cast<char>('A' + port_index);
    doc.BeginArray("tod");

    auto& discovery = rdm::Discovery::Instance();

    uint8_t uid[rdm::kUidSize];

    for (uint32_t index = 0; discovery.TodCopyUidEntry(port_index, index, uid); ++index) {
        char buffer[rdm::discovery::kUidStringLength + 1];
        rdm::discovery::FormatUid(uid, buffer);

        doc.Add(std::string_view{buffer, rdm::discovery::kUidStringLength});
    }

    doc.EndArray();
    doc.End();

    return doc.Ok() ? doc.Size() : 0U;
}
} // namespace json::status
#endif // RDM_CONTROLLER
