/**
 * @file showfileconst.h
 *
 */
/* Copyright (C) 2024 by Arjan van Vught mailto:info@orangepi-dmx.nl
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

#ifndef SHOWFILECONST_H_
#define SHOWFILECONST_H_

#include <cstdint>

namespace showfile {
enum class Status {
	IDLE, PLAYING, STOPPED, ENDED, UNDEFINED
};

static constexpr char STATUS[static_cast<int>(showfile::Status::UNDEFINED)][12] = { "Idle", "Playing", "Stopped", "Ended" };

enum class Formats {
	OLA, UNDEFINED
};

static constexpr uint32_t FORMAT_NAME_LENGTH = 6;	///< Includes '\0'
static constexpr char FORMAT[static_cast<uint32_t>(Formats::UNDEFINED)][FORMAT_NAME_LENGTH] = { "OLA" };

//enum class Protocols {
//	SACN, ARTNET, NODE_SACN, NODE_ARTNET, UNDEFINED
//};
//
//static constexpr uint32_t PROTOCOL_NAME_LENGTH = 14;	///< Includes '\0'
//static constexpr char PROTOCOL[static_cast<uint32_t>(Protocols::UNDEFINED)][PROTOCOL_NAME_LENGTH] = { "sACN", "Art-Net", "Node sACN", "Node Art-Net" };

#define SHOWFILE_PREFIX	"show"
#define SHOWFILE_SUFFIX	".txt"

static constexpr uint32_t FILE_NAME_LENGTH = sizeof(SHOWFILE_PREFIX "NN" SHOWFILE_SUFFIX) - 1U;
static constexpr uint32_t FILE_MAX_NUMBER = 99;
}  // namespace showfile

#endif /* SHOWFILECONST_H_ */
