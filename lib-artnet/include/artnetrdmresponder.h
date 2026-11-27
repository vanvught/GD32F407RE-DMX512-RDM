/**
 * @file artnetrdmresponder.h
 *
 */
/* Copyright (C) 2018-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef ARTNETRDMRESPONDER_H_
#define ARTNETRDMRESPONDER_H_

#include <cstdint>
#include <cstring>

#include "rdmdevice.h"
#include "rdmdeviceresponder.h"
#include "rdmpersonality.h"
#include "rdmhandler.h"
#include "e120.h"
#include "rdm_message_print.h"
#include "firmware/debug/debug_debug.h"
#include "rdm_debug.h"

class ArtNetRdmResponder final : public RDMDeviceResponder {
   public:
    ArtNetRdmResponder(RdmPersonality** rdm_personalities, uint32_t personality_count) : RDMDeviceResponder(rdm_personalities, personality_count) {
        DEBUG_ENTRY();

        DEBUG_EXIT();
    }

    ~ArtNetRdmResponder() override = default;

    void Print() { RDMDeviceResponder::Print(); }

    void TodCopy(uint32_t port_index, unsigned char* tod) {
        DEBUG_PRINTF("port_index=%u", port_index);

        if (port_index == 0) {
            memcpy(tod, rdm::device::Base::Instance().GetUID(), rdm::kUidSize);
        } else {
            memcpy(tod, rdm::kUidAll, rdm::kUidSize);
        }
    }

    std::span<const uint8_t> Handler(uint32_t port_index, std::span<const uint8_t> rdm_data_no_sc) {
        DEBUG_ENTRY();

        if (port_index != 0) {
            DEBUG_EXIT();
            return {};
        }

        if (rdm_data_no_sc.size() < rdm::kMessageMinimumSize - 1U) {
            DEBUG_EXIT();
            return {};
        }

        const auto* rdm_in = reinterpret_cast<const TRdmMessageNoSc*>(rdm_data_no_sc.data());

        const auto kMessageLength = rdm_in->message_length;

        if ((kMessageLength < rdm::kMessageMinimumSize) || (kMessageLength - 1U > rdm_data_no_sc.size())) {
            DEBUG_EXIT();
            return {};
        }

        if constexpr (kDebugRdmEnabled) {
            rdm::message::PrintNoStartcode(rdm_data_no_sc);
        }

        RDMHandler::Instance().HandleData(rdm_data_no_sc.data(), reinterpret_cast<uint8_t*>(&s_rdm_command), RDMHandler::Type::kTypeRdm);

        if (s_rdm_command.start_code != E120_SC_RDM) {
            DEBUG_EXIT();
            return {};
        }

        const std::span<const uint8_t> kResponse{reinterpret_cast<const uint8_t*>(&s_rdm_command), s_rdm_command.message_length + rdm::kMessageChecksumSize};

        if constexpr (kDebugRdmEnabled) {
            rdm::message::Print(kResponse);
        }

        DEBUG_EXIT();

        return kResponse;
    }

   private:
    static inline TRdmMessage s_rdm_command;
};

#endif // ARTNETRDMRESPONDER_H_
