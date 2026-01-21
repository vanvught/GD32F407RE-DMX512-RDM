/**
 * emac.cpp
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

#if defined(DEBUG_EMAC)
#undef NDEBUG
#endif

#include <cstdint>
#include <cstdio>

#include "gd32.h"
#include "emac/phy.h"
#if defined(CONFIG_NET_ENABLE_PTP)
#if !defined(DISABLE_RTC)
#include "hwclock.h"
#endif
#endif
#include "firmware/debug/debug_debug.h"

namespace console
{
void Error(const char*);
}

extern void EnetGpioConfig();
extern enet_descriptors_struct txdesc_tab[ENET_TXBUF_NUM];
extern void mac_address_get(uint8_t paddr[]);

#if defined(CONFIG_NET_ENABLE_PTP)
#include "gd32_ptp.h"
enet_descriptors_struct ptp_rxdesc_tab[ENET_RXBUF_NUM] __attribute__((aligned(4)));
enet_descriptors_struct ptp_txdesc_tab[ENET_TXBUF_NUM] __attribute__((aligned(4)));
#endif

/*
 * Public function
 */
namespace net::emac
{
void __attribute__((cold)) Config()
{
    DEBUG_ENTRY();
#if (PHY_TYPE == LAN8700)
    puts("LAN8700");
#elif (PHY_TYPE == DP83848)
    puts("DP83848");
#elif (PHY_TYPE == RTL8201F)
    puts("RTL8201F");
#else
#error PHY_TYPE is not set
#endif

    EnetGpioConfig();

    rcu_periph_clock_enable(RCU_ENET);
    rcu_periph_clock_enable(RCU_ENETTX);
    rcu_periph_clock_enable(RCU_ENETRX);

    enet_deinit(ENETx);
    enet_software_reset(ENETx);

    if (!net::phy::Config(PHY_ADDRESS))
    {
        console::Error("net::phy::Config(PHY_ADDRESS)\n");
    }

    DEBUG_EXIT();
}

void AdjustLink(net::phy::Status phy_status)
{
    DEBUG_ENTRY();

    printf("Link %s, %d, %s\n", phy_status.link == net::phy::Link::kStateUp ? "Up" : "Down", phy_status.speed == net::phy::Speed::kSpeed10 ? 10 : 100, phy_status.duplex == net::phy::Duplex::kDuplexHalf ? "HALF" : "FULL");

#ifndef NDEBUG
    {
        uint16_t phy_value;
#if defined(GD32H7XX)
        ErrStatus phy_state = enet_phy_write_read(ENETx, ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BCR, &phy_value);
#else
        ErrStatus phy_state = enet_phy_write_read(ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BCR, &phy_value);
#endif
        printf("BCR: %.4x %s\n", phy_value, phy_state == SUCCESS ? "SUCCES" : "ERROR");
#if defined(GD32H7XX)
        enet_phy_write_read(ENETx, ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BSR, &phy_value);
        phy_state = enet_phy_write_read(ENETx, ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BSR, &phy_value);
#else
        enet_phy_write_read(ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BSR, &phy_value);
        phy_state = enet_phy_write_read(ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BSR, &phy_value);
#endif
        printf("BSR: %.4x %s\n", phy_value & (PHY_AUTONEGO_COMPLETE | PHY_LINKED_STATUS | PHY_JABBER_DETECTION), phy_state == SUCCESS ? "SUCCES" : "ERROR");
    }
#endif

    enet_mediamode_enum mediamode = ENET_10M_HALFDUPLEX;

    if (phy_status.speed == net::phy::Speed::kSpeed100)
    {
        if (phy_status.duplex == net::phy::Duplex::kDuplexFull)
        {
            mediamode = ENET_100M_FULLDUPLEX;
        }
        else
        {
            mediamode = ENET_100M_HALFDUPLEX;
        }
    }
    else if (phy_status.duplex == net::phy::Duplex::kDuplexFull)
    {
        mediamode = ENET_10M_FULLDUPLEX;
    }

#if defined(GD32H7XX)
    const auto kEnetInitStatus = enet_init(ENETx, mediamode, ENET_AUTOCHECKSUM_DROP_FAILFRAMES, ENET_CUSTOM);
#else
    const auto kEnetInitStatus = enet_init(mediamode, ENET_AUTOCHECKSUM_DROP_FAILFRAMES, ENET_CUSTOM);
#endif

    if (kEnetInitStatus != SUCCESS)
    {
    }

    DEBUG_PRINTF("kEnetInitStatus=%s", kEnetInitStatus == SUCCESS ? "SUCCES" : "ERROR");

#ifndef NDEBUG
    {
        uint16_t phy_value;
#if defined(GD32H7XX)
        ErrStatus phy_state = enet_phy_write_read(ENETx, ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BCR, &phy_value);
#else
        ErrStatus phy_state = enet_phy_write_read(ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BCR, &phy_value);
#endif
        printf("BCR: %.4x %s\n", phy_value, phy_state == SUCCESS ? "SUCCES" : "ERROR");
#if defined(GD32H7XX)
        enet_phy_write_read(ENETx, ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BSR, &phy_value);
        phy_state = enet_phy_write_read(ENETx, ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BSR, &phy_value);
#else
        enet_phy_write_read(ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BSR, &phy_value);
        phy_state = enet_phy_write_read(ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BSR, &phy_value);
#endif
        printf("BSR: %.4x %s\n", phy_value & (PHY_AUTONEGO_COMPLETE | PHY_LINKED_STATUS | PHY_JABBER_DETECTION), phy_state == SUCCESS ? "SUCCES" : "ERROR");
    }
#endif
    DEBUG_EXIT();
}

void __attribute__((cold)) Start(uint8_t mac_address[], net::phy::Link& link)
{
    DEBUG_ENTRY();
    DEBUG_PRINTF("ENET_RXBUF_NUM=%u, ENET_TXBUF_NUM=%u", ENET_RXBUF_NUM, ENET_TXBUF_NUM);

    net::phy::Status phy_status;
    net::phy::Start(PHY_ADDRESS, phy_status);

    link = phy_status.link;

    AdjustLink(phy_status);

    mac_address_get(mac_address);

#if defined(GD32H7XX)
    enet_mac_address_set(ENETx, ENET_MAC_ADDRESS0, mac_address);
#if defined(CONFIG_NET_ENABLE_PTP)
    enet_ptp_normal_descriptors_chain_init(ENETx, ENET_DMA_TX, ptp_txdesc_tab);
    enet_ptp_normal_descriptors_chain_init(ENETx, ENET_DMA_RX, ptp_rxdesc_tab);
#else
    enet_descriptors_chain_init(ENETx, ENET_DMA_TX);
    enet_descriptors_chain_init(ENETx, ENET_DMA_RX);
#endif
#else
    enet_mac_address_set(ENET_MAC_ADDRESS0, mac_address);
#if defined(CONFIG_NET_ENABLE_PTP)
    enet_ptp_normal_descriptors_chain_init(ENET_DMA_TX, ptp_txdesc_tab);
    enet_ptp_normal_descriptors_chain_init(ENET_DMA_RX, ptp_rxdesc_tab);
#else
    enet_descriptors_chain_init(ENET_DMA_TX);
    enet_descriptors_chain_init(ENET_DMA_RX);
#endif
#endif

    for (uint32_t i = 0; i < ENET_TXBUF_NUM; i++)
    {
        enet_transmit_checksum_config(&txdesc_tab[i], ENET_CHECKSUM_TCPUDPICMP_FULL);
    }

#if defined(CONFIG_NET_ENABLE_PTP)
    gd32_ptp_start();
#if !defined(DISABLE_RTC)
    // Set the System Clock from the Hardware Clock
    HwClock::Get()->HcToSys();
#endif
#endif

    enet_enable(ENETx);

    DEBUG_EXIT();
}
} // namespace net::emac
