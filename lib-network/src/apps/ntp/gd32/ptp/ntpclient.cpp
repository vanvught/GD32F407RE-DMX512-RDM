/**
 * @file ntpclient.cpp
 * @brief Implementation of an NTP client with interleave mode for time synchronization in a standalone embedded environment.
 *
 * This file implements the functionality to synchronize system time using the Network Time Protocol (NTP).
 * It uses PTP to communicate with an NTP server and adjusts the system clock based on received timestamps.
 *
 * @note This implementation is optimized for Cortex-M3/M4/M7 processors and assumes a standalone environment
 *       without a standard library.
 */
/* Copyright (C) 2024-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

/**
 * https://www.ietf.org/archive/id/draft-ietf-ntp-interleaved-modes-07.html#name-interleaved-client-server-m
 */
/*
Server   t2   t3               t6   t7              t10  t11
    -----+----+----------------+----+----------------+----+-----
        /      \              /      \              /      \
Client /        \            /        \            /        \
    --+----------+----------+----------+----------+----------+--
      t1         t4         t5         t8         t9        t12

Mode: B         B           I         I           I         I
    +----+    +----+      +----+    +----+      +----+    +----+
Org | 0  |    | t1~|      | t2 |    | t4 |      | t6 |    | t8 |
Rx  | 0  |    | t2 |      | t4 |    | t6 |      | t8 |    |t10 |
Tx  | t1~|    | t3~|      | t1 |    | t3 |      | t5 |    | t7 |
    +----+    +----+      +----+    +----+      +----+    +----+

T1 - local transmit timestamp of the latest request (t5)
T2 - remote receive timestamp from the latest response (t6)
T3 - remote transmit timestamp from the latest response (t3)
T4 - local receive timestamp of the previous response (t4)
 */

#if defined(CONFIG_NET_ENABLE_NTP_CLIENT)
#error
#endif

#if defined(DEBUG_PTP_NTP_CLIENT)
#undef NDEBUG
#endif

#pragma GCC push_options
#pragma GCC optimize("O2")
#pragma GCC optimize("no-tree-loop-distribute-patterns")

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/time.h>

#include "configstore.h"
#include "network.h"
#include "core/protocol/iana.h"
#include "core/protocol/ntp.h"
#include "apps/ntpclient.h"
#include "gd32_ptp.h"
#include "softwaretimers.h"
#include "firmware/debug/debug_debug.h"

namespace net::globals
{
extern uint32_t ptpTimestamp[2];
} // namespace net::globals

#define _NTPFRAC_(x) (4294U * static_cast<uint32_t>(x) + ((1981U * static_cast<uint32_t>(x)) >> 11) + ((2911U * static_cast<uint32_t>(x)) >> 28))
#define NTPFRAC(x) _NTPFRAC_(x / 1000)

/**
 * The reverse of the above, needed if we want to set our microsecond
 * clock (via clock_settime) based on the incoming time in NTP format.
 * Basically exact.
 */
#define USEC(x) (((x) >> 12) - 759 * ((((x) >> 10) + 32768) >> 16))

struct NtpClient
{
    uint32_t server_ip;
    int32_t handle;
    TimerHandle_t timer_id;
    uint32_t request_timeout_seconds;
    uint32_t poll_seconds;
    uint32_t locked_count;
    const ntp::Packet* reply;
    ntp::Status status;
    ntp::Packet request;
    ntp::TimeStamp t1; // time request sent by client
    ntp::TimeStamp t2; // time request received by server
    ntp::TimeStamp t3; // time reply sent by server
    ntp::TimeStamp t4; // time reply received by client
    ntp::TimeStamp cookie_basic;
    struct
    {
        ntp::TimeStamp previous_receive;
        ntp::TimeStamp dst; // destination timestamp
        ntp::TimeStamp sent_a;
        ntp::TimeStamp sent_b;
        int32_t x; // interleave switch
        uint32_t missed_responses;
#ifndef NDEBUG
        ntp::Modes mode;
#endif
    } state;
};

static NtpClient s_ntp_client;
static uint16_t s_id;
static constexpr uint16_t kRequestSize = sizeof s_ntp_client.request;

static void Print([[maybe_unused]] const char* text, [[maybe_unused]] const struct ntp::TimeStamp* pNtpTime)
{
#ifndef NDEBUG
    const auto kSeconds = static_cast<time_t>(pNtpTime->seconds - ntp::kJan1970);
    const auto* local_time = localtime(&kSeconds);
    printf("%s %02d:%02d:%02d.%06d %04d [%u][0x%.8x]\n", text, local_time->tm_hour, local_time->tm_min, local_time->tm_sec, USEC(pNtpTime->fraction), local_time->tm_year + 1900, pNtpTime->seconds, pNtpTime->fraction);
#endif
}

static void Process();

static void Input(const uint8_t* buffer, uint32_t size, uint32_t from_ip, [[maybe_unused]] uint16_t from_port)
{
    DEBUG_ENTRY();

    // Invalid packet size
    if (__builtin_expect((size != sizeof(struct ntp::Packet)), 1))
    {
        DEBUG_EXIT();
        return;
    }

    // Not for us
    if (__builtin_expect((from_ip != s_ntp_client.server_ip), 0))
    {
        DEBUG_EXIT();
        return;
    }

    // Ignore duplicates
    if (s_ntp_client.state.missed_responses == 0)
    {
        DEBUG_EXIT();
        return;
    }

    s_ntp_client.reply = reinterpret_cast<const ntp::Packet*>(buffer);

    Process();

    DEBUG_EXIT();
}

static void Send();

static void PtpNtpTimer([[maybe_unused]] TimerHandle_t handle)
{
    assert(s_ntp_client.status != ntp::Status::kStopped);
    assert(s_ntp_client.status != ntp::Status::kDisabled);

    if (s_ntp_client.status == ntp::Status::kWaiting)
    {
        if (s_ntp_client.request_timeout_seconds > 1)
        {
            s_ntp_client.request_timeout_seconds--;
            return;
        }

        if (s_ntp_client.request_timeout_seconds == 1)
        {
            s_ntp_client.status = ntp::Status::kFailed;
            network::apps::ntpclient::DisplayStatus(ntp::Status::kFailed);
            s_ntp_client.poll_seconds = network::apps::ntpclient::kPollSecondsMin;
            return;
        }
        return;
    }

    if (s_ntp_client.poll_seconds > 1)
    {
        s_ntp_client.poll_seconds--;
        return;
    }

    if (s_ntp_client.poll_seconds == 1)
    {
        Send();
    }
}

/**
 * The interleaved client/server mode is similar to the basic client/ server mode.
 * The difference between the two modes is in the values saved to the origin and transmit timestamp fields.
 */

static void Send()
{
    s_ntp_client.state.missed_responses++;

    /**
     * The first request from a client is always in the basic mode and so is the server response.
     * It has a zero origin timestamp and zero receive timestamp.
     * Only when the client receives a valid response from the server,
     * it will be able to send a request in the interleaved mode
     */
    if (s_ntp_client.state.missed_responses > 4)
    {
        s_ntp_client.cookie_basic.seconds = random();
        s_ntp_client.cookie_basic.fraction = 0;

        s_ntp_client.request.origin_timestamp_s = 0;
        s_ntp_client.request.origin_timestamp_f = 0;

        s_ntp_client.request.receive_timestamp_s = 0;
        s_ntp_client.request.receive_timestamp_f = 0;

        /**
         * The origin timestamp is a cookie which is used to detect that a received packet
         * is a response to the last packet sent in the other direction of the association.
         */
        s_ntp_client.request.transmit_timestamp_s = __builtin_bswap32(s_ntp_client.cookie_basic.seconds);
        s_ntp_client.request.transmit_timestamp_f = __builtin_bswap32(s_ntp_client.cookie_basic.fraction);
    }
    else
    {
        /**
         * A client request in the interleaved mode has an origin timestamp equal to
         *  the receive timestamp from the last valid server response.
         */
        s_ntp_client.request.origin_timestamp_s = __builtin_bswap32(s_ntp_client.state.dst.seconds);
        s_ntp_client.request.origin_timestamp_f = __builtin_bswap32(s_ntp_client.state.dst.fraction);

        s_ntp_client.request.receive_timestamp_s = __builtin_bswap32(s_ntp_client.state.previous_receive.seconds);
        s_ntp_client.request.receive_timestamp_f = __builtin_bswap32(s_ntp_client.state.previous_receive.fraction);

        if (s_ntp_client.state.x > 0)
        {
            s_ntp_client.request.transmit_timestamp_s = __builtin_bswap32(s_ntp_client.state.sent_b.seconds);
            s_ntp_client.request.transmit_timestamp_f = __builtin_bswap32(s_ntp_client.state.sent_b.fraction);
        }
        else
        {
            s_ntp_client.request.transmit_timestamp_s = __builtin_bswap32(s_ntp_client.state.sent_a.seconds);
            s_ntp_client.request.transmit_timestamp_f = __builtin_bswap32(s_ntp_client.state.sent_a.fraction);
        }
    }

    network::udp::SendWithTimestamp(s_ntp_client.handle, reinterpret_cast<const uint8_t*>(&s_ntp_client.request), kRequestSize, s_ntp_client.server_ip, network::iana::Ports::kPortNtp);

#ifndef NDEBUG
    printf("Request:  org=%.8x%.8x rx=%.8x%.8x tx=%.8x%.8x\n", __builtin_bswap32(s_ntp_client.request.origin_timestamp_s), __builtin_bswap32(s_ntp_client.request.origin_timestamp_f),
           __builtin_bswap32(s_ntp_client.request.receive_timestamp_s), __builtin_bswap32(s_ntp_client.request.receive_timestamp_f), __builtin_bswap32(s_ntp_client.request.transmit_timestamp_s),
           __builtin_bswap32(s_ntp_client.request.transmit_timestamp_f));
#endif

    if (s_ntp_client.state.x > 0)
    {
        s_ntp_client.state.sent_a.seconds = net::globals::ptpTimestamp[1] + ntp::kJan1970;
        s_ntp_client.state.sent_a.fraction = NTPFRAC(gd32::ptp_subsecond_2_nanosecond(net::globals::ptpTimestamp[0]));
    }
    else
    {
        s_ntp_client.state.sent_b.seconds = net::globals::ptpTimestamp[1] + ntp::kJan1970;
        s_ntp_client.state.sent_b.fraction = NTPFRAC(gd32::ptp_subsecond_2_nanosecond(net::globals::ptpTimestamp[0]));
    }

    s_ntp_client.state.x = -s_ntp_client.state.x;
    s_id++;

    s_ntp_client.request_timeout_seconds = network::apps::ntpclient::kTimeoutSeconds;
    s_ntp_client.status = ntp::Status::kWaiting;
    network::apps::ntpclient::DisplayStatus(ntp::Status::kWaiting);
}

static void Difference(const ntp::TimeStamp& start, const ntp::TimeStamp& stop, int32_t& diff_seconds, int32_t& diff_nano_seconds)
{
    gd32::ptp::time_t r;
    const gd32::ptp::time_t kX = {.tv_sec = static_cast<int32_t>(stop.seconds), .tv_nsec = static_cast<int32_t>(USEC(stop.fraction) * 1000)};
    const gd32::ptp::time_t kY = {.tv_sec = static_cast<int32_t>(start.seconds), .tv_nsec = static_cast<int32_t>(USEC(start.fraction) * 1000)};
    gd32::sub_time(&r, &kX, &kY);

    diff_seconds = r.tv_sec;
    diff_nano_seconds = r.tv_nsec;
}

static inline int32_t AbsInt32(int32_t x)
{
    return (x < 0) ? -x : x;
}

static void UpdatePtpTime()
{
    int32_t diff_seconds1, diff_nano_seconds1;
    Difference(s_ntp_client.t1, s_ntp_client.t2, diff_seconds1, diff_nano_seconds1);

    int32_t diff_seconds2, diff_nano_seconds2;
    Difference(s_ntp_client.t4, s_ntp_client.t3, diff_seconds2, diff_nano_seconds2);

    auto offset_seconds = static_cast<int64_t>(diff_seconds1) + static_cast<int64_t>(diff_seconds2);
    auto offset_nano_seconds = static_cast<int64_t>(diff_nano_seconds1) + static_cast<int64_t>(diff_nano_seconds2);

    const int32_t kOffsetSecondsAverage = offset_seconds / 2;
    const int32_t kOffsetNanosverage = offset_nano_seconds / 2;

    gd32::ptp::time_t ptp_offset = {.tv_sec = kOffsetSecondsAverage, .tv_nsec = kOffsetNanosverage};
    gd32::normalize_time(&ptp_offset);
    gd32_ptp_update_time(&ptp_offset);

    gd32::ptp::ptptime ptp_get;
    gd32_ptp_get_time(&ptp_get);

    s_ntp_client.request.reference_timestamp_s = __builtin_bswap32(static_cast<uint32_t>(ptp_get.tv_sec) + ntp::kJan1970);
    s_ntp_client.request.reference_timestamp_f = __builtin_bswap32(NTPFRAC(ptp_get.tv_nsec));

    if ((ptp_offset.tv_sec == 0) && (ptp_offset.tv_nsec > -999999) && (ptp_offset.tv_nsec < 999999))
    {
        s_ntp_client.status = ntp::Status::kLocked;
        network::apps::ntpclient::DisplayStatus(ntp::Status::kLocked);
        if (++s_ntp_client.locked_count == 4)
        {
            s_ntp_client.poll_seconds = network::apps::ntpclient::kPollSecondsMax;
        }
    }
    else
    {
        s_ntp_client.status = ntp::Status::kIdle;
        network::apps::ntpclient::DisplayStatus(ntp::Status::kIdle);
        s_ntp_client.poll_seconds = network::apps::ntpclient::kPollSecondsMin;
        s_ntp_client.locked_count = 0;
    }

#ifndef NDEBUG
    /**
     * Network delay calculation
     */
    gd32::ptp::time_t diff1;
    gd32::ptp::time_t diff2;

    if (s_ntp_client.state.mode == ntp::Modes::kBasic)
    {
        Difference(s_ntp_client.t1, s_ntp_client.t4, diff1.tv_sec, diff1.tv_nsec);
        Difference(s_ntp_client.t2, s_ntp_client.t3, diff2.tv_sec, diff2.tv_nsec);
    }
    else
    {
        ntp::TimeStamp start;
        start.seconds = __builtin_bswap32(s_ntp_client.request.transmit_timestamp_s);
        start.fraction = __builtin_bswap32(s_ntp_client.request.transmit_timestamp_f);
        ntp::TimeStamp stop;
        stop.seconds = __builtin_bswap32(s_ntp_client.request.receive_timestamp_s);
        stop.fraction = __builtin_bswap32(s_ntp_client.request.receive_timestamp_f);
        Difference(start, stop, diff1.tv_sec, diff1.tv_nsec);

        const auto* const kReply = s_ntp_client.reply;
        start.seconds = __builtin_bswap32(s_ntp_client.request.origin_timestamp_s);
        start.fraction = __builtin_bswap32(s_ntp_client.request.origin_timestamp_f);
        stop.seconds = __builtin_bswap32(kReply->transmit_timestamp_s);
        stop.fraction = __builtin_bswap32(kReply->transmit_timestamp_f);
        Difference(start, stop, diff2.tv_sec, diff2.tv_nsec);
    }

    gd32::ptp::time_t ptp_delay;
    gd32::sub_time(&ptp_delay, &diff1, &diff2);

    char sign = '+';

    if (ptp_offset.tv_sec < 0)
    {
        ptp_offset.tv_sec = -ptp_offset.tv_sec;
        sign = '-';
    }

    if (ptp_offset.tv_nsec < 0)
    {
        ptp_offset.tv_nsec = -ptp_offset.tv_nsec;
        sign = '-';
    }

    printf(" %s : offset=%c%d.%09d delay=%d.%09d\n", s_ntp_client.state.mode == ntp::Modes::kBasic ? "Basic" : "Interleaved", sign, ptp_offset.tv_sec, ptp_offset.tv_nsec, ptp_delay.tv_sec, ptp_delay.tv_nsec);
#endif
}

/**
 * Two of the tests are modified for the interleaved mode:
 *
 * 1. The check for duplicate packets SHOULD compare both receive and
 *    transmit timestamps in order to not drop a valid response in the interleaved mode if
 *    it follows a response in the basic mode and they contain the same transmit timestamp.
 * 2. The check for bogus packets SHOULD compare the origin timestamp with both transmit and
 *    receive timestamps from the request.
 *    If the origin timestamp is equal to the transmit timestamp, the response is in the basic mode.
 *    If the origin timestamp is equal to the receive timestamp, the response is in the interleaved mode.
 */

static void Process()
{
    const auto* const kReply = s_ntp_client.reply;
#ifndef NDEBUG
    printf("Response: org=%.8x%.8x rx=%.8x%.8x tx=%.8x%.8x\n", __builtin_bswap32(kReply->origin_timestamp_s), __builtin_bswap32(kReply->origin_timestamp_f), __builtin_bswap32(kReply->receive_timestamp_s),
           __builtin_bswap32(kReply->receive_timestamp_f), __builtin_bswap32(kReply->transmit_timestamp_s), __builtin_bswap32(kReply->transmit_timestamp_f));
#endif
    /**
     * If the origin timestamp is equal to the transmit timestamp,
     * the response is in the basic mode.
     */
    if ((kReply->origin_timestamp_s == s_ntp_client.request.transmit_timestamp_s) && (kReply->origin_timestamp_f == s_ntp_client.request.transmit_timestamp_f))
    {
        if (s_ntp_client.state.x < 0)
        {
            s_ntp_client.t1.seconds = s_ntp_client.state.sent_a.seconds;
            s_ntp_client.t1.fraction = s_ntp_client.state.sent_a.fraction;
        }
        else
        {
            s_ntp_client.t1.seconds = s_ntp_client.state.sent_b.seconds;
            s_ntp_client.t1.fraction = s_ntp_client.state.sent_b.fraction;
        }

        s_ntp_client.t4.seconds = net::globals::ptpTimestamp[1] + ntp::kJan1970;
        s_ntp_client.t4.fraction = NTPFRAC(gd32::ptp_subsecond_2_nanosecond(net::globals::ptpTimestamp[0]));
#ifndef NDEBUG
        s_ntp_client.state.mode = ntp::Modes::kBasic;
#endif
    }
    else
        /**
         * If the origin timestamp is equal to the receive timestamp,
         * the response is in the interleaved mode.
         */
        if ((kReply->origin_timestamp_s == s_ntp_client.request.receive_timestamp_s) && (kReply->origin_timestamp_f == s_ntp_client.request.receive_timestamp_f))
        {
            if (s_ntp_client.state.x > 0)
            {
                s_ntp_client.t1.seconds = s_ntp_client.state.sent_b.seconds;
                s_ntp_client.t1.fraction = s_ntp_client.state.sent_b.fraction;
            }
            else
            {
                s_ntp_client.t1.seconds = s_ntp_client.state.sent_a.seconds;
                s_ntp_client.t1.fraction = s_ntp_client.state.sent_a.fraction;
            }

            s_ntp_client.t4.seconds = s_ntp_client.state.previous_receive.seconds;
            s_ntp_client.t4.fraction = s_ntp_client.state.previous_receive.fraction;
#ifndef NDEBUG
            s_ntp_client.state.mode = ntp::Modes::kInterleaved;
#endif
        }
        else
        {
            DEBUG_PUTS("INVALID RESPONSE");
            return;
        }

    s_ntp_client.t2.seconds = __builtin_bswap32(kReply->receive_timestamp_s);
    s_ntp_client.t2.fraction = __builtin_bswap32(kReply->receive_timestamp_f);

    s_ntp_client.t3.seconds = __builtin_bswap32(kReply->transmit_timestamp_s);
    s_ntp_client.t3.fraction = __builtin_bswap32(kReply->transmit_timestamp_f);

    s_ntp_client.state.dst.seconds = __builtin_bswap32(kReply->receive_timestamp_s);
    s_ntp_client.state.dst.fraction = __builtin_bswap32(kReply->receive_timestamp_f);

    s_ntp_client.state.previous_receive.seconds = net::globals::ptpTimestamp[1] + ntp::kJan1970;
    s_ntp_client.state.previous_receive.fraction = NTPFRAC(gd32::ptp_subsecond_2_nanosecond(net::globals::ptpTimestamp[0]));

    UpdatePtpTime();

    s_ntp_client.state.missed_responses = 0;

    Print("T1: ", &s_ntp_client.t1);
    Print("T2: ", &s_ntp_client.t2);
    Print("T3: ", &s_ntp_client.t3);
    Print("T4: ", &s_ntp_client.t4);
}

namespace network::apps::ntpclient::ptp
{
#pragma GCC pop_options
#pragma GCC push_options
#pragma GCC optimize("Os")

/**
 * @brief Initializes the Precision Time Protocol (PTP) NTP client.
 *
 * This function performs the initial setup for the NTP client, including
 * clearing its state, setting default values, and loading network parameters
 * such as the NTP server's IP address.
 *
 * The initialization includes:
 * - Resetting all state variables to their default values.
 * - Configuring the initial NTP request packet parameters.
 * - Setting the client status to `IDLE`.
 * - Initializing the random number generator using the current system time.
 * - Loading network parameters to retrieve the configured NTP server IP address.
 *
 * @note This function must be called before starting the NTP client.
 */
void Init()
{
    DEBUG_ENTRY();

    memset(&s_ntp_client, 0, sizeof(struct NtpClient));

    s_ntp_client.state.previous_receive.seconds = ntp::kJan1970;
    s_ntp_client.state.dst.seconds = ntp::kJan1970;
    s_ntp_client.state.sent_a.seconds = ntp::kJan1970;
    s_ntp_client.state.sent_b.seconds = ntp::kJan1970;
    s_ntp_client.state.missed_responses = 4;

    s_ntp_client.request.li_vn_mode = ntp::kVersion | ntp::kModeClient;
    s_ntp_client.request.poll = network::apps::ntpclient::kPollPowerMin;
    s_ntp_client.request.reference_id = ('A' << 0) | ('V' << 8) | ('S' << 16);

    s_ntp_client.state.x = 1;
    s_ntp_client.status = ntp::Status::kIdle;

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    srandom(static_cast<unsigned int>(tv.tv_sec ^ tv.tv_usec));

    s_ntp_client.server_ip = ConfigStore::Instance().NetworkGet(&common::store::Network::ntp_server_ip);

    DEBUG_EXIT();
}

/**
 * @brief Starts the NTP client.
 *
 * This function initializes the UDP socket for NTP communication, starts a software
 * timer for periodic tasks, and sends the first NTP request to the server.
 *
 * @note The function will not start the client if it is disabled or if the server
 *       IP address is not configured.
 */
void Start()
{
    DEBUG_ENTRY();

    if (s_ntp_client.status == ntp::Status::kDisabled)
    {
        DEBUG_EXIT();
        return;
    }

    if (s_ntp_client.server_ip == 0)
    {
        s_ntp_client.status = ntp::Status::kStopped;
        network::apps::ntpclient::DisplayStatus(ntp::Status::kStopped);
        DEBUG_EXIT();
        return;
    }

    s_ntp_client.handle = network::udp::Begin(network::iana::Ports::kPortNtp, Input);
    assert(s_ntp_client.handle != -1);

    s_ntp_client.status = ntp::Status::kIdle;
    network::apps::ntpclient::DisplayStatus(ntp::Status::kIdle);

    s_ntp_client.timer_id = SoftwareTimerAdd(1000, PtpNtpTimer);

    Send();

    DEBUG_EXIT();
}

/**
 * @brief Stops the NTP client.
 *
 * This function stops the software timer and closes the UDP socket. It optionally
 * disables the client if the `doDisable` parameter is set to `true`.
 *
 * @param[in] doDisable Set to `true` to disable the client after stopping.
 */
void Stop(bool do_disable)
{
    DEBUG_ENTRY();

    if (do_disable)
    {
        s_ntp_client.status = ntp::Status::kDisabled;
        network::apps::ntpclient::DisplayStatus(ntp::Status::kDisabled);
    }

    if (s_ntp_client.status == ntp::Status::kStopped)
    {
        return;
    }

    SoftwareTimerDelete(s_ntp_client.timer_id);

    network::udp::End(network::iana::Ports::kPortNtp);
    s_ntp_client.handle = -1;

    if (!do_disable)
    {
        s_ntp_client.status = ntp::Status::kStopped;
        network::apps::ntpclient::DisplayStatus(ntp::Status::kStopped);
    }

    DEBUG_EXIT();
}

/**
 * @brief Sets the IP address of the NTP server.
 *
 * This function updates the server IP address used by the NTP client.
 *
 * @param[in] server_ip The IP address of the NTP server.
 */
void SetServerIp(uint32_t server_ip)
{
    Stop(false);

    s_ntp_client.server_ip = server_ip;

    Start();
}

/**
 * @brief Retrieves the IP address of the configured NTP server.
 *
 * @return The IP address of the NTP server.
 */
uint32_t GetServerIp()
{
    return s_ntp_client.server_ip;
}

/**
 * @brief Retrieves the current status of the NTP client.
 *
 * This function returns the current operational status of the NTP client.
 *
 * @return The status of the NTP client as an `ntp::Status` enum.
 */
ntp::Status GetStatus()
{
    return s_ntp_client.status;
}
} // namespace network::apps::ntpclient::ptp