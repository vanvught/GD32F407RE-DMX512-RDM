/**
 * @file json_status_dmx.cpp
 */
/* Copyright (C) 2025-206 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include "dmx.h"

#include "json/json_jsondoc.h"

namespace json::status {
namespace {
void WriteDmxStatistics(JsonDoc& doc, uint32_t port_index) {
    const auto& statistics = Dmx::Get()->GetTotalStatistics(port_index);

    const char kPort[] = {static_cast<char>('A' + port_index), '\0'};

    doc["port"] = kPort;

    doc.BeginObject("dmx");
    doc["sent"] = statistics.dmx.sent;
    doc["received"] = statistics.dmx.received;
    doc.EndObject();

    doc.BeginObject("rdm");

    doc.BeginObject("sent");
    doc["class"] = statistics.rdm.sent.classes;
    doc["discovery"] = statistics.rdm.sent.discovery_response;
    doc.EndObject();

    doc.BeginObject("received");
    doc["good"] = statistics.rdm.received.good;
    doc["bad"] = statistics.rdm.received.bad;
    doc["discovery"] = statistics.rdm.received.discovery_response;
    doc.EndObject();

    doc.EndObject(); // rdm
}
} // namespace

uint32_t Dmx(char* out_buffer, uint32_t out_buffer_size, uint32_t port_index) {
    if (port_index >= ::dmx::config::max::kPorts) {
        return 0;
    }

    JsonDoc doc(out_buffer, out_buffer_size);

    WriteDmxStatistics(doc, port_index);

    doc.End();

    return doc.Ok() ? doc.Size() : 0U;
}

uint32_t Dmx(char* out_buffer, uint32_t out_buffer_size) {
    JsonDoc doc(out_buffer, out_buffer_size, JsonDoc::Root::kArray);

    for (uint32_t port_index = 0; port_index < ::dmx::config::max::kPorts; ++port_index) {
        doc.BeginObject();

        WriteDmxStatistics(doc, port_index);

        doc.EndObject();

        if (doc.HasOverflow()) {
            break;
        }
    }

    doc.End();

    return doc.Ok() ? doc.Size() : 0U;
}
} // namespace json::status
