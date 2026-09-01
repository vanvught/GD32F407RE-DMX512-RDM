/**
 * @file utils_float.h
 *
 */
/* Copyright (C) 2025-2026 by Arjan van Vught mailto:info@gd32-dmx.org */

#ifndef COMMON_UTILS_UTILS_FLOAT_H_
#define COMMON_UTILS_UTILS_FLOAT_H_

#include <cstdint>

namespace common {
inline void FloatCopyTo(uint8_t (&out)[4], float f) noexcept {
    static_assert(sizeof(float) == 4, "Requires 32-bit float");
    __builtin_memcpy(out, &f, sizeof(f));
}

inline float FloatCopyFrom(const uint8_t (&in)[4]) noexcept {
    static_assert(sizeof(float) == 4, "Requires 32-bit float");
    float f;
    __builtin_memcpy(&f, in, sizeof(f));
    return f;
}
} // namespace common

#endif // COMMON_UTILS_UTILS_FLOAT_H_
