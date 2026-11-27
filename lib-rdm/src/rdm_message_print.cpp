/**
 * @file rdm_message_print.cpp
 *
 */
/* Copyright (C) 2017-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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
#include <cstring>
#include <cstdio>
#include <span>
#include <limits>

#include "e120.h"
#include "rdm_e120.h"
#include "firmware/debug/debug_debug.h"

namespace rdm::message {
namespace {
void PrintRaw(std::span<const uint8_t> data) {
    for (const auto kByte : data) {
        printf("%.2x ", kByte);
    }

    puts("");
}
} // namespace

void Print(std::span<const uint8_t> rdm_data) {
    if (rdm_data.empty()) {
        DEBUG_PUTS("No RDM data");
        return;
    }

    if (rdm_data[0] == 0xFE) {
        printf("RDM discovery response: %zu bytes -> ", rdm_data.size());
        PrintRaw(rdm_data);
        return;
    }

    if (rdm_data[0] != E120_SC_RDM) {
        printf("Unknown RDM data: %zu bytes -> ", rdm_data.size());
        PrintRaw(rdm_data);
        return;
    }

    if (rdm_data.size() < e120::kMessageLengthMin) {
        printf("Truncated RDM header: %zu bytes -> ", rdm_data.size());
        PrintRaw(rdm_data);
        return;
    }

    const auto* rdm_message = reinterpret_cast<const struct TRdmMessage*>(rdm_data.data());

    const auto kMessageLength = rdm_message->message_length;
    const auto kPdl = rdm_message->param_data_length;

    if ((kMessageLength < e120::kMessageLengthMin) || (kMessageLength > rdm_data.size()) || (kMessageLength != e120::kMessageLengthMin + kPdl)) {
        printf("Invalid RDM length: size=%zu, ML=%u, PDL=%u -> ", rdm_data.size(), kMessageLength, kPdl);
        PrintRaw(rdm_data);
        return;
    }

    const auto* src_uid = rdm_message->source_uid;

    printf("%.2x%.2x:%.2x%.2x%.2x%.2x -> ", src_uid[0], src_uid[1], src_uid[2], src_uid[3], src_uid[4], src_uid[5]);

    const auto* dst_uid = rdm_message->destination_uid;

    printf("%.2x%.2x:%.2x%.2x%.2x%.2x ", dst_uid[0], dst_uid[1], dst_uid[2], dst_uid[3], dst_uid[4], dst_uid[5]);

    switch (rdm_message->command_class) {
        case E120_DISCOVERY_COMMAND:
            printf("DISCOVERY_COMMAND");
            break;
        case E120_DISCOVERY_COMMAND_RESPONSE:
            printf("DISCOVERY_COMMAND_RESPONSE");
            break;
        case E120_GET_COMMAND:
            printf("GET_COMMAND");
            break;
        case E120_GET_COMMAND_RESPONSE:
            printf("GET_COMMAND_RESPONSE %u", static_cast<unsigned>(rdm_message->slot16.response_type));
            break;
        case E120_SET_COMMAND:
            printf("SET_COMMAND");
            break;
        case E120_SET_COMMAND_RESPONSE:
            printf("SET_COMMAND_RESPONSE %u", static_cast<unsigned>(rdm_message->slot16.response_type));
            break;
        default:
            printf("CC {%.2x}", rdm_message->command_class);
            break;
    }

    const auto kSubDevice = static_cast<uint16_t>((rdm_message->sub_device[0] << 8) + rdm_message->sub_device[1]);

    printf(", sub-dev: %u, tn: %u, PID 0x%.2x%.2x, pdl: %u",
           static_cast<unsigned>(kSubDevice),                      //
           static_cast<unsigned>(rdm_message->transaction_number), //
           rdm_message->param_id[0],                               //
           rdm_message->param_id[1],                               //
           static_cast<unsigned>(kPdl)                             //
    );

    if (kPdl != 0) {
        printf(" -> ");

        for (uint32_t i = 0; (i < 12) && (i < kPdl); i++) {
            printf("%.2x ", rdm_message->param_data[i]);
        }
    }

    puts("");
}

void PrintNoStartcode(std::span<const uint8_t> rdm_data_no_sc) {
    if (rdm_data_no_sc.size() < e120::kMessageLengthMin - 1U) {
        printf("Truncated RDM data without start code: %u bytes -> ", rdm_data_no_sc.size());
        PrintRaw(rdm_data_no_sc);
        return;
    }

    const auto* data = reinterpret_cast<const struct TRdmMessageNoSc*>(rdm_data_no_sc.data());
    const auto kMessageLength = data->message_length;

    uint8_t message[sizeof(struct TRdmMessage)];

    if ((kMessageLength < e120::kMessageLengthMin) || (kMessageLength - 1U > rdm_data_no_sc.size())) {
        printf("Invalid RDM message length: %u\n", kMessageLength);
        PrintRaw(rdm_data_no_sc);
        return;
    }

    message[0] = E120_SC_RDM;

    memcpy(&message[1], rdm_data_no_sc.data(), kMessageLength - 1U);

    Print({message, kMessageLength});
}
} // namespace rdm::message
