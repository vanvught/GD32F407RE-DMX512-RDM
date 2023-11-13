/**
 * @file json_get_directory.cpp
 *
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

#include <cstdio>
#include <cstdint>

#include "device/usb/host.h"

namespace remoteconfig {
namespace storage {
uint32_t json_get_directory(char *pOutBuffer, const uint32_t nOutBufferSize) {
	auto nLength = static_cast<uint32_t>(snprintf(pOutBuffer, nOutBufferSize, "{\"label\":\"%s\",\"files\":[", usb::host::get_status() == usb::host::Status::READY ? "storage" : "No USB device"));
	//TODO
	nLength += static_cast<uint32_t>(snprintf(&pOutBuffer[nLength], nOutBufferSize - nLength, "]}" ));
	return nLength;
}
}  // namespace storage
}  // namespace remoteconfig
