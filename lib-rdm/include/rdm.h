/**
 * @file rdm.h
 *
 */
/* Copyright (C) 2015-2023 by Arjan van Vught mailto:info@orangepi-dmx.nl
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

#ifndef RDM_H_
#define RDM_H_

#include <cstdint>
#include <cassert>

#include "rdmconst.h"
#include "dmx.h"

#include "hal_api.h"

class Rdm {
public:
	static void Send(uint32_t nPortIndex, struct TRdmMessage *);

	static void SendRaw(const uint32_t nPortIndex, const uint8_t *pRdmData, const uint32_t nLength) {
		assert(pRdmData != nullptr);
		assert(nLength != 0);
		Dmx::Get()->SetPortDirection(nPortIndex, dmx::PortDirection::OUTP, false);
		Dmx::Get()->RdmSendRaw(nPortIndex, pRdmData, nLength);
		udelay(RDM_RESPONDER_DATA_DIRECTION_DELAY);
		Dmx::Get()->SetPortDirection(nPortIndex, dmx::PortDirection::INP, true);
	}

	static void SendRawRespondMessage(uint32_t nPortIndex, const uint8_t *pRdmData, uint32_t nLength);
	static void SendDiscoveryRespondMessage(uint32_t nPortIndex, const uint8_t *pRdmData, uint32_t nLength);

	static const uint8_t *Receive(const uint32_t nPortIndex) {
		return Dmx::Get()->RdmReceive(nPortIndex);
	}

	static const uint8_t *ReceiveTimeOut(const uint32_t nPortIndex, const uint16_t nTimeOut) {
		return Dmx::Get()->RdmReceiveTimeOut(nPortIndex, nTimeOut);
	}
};

#endif /* RDM_H_ */
