/**
 * @file arp_cache.cpp
 *
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
#include <cstring>
#include <cassert>

#include "net_private.h"
#include "net_packets.h"
#include "net_platform.h"
#include "net_debug.h"

#include "../../config/net_config.h"

#define MAX_RECORDS	32

struct t_arp_record {
	uint32_t ip;
	uint8_t mac_address[ETH_ADDR_LEN];
} ALIGNED;

typedef union pcast32 {
	uint32_t u32;
	uint8_t u8[4];
} _pcast32;

static struct t_arp_record s_arp_records[MAX_RECORDS] SECTION_NETWORK ALIGNED;
static uint16_t s_entry_current SECTION_NETWORK ALIGNED;

#ifndef NDEBUG
# define TICKER_COUNT 100	///< 10 seconds
  static volatile uint32_t s_ticker ;
#endif

void __attribute__((cold)) arp_cache_init() {
	s_entry_current = 0;

	for (auto i = 0; i < MAX_RECORDS; i++) {
		s_arp_records[i].ip = 0;
		memset(s_arp_records[i].mac_address, 0, ETH_ADDR_LEN);
	}

#ifndef NDEBUG
	s_ticker = TICKER_COUNT;
#endif
}

void arp_cache_update(const uint8_t *pMacAddress, uint32_t nIp) {
	DEBUG_ENTRY

	if (s_entry_current == MAX_RECORDS) {
		console_error("arp_cache_update\n");
		return;
	}

	for (auto i = 0; i < s_entry_current; i++) {
		if (s_arp_records[i].ip == nIp) {
			return;
		}
	}

	memcpy(s_arp_records[s_entry_current].mac_address, pMacAddress, ETH_ADDR_LEN);
	s_arp_records[s_entry_current].ip = nIp;

	s_entry_current++;

	DEBUG_EXIT
}

uint32_t arp_cache_lookup(uint32_t nIp, uint8_t *pMacAddress) {
	DEBUG_ENTRY
	DEBUG_PRINTF(IPSTR " " MACSTR, IP2STR(nIp), MAC2STR(pMacAddress));

	for (auto i = 0; i < MAX_RECORDS; i++) {
		if (s_arp_records[i].ip == nIp) {
			memcpy(pMacAddress, s_arp_records[i].mac_address, ETH_ADDR_LEN);
			return nIp;
		}

		if (s_arp_records[i].ip == 0) {
			break;
		}
	}

	const auto current_entry = s_entry_current;
	int32_t timeout;
	auto retries = 3;

	while (retries--) {
		arp_send_request(nIp);

		timeout = 0x1FFFF;

		while ((timeout-- > 0) && (current_entry == s_entry_current)) {
			net_handle();
		}

		if (current_entry != s_entry_current) {
			memcpy(pMacAddress, s_arp_records[current_entry].mac_address, ETH_ADDR_LEN);
			DEBUG_PRINTF("timeout=%x", timeout);
			return nIp;
		}

		DEBUG_PRINTF("i=%d, timeout=%d, current_entry=%d, s_entry_current=%d", i, timeout, current_entry, s_entry_current);
	}

	DEBUG_EXIT
	return 0;
}

void arp_cache_dump() {
#ifndef NDEBUG
	printf("ARP Cache size=%d\n", s_entry_current);

	for (auto i = 0; i < s_entry_current; i++) {
		printf("%02d " IPSTR " " MACSTR "\n", i, IP2STR(s_arp_records[i].ip),MAC2STR(s_arp_records[i].mac_address));
	}
#endif
}

#ifndef NDEBUG
void arp_cache_timer(void) {
	s_ticker--;

	if (s_ticker == 0) {
		s_ticker = TICKER_COUNT;
		arp_cache_dump();
	}
}
#endif

