/**
 * net_phy.cpp
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

#if defined(DEBUG_NET_PHY)
#undef NDEBUG
#endif

#include <cstdint>

#include "emac/phy.h"
#include "emac/mmi.h"
#include "gd32.h"
#include "gd32_millis.h"
#include "firmware/debug/debug_debug.h"

namespace net::phy
{
bool Read(uint32_t address, uint32_t reg, uint16_t& value)
{
#if defined(GD32H7XX)
    const auto kResult = enet_phy_write_read(ENETx, ENET_PHY_READ, address, reg, &value) == SUCCESS;
#else
    const auto kResult = enet_phy_write_read(ENET_PHY_READ, address, reg, &value) == SUCCESS;
#endif
    return kResult;
}

bool Write(uint32_t address, uint32_t reg, uint16_t value)
{
#if defined(GD32H7XX)
    const auto kResult = enet_phy_write_read(ENETx, ENET_PHY_WRITE, address, reg, &value) == SUCCESS;
#else
    const auto kResult = enet_phy_write_read(ENET_PHY_WRITE, address, reg, &value) == SUCCESS;
#endif
    return kResult;
}

bool Config(uint32_t address)
{
    DEBUG_ENTRY();

#if defined(GD32H7XX)
    auto reg = ENET_MAC_PHY_CTL(ENETx);
#else
    auto reg = ENET_MAC_PHY_CTL;
#endif
    reg &= ~ENET_MAC_PHY_CTL_CLR;

    const auto kAhbClk = rcu_clock_freq_get(CK_AHB);

    DEBUG_PRINTF("kAhbClk=%u", kAhbClk);

#if defined GD32F10X_CL
    if (ENET_RANGE(kAhbClk, 20000000U, 35000000U))
    {
        reg |= ENET_MDC_HCLK_DIV16;
    }
    else if (ENET_RANGE(kAhbClk, 35000000U, 60000000U))
    {
        reg |= ENET_MDC_HCLK_DIV26;
    }
    else if (ENET_RANGE(kAhbClk, 60000000U, 90000000U))
    {
        reg |= ENET_MDC_HCLK_DIV42;
    }
    else if ((ENET_RANGE(kAhbClk, 90000000U, 108000000U)) || (108000000U == kAhbClk))
    {
        reg |= ENET_MDC_HCLK_DIV62;
    }
    else
    {
        DEBUG_EXIT();
        return false;
    }
#elif defined GD32F20X
    if (ENET_RANGE(kAhbClk, 20000000U, 35000000U))
    {
        reg |= ENET_MDC_HCLK_DIV16;
    }
    else if (ENET_RANGE(kAhbClk, 35000000U, 60000000U))
    {
        reg |= ENET_MDC_HCLK_DIV26;
    }
    else if (ENET_RANGE(kAhbClk, 60000000U, 100000000U))
    {
        reg |= ENET_MDC_HCLK_DIV42;
    }
    else if ((ENET_RANGE(kAhbClk, 100000000U, 120000000U)) || (120000000U == kAhbClk))
    {
        reg |= ENET_MDC_HCLK_DIV62;
    }
    else
    {
        DEBUG_EXIT();
        return false;
    }
#elif defined GD32F4XX
    if (ENET_RANGE(kAhbClk, 20000000U, 35000000U))
    {
        reg |= ENET_MDC_HCLK_DIV16;
    }
    else if (ENET_RANGE(kAhbClk, 35000000U, 60000000U))
    {
        reg |= ENET_MDC_HCLK_DIV26;
    }
    else if (ENET_RANGE(kAhbClk, 60000000U, 100000000U))
    {
        reg |= ENET_MDC_HCLK_DIV42;
    }
    else if (ENET_RANGE(kAhbClk, 100000000U, 150000000U))
    {
        reg |= ENET_MDC_HCLK_DIV62;
    }
    else if ((ENET_RANGE(kAhbClk, 150000000U, 240000000U)) || (240000000U == kAhbClk))
    {
        reg |= ENET_MDC_HCLK_DIV102;
    }
    else
    {
        DEBUG_EXIT();
        return false;
    }
#elif defined GD32H7XX
    if (ENET_RANGE(kAhbClk, 20000000U, 35000000U))
    {
        reg |= ENET_MDC_HCLK_DIV16;
    }
    else if (ENET_RANGE(kAhbClk, 35000000U, 60000000U))
    {
        reg |= ENET_MDC_HCLK_DIV26;
    }
    else if (ENET_RANGE(kAhbClk, 60000000U, 100000000U))
    {
        reg |= ENET_MDC_HCLK_DIV42;
    }
    else if (ENET_RANGE(kAhbClk, 100000000U, 150000000U))
    {
        reg |= ENET_MDC_HCLK_DIV62;
    }
    else if ((ENET_RANGE(kAhbClk, 150000000U, 180000000U)) || (180000000U == kAhbClk))
    {
        reg |= ENET_MDC_HCLK_DIV102;
    }
    else if (ENET_RANGE(kAhbClk, 250000000U, 300000000U))
    {
        reg |= ENET_MDC_HCLK_DIV124;
    }
    else if (ENET_RANGE(kAhbClk, 300000000U, 350000000U))
    {
        reg |= ENET_MDC_HCLK_DIV142;
    }
    else if ((ENET_RANGE(kAhbClk, 350000000U, 400000000U)) || (400000000U == kAhbClk))
    {
        reg |= ENET_MDC_HCLK_DIV162;
    }
    else
    {
        DEBUG_EXIT();
        return false;
    }
#else
#error
#endif

#if defined(GD32H7XX)
    ENET_MAC_PHY_CTL(ENETx) = reg;
#else
    ENET_MAC_PHY_CTL = reg;
#endif

    if (!phy::Write(address, mmi::REG_BMCR, mmi::BMCR_RESET))
    {
        DEBUG_PUTS("PHY reset failed");
        DEBUG_EXIT();
        return false;
    }

    /*
     * Poll the control register for the reset bit to go to 0 (it is
     * auto-clearing).  This should happen within 0.5 seconds per the
     * IEEE spec.
     */

    const auto kMillis = millis();
    uint16_t value;

    while (millis() - kMillis < 500U)
    {
        if (!phy::Read(address, mmi::REG_BMCR, value))
        {
            DEBUG_PUTS("PHY status read failed");
            DEBUG_EXIT();
            return false;
        }

        if (!(value & mmi::BMCR_RESET))
        {
            DEBUG_PRINTF("%u", millis() - kMillis);
            DEBUG_EXIT();
            return true;
        }
    }

    if (value & mmi::BMCR_RESET)
    {
        DEBUG_PUTS("PHY reset timed out");
        DEBUG_EXIT();
        return false;
    }

    DEBUG_PRINTF("%u", millis() - kMillis);
    DEBUG_EXIT();
    return true;
}
} // namespace net::phy
