/**
 * NetworkInit.cpp
 *
 */
/* Copyright (C) 2025-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include "emac/emac.h"
#include "emac/emac_phy.h"
#include "emac/network.h"
#include "../src/core/network_private.h"
#include "core/ip4/dhcp.h"
#include "core/ip4/arp.h"
#include "core/netif.h"
#if defined(CONFIG_NET_ENABLE_NTP_CLIENT) || defined(CONFIG_NET_ENABLE_PTP_NTP_CLIENT)
#include "apps/ntpclient.h"
#endif // defined(CONFIG_NET_ENABLE_NTP_CLIENT) || defined(CONFIG_NET_ENABLE_PTP_NTP_CLIENT)
#if !defined(CONFIG_NET_APPS_NO_MDNS)
#include "apps/mdns.h"
#endif // CONFIG_NET_APPS_NO_MDNS
#include "network_event.h"
#include "common/utils/utils_flags.h"
#include "configstore.h"
#include "apps/mdns.h"
#include "network_store.h"
#include "configurationstore.h"
#include "firmware/debug/debug_debug.h"

#ifdef DEBUG_NETWORK
#define NETWORK_DEBUG_ENTRY() DEBUG_ENTRY()
#define NETWORK_DEBUG_EXIT() DEBUG_EXIT()
#define NETWORK_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define NETWORK_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define NETWORK_DEBUG_ENTRY() \
    do {                      \
    } while (false)
#define NETWORK_DEBUG_EXIT() \
    do {                     \
    } while (false)
#define NETWORK_DEBUG_PRINTF(...) \
    do {                          \
    } while (false)
#define NETWORK_DEBUG_PUTS(...) \
    do {                        \
    } while (false)
#endif // DEBUG_NETWORK

#if !defined(PHY_ADDRESS)
#define PHY_ADDRESS 1
#endif // PHY_ADDRESS

using common::store::network::Flags;

namespace net {
#if defined(CONFIG_NET_ENABLE_PTP)
__attribute__((weak)) void ptp_init() {}
#endif // CONFIG_NET_ENABLE_PTP

} // namespace net

namespace network {
namespace global {
emac::phy::Link link_state;
uint32_t broadcast_mask;
uint32_t on_network_mask;
} // namespace global

void Set(ip4_addr_t ipaddr, ip4_addr_t netmask, ip4_addr_t gw, bool use_dhcp);

static void NetifExtCallback(uint16_t reason, [[maybe_unused]] const netif::netif_ext_callback_args_t* args) {
    NETWORK_DEBUG_ENTRY();

    if ((reason & netif::NetifReason::kIpv4AddressChanged) == netif::NetifReason::kIpv4AddressChanged) {
        printf("ip: " IPSTR " -> " IPSTR "\n", IP2STR(args->ipv4_changed.old_address.addr), IP2STR(netif::IpAddr()));

        network::event::Ipv4AddressChanged();
#if defined(CONFIG_NET_ENABLE_NTP_CLIENT)
        network::apps::ntpclient::Start();
#endif // CONFIG_NET_ENABLE_NTP_CLIENT
#if defined(CONFIG_NET_ENABLE_PTP_NTP_CLIENT)
        network::apps::ntpclient::ptp::Start();
#endif // CONFIG_NET_ENABLE_PTP_NTP_CLIENT
#if !defined(CONFIG_NET_APPS_NO_MDNS)
        network::apps::mdns::Start();
#endif // CONFIG_NET_APPS_NO_MDNS
    }

    if ((reason & netif::NetifReason::kIpv4NetmaskChanged) == netif::NetifReason::kIpv4NetmaskChanged) {
        printf("netmask: " IPSTR " -> " IPSTR "\n", IP2STR(args->ipv4_changed.old_netmask.addr), IP2STR(netif::Netmask()));

        network::event::Ipv4NetmaskChanged();
    }

    if ((reason & netif::NetifReason::kIpv4GatewayChanged) == netif::NetifReason::kIpv4GatewayChanged) {
        printf("gw: " IPSTR " -> " IPSTR "\n", IP2STR(args->ipv4_changed.old_gw.addr), IP2STR(netif::Gw()));

        network::event::Ipv4GatewayChanged();
    }

    if ((reason & netif::NetifReason::kLinkChanged) == netif::NetifReason::kLinkChanged) {
        if (args->link_changed.state == 0) { // Link down
            network::event::LinkDown();
            NETWORK_DEBUG_EXIT();
            return;
        }

        network::event::LinkUp();
        NETWORK_DEBUG_EXIT();
    }

    NETWORK_DEBUG_EXIT();
}
void Init() {
    NETWORK_DEBUG_ENTRY();

    emac::display::Config();
    emac::Config();

    emac::phy::CustomizedTiming();
    emac::phy::CustomizedLed();

    emac::display::Start();
    emac::Start(netif::global::netif_default.hwaddr, global::link_state);

    printf(MACSTR "\n", MAC2STR(netif::global::netif_default.hwaddr));

    emac::display::Status(emac::phy::Link::kStateUp == global::link_state);

    network::arp::Init();
    network::udp::Init();
    network::igmp::Init();

#ifdef ENABLE_HTTPD
    network::tcp::Init();
#endif // ENABLE_HTTPD

#ifdef CONFIG_NET_ENABLE_PTP
    net::ptp_init();
#endif // CONFIG_NET_ENABLE_PTP

#ifdef CONFIG_NET_ENABLE_NTP_CLIENT
    network::apps::ntpclient::Init();
#endif // CONFIG_NET_ENABLE_NTP_CLIENT

#ifdef CONFIG_NET_ENABLE_PTP_NTP_CLIENT
    network::apps::ntpclient::ptp::Init();
#endif // CONFIG_NET_ENABLE_PTP_NTP_CLIENT

#ifndef CONFIG_NET_APPS_NO_MDNS
    network::apps::mdns::Init();
#endif // CONFIG_NET_APPS_NO_MDNS

    netif::Init();
    netif::AddExtCallback(NetifExtCallback);

    network::ip4_addr_t ipaddr;
    network::ip4_addr_t netmask;
    network::ip4_addr_t gw;

    common::store::Network store;
    ConfigStore::Instance().Copy(&store, &ConfigurationStore::network);

    network::iface::SetHostname(reinterpret_cast<char*>(store.host_name));

    ipaddr.addr = ConfigStore::Instance().NetworkGet(&common::store::Network::local_ip);
    netmask.addr = ConfigStore::Instance().NetworkGet(&common::store::Network::netmask);
    gw.addr = ConfigStore::Instance().NetworkGet(&common::store::Network::gateway_ip);

    const auto kFlags = ConfigStore::Instance().NetworkGet(&common::store::Network::flags);

    network::Set(ipaddr, netmask, gw, !common::IsFlagSet(kFlags, Flags::Flag::kUseStaticIp));

    emac::phy::link::Init();
    NETWORK_DEBUG_EXIT();
}

static struct network::acd::Acd s_acd;

static void PrimaryIpConflictCallback(network::acd::Callback callback) {
    auto& netif = netif::global::netif_default;

    switch (callback) {
        case network::acd::Callback::kAcdIpOk:
            if (s_acd.ipaddr.addr == netif.secondary_ip.addr) {
                network::SetSecondaryIp();
            } else {
                netif::SetIpAddr(s_acd.ipaddr);
            }
            network::dhcp::Inform();
            netif::SetFlags(netif::Netif::kNetifFlagStaticipOk);
            break;
        case network::acd::Callback::kAcdRestartClient:
            break;
        case network::acd::Callback::kAcdDecline:
            netif::ClearFlags(netif::Netif::kNetifFlagStaticipOk);
            break;
        default:
            break;
    }
}

void Set(network::ip4_addr_t ipaddr, network::ip4_addr_t netmask, network::ip4_addr_t gw, bool use_dhcp) {
    NETWORK_DEBUG_ENTRY();

    netif::global::netif_default.secondary_ip.addr = 2 + ((static_cast<uint32_t>(static_cast<uint8_t>(netif::global::netif_default.hwaddr[3] + 0xFF + 0xFF))) << 8) + ((static_cast<uint32_t>(netif::global::netif_default.hwaddr[4])) << 16) +
                                                     ((static_cast<uint32_t>(netif::global::netif_default.hwaddr[5])) << 24);

    if (!use_dhcp) {
        network::acd::Add(&s_acd, PrimaryIpConflictCallback);

        if (ipaddr.addr != 0) {
            netif::SetNetmask(netmask);
            netif::SetGw(gw);
        }
    }

    if (emac::phy::Link::kStateUp == emac::phy::GetLink(PHY_ADDRESS)) {
        netif::SetFlags(netif::Netif::kNetifFlagLinkUp);
    } else {
        netif::ClearFlags(netif::Netif::kNetifFlagLinkUp);
    }

    if (use_dhcp) {
        network::dhcp::Start();
    } else {
        if (ipaddr.addr == 0) {
            network::acd::Start(&s_acd, netif::global::netif_default.secondary_ip);
        } else {
            network::acd::Start(&s_acd, ipaddr);
        }
    }

    NETWORK_DEBUG_EXIT();
}

void SetPrimaryIp(uint32_t ip) {
    NETWORK_DEBUG_ENTRY();

    auto& netif = netif::global::netif_default;

    if (ip == netif.ip.addr) {
        NETWORK_DEBUG_EXIT();
        return;
    }

    network::dhcp::ReleaseAndStop();

    network::store::SaveDhcp(false);

    network::acd::Add(&s_acd, PrimaryIpConflictCallback);

    if (ip == 0) {
        network::acd::Start(&s_acd, netif.secondary_ip);
    } else {
        network::ip_addr ipaddr;
        ipaddr.addr = ip;
        network::acd::Start(&s_acd, ipaddr);
    }

    network::store::SaveIp(ip);

    NETWORK_DEBUG_EXIT();
}

void SetSecondaryIp() {
    NETWORK_DEBUG_ENTRY();

    auto& netif = netif::global::netif_default;
    network::ip4_addr_t netmask;
    netmask.addr = 255;
    netif::SetAddr(netif.secondary_ip, netmask, netif.secondary_ip);

    NETWORK_DEBUG_EXIT();
}

void SetNetmask(uint32_t netmask_new) {
    NETWORK_DEBUG_ENTRY();

    if (netmask_new == netif::Netmask()) {
        NETWORK_DEBUG_EXIT();
        return;
    }

    network::ip4_addr_t netmask;
    netmask.addr = netmask_new;

    netif::SetNetmask(netmask);

    network::store::SaveNetmask(netmask_new);

    NETWORK_DEBUG_EXIT();
}

void SetGatewayIp(uint32_t gateway_ip) {
    NETWORK_DEBUG_ENTRY();

    if (gateway_ip == netif::Gw()) {
        NETWORK_DEBUG_EXIT();
        return;
    }

    network::ip4_addr_t gw;
    gw.addr = gateway_ip;

    netif::SetGw(gw);

    network::store::SaveGatewayIp(gateway_ip);

    NETWORK_DEBUG_EXIT();
}

void Shutdown() {
    NETWORK_DEBUG_ENTRY();

#ifndef CONFIG_NET_APPS_NO_MDNS
    network::apps::mdns::Stop();
#endif // CONFIG_NET_APPS_NO_MDNS
    network::igmp::Shutdown();
    netif::SetLinkDown();

    NETWORK_DEBUG_EXIT();
}
} // namespace network
