/**
 * @file net.h
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

#ifndef NET_H_
#define NET_H_

#include <cstdint>

struct ip_addr {
    uint32_t addr;
};

typedef struct ip_addr ip_addr_t;

struct ip_info {
    struct ip_addr ip;
    struct ip_addr netmask;
    struct ip_addr gw;
};

#define IP_BROADCAST	(0xFFFFFFFF)
#define HOST_NAME_MAX 	64	/* including a terminating null byte. */

extern void net_init(const uint8_t *const, struct ip_info *, const char *, bool *, bool *);
extern void net_shutdown();
extern void net_handle();

extern void net_set_ip(uint32_t);
extern void net_set_gw(uint32_t);
extern bool net_set_zeroconf(struct ip_info *);

extern bool net_set_dhcp(struct ip_info *, const char *const, bool *);
extern void net_dhcp_release();

extern int udp_begin(uint16_t);
extern int udp_end(uint16_t);
extern uint16_t udp_recv1(int, uint8_t *, uint16_t, uint32_t *, uint16_t *);
extern uint16_t udp_recv2(int, const uint8_t **, uint32_t *, uint16_t *);
extern int udp_send(int, const uint8_t *, uint16_t, uint32_t, uint16_t);

extern int igmp_join(uint32_t);
extern int igmp_leave(uint32_t);

extern int tcp_begin(const uint16_t);
extern uint16_t tcp_read(const int32_t, const uint8_t **, uint32_t &);
extern void tcp_write(const int32_t, const uint8_t *, uint16_t, const uint32_t);

#endif /* NET_H_ */
