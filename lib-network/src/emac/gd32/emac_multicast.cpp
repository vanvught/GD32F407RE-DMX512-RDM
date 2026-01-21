/**
 * @file emac_multicast.cpp
 *
 */
/* Copyright (C) 2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#if defined(DEBUG_EMAC_IGMP)
#undef NDEBUG
#endif

#include <cstdint>
#include <cstddef>

#include "gd32.h"
#include "gd32_enet.h"
#include "ip4/ip4_address.h"
#include "firmware/debug/debug_debug.h"

namespace network
{
uint32_t Crc(const uint8_t* data, size_t length);
}

namespace emac::multicast
{

void EnableHashFilter()
{
    DEBUG_ENTRY();

    Gd32EnetResetHash();
    Gd32EnetFilterFeatureDisable<ENET_MULTICAST_FILTER_PASS>();
    Gd32EenetFilterFeatureEnable<ENET_MULTICAST_FILTER_HASH_MODE>();

    DEBUG_EXIT();
}
void DisableHashFilter()
{
    DEBUG_ENTRY();

    Gd32EnetFilterFeatureDisable<ENET_MULTICAST_FILTER_HASH_MODE>();
    Gd32EenetFilterFeatureEnable<ENET_MULTICAST_FILTER_PASS>();

    DEBUG_EXIT();
}

void SetHash(const uint8_t* mac_addr)
{
    DEBUG_ENTRY();

    const auto kCrc = network::Crc(mac_addr, 6);
    const auto kHash = (kCrc >> 26) & 0x3F;

    Gd32EnetFilterSetHash(kHash);

    DEBUG_PRINTF("MAC: " MACSTR " -> CRC32: 0x%08X -> Hash Index: %d", MAC2STR(mac_addr), kCrc, kHash);
    DEBUG_EXIT();
}

void ResetHash()
{
    DEBUG_ENTRY();

    Gd32EnetResetHash();

    DEBUG_EXIT();
}
} // namespace emac::multicast