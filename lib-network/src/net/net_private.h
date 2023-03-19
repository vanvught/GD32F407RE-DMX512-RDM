/**
 * @file net_private.h
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

#ifndef NET_PRIVATE_H_
#define NET_PRIVATE_H_

#include <cstdint>

#include "net_packets.h"

#ifndef ALIGNED
# define ALIGNED __attribute__ ((aligned (4)))
#endif

#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int console_error(const char *);

extern void emac_eth_send(void *, int);
extern int emac_eth_recv(uint8_t **);
extern void emac_free_pkt(void);

#ifdef __cplusplus
}
#endif

void net_handle();

uint16_t net_chksum(const void *, uint32_t);
void net_timers_run();

void arp_init();
void arp_handle(struct t_arp *);
void arp_cache_init();
void arp_send_request(uint32_t);
void arp_cache_update(const uint8_t *, uint32_t);
uint32_t arp_cache_lookup(uint32_t ip, uint8_t *);

void ip_init();
void ip_set_ip();
void ip_shutdown();
void ip_handle(struct t_ip4 *);

int dhcp_client(const char *);
void dhcp_client_release();

bool rfc3927();

void udp_init();
void udp_set_ip();
void udp_handle(struct t_udp *);
void udp_shutdown();

void igmp_init();
void igmp_set_ip();
void igmp_handle(struct t_igmp *);
void igmp_shutdown();
void igmp_timer();

void icmp_handle(struct t_icmp *);
void icmp_shutdown();

void tcp_init();
void tcp_run();
void tcp_handle(struct t_tcp *);

#endif /* NET_PRIVATE_H_ */
