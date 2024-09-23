/**
 * @file gd32_ptp.h
 *
 */
/* Copyright (C) 2024 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef GD32_PTP_H_
#define GD32_PTP_H_

#include <cstdint>

#include "gd32.h"

namespace gd32 {
namespace ptp {
#if !defined(MCU_CLOCK_FREQ)
# error MCU_CLOCK_FREQ is not defined
#endif
#if !defined(PTP_ACCARACY_NS)
 static constexpr uint8_t PTP_TICK = 20;
#else
 static constexpr uint8_t PTP_TICK = PTP_ACCARACY_NS;
#endif
static constexpr uint8_t  ADJ_FREQ_BASE_INCREMENT = static_cast<uint8_t>((PTP_TICK * static_cast<uint64_t>(1ULL << 31) / 1E9) + 0.5f);
static constexpr uint32_t ADJ_FREQ_BASE_ADDEND = (static_cast<uint64_t>(1ULL << 63) / AHB_CLOCK_FREQ) / ADJ_FREQ_BASE_INCREMENT;
static constexpr int32_t ADJ_FREQ_MAX  = 5120000;

struct time_t {
	int32_t tv_sec;
	int32_t tv_nsec;
};

struct ptptime {
	uint32_t tv_sec;
	uint32_t tv_nsec;
};
}  // namespace ptp

inline uint32_t ptp_nanosecond_2_subsecond(const uint32_t nanosecond) {
	uint64_t val = nanosecond * 0x80000000Ull;
	val /= 1000000000U;
	return static_cast<uint32_t>(val);
}

inline uint32_t ptp_subsecond_2_nanosecond(const uint32_t subsecond) {
	uint64_t val = subsecond * 1000000000Ull;
	val >>= 31U;
	return (uint32_t) val;
}

inline void normalize_time(ptp::time_t *r) {
	r->tv_sec += r->tv_nsec / 1000000000;
	r->tv_nsec -= r->tv_nsec / 1000000000 * 1000000000;

	if (r->tv_sec > 0 && r->tv_nsec < 0) {
		r->tv_sec -= 1;
		r->tv_nsec += 1000000000;
	} else if (r->tv_sec < 0 && r->tv_nsec > 0) {
		r->tv_sec += 1;
		r->tv_nsec -= 1000000000;
	}
}

inline void sub_time(struct ptp::time_t *r, const struct ptp::time_t *x, const struct ptp::time_t *y) {
    r->tv_sec = x->tv_sec - y->tv_sec;
    r->tv_nsec = x->tv_nsec - y->tv_nsec;

    normalize_time(r);
}

}  // namespace gd32

void gd32_ptp_start();
void gd32_ptp_get_time(gd32::ptp::ptptime *ptp_time);
void gd32_ptp_set_time(const gd32::ptp::ptptime *ptp_time);
void gd32_ptp_update_time(const gd32::ptp::time_t *ptp_time);
bool gd32_adj_frequency(const int32_t adjust_pbb);

#endif /* GD32_PTP_H_ */
