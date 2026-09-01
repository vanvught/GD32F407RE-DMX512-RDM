/**
 * @file scenes.cpp
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

#include <cstdint>
#include <cassert>

#include "spi/spi_flash.h"
#include "dmxnode.h"
#include "dmxnode_debug.h"

namespace dmxnode::scenes {
static bool s_has_flash;
static uint32_t s_offset_base;

static bool CheckHaveFlash() {
    DMXNODE_DEBUG_ENTRY();
    DMXNODE_DEBUG_PRINTF("s_hasFlash=%d", s_has_flash);

    if (!s_has_flash) {
        if (!spi::flash::Probe()) {
            DMXNODE_DEBUG_EXIT();
            return false;
        }

        const auto kEraseSize = spi::flash::SectorSize();
        assert(kEraseSize <= dmxnode::scenes::kBytesNeeded);
        const auto kPages = 1 + (dmxnode::scenes::kBytesNeeded / kEraseSize);

        DMXNODE_DEBUG_PRINTF("Bytes needed=%u, nEraseSize=%u, nPages=%u", dmxnode::scenes::kBytesNeeded, kEraseSize, kPages);

        assert(((kPages + 1) * kEraseSize) <= spi::flash::get_size());

        s_offset_base = spi::flash::Size() - ((kPages + 1) * kEraseSize);

        DMXNODE_DEBUG_PRINTF("nOffsetBase=%p", s_offset_base);
    }

    DMXNODE_DEBUG_EXIT();
    return true;
}

void WriteStart() {
    DMXNODE_DEBUG_ENTRY();
    DMXNODE_DEBUG_PRINTF("s_hasFlash=%d", s_has_flash);

    if (!CheckHaveFlash()) {
        DMXNODE_DEBUG_EXIT();
        return;
    }

    s_has_flash = spi::flash::cmd::Erase(s_offset_base, spi::flash::SectorSize());

    DMXNODE_DEBUG_PRINTF("s_hasFlash=%d", s_has_flash);
    DMXNODE_DEBUG_EXIT();
}

void Write(uint32_t port_index, std::span<const uint8_t> data) {
    DMXNODE_DEBUG_ENTRY();
    assert(port_index < dmxnode::kMaxPorts);
    assert(data.size() >= dmxnode::kUniverseSize);

    if (!s_has_flash) {
        DMXNODE_DEBUG_EXIT();
        return;
    }

    const auto kOffset = s_offset_base + (port_index * dmxnode::kUniverseSize);

    DMXNODE_DEBUG_PRINTF("s_offset_base=%p, offset=%p", reinterpret_cast<void*>(s_offset_base), reinterpret_cast<void*>(offset));

    spi::flash::cmd::Write(kOffset, data.first(dmxnode::kUniverseSize));

    DMXNODE_DEBUG_EXIT();
}

void WriteEnd() {
    DMXNODE_DEBUG_ENTRY();

    // No code needed here

    DMXNODE_DEBUG_EXIT();
}

void ReadStart() {
    DMXNODE_DEBUG_ENTRY();
    DMXNODE_DEBUG_PRINTF("s_has_flash=%d", s_has_flash);

    if (!CheckHaveFlash()) {
        DMXNODE_DEBUG_EXIT();
        return;
    }

    s_has_flash = true;

    DMXNODE_DEBUG_EXIT();
}

void Read(uint32_t port_index, std::span<uint8_t> data) {
    DMXNODE_DEBUG_ENTRY();
    assert(port_index < dmxnode::kMaxPorts);
    assert(data.size() >= dmxnode::kUniverseSize);

    if (!s_has_flash) {
        DMXNODE_DEBUG_EXIT();
        return;
    }

    const auto kOffset = s_offset_base + (port_index * dmxnode::kUniverseSize);

    DMXNODE_DEBUG_PRINTF("s_offset_base=%p, offset=%u", reinterpret_cast<void*>(s_offset_base), static_cast<unsigned>(offset));

    spi::flash::cmd::Read(kOffset, data.first(dmxnode::kUniverseSize));

    DMXNODE_DEBUG_EXIT();
}

void ReadEnd() {
    DMXNODE_DEBUG_ENTRY();

    // No code needed here

    DMXNODE_DEBUG_EXIT();
}
} // namespace dmxnode::scenes
