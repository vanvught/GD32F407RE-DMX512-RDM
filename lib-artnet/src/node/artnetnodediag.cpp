#if defined ( ARTNET_ENABLE_SENDDIAG )
/**
 * @file artnetnodediag.cpp
 *
 */
/**
 * Art-Net Designed by and Copyright Artistic Licence Holdings Ltd.
 */
/* Copyright (C) 2019-2022 by Arjan van Vught mailto:info@orangepi-dmx.nl
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

#include "artnetnode.h"

#include "network.h"

void ArtNetNode::FillDiagData(void) {
	memset(&m_DiagData, 0, sizeof(struct TArtDiagData));

	memcpy(m_DiagData.Id, artnet::NODE_ID, sizeof(m_DiagData.Id));
	m_DiagData.OpCode = OP_DIAGDATA;
	m_DiagData.ProtVerLo = artnet::PROTOCOL_REVISION;
}

void ArtNetNode::SendDiag(const char *pText, artnet::PriorityCodes priorityCode) {
	if (!m_State.SendArtDiagData) {
		return;
	}

	if (static_cast<uint8_t>(priorityCode) < m_State.DiagPriority) {
		return;
	}

	m_DiagData.Priority = static_cast<uint8_t>(priorityCode);

	strncpy(reinterpret_cast<char *>(m_DiagData.Data), pText, sizeof(m_DiagData.Data) - 1);
	m_DiagData.Data[sizeof(m_DiagData.Data) - 1] = '\0';	// Just be sure we have a last '\0'
	m_DiagData.LengthLo = static_cast<uint8_t>(strlen(reinterpret_cast<char *>(m_DiagData.Data)) + 1);	// Text length including the '\0'

	const uint16_t nSize = sizeof(struct TArtDiagData) - sizeof(m_DiagData.Data) + m_DiagData.LengthLo;

	Network::Get()->SendTo(m_nHandle, &m_DiagData, nSize, m_State.DiagSendIPAddress, artnet::UDP_PORT);
}
#endif
