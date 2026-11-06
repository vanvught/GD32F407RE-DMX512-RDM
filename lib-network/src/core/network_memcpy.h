/**
 * @file network_memcpy.h
 *
 */
/* Copyright (C) 2021-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef NET_MEMCPY_H_
#define NET_MEMCPY_H_

#include <cstdint>
#include <cstring>

namespace network {
template <uint8_t V, size_t L>
inline void Memset(void* dest) {
    static_assert(L > 0, "Length must be greater than 0");
    // The compiler will inline small constants and call
    // the optimized library function for large blocks automatically.
    __builtin_memset(dest, V, L);
}

inline void MemcpyIp(uint8_t* pip, uint32_t nip) {
    // Both pip and &nip are safely treated as memory blocks.
    // The compiler knows exactly how to move 4 bytes based on core.
    __builtin_memcpy(pip, &nip, sizeof(uint32_t));
}

[[nodiscard]]
inline uint32_t MemcpyIp(const uint8_t* ip) {
    uint32_t nip;
    // The compiler knows exactly how to load 4 bytes safely
    __builtin_memcpy(&nip, ip, sizeof(uint32_t));
    return nip;
}
} // namespace network

#endif // NET_MEMCPY_H_
