/**
 * @file emac_phy.cpp
 *
 */
/* Copyright (C) 2023-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef ENET_LINK_CHECK_REG_POLL
#error Register poll must be enabled
#endif // ENET_LINK_CHECK_REG_POLL

#include <cstdint>

#include "emac/emac_phy.h"
#include "emac/mmi.h"
#include "emac/emac_debug.h"

#define PHY_REG_MICR 0x11U
#define PHY_REG_MISR 0x12U
#define PHY_INT_AND_OUTPUT_ENABLE 0x03U
#define PHY_LINK_INT_ENABLE 0x20U

#if !defined(BIT)
#define BIT(x) static_cast<uint16_t>(1U << (x))
#endif // BIT

namespace emac::phy {
void CustomizedLed() {
    EMAC_PHY_DEBUG_ENTRY();

    EMAC_PHY_DEBUG_EXIT();
}

void CustomizedTiming() {
    EMAC_PHY_DEBUG_ENTRY();

    EMAC_PHY_DEBUG_EXIT();
}

// PHY Status Register (PHYSTS), address 10h

void CustomizedStatus(phy::Status& phy_status) {
    uint16_t value;
    phy::Read(emac::phy::kAddress, 0x10, value);

    phy_status.link = ((value & BIT(0)) == BIT(0)) ? phy::Link::kStateUp : phy::Link::kStateDown;
    phy_status.duplex = ((value & BIT(2)) == BIT(2)) ? phy::Duplex::kDuplexFull : phy::Duplex::kDuplexHalf;
    phy_status.speed = ((value & BIT(1)) == BIT(1)) ? phy::Speed::kSpeed10 : phy::Speed::kSpeed100;
    phy_status.autonegotiation = ((value & BIT(4)) == BIT(4));
}

namespace link {
// ENET_LINK_CHECK_USE_INT
// ENET_LINK_CHECK_USE_PIN_POLL
void PinEnable() {
    uint16_t phy_value = PHY_INT_AND_OUTPUT_ENABLE;
    phy::Write(emac::phy::kAddress, PHY_REG_MICR, phy_value);

    phy::Read(emac::phy::kAddress, PHY_REG_MICR, phy_value);

    if (PHY_INT_AND_OUTPUT_ENABLE != phy_value) {
        DEBUG_PUTS("PHY_INT_AND_OUTPUT_ENABLE != phy_value");
    }

    phy_value = PHY_LINK_INT_ENABLE;
    phy::Write(emac::phy::kAddress, PHY_REG_MISR, phy_value);
}

void PinRecovery() {
    uint16_t phy_value;
    phy::Read(emac::phy::kAddress, PHY_REG_MISR, phy_value);
    phy::Read(emac::phy::kAddress, mmi::REG_BMSR, phy_value);
}
} // namespace link
} // namespace emac::phy
