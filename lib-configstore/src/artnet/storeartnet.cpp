/**
 * @file storeartnet.cpp
 *
 */
/**
 * Art-Net Designed by and Copyright Artistic Licence Holdings Ltd.
 */
/* Copyright (C) 2018-2023 by Arjan van Vught mailto:info@orangepi-dmx.nl
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

#include "storeartnet.h"

#include "artnetstore.h"
#include "artnetparams.h"
#include "artnet.h"

#include "configstore.h"

#include "debug.h"

#if (ARTNET_PAGE_SIZE == 4)
# define UNUSED
#else
# define UNUSED __attribute__((unused))
# include "artnetnode.h"
#endif

uint32_t StoreArtNet::s_nPortIndexOffset;
StoreArtNet *StoreArtNet::s_pThis;

StoreArtNet::StoreArtNet(uint32_t nPortIndexOffset) {
	DEBUG_ENTRY
	DEBUG_PRINTF("nPortIndexOffset=%u, ARTNET_PAGE_SIZE=%u", nPortIndexOffset, ARTNET_PAGE_SIZE);

	assert(s_pThis == nullptr);
	s_pThis = this;

	s_nPortIndexOffset = nPortIndexOffset;

	DEBUG_PRINTF("%p", reinterpret_cast<void *>(s_pThis));
	DEBUG_EXIT
}

void StoreArtNet::SaveUniverseSwitch(uint32_t nPortIndex, UNUSED const uint8_t nAddress) {
	DEBUG_ENTRY
	DEBUG_PRINTF("s_nPortIndexOffset=%u, nPortIndex=%u, nAddress=%u", s_nPortIndexOffset, nPortIndex, static_cast<uint32_t>(nAddress));

	if (nPortIndex >= s_nPortIndexOffset) {
		nPortIndex -= s_nPortIndexOffset;
	} else {
		DEBUG_EXIT
		return;
	}

	DEBUG_PRINTF("nPortIndex=%u", nPortIndex);

	if (nPortIndex >= artnet::PORTS) {
		DEBUG_EXIT
		return;
	}

#if (ARTNET_PAGE_SIZE == 4)
	ConfigStore::Get()->Update(configstore::Store::ARTNET, nPortIndex + __builtin_offsetof(struct artnetparams::Params, nUniversePort), &nAddress, sizeof(uint8_t), artnetparams::Mask::UNIVERSE_A << nPortIndex);
#else
	SaveUniverse(nPortIndex);
#endif

	DEBUG_EXIT
}

void StoreArtNet::SaveNetSwitch(uint32_t nPage, UNUSED const uint8_t nAddress) {
	DEBUG_ENTRY
	DEBUG_PRINTF("nPage=%u, nAddress=%u", nPage, static_cast<uint32_t>(nAddress));

#if (ARTNET_PAGE_SIZE == 4)
	if (nPage > 0) {
		DEBUG_EXIT
		return;
	}

	ConfigStore::Get()->Update(configstore::Store::ARTNET, __builtin_offsetof(struct artnetparams::Params, nNet), &nAddress, sizeof(uint8_t), artnetparams::Mask::NET);
#else
	SaveUniverse(nPage);
#endif

	DEBUG_EXIT
}

void StoreArtNet::SaveSubnetSwitch(uint32_t nPage, UNUSED const uint8_t nAddress) {
	DEBUG_ENTRY
	DEBUG_PRINTF("nPage=%u, nAddress=%u", nPage, static_cast<uint32_t>(nAddress));


#if (ARTNET_PAGE_SIZE == 4)
	if (nPage > 0) {
		DEBUG_EXIT
		return;
	}

	ConfigStore::Get()->Update(configstore::Store::ARTNET, __builtin_offsetof(struct artnetparams::Params, nSubnet), &nAddress, sizeof(uint8_t), artnetparams::Mask::SUBNET);
#else
	SaveUniverse(nPage);
#endif

	DEBUG_EXIT
}

#if (ARTNET_PAGE_SIZE == 1)
void StoreArtNet::SaveUniverse(uint32_t nPortIndex) {
	DEBUG_ENTRY
	assert(nPortIndex < artnet::PORTS);

	uint16_t nUniverse;

	if (ArtNetNode::Get()->GetPortAddress(nPortIndex, nUniverse)) {
		DEBUG_PRINTF("nPortIndex=%u, nUniverse=%u", nPortIndex, nUniverse);
		ConfigStore::Get()->Update(configstore::Store::ARTNET, (sizeof(uint16_t) * nPortIndex) + __builtin_offsetof(struct artnetparams::Params, nUniverse), &nUniverse, sizeof(uint16_t), artnetparams::Mask::UNIVERSE_A << nPortIndex);
	}

	DEBUG_EXIT
}
#endif

void StoreArtNet::SaveMergeMode(uint32_t nPortIndex, const lightset::MergeMode mergeMode) {
	DEBUG_ENTRY
	DEBUG_PRINTF("s_nPortIndexOffset=%u, nPortIndex=%u, mergeMode=%u", s_nPortIndexOffset, nPortIndex, static_cast<uint32_t>(mergeMode));

	if (nPortIndex >= s_nPortIndexOffset) {
		nPortIndex -= s_nPortIndexOffset;
	} else {
		DEBUG_EXIT
		return;
	}

	DEBUG_PRINTF("nPortIndex=%u", nPortIndex);

	if (nPortIndex >= artnet::PORTS) {
		DEBUG_EXIT
		return;
	}

	ConfigStore::Get()->Update(configstore::Store::ARTNET, nPortIndex + __builtin_offsetof(struct artnetparams::Params, nMergeModePort), &mergeMode, sizeof(uint8_t), artnetparams::Mask::MERGE_MODE_A << nPortIndex);

	DEBUG_EXIT
}

void StoreArtNet::SavePortProtocol(uint32_t nPortIndex, const artnet::PortProtocol portProtocol) {
	DEBUG_ENTRY
	DEBUG_PRINTF("s_nPortIndexOffset=%u, nPortIndex=%u, portProtocol=%u", s_nPortIndexOffset, nPortIndex, static_cast<uint32_t>(portProtocol));

	if (nPortIndex >= s_nPortIndexOffset) {
		nPortIndex -= s_nPortIndexOffset;
	} else {
		DEBUG_EXIT
		return;
	}

	DEBUG_PRINTF("nPortIndex=%u", nPortIndex);

	if (nPortIndex >= artnet::PORTS) {
		DEBUG_EXIT
		return;
	}

	ConfigStore::Get()->Update(configstore::Store::ARTNET, nPortIndex + __builtin_offsetof(struct artnetparams::Params, nProtocolPort), &portProtocol, sizeof(uint8_t), artnetparams::Mask::PROTOCOL_A << nPortIndex);

	DEBUG_EXIT
}

void  StoreArtNet::SaveOutputStyle(uint32_t nPortIndex, const artnet::OutputStyle outputStyle) {
	DEBUG_ENTRY
	DEBUG_PRINTF("s_nPortIndexOffset=%u, nPortIndex=%u, outputStyle=%u", s_nPortIndexOffset, nPortIndex, static_cast<uint32_t>(outputStyle));

	if (nPortIndex >= s_nPortIndexOffset) {
		nPortIndex -= s_nPortIndexOffset;
	} else {
		DEBUG_EXIT
		return;
	}

	DEBUG_PRINTF("nPortIndex=%u", nPortIndex);

	if (nPortIndex >= artnet::PORTS) {
		DEBUG_EXIT
		return;
	}

	uint8_t nOutputStyle;
	ConfigStore::Get()->Copy(configstore::Store::ARTNET, &nOutputStyle, sizeof(uint8_t), __builtin_offsetof(struct artnetparams::Params, nOutputStyle), false);

	if (outputStyle == artnet::OutputStyle::CONSTANT) {
		nOutputStyle |= static_cast<uint8_t>(1U << nPortIndex);
	} else {
		nOutputStyle &= static_cast<uint8_t>(~(1U << nPortIndex));
	}

	ConfigStore::Get()->Update(configstore::Store::ARTNET, __builtin_offsetof(struct artnetparams::Params, nOutputStyle), &nOutputStyle, sizeof(uint8_t));

	DEBUG_EXIT
}

void StoreArtNet::SaveRdmEnabled(uint32_t nPortIndex, const bool isEnabled) {
	DEBUG_ENTRY
	DEBUG_PRINTF("s_nPortIndexOffset=%u, nPortIndex=%u, isEnabled=%d", s_nPortIndexOffset, nPortIndex, isEnabled);

	if (nPortIndex >= s_nPortIndexOffset) {
		nPortIndex -= s_nPortIndexOffset;
	} else {
		DEBUG_EXIT
		return;
	}

	DEBUG_PRINTF("nPortIndex=%u", nPortIndex);

	if (nPortIndex >= artnet::PORTS) {
		DEBUG_EXIT
		return;
	}

	uint16_t nRdm;
	ConfigStore::Get()->Copy(configstore::Store::ARTNET, &nRdm, sizeof(uint16_t), __builtin_offsetof(struct artnetparams::Params, nRdm), false);

	nRdm &= artnetparams::clear_mask(nPortIndex);

	if (isEnabled) {
		nRdm |= artnetparams::shift_left(1, nPortIndex);
		nRdm |= static_cast<uint16_t>(1U << (nPortIndex + 8));
	}

	ConfigStore::Get()->Update(configstore::Store::ARTNET, __builtin_offsetof(struct artnetparams::Params, nRdm), &nRdm, sizeof(uint16_t));

	DEBUG_EXIT
}
