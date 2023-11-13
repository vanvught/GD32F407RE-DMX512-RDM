/**
 * @file artnetparamsdump.cpp
 *
 */
/**
 * Art-Net Designed by and Copyright Artistic Licence Holdings Ltd.
 */
/* Copyright (C) 2020-2023 by Arjan van Vught mailto:info@orangepi-dmx.nl
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

#if !defined(__clang__)	// Needed for compiling on MacOS
# pragma GCC push_options
# pragma GCC optimize ("Os")
#endif

#include <cstdio>

#include "artnetparams.h"
#include "artnetparamsconst.h"

#include "lightsetparamsconst.h"

#include "network.h"

void ArtNetParams::Dump() {
#ifndef NDEBUG
	printf("%s::%s \'%s\':\n", __FILE__, __FUNCTION__, ArtNetParamsConst::FILE_NAME);

	printf(" %s=%d [%s]\n", LightSetParamsConst::FAILSAFE, m_Params.nFailSafe, lightset::get_failsafe(static_cast<lightset::FailSafe>(m_Params.nFailSafe)));


	for (uint32_t i = 0; i < artnet::PORTS; i++) {
		printf(" %s=%s\n", LightSetParamsConst::NODE_LABEL[i], m_Params.aLabel[i]);
	}

	printf(" %s=%s\n", LightSetParamsConst::NODE_LONG_NAME, m_Params.aLongName);
	printf(" %s=1 [Yes]\n", ArtNetParamsConst::ENABLE_RDM);


	for (uint32_t i = 0; i < artnet::PORTS; i++) {
		printf(" %s=%d\n", LightSetParamsConst::UNIVERSE_PORT[i], m_Params.nUniverse[i]);
	}

	for (uint32_t i = 0; i < artnet::PORTS; i++) {
		printf(" %s=%s\n", LightSetParamsConst::MERGE_MODE_PORT[i], lightset::get_merge_mode(mergemode_get(i)));
	}

	for (uint32_t i = 0; i < artnet::PORTS; i++) {
		printf(" %s=%s\n", ArtNetParamsConst::PROTOCOL_PORT[i], artnet::get_protocol_mode(i));
	}

	for (uint32_t i = 0; i < artnet::PORTS; i++) {
		const auto portDir = portdir_get(i);
		printf(" %s=%u [%s]\n", LightSetParamsConst::DIRECTION[i], static_cast<uint32_t>(portDir), lightset::get_direction(portDir));
	}

	for (uint32_t i = 0; i < artnet::PORTS; i++) {
		printf(" %s=" IPSTR "\n", ArtNetParamsConst::DESTINATION_IP_PORT[i], IP2STR(m_Params.nDestinationIp[i]));

	}

	for (uint32_t i = 0; i < artnet::PORTS; i++) {
		const auto nOutputStyle = static_cast<uint32_t>(isOutputStyleSet(1U << i));
		printf(" %s=%u [%s]\n", LightSetParamsConst::OUTPUT_STYLE[i], nOutputStyle, lightset::get_output_style(static_cast<lightset::OutputStyle>(nOutputStyle)));
	}

	/**
	 * Art-Net 4
	 */

	printf(" %s=1 [Yes]\n", ArtNetParamsConst::MAP_UNIVERSE0);

	for (uint32_t i = 0; i < artnet::PORTS; i++) {
		printf(" %s=%u\n", LightSetParamsConst::PRIORITY[i], m_Params.nPriority[i]);
	}

	/**
	 * Extra's
	 */

	printf(" %s=1 [Yes]\n", LightSetParamsConst::DISABLE_MERGE_TIMEOUT);
#endif
}
