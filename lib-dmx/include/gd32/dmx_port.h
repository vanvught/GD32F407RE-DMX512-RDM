/**
 * @file dmx_port.h
 *
 */
/* Copyright (C) 2026 by Arjan van Vught mailto:info@gd32-dmx.org
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of thnDmxDataDirecte Software, and to permit persons to whom the Software is
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

#ifndef GD32_DMX_PORT_H_
#define GD32_DMX_PORT_H_

#include <cstdint>

#include "gd32_uart.h"

namespace dmx::port {
enum class Usage {
    kTxRx = 0,   // DMX In/Out and RDM In/Out
    kTxOnly = 1, // DMX Out only
    kRxOnly = 2, // DMX In, no DMA output
};

struct Info {
    gd32::Uart uart;
    uint32_t port;
    uint32_t pin;
    Usage usage;
};
} // namespace dmx::port

#endif // GD32_DMX_PORT_H_
