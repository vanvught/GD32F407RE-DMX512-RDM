/**
 * @file rdm.cpp
 *
 */
/* Copyright (C) 2021-2022 by Arjan van Vught mailto:info@gd32-dmx.org
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
#include <cassert>

#include "rdm.h"

#include "dmx.h"
#include "dmxconst.h"
#include "dmx_internal.h"

#include "gd32.h"
#include "gd32_micros.h"
#include "gd32/dmx_config.h"

#include "debug.h"

static uint8_t s_TransactionNumber[dmx::config::max::OUT];
static uint32_t s_nLastSendMicros[dmx::config::max::OUT];

extern volatile uint32_t gv_RdmDataReceiveEnd;

void Rdm::Send(uint32_t nPort, struct TRdmMessage *pRdmMessage, uint32_t nSpacingMicros) {
	DEBUG_PRINTF("nPort=%u, pRdmData=%p, nSpacingMicros=%u", nPort, pRdmMessage, nSpacingMicros);
	assert(nPort < DMX_MAX_PORTS);
	assert(pRdmMessage != nullptr);

	if (nSpacingMicros != 0) {
		const auto nMicros = micros();
		const auto nDeltaMicros = nMicros - s_nLastSendMicros[nPort];
		if (nDeltaMicros < nSpacingMicros) {
			const auto nWait = nSpacingMicros - nDeltaMicros;
			do {
			} while ((micros() - nMicros) < nWait);
		}
	}

	auto *pData = reinterpret_cast<uint8_t*>(pRdmMessage);
	uint32_t i;
	uint16_t nChecksum = 0;

	pRdmMessage->transaction_number = s_TransactionNumber[nPort];

	for (i = 0; i < pRdmMessage->message_length; i++) {
		nChecksum = static_cast<uint16_t>(nChecksum + pData[i]);
	}

	pData[i++] = static_cast<uint8_t>(nChecksum >> 8);
	pData[i] = static_cast<uint8_t>(nChecksum & 0XFF);

	SendRaw(nPort, reinterpret_cast<const uint8_t*>(pRdmMessage), pRdmMessage->message_length + RDM_MESSAGE_CHECKSUM_SIZE);

	s_nLastSendMicros[nPort] = micros();
	s_TransactionNumber[nPort]++;

	DEBUG_EXIT
}

void Rdm::SendRawRespondMessage(uint32_t nPort, const uint8_t *pRdmData, uint32_t nLength) {
	DEBUG_PRINTF("nPort=%u, pRdmData=%p, nLength=%u", nPort, pRdmData, nLength);
	assert(nPort < DMX_MAX_PORTS);
	assert(pRdmData != nullptr);
	assert(nLength != 0);

	// 3.2.2 Responder Packet spacing
	udelay(RDM_RESPONDER_PACKET_SPACING, gv_RdmDataReceiveEnd);

	SendRaw(nPort, pRdmData, nLength);
}

void Rdm::SendDiscoveryRespondMessage(uint32_t nPort, const uint8_t *pRdmData, uint32_t nLength) {
	DEBUG_PRINTF("nPort=%u, pRdmData=%p, nLength=%u", nPort, pRdmData, nLength);
	assert(nPort < DMX_MAX_PORTS);
	assert(pRdmData != nullptr);
	assert(nLength != 0);

	// 3.2.2 Responder Packet spacing
	udelay(RDM_RESPONDER_PACKET_SPACING, gv_RdmDataReceiveEnd);

	Dmx::Get()->SetPortDirection(nPort, dmx::PortDirection::OUTP, false);

	const auto nUart = _port_to_uart(nPort);

	for (uint32_t i = 0; i < nLength; i++) {
		while (RESET == usart_flag_get(nUart, USART_FLAG_TBE))
			;
		USART_DATA(nUart) = static_cast<uint16_t>(USART_DATA_DATA & pRdmData[i]);
	}

	while (SET != usart_flag_get(nUart, USART_FLAG_TC)) {
		static_cast<void>(GET_BITS(USART_DATA(nUart), 0U, 8U));
	}

	udelay(RDM_RESPONDER_DATA_DIRECTION_DELAY);

	Dmx::Get()->SetPortDirection(nPort, dmx::PortDirection::INP, true);
}
