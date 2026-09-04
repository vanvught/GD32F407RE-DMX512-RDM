/**
 * net_phy.cpp
 */

/* Copyright (C) 2023-2026 by Arjan van Vught mailto:info@gd32-dmx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
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
#include <cstdio>
#include "cassert"

#include "emac/emac.h"
#include "emac/emac_phy.h"
#include "core/netif.h"
#include "emac/mmi.h"
#include "emac/emac_debug.h"
#include "firmware/debug/debug_printbits.h"
#include "softwaretimers.h"

namespace network::global {
extern emac::phy::Link link_state;
}

namespace emac::phy {
constexpr const char* kSpeedNames[] = {"Unknown", "10baseT", "100baseTX", "1000baseT"};

static_assert(static_cast<size_t>(phy::Speed::kUnknown) == 0, "Enum ordering mismatch");
static_assert(static_cast<size_t>(phy::Speed::kSpeed1000) < (sizeof(kSpeedNames) / sizeof(kSpeedNames[0])), "Enum range mismatch");

auto s_timer_id = kTimerIdNone;

namespace {
bool ReadBmsr(uint16_t address, uint16_t& bmsr) {
    // BMSR link status is latch-low.
    // first read clears the latched state and the second read gives us
    // the current state.
    if (!phy::Read(address, mmi::REG_BMSR, bmsr)) {
        return false;
    }

    return phy::Read(address, mmi::REG_BMSR, bmsr);
}

int32_t ConfigAdvertisement(uint16_t address, uint16_t advertisement) {
    EMAC_PHY_DEBUG_ENTRY();

    uint16_t current;

    if (!phy::Read(address, mmi::REG_ADVERTISE, current)) {
        EMAC_PHY_DEBUG_EXIT();
        return -1;
    }

    if (current == advertisement) {
        EMAC_PHY_DEBUG_EXIT();
        return 0;
    }

    if (!phy::Write(address, mmi::REG_ADVERTISE, advertisement)) {
        EMAC_PHY_DEBUG_EXIT();
        return -1;
    }

    EMAC_PHY_DEBUG_EXIT();
    return 1;
}

bool RestartAutonegotiation(uint16_t address) {
    uint16_t bmcr;

    if (!phy::Read(address, mmi::REG_BMCR, bmcr)) {
        return false;
    }

    bmcr |= mmi::BMCR_AUTONEGOTIATION | mmi::BMCR_RESTART_AUTONEGOTIATION;

    bmcr &= static_cast<uint16_t>(~mmi::BMCR_ISOLATE);

    return phy::Write(address, mmi::REG_BMCR, bmcr);
}

bool ConfigAutonegotiation(uint16_t address, uint16_t advertisement) {
    EMAC_PHY_DEBUG_ENTRY();

    auto result = ConfigAdvertisement(address, advertisement);

    if (result < 0) {
        EMAC_PHY_DEBUG_EXIT();
        return false;
    }

    if (result == 0) {
        uint16_t bmcr;

        if (!phy::Read(address, mmi::REG_BMCR, bmcr)) {
            EMAC_PHY_DEBUG_EXIT();
            return false;
        }

        if (!(bmcr & mmi::BMCR_AUTONEGOTIATION) || (bmcr & mmi::BMCR_ISOLATE)) {
            result = 1;
        }
    }

    if (result > 0) {
        const auto kResult = RestartAutonegotiation(address);

        EMAC_PHY_DEBUG_EXIT();
        return kResult;
    }

    EMAC_PHY_DEBUG_EXIT();
    return true;
}

bool ParseLink(uint16_t address, Status& phy_status) {
    uint16_t advertise;

    if (!phy::Read(address, mmi::REG_ADVERTISE, advertise)) {
        return false;
    }

    debug::PrintBits(advertise, "advertise");

    uint16_t lpa;

    if (!phy::Read(address, mmi::REG_LPA, lpa)) {
        return false;
    }

    debug::PrintBits(lpa, "LPA");

    const auto kCommon = static_cast<uint16_t>(advertise & lpa);

    if (kCommon & mmi::LPA_100FULL) {
        phy_status.speed = Speed::kSpeed100;
        phy_status.duplex = Duplex::kDuplexFull;
    } else if (kCommon & mmi::LPA_100HALF) {
        phy_status.speed = Speed::kSpeed100;
        phy_status.duplex = Duplex::kDuplexHalf;
    } else if (kCommon & mmi::LPA_10FULL) {
        phy_status.speed = Speed::kSpeed10;
        phy_status.duplex = Duplex::kDuplexFull;
    } else if (kCommon & mmi::LPA_10HALF) {
        phy_status.speed = Speed::kSpeed10;
        phy_status.duplex = Duplex::kDuplexHalf;
    } else {
        phy_status.speed = Speed::kUnknown;
        phy_status.duplex = Duplex::kUnknown;
    }

    return true;
}

bool ReadStatus(uint16_t address, Status& phy_status) {
    uint16_t bmsr;

    if (!ReadBmsr(address, bmsr)) {
        return false;
    }

    debug::PrintBits(bmsr, "BMSR");

    phy_status.link = (bmsr & mmi::BMSR_LINKED_STATUS) ? Link::kStateUp : Link::kStateDown;

    phy_status.autonegotiation = (bmsr & mmi::BMSR_AUTONEGO_COMPLETE) != 0;

    if (phy_status.link == Link::kStateDown) {
        phy_status.speed = Speed::kUnknown;
        phy_status.duplex = Duplex::kUnknown;
        return true;
    }

    if (!phy_status.autonegotiation) {
        phy_status.speed = Speed::kUnknown;
        phy_status.duplex = Duplex::kUnknown;
        return true;
    }

    return ParseLink(address, phy_status);
}

} // namespace

bool GetId(uint16_t address, Identifier& phy_identifier) {
    EMAC_PHY_DEBUG_ENTRY();

    EMAC_PHY_DEBUG_PRINTF("address=%.2x", address);

    uint16_t value;

    if (!phy::Read(address, mmi::REG_PHYSID1, value)) {
        EMAC_PHY_DEBUG_EXIT();
        return false;
    }

    phy_identifier.oui = static_cast<uint32_t>(value) << 14;

    if (!phy::Read(address, mmi::REG_PHYSID2, value)) {
        EMAC_PHY_DEBUG_EXIT();
        return false;
    }

    phy_identifier.oui |= ((value & 0xfc00U) >> 10);
    phy_identifier.vendor_model = (value & 0x03f0U) >> 4;
    phy_identifier.model_revision = value & 0x000fU;

    EMAC_PHY_DEBUG_PRINTF("%.8x %.4x %.4x", static_cast<unsigned>(phy_identifier.oui), static_cast<unsigned>(phy_identifier.vendor_model), static_cast<unsigned>(phy_identifier.model_revision));

    EMAC_PHY_DEBUG_EXIT();
    return true;
}

Link GetLink(uint16_t address) {
    uint16_t bmsr;

    if (!ReadBmsr(address, bmsr)) {
        return Link::kStateDown;
    }

    return (bmsr & mmi::BMSR_LINKED_STATUS) ? Link::kStateUp : Link::kStateDown;
}

bool Powerdown(uint16_t address) {
    return phy::Write(address, mmi::REG_BMCR, mmi::BMCR_POWERDOWN);
}

bool Start(uint16_t address, Status& phy_status) {
    EMAC_PHY_DEBUG_ENTRY();

    phy_status = {.link = Link::kStateDown, .duplex = Duplex::kUnknown, .speed = Speed::kUnknown, .autonegotiation = false};

    constexpr auto kAdvertisement = mmi::ADVERTISE_ALL;

    if (!ConfigAutonegotiation(address, kAdvertisement)) {
        EMAC_PHY_DEBUG_EXIT();
        return false;
    }

    if (!ReadStatus(address, phy_status)) {
        EMAC_PHY_DEBUG_EXIT();
        return false;
    }

    EMAC_PHY_DEBUG_PRINTF("Link %s, %s, %s, autoneg %s", ToString(phy_status.link), ToString(phy_status.speed), ToString(phy_status.duplex), ToStringAutonegotiation(phy_status.autonegotiation));
    EMAC_PHY_DEBUG_EXIT();
    return true;
}

const char* ToString(phy::Link link) {
    return link == phy::Link::kStateUp ? "up" : "down";
}

const char* ToString(phy::Duplex duplex) {
    switch (duplex) {
        case phy::Duplex::kUnknown:
            return "unknown";
        case phy::Duplex::kDuplexHalf:
            return "half";
        case phy::Duplex::kDuplexFull:
            return "full";
    }

    return "error";
}

const char* ToString(phy::Speed speed) {
    const auto kIndex = static_cast<size_t>(speed);

    if (kIndex < sizeof(kSpeedNames) / sizeof(kSpeedNames[0])) {
        return kSpeedNames[kIndex];
    }

    return "error";
}

const char* ToStringAutonegotiation(bool autonegotiation) {
    return autonegotiation ? "complete" : "incomplete";
}

namespace link {
void HandleChange() {
    EMAC_PHY_DEBUG_ENTRY();

    phy::Status status{};

    if (!phy::ReadStatus(phy::kAddress, status)) {
        EMAC_PHY_DEBUG_PRINTF("Unable to read PHY status");
        EMAC_PHY_DEBUG_EXIT();
        return;
    }

    printf("Event link %s\n", emac::phy::ToString(status.link));

    if (status.link == phy::Link::kStateDown) {
        netif::SetLinkDown();
        EMAC_PHY_DEBUG_EXIT();
        return;
    }

    if (!status.autonegotiation) {
        EMAC_PHY_DEBUG_PRINTF("Link up, autonegotiation incomplete");
        EMAC_PHY_DEBUG_EXIT();
        return;
    }

    EMAC_PHY_DEBUG_PRINTF("Link %s, %s, %s", emac::phy::ToString(status.link), emac::phy::ToString(status.speed), emac::phy::ToString(status.duplex));

    emac::AdjustLink(status);

    netif::SetLinkUp();
    EMAC_PHY_DEBUG_EXIT();
}

void LinkPollTimer([[maybe_unused]] TimerHandle_t handle) {
#ifdef ENET_LINK_CHECK_USE_PIN_POLL
    PinPoll();
#elifdef ENET_LINK_CHECK_REG_POLL
    const emac::phy::Link link_state = GetLink(emac::phy::kAddress);
    if (link_state != network::global::link_state) {
        network::global::link_state = link_state;
        HandleChange();
    }
#endif // ENET_LINK_CHECK_USE_PIN_POLL
}

void Init() {
#ifdef ENET_LINK_CHECK_USE_INT
    InterruptInit();
#elifdef ENET_LINK_CHECK_USE_PIN_POLL
    PinPollInit();
#elifdef ENET_LINK_CHECK_REG_POLL
    GetLink(emac::phy::kAddress);
#endif // ENET_LINK_CHECK_USE_INT

#if defined(ENET_LINK_CHECK_USE_PIN_POLL) || defined(ENET_LINK_CHECK_REG_POLL)
    if (s_timer_id != kTimerIdNone) {
        SoftwareTimerDelete(s_timer_id);
    }
    s_timer_id = SoftwareTimerAdd(1000, LinkPollTimer);
    assert(s_timer_id >= 0);
#endif
}

#ifdef ENET_LINK_CHECK_USE_INT
void InterruptInit() {
    EMAC_PHY_DEBUG_ENTRY();

    link::PinEnable();
    link::PinRecovery();
    link::GpioInit();
    link::ExtiInit();

    EMAC_PHY_DEBUG_EXIT();
}
#endif // ENET_LINK_CHECK_USE_INT

#ifdef ENET_LINK_CHECK_USE_PIN_POLL
void PinPollInit() {
    EMAC_PHY_DEBUG_ENTRY();

    link::PinEnable();
    link::PinRecovery();
    link::GpioInit();

    EMAC_PHY_DEBUG_EXIT();
}
#endif // ENET_LINK_CHECK_USE_PIN_POLL
} // namespace link
} // namespace emac::phy