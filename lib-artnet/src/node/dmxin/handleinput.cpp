/**
 * @file handleinput.cpp
 *
 */
/**
 * Art-Net Designed by and Copyright Artistic Licence Holdings Ltd.
 */
/* Copyright (C) 2023 by Arjan van Vught mailto:info@orangepi-dmx.nl
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
#include <cassert>

#include "artnetnode.h"
#include "artnet.h"

#include "debug.h"

/**
 * A Controller or monitoring device on the network can
 * enable or disable individual DMX512 inputs on any of the network nodes.
 * This allows the Controller to directly control network traffic and
 * ensures that unused inputs are disabled and therefore not wasting bandwidth.
 */

void ArtNetNode::HandleInput() {
	DEBUG_ENTRY

	const auto *const pArtInput = reinterpret_cast<TArtInput *>(m_pReceiveBuffer);
	const auto nPage = static_cast<uint32_t>(pArtInput->BindIndex > 0 ? pArtInput->BindIndex - 1 : 0);
	const auto nPortIndexMax = std::min(std::min(artnet::PORTS, artnetnode::PAGE_SIZE), static_cast<uint32_t>(pArtInput->NumPortsLo));

	DEBUG_PRINTF("nPage=%u, nPortIndexMax=%u", nPage, nPortIndexMax);

	for (uint32_t NumPorts = 0; NumPorts < nPortIndexMax ; NumPorts++) {
		uint32_t nPortIndex;

		const auto isPortIndexInput __attribute__((unused)) = GetPortIndexInput(nPage, NumPorts, nPortIndex);
		DEBUG_PRINTF("isPortIndexInput=%c, nPortIndex=%u", isPortIndexInput ? 'Y' : 'N', nPortIndex);
		assert(isPortIndexInput);

		uint8_t nAddress;

		if (GetUniverseSwitch(nPortIndex, nAddress, lightset::PortDir::INPUT)) {
			if (pArtInput->Input[NumPorts] & 0x01) {
				m_InputPort[nPortIndex].GoodInput |= static_cast<uint8_t>(artnet::GoodInput::DISABLED);
			} else {
				m_InputPort[nPortIndex].GoodInput &= static_cast<uint8_t>(~static_cast<uint8_t>(artnet::GoodInput::DISABLED));
			}
		}
	}

	if (m_State.SendArtPollReplyOnChange) {
		SendPollRelply();
	}

	DEBUG_EXIT
}
