/**
 * @file mac_address.cpp
 *
 */
/* Copyright (C) 2021-2024 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include "gd32.h"
#include "debug.h"

void mac_address_get(uint8_t paddr[]) {
#if defined (GD32H7XX)
	const auto mac_hi = *(volatile uint32_t *) (0x1FF0F7E8);
	const auto mac_lo = *(volatile uint32_t *) (0x1FF0F7EC);
#elif defined (GD32F4XX)
	const auto mac_hi = *(volatile uint32_t *) (0x1FFF7A10);
	const auto mac_lo = *(volatile uint32_t *) (0x1FFF7A14);
#else
	const auto mac_hi = *(volatile uint32_t *) (0x1FFFF7E8);
	const auto mac_lo = *(volatile uint32_t *) (0x1FFFF7EC);
#endif

	paddr[0] = 2;
	paddr[1] = (mac_lo >> 0) & 0xff;
	paddr[2] = (mac_hi >> 24) & 0xff;
	paddr[3] = (mac_hi >> 16) & 0xff;
	paddr[4] = (mac_hi >> 8) & 0xff;
	paddr[5] = (mac_hi >> 0) & 0xff;

	DEBUG_PRINTF("%02x:%02x:%02x:%02x:%02x:%02x", paddr[0], paddr[1], paddr[2], paddr[3], paddr[4], paddr[5]);
}
