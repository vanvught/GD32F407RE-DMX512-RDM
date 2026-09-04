/**
 * @file emac_phy.h
 *
 */
/* Copyright (C) 2023-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef EMAC_PHY_H_
#define EMAC_PHY_H_

#include <cstdint>

namespace emac::phy {
static constexpr uint16_t kAddress =
#ifndef PHY_ADDRESS
    1;
#else
    PHY_ADDRESS;
#endif // PHY_ADDRESS

enum class Link { kStateDown, kStateUp };
enum class Duplex { kUnknown, kDuplexHalf, kDuplexFull };
enum class Speed { kUnknown, kSpeed10, kSpeed100, kSpeed1000 };

struct Status {
    Link link;
    Duplex duplex;
    Speed speed;
    bool autonegotiation;
};

struct Identifier {
    uint32_t oui;            ///< 24-bit Organizationally Unique Identifier.
    uint16_t vendor_model;   ///< 6-bit Manufacturer’s model number.
    uint16_t model_revision; ///< 4-bit Manufacturer’s revision number.
};

// Generic implementation
bool GetId(uint16_t address, Identifier& phy_identifier);
Link GetLink(uint16_t address);
bool Powerdown(uint16_t address);
bool Start(uint16_t address, Status& phy_status);

const char* ToString(Link link);
const char* ToString(Duplex duplex);
const char* ToString(Speed speed);
const char* ToStringAutonegotiation(bool autonegotiation);

// Platform Platform implementation
bool Read(uint16_t address, uint16_t reg, uint16_t& value);
bool Write(uint16_t address, uint16_t reg, uint16_t value);
// PHY interface configuration (configure SMI and reset PHY)
bool Config(uint16_t address);

void CustomizedLed();
void CustomizedTiming();
void CustomizedStatus(Status& phy_status);

namespace link {
// Generic implementation
void Init();
void HandleChange();
// Platform defined implementations
// #if defined(ENET_LINK_CHECK_USE_INT) || defined(ENET_LINK_CHECK_USE_PIN_POLL)
void GpioInit();
void PinEnable();
void PinRecovery();
// #endif
// #if defined(ENET_LINK_CHECK_USE_INT)
void ExtiInit();
void InterruptInit();
// #elif defined(ENET_LINK_CHECK_USE_PIN_POLL)
void PinPollInit();
void PinPoll();
// #endif
} // namespace link
} // namespace emac::phy

#endif // EMAC_PHY_H_
