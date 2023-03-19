/**
 * @file net.cpp
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

#include "net.h"
#include "net_private.h"
#include "net_packets.h"
#include "net_debug.h"

#include "debug.h"

#include "../../config/net_config.h"

struct ip_info g_ip_info  __attribute__ ((aligned (4)));
uint8_t g_mac_address[ETH_ADDR_LEN] __attribute__ ((aligned (4)));

static uint8_t *s_p;
static bool s_isDhcp = false;

void __attribute__((cold)) net_init(const uint8_t *const pMacAddress, struct ip_info *p_ip_info, const char *pHostname, bool *bUseDhcp, bool *isZeroconfUsed) {
	DEBUG_ENTRY

	for (auto i = 0; i < ETH_ADDR_LEN; i++) {
		g_mac_address[i] = pMacAddress[i];
	}

	const auto *src = reinterpret_cast<const uint8_t *>(p_ip_info);
	auto *dst = reinterpret_cast<uint8_t *>(&g_ip_info);

	for (uint32_t i = 0; i < sizeof(struct ip_info); i++) {
		*dst++ = *src++;
	}

	ip_init();

	*isZeroconfUsed = false;

	if (*bUseDhcp) {
		if (dhcp_client(pHostname) < 0) {
			*bUseDhcp = false;
			DEBUG_PUTS("DHCP Client failed");
			*isZeroconfUsed = rfc3927();
		}
	}

	arp_init();
	ip_set_ip();
	tcp_init();

	src = reinterpret_cast<const uint8_t *>(&g_ip_info);
	dst = reinterpret_cast<uint8_t *>(p_ip_info);

	for (uint32_t i = 0; i < sizeof(struct ip_info); i++) {
		*dst++ = *src++;
	}

	s_isDhcp = *bUseDhcp;

	DEBUG_EXIT
}

void __attribute__((cold)) net_shutdown() {
	ip_shutdown();

	if (s_isDhcp) {
		dhcp_client_release();
	}
}

void net_set_ip(uint32_t ip) {
	g_ip_info.ip.addr = ip;

	arp_init();
	ip_set_ip();
}

void net_set_gw(uint32_t gw) {
	g_ip_info.gw.addr = gw;

	ip_set_ip();
}

bool net_set_dhcp(struct ip_info *p_ip_info, const char *const pHostname, bool *isZeroconfUsed) {
	bool isDhcp = false;
	*isZeroconfUsed = false;

	if (dhcp_client(pHostname) < 0) {
		DEBUG_PUTS("DHCP Client failed");
		*isZeroconfUsed = rfc3927();
	} else {
		isDhcp = true;
	}

	arp_init();
	ip_set_ip();

	const auto *pSrc = reinterpret_cast<const uint8_t *>(&g_ip_info);
	auto *pDst = reinterpret_cast<uint8_t *>(p_ip_info);

	for (uint32_t i = 0; i < sizeof(struct ip_info); i++) {
		*pDst++ = *pSrc++;
	}

	s_isDhcp = isDhcp;
	return isDhcp;
}

void net_dhcp_release() {
	dhcp_client_release();
	s_isDhcp = false;
}

bool net_set_zeroconf(struct ip_info *p_ip_info) {
	const auto b = rfc3927();

	if (b) {
		arp_init();
		ip_set_ip();

		const auto *pSrc = reinterpret_cast<const uint8_t *>(&g_ip_info);
		auto *pDst = reinterpret_cast<uint8_t *>(p_ip_info);

		for (uint32_t i = 0; i < sizeof(struct ip_info); i++) {
			*pDst++ = *pSrc++;
		}

		s_isDhcp = false;
		return true;
	}

	DEBUG_PUTS("Zeroconf failed");
	return false;
}

__attribute__((hot)) void net_handle() {
	const auto nLength = emac_eth_recv(&s_p);

	if (__builtin_expect((nLength > 0), 0)) {
		const auto *const eth = reinterpret_cast<struct ether_header *>( s_p);

		if (eth->type == __builtin_bswap16(ETHER_TYPE_IPv4)) {
			ip_handle(reinterpret_cast<struct t_ip4 *>( s_p));
		} else if (eth->type == __builtin_bswap16(ETHER_TYPE_ARP)) {
			arp_handle(reinterpret_cast<struct t_arp *>( s_p));
		} else {
			DEBUG_PRINTF("type %04x is not implemented", __builtin_bswap16(eth->type));
		}

		emac_free_pkt();
	}

	net_timers_run();
}
