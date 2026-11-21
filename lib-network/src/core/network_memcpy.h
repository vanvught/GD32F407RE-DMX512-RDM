
/**
 * @file network_memcpy.h
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

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace network {

template <uint8_t V, size_t L>
inline void Memset(void* dest) {
    static_assert(L > 0, "Length must be greater than 0");

#ifdef __ARM_ARCH_7A__
    if ((reinterpret_cast<uintptr_t>(dest) & 3U) != 0) {
        auto* dst = reinterpret_cast<volatile uint8_t*>(dest);

        for (size_t i = 0; i < L; ++i) {
            dst[i] = V;
        }
        return;
    }
#endif

    __builtin_memset(dest, V, L);
}

inline void MemcpyIp(uint8_t* pip, uint32_t nip) {
#ifdef __ARM_ARCH_7A__
    if ((reinterpret_cast<uintptr_t>(pip) & 3U) != 0) {
        const auto* src = reinterpret_cast<const uint8_t*>(&nip);

        auto* dst = reinterpret_cast<volatile uint8_t*>(pip);

        for (size_t i = 0; i < sizeof(nip); ++i) {
            dst[i] = src[i];
        }
        return;
    }
#endif

    __builtin_memcpy(pip, &nip, sizeof(nip));
}

[[nodiscard]]
inline uint32_t MemcpyIp(const uint8_t* ip) {
    uint32_t nip;

#ifdef __ARM_ARCH_7A__
    if ((reinterpret_cast<uintptr_t>(ip) & 3U) != 0) {
        const auto* src = reinterpret_cast<const volatile uint8_t*>(ip);

        auto* dst = reinterpret_cast<uint8_t*>(&nip);

        for (size_t i = 0; i < sizeof(nip); ++i) {
            dst[i] = src[i];
        }
        return nip;
    }
#endif

    __builtin_memcpy(&nip, ip, sizeof(nip));
    return nip;
}

inline void CopyMacAddress(uint8_t* dst, const uint8_t* src) {
#ifdef __ARM_ARCH_7A__
    const auto* s = reinterpret_cast<const volatile uint8_t*>(src);

    auto* d = reinterpret_cast<volatile uint8_t*>(dst);

    for (size_t i = 0; i < 6; ++i) {
        d[i] = s[i];
    }
#else
    __builtin_memcpy(dst, src, 6);
#endif
}

} // namespace network

#endif // NET_MEMCPY_H_
