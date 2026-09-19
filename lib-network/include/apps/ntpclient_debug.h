/**
 * @file ntpclient_debug.h
 *
 */

#ifndef APPS_NTPCLIENT_DEBUG_H_
#define APPS_NTPCLIENT_DEBUG_H_

#include "firmware/debug/debug_debug.h"

#if defined DEBUG_NTP_CLIENT || defined DEBUG_PTP_NTP_CLIENT
#define NTP_CLIENT_DEBUG_ENTRY() DEBUG_ENTRY()
#define NTP_CLIENT_DEBUG_EXIT() DEBUG_EXIT()
#define NTP_CLIENT_DEBUG_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#define NTP_CLIENT_DEBUG_PUTS(...) DEBUG_PUTS(__VA_ARGS__)
#else
#define NTP_CLIENT_DEBUG_ENTRY() \
    do {                         \
    } while (false)
#define NTP_CLIENT_DEBUG_EXIT() \
    do {                        \
    } while (false)
#define NTP_CLIENT_DEBUG_PRINTF(...) \
    do {                             \
    } while (false)
#define NTP_CLIENT_DEBUG_PUTS(...) \
    do {                           \
    } while (false)
#endif // defined DEBUG_NTP_CLIENT || defined DEBUG_PTP_NTP_CLIENT


#endif // APPS_NTPCLIENT_DEBUG_H_
