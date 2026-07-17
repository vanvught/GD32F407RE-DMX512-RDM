/**
 * @file dmx_port.h
 *
 */

#ifndef GD32_DMX_PORT_H_
#define GD32_DMX_PORT_H_

#include <cstdint>
#include "gd32_uart.h"

namespace dmx::port {
enum class Usage { kTxRx = 0, kTxOnly = 1, kRxOnly = 2 };

struct Info {
    gd32::Uart uart;
    uint32_t port;
    uint32_t pin;
    Usage usage;
};
} // namespace dmx::port

#endif // GD32_DMX_PORT_H_
