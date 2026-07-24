/**
 * @file dmx.cpp
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

#if defined(CONFIG_TIMER6_HAVE_NO_IRQ_HANDLER)
#error
#endif // defined(CONFIG_TIMER6_HAVE_NO_IRQ_HANDLER)

#if !defined(CONFIG_DMX_NO_OPTIMIZE)
#pragma GCC push_options
#pragma GCC optimize("O3")
#endif // !defined(CONFIG_DMX_NO_OPTIMIZE)

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <utility>
#include <cassert>

#include "gd32/dmx.h" // IWYU pragma: keep
#include "dmx/dmx_config.h"
#include "gd32/dmx_assert.h"
#include "gd32/dmx_port.h"
#include "dmxconst.h"
#include "e120.h"
#include "rdmconst.h"
#include "rdm_e120.h"
#include "timing.h"
#include "gd32.h"
#include "gd32_dma.h"
#include "gd32_uart.h"
#include "gd32_gpio.h"
#include "dmx_internal.h"
#if defined(LOGIC_ANALYZER)
#include "logic_analyzer.h" // IWYU pragma: keep
#endif                      // defined(LOGIC_ANALYZER)
#include "dmx_debug.h"

static_assert(dmx::buffer::kSize % 4 == 0); // multiple of uint32_t

namespace dmx {
namespace {
constexpr uint32_t kDmxSlotsCompleteFlag = 0x8000;
constexpr uint32_t kRdmSlotsCompleteFlag = 0x4000;

enum class TxRxState { kIdle, kDmxBreak, kDmxMab, kDmxData, kDmxInter, kRdmData, kRdmChecksumh, kRdmChecksuml, kRdmdisc };
enum class RdmTxState { kIdle, kBreak, kMab, kData, kDirection };
enum class PortState { kIdle, kTx, kRx };

struct DmxTxDataPacket {
    uint8_t data[dmx::buffer::kSize];
    uint32_t length;
};

struct DmxTxPacket {
    DmxTxDataPacket data[2];
    uint32_t write_index;
    uint32_t read_index;
    bool data_pending;
};

struct DmxTxData {
    DmxTxPacket dmx;
    OutputStyle output_style ALIGNED;
    volatile TxRxState state;
};

struct RdmTxDataPacket {
    uint8_t data[sizeof(struct TRdmMessage)];
    uint32_t length;
};

struct RdmTxPacket {
    struct RdmTxDataPacket data;
};

struct RdmTxData {
    RdmTxPacket rdm;
    volatile RdmTxState state;
};

struct DmxTransmit {
    uint32_t break_time;
    uint32_t mab_time;
    uint32_t inter_time;
};

struct RxDmxPackets {
    uint32_t per_second;
    uint32_t count;
    uint32_t count_previous;
};

struct RxDmxData {
    uint8_t data[dmx::buffer::kSize] ALIGNED;
    uint32_t slots_in_packet;
};

struct RxData {
    struct Dmx {
        volatile RxDmxData current;
        RxDmxData previous;
    } dmx ALIGNED;
    struct Rdm {
        volatile uint8_t data[sizeof(struct TRdmMessage)] ALIGNED;
        volatile uint32_t index;
    } rdm ALIGNED;
    volatile TxRxState state;
} ALIGNED;
} // namespace
} // namespace dmx

namespace {
constexpr dmx::port::Info kDirGpio[dmx::config::max::kPorts] = {
    {dmx::config::kPort0},
#if DMX_MAX_PORTS >= 2
    {dmx::config::kPort1},
#endif // DMX_MAX_PORTS >= 2
#if DMX_MAX_PORTS >= 3
    {dmx::config::kPort2},
#endif // DMX_MAX_PORTS >= 3
#if DMX_MAX_PORTS >= 4
    {dmx::config::kPort3},
#endif // DMX_MAX_PORTS >= 4
#if DMX_MAX_PORTS >= 5
    {dmx::config::kPort4},
#endif // DMX_MAX_PORTS >= 5
#if DMX_MAX_PORTS >= 6
    {dmx::config::kPort5},
#endif // DMX_MAX_PORTS >= 6
#if DMX_MAX_PORTS >= 7
    {dmx::config::kPort6},
#endif // DMX_MAX_PORTS >= 7
#if DMX_MAX_PORTS == 8
    {dmx::config::kPort7},
#endif // DMX_MAX_PORTS == 8
};

consteval bool AreUartsUnique() {
    for (uint32_t i = 0; i < dmx::config::max::kPorts; ++i) {
        for (uint32_t j = i + 1; j < dmx::config::max::kPorts; ++j) {
            if (kDirGpio[i].uart == kDirGpio[j].uart) {
                return false;
            }
        }
    }

    return true;
}

static_assert(AreUartsUnique(), "DMX port UARTs must be unique");

consteval uint32_t GetPortByUart(uint32_t uart) {
    for (uint32_t port = 0; port < dmx::config::max::kPorts; ++port) {
        if (std::to_underlying(kDirGpio[port].uart) == uart) {
            return port;
        }
    }

    return dmx::config::max::kPorts;
}

volatile dmx::PortState sv_port_state[dmx::config::max::kPorts] ALIGNED;

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
volatile dmx::TotalStatistics sv_total_statistics[dmx::config::max::kPorts] ALIGNED;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)

// DMX RX
volatile dmx::RxDmxPackets sv_rx_dmx_packets[dmx::config::max::kPorts] ALIGNED;
// DMX RDM RX
volatile dmx::RxData sv_rx_buffer[dmx::config::max::kPorts] ALIGNED;
// DMX TX
dmx::DmxTxData s_DmxTxBuffer[dmx::config::max::kPorts] ALIGNED SECTION_DMA_BUFFER;
dmx::DmxTransmit s_dmx_transmit;
// RDM TX
dmx::RdmTxData s_RdmTxBuffer[dmx::config::max::kPorts] ALIGNED SECTION_DMA_BUFFER;
} // namespace

// RDM RX
volatile uint32_t gsv_rdm_data_receive_end[dmx::config::max::kPorts];

template <uint32_t kUsartPeripheral>
void IrqHandlerDmxRdmInput() {
    constexpr auto kPortIndex = GetPortByUart(kUsartPeripheral);
    auto& rx_buffer = sv_rx_buffer[kPortIndex];
    const auto kIsFlagIdleFrame = (USART_REG_VAL(kUsartPeripheral, USART_FLAG_IDLE) & BIT(USART_BIT_POS(USART_FLAG_IDLE))) == BIT(USART_BIT_POS(USART_FLAG_IDLE));

    // Software can clear this bit by reading the USART_STAT and USART_DATA registers one by one.
    if (kIsFlagIdleFrame) {
        static_cast<void>(GET_BITS(USART_RDATA(kUsartPeripheral), 0U, 8U));

        if (rx_buffer.state == dmx::TxRxState::kDmxData) {
            rx_buffer.state = dmx::TxRxState::kIdle;
            rx_buffer.dmx.current.slots_in_packet |= dmx::kDmxSlotsCompleteFlag;
            return;
        }

        if (rx_buffer.state == dmx::TxRxState::kRdmdisc) {
            rx_buffer.state = dmx::TxRxState::kIdle;
            rx_buffer.rdm.index |= dmx::kRdmSlotsCompleteFlag;
            return;
        }

        // TODO (a) What kind of status is this?
        return;
    }

    const auto kIsFlagFrameError = (USART_REG_VAL(kUsartPeripheral, USART_FLAG_FERR) & BIT(USART_BIT_POS(USART_FLAG_FERR))) == BIT(USART_BIT_POS(USART_FLAG_FERR));

    // Software can clear this bit by reading the USART_STAT and USART_DATA registers one by one.
    if (kIsFlagFrameError) {
        static_cast<void>(GET_BITS(USART_RDATA(kUsartPeripheral), 0U, 8U));

        if (rx_buffer.state == dmx::TxRxState::kIdle) {
            rx_buffer.state = dmx::TxRxState::kDmxBreak;
        }

        return;
    }

    const auto kData = static_cast<uint8_t>(GET_BITS(USART_RDATA(kUsartPeripheral), 0U, 8U));

    switch (rx_buffer.state) {
        case dmx::TxRxState::kIdle:
            rx_buffer.state = dmx::TxRxState::kRdmdisc;
            rx_buffer.rdm.data[0] = kData;
            rx_buffer.rdm.index = 1;
            break;

        case dmx::TxRxState::kDmxBreak:
            switch (kData) {
                case dmx::kStartCode: {
                    rx_buffer.dmx.current.data[0] = dmx::kStartCode;
                    rx_buffer.dmx.current.slots_in_packet = 1;
                    sv_rx_dmx_packets[kPortIndex].count = sv_rx_dmx_packets[kPortIndex].count + 1;
                    rx_buffer.state = dmx::TxRxState::kDmxData;
                } break;

                case E120_SC_RDM: {
                    rx_buffer.rdm.data[0] = E120_SC_RDM;
                    rx_buffer.rdm.index = 1;
                    rx_buffer.state = dmx::TxRxState::kRdmData;
                } break;

                default:
                    [[unlikely]] {
                        rx_buffer.dmx.current.slots_in_packet = 0;
                        rx_buffer.rdm.index = 0;
                        rx_buffer.state = dmx::TxRxState::kIdle;
                    }
                    break;
            }
            break;

        case dmx::TxRxState::kDmxData: {
            auto index = rx_buffer.dmx.current.slots_in_packet;
            rx_buffer.dmx.current.data[index] = kData;
            index++;
            rx_buffer.dmx.current.slots_in_packet = index;

            if (index > dmx::kChannelsMax) {
                index |= dmx::kDmxSlotsCompleteFlag;
                rx_buffer.dmx.current.slots_in_packet = index;
                rx_buffer.state = dmx::TxRxState::kIdle;
                break;
            }
        } break;

        case dmx::TxRxState::kRdmData: {
            auto index = rx_buffer.rdm.index;
            rx_buffer.rdm.data[index] = kData;
            index++;
            rx_buffer.rdm.index = index;

            const auto* data = reinterpret_cast<volatile struct TRdmMessage*>(&rx_buffer.rdm.data[0]);

            if ((index >= e120::kMessageLengthMin) && (index <= sizeof(struct TRdmMessage)) && (index == data->message_length)) {
                rx_buffer.state = dmx::TxRxState::kRdmChecksumh;
            } else if (index > sizeof(struct TRdmMessage)) {
                rx_buffer.state = dmx::TxRxState::kIdle;
            }
        } break;

        case dmx::TxRxState::kRdmChecksumh: {
            auto index = rx_buffer.rdm.index;
            rx_buffer.rdm.data[index] = kData;
            index++;
            rx_buffer.rdm.index = index;
            rx_buffer.state = dmx::TxRxState::kRdmChecksuml;
        } break;

        case dmx::TxRxState::kRdmChecksuml: {
            auto index = rx_buffer.rdm.index;
            rx_buffer.rdm.data[index] = kData;
            index |= dmx::kRdmSlotsCompleteFlag;
            rx_buffer.rdm.index = index;
            rx_buffer.state = dmx::TxRxState::kIdle;
            gsv_rdm_data_receive_end[kPortIndex] = DWT->CYCCNT;
        } break;

        case dmx::TxRxState::kRdmdisc: {
            auto index = rx_buffer.rdm.index;

            if (index < 24) { // TODO (a) Replace 24 with constepr
                rx_buffer.rdm.data[index] = kData;
                index++;
                rx_buffer.rdm.index = index;
            }
        } break;

        default:
            [[unlikely]] {
                rx_buffer.dmx.current.slots_in_packet = 0;
                rx_buffer.rdm.index = 0;
                rx_buffer.state = dmx::TxRxState::kIdle;
            }
            break;
    }
}

template <uint32_t kUsartPeripheral, uint32_t kDmaController, dma_channel_enum kDmaChannel>
void DmaStartTx(const uint8_t* data, uint32_t length) {
    auto dma_chctl = DMA_CHCTL(kDmaController, kDmaChannel);
    // Disable channel
    dma_chctl &= ~DMA_CHXCTL_CHEN;
    DMA_CHCTL(kDmaController, kDmaChannel) = dma_chctl;
    // Clear transfer complete interrupt
    Gd32DmaInterruptFlagClear<kDmaController, kDmaChannel, DMA_INTF_FTFIF>();
    // Configure transfer
    DMA_CHMADDR(kDmaController, kDmaChannel) = reinterpret_cast<uint32_t>(data);
    DMA_CHCNT(kDmaController, kDmaChannel) = length & DMA_CHXCNT_CNT;
    // Re-enable channel and interrupt
    dma_chctl |= DMA_CHXCTL_CHEN | DMA_INTERRUPT_ENABLE;
    DMA_CHCTL(kDmaController, kDmaChannel) = dma_chctl;
    // Enable USART DMA transmission
    USART_CTL2(kUsartPeripheral) |= USART_TRANSMIT_DMA_ENABLE;
}

template <uint32_t kUsartPeripheral, uint32_t kDmaController, dma_channel_enum kDmaChannel, typename TxBufferType>
void DmaRestartDmxTx(TxBufferType& tx_buffer) {
    auto& dmx = tx_buffer.dmx;

    if (dmx.read_index != dmx.write_index) {
        dmx.read_index ^= 1;
    }

    const auto& packet = dmx.data[dmx.read_index];

    DmaStartTx<kUsartPeripheral, kDmaController, kDmaChannel>(packet.data, packet.length);
}

#define DMA_RESTART_DMX_TX(PORT_INDEX, USARTx, DMAx, CHx) DmaRestartDmxTx<USARTx, DMAx, CHx>(s_DmxTxBuffer[PORT_INDEX])

template <uint32_t kUsartPeripheral, uint32_t kDmaController, dma_channel_enum kDmaChannel, typename TxBufferType>
void DmaStartRdmTx(TxBufferType& tx_buffer) {
    const auto& packet = tx_buffer.rdm.data;

    DmaStartTx<kUsartPeripheral, kDmaController, kDmaChannel>(packet.data, packet.length);
}

#define DMA_START_RDM_TX(PORT_INDEX, USARTx, DMAx, CHx) DmaStartRdmTx<USARTx, DMAx, CHx>(s_RdmTxBuffer[PORT_INDEX])

extern "C" {
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
#if defined(DMX_USE_USART0) || defined(DMX_USE_USART0_RX)
void USART0_IRQHandler() {
    IrqHandlerDmxRdmInput<USART0>();
}
#endif // defined(DMX_USE_USART0) || defined(DMX_USE_USART0_RX)

#if defined(DMX_USE_USART1) || defined(DMX_USE_USART1_RX)
void USART1_IRQHandler() {
    IrqHandlerDmxRdmInput<USART1>();
}
#endif // defined(DMX_USE_USART1) || defined(DMX_USE_USART1_RX)

#if defined(DMX_USE_USART2) || defined(DMX_USE_USART2_RX)
void USART2_IRQHandler() {
    IrqHandlerDmxRdmInput<USART2>();
}
#endif // defined(DMX_USE_USART2) || defined(DMX_USE_USART2_RX)

#if defined(DMX_USE_UART3) || defined(DMX_USE_UART3_RX)
void UART3_IRQHandler() {
    IrqHandlerDmxRdmInput<UART3>();
}
#endif // defined(DMX_USE_UART3) || defined(DMX_USE_UART3_RX)

#if defined(DMX_USE_UART4) || defined(DMX_USE_UART4_RX)
void UART4_IRQHandler() {
    IrqHandlerDmxRdmInput<UART4>();
}
#endif // defined(DMX_USE_UART4) || defined(DMX_USE_UART4_RX)

#if defined(DMX_USE_USART5) || defined(DMX_USE_USART5_RX)
void USART5_IRQHandler() {
    IrqHandlerDmxRdmInput<USART5>();
}
#endif // defined(DMX_USE_USART5) || defined(DMX_USE_USART5_RX)

#if defined(DMX_USE_UART6) || defined(DMX_USE_UART6_RX)
void UART6_IRQHandler() {
    IrqHandlerDmxRdmInput<UART6>();
}
#endif // defined(DMX_USE_UART6) || defined(DMX_USE_UART6_RX)

#if defined(DMX_USE_UART7) || defined(DMX_USE_UART7_RX)
void UART7_IRQHandler() {
    IrqHandlerDmxRdmInput<UART7>();
}
#endif // defined(DMX_USE_UART7) || defined(DMX_USE_UART7_RX)
#endif // !defined(CONFIG_DMX_TRANSMIT_ONLY)

void TIMER1_IRQHandler() {
// USART 0
#if defined(DMX_USE_USART0)
    if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0) {
        constexpr auto kPortIndex = GetPortByUart(USART0);

        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            switch (s_DmxTxBuffer[kPortIndex].state) {
                case dmx::TxRxState::kDmxInter:
                    [[likely]] {
                        Gd32GpioModeOutput<USART0_GPIOx, USART0_TX_GPIO_PINx>();
                        GPIO_BC(USART0_GPIOx) = USART0_TX_GPIO_PINx;
                        s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
                        TIMER_CH0CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
                    }
                    break;

                case dmx::TxRxState::kDmxBreak:
                    [[likely]] {
                        Gd32GpioModeAf<USART0_GPIOx, USART0_TX_GPIO_PINx, USART0>();
                        s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxMab;
                        TIMER_CH0CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.mab_time;
                    }
                    break;

                case dmx::TxRxState::kDmxMab:
                    [[likely]] {
                        DMA_RESTART_DMX_TX(kPortIndex, USART0, USART0_DMAx, USART0_TX_DMA_CHx);
                    }
                    break;

                default:
                    [[unlikely]] break;
            }
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            switch (s_RdmTxBuffer[kPortIndex].state) {
                case dmx::RdmTxState::kBreak:
                    [[likely]] {
                        Gd32GpioModeAf<USART0_GPIOx, USART0_TX_GPIO_PINx, USART0>();
                        s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kMab;
                        TIMER_CH0CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kMabTimeTypical;
                    }
                    break;

                case dmx::RdmTxState::kMab:
                    [[likely]] {
                        DMA_START_RDM_TX(kPortIndex, USART0, USART0_DMAx, USART0_TX_DMA_CHx);
                    }
                    break;

                case dmx::RdmTxState::kDirection: {
                    s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kIdle;
                    sv_port_state[kPortIndex] = dmx::PortState::kIdle;
                    Dmx::Get()->SetPortDirection<kPortIndex, dmx::Direction::kInput, true>();

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
                    const auto kSent = sv_total_statistics[kPortIndex].rdm.sent.classes + 1;
                    sv_total_statistics[kPortIndex].rdm.sent.classes = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
                } break;

                default:
                    [[unlikely]] assert(false && "switch");
                    break;
            }
        }

        TIMER_INTF(TIMER1) = (~TIMER_INT_FLAG_CH0);
    }
#endif // defined(DMX_USE_USART0)
// USART 1
#if defined(DMX_USE_USART1)
    if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH1) == TIMER_INT_FLAG_CH1) {
        constexpr auto kPortIndex = GetPortByUart(USART1);
        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            switch (s_DmxTxBuffer[kPortIndex].state) {
                case dmx::TxRxState::kDmxInter:
                    [[likely]] {
                        Gd32GpioModeOutput<USART1_GPIOx, USART1_TX_GPIO_PINx>();
                        GPIO_BC(USART1_GPIOx) = USART1_TX_GPIO_PINx;
                        s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
                        TIMER_CH1CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
                    }
                    break;

                case dmx::TxRxState::kDmxBreak:
                    [[likely]] {
                        Gd32GpioModeAf<USART1_GPIOx, USART1_TX_GPIO_PINx, USART1>();
                        s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxMab;
                        TIMER_CH1CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.mab_time;
                    }
                    break;

                case dmx::TxRxState::kDmxMab:
                    [[likely]] {
                        DMA_RESTART_DMX_TX(kPortIndex, USART1, USART1_DMAx, USART1_TX_DMA_CHx);
                    }

                    break;
                default:
                    [[unlikely]] break;
            }
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            switch (s_RdmTxBuffer[kPortIndex].state) {
                case dmx::RdmTxState::kBreak:
                    [[likely]] {
                        Gd32GpioModeAf<USART1_GPIOx, USART1_TX_GPIO_PINx, USART1>();
                        s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kMab;
                        TIMER_CH1CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kMabTimeTypical;
                    }
                    break;

                case dmx::RdmTxState::kMab:
                    [[likely]] {
                        DMA_START_RDM_TX(kPortIndex, USART1, USART1_DMAx, USART1_TX_DMA_CHx);
                    }
                    break;

                case dmx::RdmTxState::kDirection: {
                    s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kIdle;
                    sv_port_state[kPortIndex] = dmx::PortState::kIdle;
                    Dmx::Get()->SetPortDirection<kPortIndex, dmx::Direction::kInput, true>();

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
                    const auto kSent = sv_total_statistics[kPortIndex].rdm.sent.classes + 1;
                    sv_total_statistics[kPortIndex].rdm.sent.classes = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
                } break;

                default:
                    [[unlikely]] assert(false && "switch");
                    break;
            }
        }

        TIMER_INTF(TIMER1) = (~TIMER_INT_FLAG_CH1);
    }
#endif // defined(DMX_USE_USART1)
// USART 2
#if defined(DMX_USE_USART2)
    if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH2) == TIMER_INT_FLAG_CH2) {
        constexpr auto kPortIndex = GetPortByUart(USART2);

        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            switch (s_DmxTxBuffer[kPortIndex].state) {
                case dmx::TxRxState::kDmxInter:
                    [[likely]] {
                        Gd32GpioModeOutput<USART2_GPIOx, USART2_TX_GPIO_PINx>();
                        GPIO_BC(USART2_GPIOx) = USART2_TX_GPIO_PINx;
                        s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
                        TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
                    }
                    break;

                case dmx::TxRxState::kDmxBreak:
                    [[likely]] {
                        Gd32GpioModeAf<USART2_GPIOx, USART2_TX_GPIO_PINx, USART2>();
                        s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxMab;
                        TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.mab_time;
                    }
                    break;

                case dmx::TxRxState::kDmxMab:
                    [[likely]] {
                        DMA_RESTART_DMX_TX(kPortIndex, USART2, USART2_DMAx, USART2_TX_DMA_CHx);
                    }
                    break;

                default:
                    [[unlikely]] assert(false && "switch");
                    break;
            }
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            switch (s_RdmTxBuffer[kPortIndex].state) {
                case dmx::RdmTxState::kBreak:
                    [[likely]] {
                        Gd32GpioModeAf<USART2_GPIOx, USART2_TX_GPIO_PINx, USART2>();
                        s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kMab;
                        TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kMabTimeTypical;
                    }
                    break;

                case dmx::RdmTxState::kMab:
                    [[likely]] {
                        DMA_START_RDM_TX(kPortIndex, USART2, USART2_DMAx, USART2_TX_DMA_CHx);
                    }
                    break;

                case dmx::RdmTxState::kDirection: {
                    s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kIdle;
                    sv_port_state[kPortIndex] = dmx::PortState::kIdle;
                    Dmx::Get()->SetPortDirection<kPortIndex, dmx::Direction::kInput, true>();

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
                    const auto kSent = sv_total_statistics[kPortIndex].rdm.sent.classes + 1;
                    sv_total_statistics[kPortIndex].rdm.sent.classes = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
                } break;

                default:
                    [[unlikely]] assert(false && "switch");
                    break;
            }
        }

        TIMER_INTF(TIMER1) = (~TIMER_INT_FLAG_CH2);
    }
#endif // defined(DMX_USE_USART2)
// UART 3
#if defined(DMX_USE_UART3)
    if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH3) == TIMER_INT_FLAG_CH3) {
        constexpr auto kPortIndex = GetPortByUart(UART3);
        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            switch (s_DmxTxBuffer[kPortIndex].state) {
                case dmx::TxRxState::kDmxInter:
                    [[likely]] {
                        Gd32GpioModeOutput<UART3_GPIOx, UART3_TX_GPIO_PINx>();
                        GPIO_BC(UART3_GPIOx) = UART3_TX_GPIO_PINx;
                        s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
                        TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
                    }
                    break;
                case dmx::TxRxState::kDmxBreak:
                    [[likely]] {
                        Gd32GpioModeAf<UART3_GPIOx, UART3_TX_GPIO_PINx, UART3>();
                        s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxMab;
                        TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.mab_time;
                    }
                    break;
                case dmx::TxRxState::kDmxMab:
                    [[likely]] {
                        DMA_RESTART_DMX_TX(kPortIndex, UART3, UART3_DMAx, UART3_TX_DMA_CHx);
                    }
                    break;
                default:
                    [[unlikely]] break;
            }
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            switch (s_RdmTxBuffer[kPortIndex].state) {
                case dmx::RdmTxState::kBreak:
                    [[likely]] {
                        Gd32GpioModeAf<UART3_GPIOx, UART3_TX_GPIO_PINx, UART3>();
                        s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kMab;
                        TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kMabTimeTypical;
                    }
                    break;

                case dmx::RdmTxState::kMab:
                    [[likely]] {
                        DMA_START_RDM_TX(kPortIndex, UART3, UART3_DMAx, UART3_TX_DMA_CHx);
                    }
                    break;

                case dmx::RdmTxState::kDirection:
                    [[likely]] {
                        s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kIdle;
                        sv_port_state[kPortIndex] = dmx::PortState::kIdle;
                        Dmx::Get()->SetPortDirection<kPortIndex, dmx::Direction::kInput, true>();

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
                        const auto kSent = sv_total_statistics[kPortIndex].rdm.sent.classes + 1;
                        sv_total_statistics[kPortIndex].rdm.sent.classes = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
                    }
                    break;

                default:
                    [[unlikely]] assert(false && "switch");
                    break;
            }
        }

        TIMER_INTF(TIMER1) = (~TIMER_INT_FLAG_CH3);
    }
#endif // defined(DMX_USE_UART3)
    // Clear all remaining interrupt flags (safety measure)
    TIMER_INTF(TIMER1) = UINT32_MAX;
}

void TIMER4_IRQHandler() {
// UART 4
#if defined(DMX_USE_UART4)
    if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0) [[likely]] {
        constexpr auto kPortIndex = GetPortByUart(UART4);
        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            switch (s_DmxTxBuffer[kPortIndex].state) {
                case dmx::TxRxState::kDmxInter:
                    [[likely]] {
                        Gd32GpioModeOutput<UART4_TX_GPIOx, UART4_TX_GPIO_PINx>();
                        GPIO_BC(UART4_TX_GPIOx) = UART4_TX_GPIO_PINx;
                        s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
                        TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
                    }
                    break;

                case dmx::TxRxState::kDmxBreak:
                    [[likely]] {
                        Gd32GpioModeAf<UART4_TX_GPIOx, UART4_TX_GPIO_PINx, UART4>();
                        s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxMab;
                        TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.mab_time;
                    }
                    break;

                case dmx::TxRxState::kDmxMab:
                    [[likely]] {
                        DMA_RESTART_DMX_TX(kPortIndex, UART4, UART4_DMAx, UART4_TX_DMA_CHx);
                    }
                    break;

                default:
                    [[unlikely]] assert(false && "switch");
                    break;
            }
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            switch (s_RdmTxBuffer[kPortIndex].state) {
                case dmx::RdmTxState::kBreak:
                    [[likely]] {
                        Gd32GpioModeAf<UART4_TX_GPIOx, UART4_TX_GPIO_PINx, UART4>();
                        s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kMab;
                        TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kMabTimeTypical;
                    }
                    break;

                case dmx::RdmTxState::kMab:
                    [[likely]] {
                        DMA_START_RDM_TX(kPortIndex, UART4, UART4_DMAx, UART4_TX_DMA_CHx);
                    }
                    break;

                case dmx::RdmTxState::kDirection:
                    [[likely]] {
                        s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kIdle;
                        sv_port_state[kPortIndex] = dmx::PortState::kIdle;
                        Dmx::Get()->SetPortDirection<kPortIndex, dmx::Direction::kInput, true>();

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
                        const auto kSent = sv_total_statistics[kPortIndex].rdm.sent.classes + 1;
                        sv_total_statistics[kPortIndex].rdm.sent.classes = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
                    }
                    break;

                default:
                    [[unlikely]] assert(false && "switch");
                    break;
            }
        }

        TIMER_INTF(TIMER4) = (~TIMER_INT_FLAG_CH0);
    }
#endif // defined(DMX_USE_UART4)
// USART 5
#if defined(DMX_USE_USART5)
    if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH1) == TIMER_INT_FLAG_CH1) {
        constexpr auto kPortIndex = GetPortByUart(USART5);
        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            switch (s_DmxTxBuffer[kPortIndex].state) {
                case dmx::TxRxState::kDmxInter:
                    [[likely]] {
                        Gd32GpioModeOutput<USART5_GPIOx, USART5_TX_GPIO_PINx>();
                        GPIO_BC(USART5_GPIOx) = USART5_TX_GPIO_PINx;
                        s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
                        TIMER_CH1CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
                    }
                    break;

                case dmx::TxRxState::kDmxBreak:
                    [[likely]] {
                        Gd32GpioModeAf<USART5_GPIOx, USART5_TX_GPIO_PINx, USART5>();
                        s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxMab;
                        TIMER_CH1CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.mab_time;
                    }
                    break;

                case dmx::TxRxState::kDmxMab:
                    [[likely]] {
                        DMA_RESTART_DMX_TX(kPortIndex, USART5, USART5_DMAx, USART5_TX_DMA_CHx);
                    }
                    break;

                default:
                    [[unlikely]] assert(false && "switch");
                    break;
            }
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            switch (s_RdmTxBuffer[kPortIndex].state) {
                case dmx::RdmTxState::kBreak:
                    [[likely]] {
                        Gd32GpioModeAf<USART5_GPIOx, USART5_TX_GPIO_PINx, USART5>();
                        s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kMab;
                        TIMER_CH1CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kMabTimeTypical;
                    }
                    break;

                case dmx::RdmTxState::kMab:
                    [[likely]] {
                        DMA_START_RDM_TX(kPortIndex, USART5, USART5_DMAx, USART5_TX_DMA_CHx);
                    }
                    break;

                case dmx::RdmTxState::kDirection:
                    [[likely]] {
                        s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kIdle;
                        sv_port_state[kPortIndex] = dmx::PortState::kIdle;
                        Dmx::Get()->SetPortDirection<kPortIndex, dmx::Direction::kInput, true>();
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
                        const auto kSent = sv_total_statistics[kPortIndex].rdm.sent.classes + 1;
                        sv_total_statistics[kPortIndex].rdm.sent.classes = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
                    }
                    break;

                default:
                    [[unlikely]] assert(false && "switch");
                    break;
            }
        }

        TIMER_INTF(TIMER4) = (~TIMER_INT_FLAG_CH1);
    }
#endif // defined(DMX_USE_USART5)
// UART 6
#if defined(DMX_USE_UART6)
    if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH2) == TIMER_INT_FLAG_CH2) {
        constexpr auto kPortIndex = GetPortByUart(UART6);
        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            switch (s_DmxTxBuffer[kPortIndex].state) {
                case dmx::TxRxState::kDmxInter:
                    Gd32GpioModeOutput<UART6_GPIOx, UART6_TX_GPIO_PINx>();
                    GPIO_BC(UART6_GPIOx) = UART6_TX_GPIO_PINx;
                    s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
                    TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
                    break;
                case dmx::TxRxState::kDmxBreak:
                    Gd32GpioModeAf<UART6_GPIOx, UART6_TX_GPIO_PINx, UART6>();
                    s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxMab;
                    TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.mab_time;
                    break;
                case dmx::TxRxState::kDmxMab: {
                    DMA_RESTART_DMX_TX(kPortIndex, UART6, UART6_DMAx, UART6_TX_DMA_CHx);
                } break;
                default:
                    break;
            }
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            switch (s_RdmTxBuffer[kPortIndex].state) {
                case dmx::RdmTxState::kBreak:
                    [[likely]] {
                        Gd32GpioModeAf<UART4_TX_GPIOx, UART4_TX_GPIO_PINx, UART4>();
                        s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kMab;
                        TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kMabTimeTypical;
                    }
                    break;

                case dmx::RdmTxState::kMab:
                    [[likely]] {
                        DMA_START_RDM_TX(kPortIndex, UART6, UART6_DMAx, UART6_TX_DMA_CHx);
                    }
                    break;

                case dmx::RdmTxState::kDirection:
                    [[likely]] {
                        s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kIdle;
                        sv_port_state[kPortIndex] = dmx::PortState::kIdle;
                        Dmx::Get()->SetPortDirection<kPortIndex, dmx::Direction::kInput, true>();
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
                        const auto kSent = sv_total_statistics[kPortIndex].rdm.sent.classes + 1;
                        sv_total_statistics[kPortIndex].rdm.sent.classes = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
                    }
                    break;

                default:
                    [[unlikely]] assert(false && "switch");
                    break;
            }
        }

        TIMER_INTF(TIMER4) = (~TIMER_INT_FLAG_CH2);
    }
#endif // defined(DMX_USE_UART6)
// UART 7
#if defined(DMX_USE_UART7)
    if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH3) == TIMER_INT_FLAG_CH3) {
        constexpr auto kPortIndex = GetPortByUart(UART7);
        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            switch (s_DmxTxBuffer[kPortIndex].state) {
                case dmx::TxRxState::kDmxInter:
                    Gd32GpioModeOutput<UART7_GPIOx, UART7_TX_GPIO_PINx>();
                    GPIO_BC(UART7_GPIOx) = UART7_TX_GPIO_PINx;
                    s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
                    TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
                    break;
                case dmx::TxRxState::kDmxBreak:
                    Gd32GpioModeAf<UART7_GPIOx, UART7_TX_GPIO_PINx, UART7>();
                    s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxMab;
                    TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.mab_time;
                    break;
                case dmx::TxRxState::kDmxMab: {
                    DMA_RESTART_DMX_TX(kPortIndex, UART7, UART7_DMAx, UART7_TX_DMA_CHx);
                } break;
                default:
                    break;
            }
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            switch (s_RdmTxBuffer[kPortIndex].state) {
                case dmx::RdmTxState::kBreak:
                    [[likely]] {
                        Gd32GpioModeAf<UART7_GPIOx, UART7_TX_GPIO_PINx, UART7>();
                        s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kMab;
                        TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kMabTimeTypical;
                    }
                    break;

                case dmx::RdmTxState::kMab:
                    [[likely]] {
                        DMA_START_RDM_TX(kPortIndex, UART7, UART7_DMAx, UART7_TX_DMA_CHx);
                    }
                    break;

                case dmx::RdmTxState::kDirection:
                    [[likely]] {
                        s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kIdle;
                        sv_port_state[kPortIndex] = dmx::PortState::kIdle;
                        Dmx::Get()->SetPortDirection<kPortIndex, dmx::Direction::kInput, true>();
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
                        const auto kSent = sv_total_statistics[kPortIndex].rdm.sent.classes + 1;
                        sv_total_statistics[kPortIndex].rdm.sent.classes = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
                    }
                    break;

                default:
                    [[unlikely]] assert(false && "switch");
                    break;
            }
        }

        TIMER_INTF(TIMER4) = (~TIMER_INT_FLAG_CH3);
    }
#endif // defined(DMX_USE_UART7)
    // Clear all remaining interrupt flags (safety measure)
    TIMER_INTF(TIMER4) = UINT32_MAX;
}

void TIMER6_IRQHandler() {
    const auto kIntFlag = TIMER_INTF(TIMER6);

    if ((kIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP) {
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
        for (uint32_t i = 0; i < DMX_MAX_PORTS; i++) {
            auto& packet = sv_rx_dmx_packets[i];
            packet.per_second = packet.count - packet.count_previous;
            packet.count_previous = packet.count;
        }
#endif // !defined(CONFIG_DMX_TRANSMIT_ONLY)
        gv_seconds.uptime = gv_seconds.uptime + 1;
    }

    // Clear all remaining interrupt flags (safety measure)
    TIMER_INTF(TIMER6) = (~kIntFlag);
}

// USART 0
#if defined(DMX_USE_USART0)
#if defined(GD32F4XX) || defined(GD32H7XX)
void DMA1_Channel7_IRQHandler() {
    if (Gd32DmaInterruptFlagGet<DMA1, DMA_CH7, DMA_INTERRUPT_FLAG_GET>()) {
        Gd32DmaInterruptDisable<DMA1, DMA_CH7, DMA_INTERRUPT_DISABLE>();

        constexpr auto kPortIndex = GetPortByUart(USART0);

        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            if (s_DmxTxBuffer[kPortIndex].output_style == dmx::OutputStyle::kDelta) {
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kIdle;
            } else {
                TIMER_CH0CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.inter_time;
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxInter;
            }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[kPortIndex].dmx.sent + 1;
            sv_total_statistics[kPortIndex].dmx.sent = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            TIMER_CH0CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kDirectionTime;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kDirection;
        }
    }

    Gd32DmaInterruptFlagClear<DMA1, DMA_CH7, DMA_INTERRUPT_FLAG_CLEAR>();
}
#else // defined(GD32F4XX) || defined(GD32H7XX)
void DMA0_Channel3_IRQHandler() {
    if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_GET>()) {
        Gd32DmaInterruptDisable<DMA0, DMA_CH3, DMA_INTERRUPT_DISABLE>();

        constexpr auto kPortIndex = GetPortByUart(USART0);

        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            if (s_DmxTxBuffer[kPortIndex].output_style == dmx::OutputStyle::kDelta) {
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kIdle;
            } else {
                TIMER_CH0CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.inter_time;
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxInter;
            }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[kPortIndex].dmx.sent + 1;
            sv_total_statistics[kPortIndex].dmx.sent = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            TIMER_CH0CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kDirectionTime;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kDirection;
        }

        Gd32DmaInterruptFlagClear<DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR>();
    }
}
#endif // defined(GD32F4XX) || defined(GD32H7XX)
#endif // defined(DMX_USE_USART0)
// USART 1
#if defined(DMX_USE_USART1)
void DMA0_Channel6_IRQHandler() {
    if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH6, DMA_INTERRUPT_FLAG_GET>()) {
        Gd32DmaInterruptDisable<DMA0, DMA_CH6, DMA_INTERRUPT_DISABLE>();

        constexpr auto kPortIndex = GetPortByUart(USART1);

        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            if (s_DmxTxBuffer[kPortIndex].output_style == dmx::OutputStyle::kDelta) {
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kIdle;
            } else {
                TIMER_CH1CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.inter_time;
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxInter;
            }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[kPortIndex].dmx.sent + 1;
            sv_total_statistics[kPortIndex].dmx.sent = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            TIMER_CH1CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kDirectionTime;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kDirection;
        }
    }

    Gd32DmaInterruptFlagClear<DMA0, DMA_CH6, DMA_INTERRUPT_FLAG_CLEAR>();
}
#endif // defined(DMX_USE_USART1)
// USART 2
#if defined(DMX_USE_USART2)
#if defined(GD32F4XX) || defined(GD32H7XX)
void DMA0_Channel3_IRQHandler() {
    if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_GET>()) {
        Gd32DmaInterruptDisable<DMA0, DMA_CH3, DMA_INTERRUPT_DISABLE>();

        constexpr auto kPortIndex = GetPortByUart(USART2);

        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            if (s_DmxTxBuffer[kPortIndex].output_style == dmx::OutputStyle::kDelta) {
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kIdle;
            } else {
                TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.inter_time;
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxInter;
            }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[kPortIndex].dmx.sent + 1;
            sv_total_statistics[kPortIndex].dmx.sent = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kDirectionTime;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kDirection;
        }
    }

    Gd32DmaInterruptFlagClear<DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR>();
}
#else
void DMA0_Channel1_IRQHandler() {
    if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_GET>()) {
        Gd32DmaInterruptDisable<DMA0, DMA_CH1, DMA_INTERRUPT_DISABLE>();

        constexpr auto kPortIndex = GetPortByUart(USART2);

        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            if (s_DmxTxBuffer[kPortIndex].output_style == dmx::OutputStyle::kDelta) {
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kIdle;
            } else {
                TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.inter_time;
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxInter;
            }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[kPortIndex].dmx.sent + 1;
            sv_total_statistics[kPortIndex].dmx.sent = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kDirectionTime;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kDirection;
        }
    }

    Gd32DmaInterruptFlagClear<DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_CLEAR>();
}
#endif // defined(GD32F4XX) || defined(GD32H7XX)
#endif // defined(DMX_USE_USART2)
// UART 3
#if defined(DMX_USE_UART3)
#if defined(GD32F4XX) || defined(GD32H7XX)
void DMA0_Channel4_IRQHandler() {
    if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH4, DMA_INTERRUPT_FLAG_GET>()) {
        Gd32DmaInterruptDisable<DMA0, DMA_CH4, DMA_INTERRUPT_DISABLE>();

        constexpr auto kPortIndex = GetPortByUart(UART3);

        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            if (s_DmxTxBuffer[kPortIndex].output_style == dmx::OutputStyle::kDelta) {
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kIdle;
            } else {
                TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.inter_time;
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxInter;
            }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[kPortIndex].dmx.sent + 1;
            sv_total_statistics[kPortIndex].dmx.sent = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kDirectionTime;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kDirection;
        }
    }

    Gd32DmaInterruptFlagClear<DMA0, DMA_CH4, DMA_INTERRUPT_FLAG_CLEAR>();
}
#else
void DMA1_Channel4_IRQHandler() {
    if (Gd32DmaInterruptFlagGet<DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_GET>()) {
        Gd32DmaInterruptDisable<DMA1, DMA_CH4, DMA_INTERRUPT_DISABLE>();

        constexpr auto kPortIndex = GetPortByUart(UART3);

        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            if (s_DmxTxBuffer[kPortIndex].output_style == dmx::OutputStyle::kDelta) {
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kIdle;
            } else {
                TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.inter_time;
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxInter;
            }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[kPortIndex].dmx.sent + 1;
            sv_total_statistics[kPortIndex].dmx.sent = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kDirectionTime;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kDirection;
        }
    }

    Gd32DmaInterruptFlagClear<DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_CLEAR>();
}
#endif // defined(GD32F4XX) || defined(GD32H7XX)
#endif // defined(DMX_USE_UART3)
// UART 4
#if defined(DMX_USE_UART4)
#if defined(GD32F20X)
void DMA1_Channel3_IRQHandler() {
    if (Gd32DmaInterruptFlagGet<DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_GET>()) {
        Gd32DmaInterruptDisable<DMA1, DMA_CH3, DMA_INTERRUPT_DISABLE>();

        constexpr auto kPortIndex = GetPortByUart(UART4);

        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            if (s_DmxTxBuffer[kPortIndex].output_style == dmx::OutputStyle::kDelta) {
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kIdle;
            } else {
                TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.inter_time;
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxInter;
            }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[kPortIndex].dmx.sent + 1;
            sv_total_statistics[kPortIndex].dmx.sent = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kDirectionTime;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kDirection;
        }
    }

    Gd32DmaInterruptFlagClear<DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR>();
}
#endif // defined(GD32F20X)
#if defined(GD32F4XX) || defined(GD32H7XX)
void DMA0_Channel7_IRQHandler() {
    if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH7, DMA_INTERRUPT_FLAG_GET>()) {
        Gd32DmaInterruptDisable<DMA0, DMA_CH7, DMA_INTERRUPT_DISABLE>();

        constexpr auto kPortIndex = GetPortByUart(UART4);

        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            if (s_DmxTxBuffer[kPortIndex].output_style == dmx::OutputStyle::kDelta) {
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kIdle;
            } else {
                TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.inter_time;
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxInter;
            }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[kPortIndex].dmx.sent + 1;
            sv_total_statistics[kPortIndex].dmx.sent = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kDirectionTime;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kDirection;
        }
    }

    Gd32DmaInterruptFlagClear<DMA0, DMA_CH7, DMA_INTERRUPT_FLAG_CLEAR>();
}
#endif // defined(GD32F4XX) || defined(GD32H7XX)
#if defined(GD32F10X) || defined(GD32F30X)
#error Not available
#endif // defined(GD32F10X) || defined(GD32F30X)
#endif // defined(DMX_USE_UART4)
// USART 5
#if defined(DMX_USE_USART5)
void DMA1_Channel6_IRQHandler() {
    if (Gd32DmaInterruptFlagGet<DMA1, DMA_CH6, DMA_INTERRUPT_FLAG_GET>()) {
        Gd32DmaInterruptDisable<DMA1, DMA_CH6, DMA_INTERRUPT_DISABLE>();

        constexpr auto kPortIndex = GetPortByUart(USART5);

        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            if (s_DmxTxBuffer[kPortIndex].output_style == dmx::OutputStyle::kDelta) {
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kIdle;
            } else {
                TIMER_CH1CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.inter_time;
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxInter;
            }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[kPortIndex].dmx.sent + 1;
            sv_total_statistics[kPortIndex].dmx.sent = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            TIMER_CH1CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kDirectionTime;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kDirection;
        }
    }

    Gd32DmaInterruptFlagClear<DMA1, DMA_CH6, DMA_INTERRUPT_FLAG_CLEAR>();
}
#endif // defined(DMX_USE_USART5)
// UART 6
#if defined(DMX_USE_UART6)
#if defined(GD32F20X)
void DMA1_Channel4_IRQHandler() {
    if (Gd32DmaInterruptFlagGet<DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_GET>()) {
        Gd32DmaInterruptDisable<DMA1, DMA_CH4, DMA_INTERRUPT_DISABLE>();

        if (s_DmxTxBuffer[dmx::config::kUart6Port].state != dmx::TxRxState::kIdle) [[likely]] {
            if (s_DmxTxBuffer[dmx::config::kUart6Port].output_style == dmx::OutputStyle::kDelta) {
                s_DmxTxBuffer[dmx::config::kUart6Port].state = dmx::TxRxState::kIdle;
            } else {
                TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.inter_time;
                s_DmxTxBuffer[dmx::config::kUart6Port].state = dmx::TxRxState::kDmxInter;
            }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[dmx::config::kUart6Port].dmx.sent + 1;
            sv_total_statistics[dmx::config::kUart6Port].dmx.sent = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        } else if (s_RdmTxBuffer[dmx::config::kUart6Port].state != dmx::RdmTxState::kIdle) {
            TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kDirectionTime;
            s_RdmTxBuffer[dmx::config::kUart6Port].state = dmx::RdmTxState::kDirection;
        }
    }
}

Gd32DmaInterruptFlagClear<DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_CLEAR>();
}
#endif // defined(GD32F20X)
#if defined(GD32F4XX) || defined(GD32H7XX)
void DMA0_Channel1_IRQHandler() {
    if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_GET>()) {
        Gd32DmaInterruptDisable<DMA0, DMA_CH1, DMA_INTERRUPT_DISABLE>();

        constexpr auto kPortIndex = GetPortByUart(UART6);

        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            if (s_DmxTxBuffer[kPortIndex].output_style == dmx::OutputStyle::kDelta) {
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kIdle;
            } else {
                TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.inter_time;
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxInter;
            }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[kPortIndex].dmx.sent + 1;
            sv_total_statistics[kPortIndex].dmx.sent = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kDirectionTime;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kDirection;
        }
    }

    Gd32DmaInterruptFlagClear<DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_CLEAR>();
}
#endif // defined(GD32F4XX) || defined(GD32H7XX)
#endif // defined(DMX_USE_UART6)
// UART 7
#if defined(DMX_USE_UART7)
#if defined(GD32F20X)
void DMA1_Channel3_IRQHandler() {
    if (Gd32DmaInterruptFlagGet<DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_GET>()) {
        Gd32DmaInterruptDisable<DMA1, DMA_CH3, DMA_INTERRUPT_DISABLE>();

        if (s_DmxTxBuffer[dmx::config::kUart7Port].state != dmx::TxRxState::kIdle) [[likely]] {
            if (s_DmxTxBuffer[dmx::config::kUart7Port].output_style == dmx::OutputStyle::kDelta) {
                s_DmxTxBuffer[dmx::config::kUart7Port].state = dmx::TxRxState::kIdle;
            } else {
                TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.inter_time;
                s_DmxTxBuffer[dmx::config::kUart7Port].state = dmx::TxRxState::kDmxInter;
            }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            sv_total_statistics[dmx::config::kUart7Port].dmx.sent++;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        } else if (s_RdmTxBuffer[dmx::config::kUart7Port].state != dmx::RdmTxState::kIdle) {
            TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kDirectionTime;
            s_RdmTxBuffer[dmx::config::kUart7Port].state = dmx::RdmTxState::kDirection;
        }
    }

    Gd32DmaInterruptFlagClear<DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR>();
}
#endif // defined(GD32F20X)
#if defined(GD32F4XX) || defined(GD32H7XX)
void DMA0_Channel0_IRQHandler() {
    if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH0, DMA_INTERRUPT_FLAG_GET>()) {
        Gd32DmaInterruptDisable<DMA0, DMA_CH0, DMA_INTERRUPT_DISABLE>();

        constexpr auto kPortIndex = GetPortByUart(UART7);

        if (s_DmxTxBuffer[kPortIndex].state != dmx::TxRxState::kIdle) [[likely]] {
            if (s_DmxTxBuffer[kPortIndex].output_style == dmx::OutputStyle::kDelta) {
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kIdle;
            } else {
                TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.inter_time;
                s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxInter;
            }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[kPortIndex].dmx.sent + 1;
            sv_total_statistics[kPortIndex].dmx.sent = kSent;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        } else if (s_RdmTxBuffer[kPortIndex].state != dmx::RdmTxState::kIdle) {
            TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kDirectionTime;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kDirection;
        }
    }

    Gd32DmaInterruptFlagClear<DMA0, DMA_CH0, DMA_INTERRUPT_FLAG_CLEAR>();
}
#endif // defined(GD32F4XX) || defined(GD32H7XX)
#if defined(GD32F10X) || defined(GD32F30X)
#error Not available
#endif // defined(GD32F10X) || defined(GD32F30X)
#endif // defined(DMX_USE_UART7)
}

[[gnu::noinline]]
void Dmx::SetPortDirection(uint32_t port_index, dmx::Direction port_direction, bool enable_data) {
    DMX_CHECK_PORT_INDEX_VOID(port_index);

    if (port_direction_[port_index] != port_direction) {
        port_direction_[port_index] = port_direction;

        DataDisable(port_index);

        if (port_direction == dmx::Direction::kOutput) {
            GPIO_BOP(kDirGpio[port_index].port) = kDirGpio[port_index].pin;
        } else if (port_direction == dmx::Direction::kInput) {
            GPIO_BC(kDirGpio[port_index].port) = kDirGpio[port_index].pin;
        } else [[unlikely]] {
            assert(false && "Invalid direction");
        }
    } else if (!enable_data) {
        DataDisable(port_index);
    }

    if (enable_data) {
        DataEnable(port_index);
    }
}

template <uint32_t kPortIndex, dmx::Direction kPortDirection, bool kEnableData>
void Dmx::SetPortDirection() {
    static_assert(kPortIndex < dmx::config::max::kPorts);

    if constexpr ((kDirGpio[kPortIndex].port == 0) && (kDirGpio[kPortIndex].pin == 0)) {
        return;
    }

    if constexpr (kDirGpio[kPortIndex].usage == dmx::port::Usage::kTxRx) {
        if (port_direction_[kPortIndex] != kPortDirection) {
            port_direction_[kPortIndex] = kPortDirection;

            DataDisable(kPortIndex);

            if constexpr (kPortDirection == dmx::Direction::kOutput) {
                GPIO_BOP(kDirGpio[kPortIndex].port) = kDirGpio[kPortIndex].pin;
            } else if constexpr (kPortDirection == dmx::Direction::kInput) {
                GPIO_BC(kDirGpio[kPortIndex].port) = kDirGpio[kPortIndex].pin;
            } else {
                static_assert(false, "Invalid direction");
            }
        } else if constexpr (!kEnableData) {
            DataDisable(kPortIndex);
        }

        if constexpr (kEnableData) {
            DataEnable(kPortIndex);
        }
    }
}

void Dmx::DataEnable(uint32_t port_index) {
    DMX_DEBUG_PRINTF("port_index=%u", port_index);
    DMX_CHECK_PORT_INDEX_VOID(port_index);
    assert(sv_port_state[port_index] == dmx::PortState::kIdle);

    if (port_direction_[port_index] == dmx::Direction::kOutput) {
        sv_port_state[port_index] = dmx::PortState::kTx;
        s_DmxTxBuffer[port_index].state = dmx::TxRxState::kIdle;
        SetOutputStyle(port_index, GetOutputStyle(port_index));
        return;
    }

    if (port_direction_[port_index] == dmx::Direction::kInput) {
        sv_rx_buffer[port_index].state = dmx::TxRxState::kIdle;

        const auto kUart = std::to_underlying(kDirGpio[port_index].uart);

        do {
            __DMB();
        } while (!gd32::UartFlagGet<USART_FLAG_TBE>(kUart));

        gd32::UartInterruptFlagClear<USART_INT_FLAG_RBNE>(kUart);
        gd32::UartInterruptFlagClear<USART_INT_FLAG_IDLE>(kUart);
        gd32::UartInterruptEnable<USART_INT_RBNE>(kUart);
        gd32::UartInterruptEnable<USART_INT_FLAG_IDLE>(kUart);

        sv_port_state[port_index] = dmx::PortState::kRx;
        return;
    }

    assert(false && "Not reachable");
}

void Dmx::DataDisable(uint32_t port_index) {
    DMX_CHECK_PORT_INDEX_VOID(port_index);

    if (sv_port_state[port_index] == dmx::PortState::kIdle) {
        return;
    }

    sv_port_state[port_index] = dmx::PortState::kIdle;

    const auto kUart = std::to_underlying(kDirGpio[port_index].uart);

    if (port_direction_[port_index] == dmx::Direction::kOutput) {
        do {
            if (s_DmxTxBuffer[port_index].state == dmx::TxRxState::kDmxInter) {
                gd32::UartFlagClear<USART_FLAG_TC>(kUart);
                do {
                    __DMB();
                } while (!gd32::UartFlagGet<USART_FLAG_TC>(kUart));

                s_DmxTxBuffer[port_index].state = dmx::TxRxState::kIdle;
            }
        } while (s_DmxTxBuffer[port_index].state != dmx::TxRxState::kIdle);

        return;
    }

    if (port_direction_[port_index] == dmx::Direction::kInput) {
        gd32::UartInterruptDisable<USART_INT_RBNE>(kUart);
        gd32::UartInterruptDisable<USART_INT_FLAG_IDLE>(kUart);
        sv_rx_buffer[port_index].state = dmx::TxRxState::kIdle;
        return;
    }

    assert(false && "Not reachable");
}

void Dmx::ClearData(uint32_t port_index) {
    assert(port_index < dmx::config::max::kPorts);

    auto* data = &s_DmxTxBuffer[port_index].dmx.data[0];
    data->length = dmx::kSlotsMax; // Including START Code
    __builtin_memset(data->data, 0, dmx::buffer::kSize);
}

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
volatile dmx::TotalStatistics& Dmx::GetTotalStatistics(uint32_t port_index) {
    sv_total_statistics[port_index].dmx.received = sv_rx_dmx_packets[port_index].count;
    return sv_total_statistics[port_index];
}
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)

void Dmx::Blackout() {
    DMX_DEBUG_ENTRY();

    for (uint32_t port_index = 0; port_index < dmx::config::max::kPorts; port_index++) {
        if (port_direction_[port_index] == dmx::Direction::kOutput) {
            DataDisable(port_index);
            ClearData(port_index);
            DataEnable(port_index);
        }
    }

    DMX_DEBUG_EXIT();
}

void Dmx::FullOn() {
    DMX_DEBUG_ENTRY();

    for (uint32_t port_index = 0; port_index < dmx::config::max::kPorts; port_index++) {
        if (port_direction_[port_index] == dmx::Direction::kOutput) {
            DataDisable(port_index);

            auto* __restrict__ data = &s_DmxTxBuffer[port_index].dmx.data[0];
            auto* __restrict__ p32 = reinterpret_cast<uint32_t*>(data->data);

            for (auto i = 0; i < dmx::buffer::kSize / 4; i++) {
                *p32++ = UINT32_MAX;
            }

            data->data[0] = dmx::kStartCode;
            data->length = dmx::kSlotsMax;

            DataEnable(port_index);
        }
    }

    DMX_DEBUG_EXIT();
}

// DMX Send
template <uint32_t kPortIndex, bool kHasStartCode, dmx::SendStyle kSendStyle>
void Dmx::SetSendDataInternal(const uint8_t* data, uint32_t length) {
    static_assert(kPortIndex < dmx::config::max::kPorts);

    if constexpr (kDirGpio[kPortIndex].usage == dmx::port::Usage::kRxOnly) {
        return;
    }

    auto& tx_buffer = s_DmxTxBuffer[kPortIndex];
    const auto kHasDataPending = tx_buffer.dmx.read_index != tx_buffer.dmx.write_index;

    if (!kHasDataPending) {
        // No pending data — switch to the other buffer
        tx_buffer.dmx.write_index ^= 1;
    }

    const auto kWriteIndex = tx_buffer.dmx.write_index;

    auto* dst_data = tx_buffer.dmx.data[kWriteIndex].data;

    const auto kCappedLength = (length < transmit_slots_) ? length : transmit_slots_;
    tx_buffer.dmx.data[kWriteIndex].length = kCappedLength + 1;

    tx_buffer.dmx.data_pending = true;

    if constexpr (kHasStartCode) {
        memcpy(dst_data, data, kCappedLength);
    } else {
        dst_data[0] = dmx::kStartCode;
        memcpy(&dst_data[1], data, kCappedLength);
    }

    if (kCappedLength != transmit_length_[kPortIndex]) {
        transmit_length_[kPortIndex] = kCappedLength;
        SetTransmitPeriodTime(transmit_period_requested_);
    }

    if constexpr (kSendStyle == dmx::SendStyle::kDirect) {
        StartSendStyleDirect(kPortIndex);
    }
}

void Dmx::StartSendStyleDirect(uint32_t port_index) {
    DMX_CHECK_PORT_INDEX_VOID(port_index);

    if ((sv_port_state[port_index] == dmx::PortState::kTx) && (s_DmxTxBuffer[port_index].output_style == dmx::OutputStyle::kDelta) && (s_DmxTxBuffer[port_index].state == dmx::TxRxState::kIdle)) {
        StartDmxOutput(port_index);
    }
}

template <uint32_t kPortIndex>
void StartDmxOutputBreak() {
    if constexpr ((kDirGpio[kPortIndex].port == 0) && (kDirGpio[kPortIndex].pin == 0)) {
        return;
    }

    constexpr auto kUsartPeripheral = std::to_underlying(kDirGpio[kPortIndex].uart);

    // USART_FLAG_TC is set after power on.
    // The flag is cleared by DMA interrupt when maximum slots - 1 are transmitted.
    // TODO(a): Do we need a timeout just to be safe?
    while (SET != usart_flag_get(kUsartPeripheral, USART_FLAG_TC)) {
    }

    switch (kUsartPeripheral) {
// TIMER 1
#if defined(DMX_USE_USART0)
        case USART0:
            Gd32GpioModeOutput<USART0_GPIOx, USART0_TX_GPIO_PINx>();
            GPIO_BC(USART0_GPIOx) = USART0_TX_GPIO_PINx;
            TIMER_CH0CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
            s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
            return;
            break;
#endif // defined(DMX_USE_USART0)
#if defined(DMX_USE_USART1)
        case USART1:
            Gd32GpioModeOutput<USART1_GPIOx, USART1_TX_GPIO_PINx>();
            GPIO_BC(USART1_GPIOx) = USART1_TX_GPIO_PINx;
            TIMER_CH1CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
            s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
            return;
            break;
#endif // defined(DMX_USE_USART1)
#if defined(DMX_USE_USART2)
        case USART2:
            Gd32GpioModeOutput<USART2_GPIOx, USART2_TX_GPIO_PINx>();
            GPIO_BC(USART2_GPIOx) = USART2_TX_GPIO_PINx;
            TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
            s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
            return;
            break;
#endif // defined(DMX_USE_USART2)
#if defined(DMX_USE_UART3)
        case UART3:
            Gd32GpioModeOutput<UART3_GPIOx, UART3_TX_GPIO_PINx>();
            GPIO_BC(UART3_GPIOx) = UART3_TX_GPIO_PINx;
            TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
            s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
            return;
            break;
#endif // defined(DMX_USE_UART3)
// TIMER 4
#if defined(DMX_USE_UART4)
        case UART4:
            Gd32GpioModeOutput<UART4_TX_GPIOx, UART4_TX_GPIO_PINx>();
            GPIO_BC(UART4_TX_GPIOx) = UART4_TX_GPIO_PINx;
            TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
            s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
            return;
            break;
#endif // defined(DMX_USE_UART4)
#if defined(DMX_USE_USART5)
        case USART5:
            Gd32GpioModeOutput<USART5_GPIOx, USART5_TX_GPIO_PINx>();
            GPIO_BC(USART5_GPIOx) = USART5_TX_GPIO_PINx;
            TIMER_CH1CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
            s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
            return;
            break;
#endif // defined(DMX_USE_USART5)
#if defined(DMX_USE_UART6)
        case UART6:
            Gd32GpioModeOutput<UART6_GPIOx, UART6_TX_GPIO_PINx>();
            GPIO_BC(UART6_GPIOx) = UART6_TX_GPIO_PINx;
            TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
            s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
            return;
            break;
#endif // defined(DMX_USE_UART6)
#if defined(DMX_USE_UART7)
        case UART7:
            Gd32GpioModeOutput<UART7_GPIOx, UART7_TX_GPIO_PINx>();
            GPIO_BC(UART7_GPIOx) = UART7_TX_GPIO_PINx;
            TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
            s_DmxTxBuffer[kPortIndex].state = dmx::TxRxState::kDmxBreak;
            return;
            break;
#endif // defined(DMX_USE_UART7)
        default:
            [[unlikely]] assert(false && "switch");
            break;
    }

    assert(false && "Not reachable");
}

template <uint32_t kPortIndex>
void StartDmxOutputPort() {
    if constexpr (kPortIndex < dmx::config::max::kPorts) {
        if constexpr (kDirGpio[kPortIndex].usage != dmx::port::Usage::kRxOnly) {
            StartDmxOutputBreak<kPortIndex>();
        }
    }
}

void Dmx::StartDmxOutput(uint32_t port_index) {
    assert(port_index < dmx::config::max::kPorts);

    switch (port_index) {
        case 0:
            StartDmxOutputPort<0>();
            break;
#if DMX_MAX_PORTS >= 2
        case 1:
            StartDmxOutputPort<1>();
            break;
#endif // DMX_MAX_PORTS >= 2
#if DMX_MAX_PORTS >= 3
        case 2:
            StartDmxOutputPort<2>();
            break;
#endif // DMX_MAX_PORTS >= 3
#if DMX_MAX_PORTS >= 4
        case 3:
            StartDmxOutputPort<3>();
            break;
#endif // DMX_MAX_PORTS >= 4
#if DMX_MAX_PORTS >= 5
        case 4:
            StartDmxOutputPort<4>();
            break;
#endif // DMX_MAX_PORTS >= 5
#if DMX_MAX_PORTS >= 6
        case 5:
            StartDmxOutputPort<5>();
            break;
#endif // DMX_MAX_PORTS >= 6
#if DMX_MAX_PORTS >= 7
        case 6:
            StartDmxOutputPort<6>();
            break;
#endif // DMX_MAX_PORTS >= 7
#if DMX_MAX_PORTS >= 8
        case 7:
            StartDmxOutputPort<7>();
            break;
#endif // DMX_MAX_PORTS >= 8
        default:
            [[unlikely]] break;
    }
}

// RDM DMA Send
#define RDM_HANDLE_SEND_CASE(i) \
    case i:                     \
        return RdmSendDataInternal<i>(data, length)

void Dmx::RdmTransmit(uint32_t port_index, const uint8_t* data, uint32_t length) {
    switch (port_index) {
        RDM_HANDLE_SEND_CASE(0);
#if DMX_MAX_PORTS >= 2
        RDM_HANDLE_SEND_CASE(1);
#endif // DMX_MAX_PORTS >= 2
#if DMX_MAX_PORTS >= 3
        RDM_HANDLE_SEND_CASE(2);
#endif // DMX_MAX_PORTS >= 3
#if DMX_MAX_PORTS >= 4
        RDM_HANDLE_SEND_CASE(3);
#endif // DMX_MAX_PORTS >= 4
#if DMX_MAX_PORTS >= 5
        RDM_HANDLE_SEND_CASE(4);
#endif // DMX_MAX_PORTS >= 5
#if DMX_MAX_PORTS >= 6
        RDM_HANDLE_SEND_CASE(5);
#endif // DMX_MAX_PORTS >= 6
#if DMX_MAX_PORTS >= 7
        RDM_HANDLE_SEND_CASE(6);
#endif // DMX_MAX_PORTS >= 7
#if DMX_MAX_PORTS == 8
        RDM_HANDLE_SEND_CASE(7);
#endif // DMX_MAX_PORTS == 8
        default:
            [[unlikely]] assert(false && "switch");
            return;
    }
}

template <uint32_t kPortIndex>
void Dmx::RdmSendDataInternal(const uint8_t* data, uint32_t length) {
    static_assert(kPortIndex < dmx::config::max::kPorts);

    if constexpr (kDirGpio[kPortIndex].usage == dmx::port::Usage::kRxOnly) {
        return;
    }

    assert(data != nullptr);
    assert(length <= sizeof(TRdmMessage));

    SetPortDirection<kPortIndex, dmx::Direction::kOutput, false>();

    auto& tx_buffer = s_RdmTxBuffer[kPortIndex];

    auto* dst_data = tx_buffer.rdm.data.data;
    auto& dst_length = tx_buffer.rdm.data.length;
    dst_length = length;

    memcpy(dst_data, data, length);

    StartRdmOutput(kPortIndex);
}

template <uint32_t kPortIndex>
void StartRdmOutput() {
    static_assert(kPortIndex < dmx::config::max::kPorts);

    if constexpr (kDirGpio[kPortIndex].usage == dmx::port::Usage::kRxOnly) {
        return;
    }

    constexpr auto kUsartPeripheral = std::to_underlying(kDirGpio[kPortIndex].uart);

    DMX_DEBUG_PRINTF("port_index=%u, uart=%p", kPortIndex, reinterpret_cast<void*>(kUsartPeripheral));
    // USART_FLAG_TC is set after power on.
    // The flag is cleared by DMA interrupt when maximum slots - 1 are transmitted.
    // TODO(a): Do we need a timeout just to be safe?
    while (SET != usart_flag_get(kUsartPeripheral, USART_FLAG_TC)) {
    }

    switch (kUsartPeripheral) {
        // TIMER 1
#if defined(DMX_USE_USART0)
        case USART0: {
            Gd32GpioModeOutput<USART0_GPIOx, USART0_TX_GPIO_PINx>();
            GPIO_BC(USART0_GPIOx) = USART0_TX_GPIO_PINx;
            TIMER_CH0CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kBreakTimeTypical;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kBreak;
            return;
        } break;
#endif // defined(DMX_USE_USART0)

#if defined(DMX_USE_USART1)
        case USART1: {
            Gd32GpioModeOutput<USART1_GPIOx, USART1_TX_GPIO_PINx>();
            GPIO_BC(USART1_GPIOx) = USART1_TX_GPIO_PINx;
            TIMER_CH1CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kBreakTimeTypical;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kBreak;
            return;
        } break;
#endif // defined(DMX_USE_USART1)

#if defined(DMX_USE_USART2)
        case USART2: {
            Gd32GpioModeOutput<USART2_GPIOx, USART2_TX_GPIO_PINx>();
            GPIO_BC(USART2_GPIOx) = USART2_TX_GPIO_PINx;
            TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kBreakTimeTypical;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kBreak;
            return;
        } break;
#endif // defined(DMX_USE_USART2)

#if defined(DMX_USE_UART3)
        case UART3: {
            Gd32GpioModeOutput<UART3_GPIOx, UART3_TX_GPIO_PINx>();
            GPIO_BC(UART3_GPIOx) = UART3_TX_GPIO_PINx;
            TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + rdm::transmit::kBreakTimeTypical;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kBreak;
            return;
        } break;
#endif // defined(DMX_USE_UART3)
       // TIMER 4
#if defined(DMX_USE_UART4)
        case UART4: {
            Gd32GpioModeOutput<UART4_TX_GPIOx, UART4_TX_GPIO_PINx>();
            GPIO_BC(UART4_TX_GPIOx) = UART4_TX_GPIO_PINx;
            TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kBreakTimeTypical;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kBreak;
            return;
        } break;
#endif // defined(DMX_USE_UART4)

#if defined(DMX_USE_USART5)
        case USART5: {
            Gd32GpioModeOutput<USART5_GPIOx, USART5_TX_GPIO_PINx>();
            GPIO_BC(USART5_GPIOx) = USART5_TX_GPIO_PINx;
            TIMER_CH1CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kBreakTimeTypical;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kBreak;
            return;
        } break;
#endif // defined(DMX_USE_USART5)

#if defined(DMX_USE_UART6)
        case UART6: {
            Gd32GpioModeOutput<UART6_GPIOx, UART6_TX_GPIO_PINx>();
            GPIO_BC(UART6_GPIOx) = UART6_TX_GPIO_PINx;
            TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kBreakTimeTypical;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kBreak;
            return;
        } break;
#endif // defined(DMX_USE_UART6)

#if defined(DMX_USE_UART7)
        case UART7: {
            Gd32GpioModeOutput<UART7_GPIOx, UART7_TX_GPIO_PINx>();
            GPIO_BC(UART7_GPIOx) = UART7_TX_GPIO_PINx;
            TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + rdm::transmit::kBreakTimeTypical;
            s_RdmTxBuffer[kPortIndex].state = dmx::RdmTxState::kBreak;
            return;
        } break;
#endif // defined(DMX_USE_UART7)

        default:
            [[unlikely]] assert(false && "switch");
            break;
    }

    assert(false && "Not reachable");
}

template <uint32_t kPortIndex>
void StartRdmOutputPort() {
    if constexpr (kPortIndex < dmx::config::max::kPorts) {
        if constexpr (kDirGpio[kPortIndex].usage != dmx::port::Usage::kRxOnly) {
            StartRdmOutput<kPortIndex>();
        }
    }
}

void Dmx::StartRdmOutput(uint32_t port_index) {
    assert(port_index < dmx::config::max::kPorts);

    switch (port_index) {
        case 0:
            StartRdmOutputPort<0>();
            break;
#if DMX_MAX_PORTS >= 2
        case 1:
            StartRdmOutputPort<1>();
            break;
#endif // DMX_MAX_PORTS >= 2
#if DMX_MAX_PORTS >= 3
        case 2:
            StartRdmOutputPort<2>();
            break;
#endif // DMX_MAX_PORTS >= 3
#if DMX_MAX_PORTS >= 4
        case 3:
            StartRdmOutputPort<3>();
            break;
#endif // DMX_MAX_PORTS >= 4
#if DMX_MAX_PORTS >= 5
        case 4:
            StartRdmOutputPort<4>();
            break;
#endif // DMX_MAX_PORTS >= 5
#if DMX_MAX_PORTS >= 6
        case 5:
            StartRdmOutputPort<5>();
            break;
#endif // DMX_MAX_PORTS >= 6
#if DMX_MAX_PORTS >= 7
        case 6:
            StartRdmOutputPort<6>();
            break;
#endif // DMX_MAX_PORTS >= 7
#if DMX_MAX_PORTS >= 8
        case 7:
            StartRdmOutputPort<7>();
            break;
#endif // DMX_MAX_PORTS >= 8
        default:
            [[unlikely]] break;
    }
}

// DMX Output Synchronization
void Dmx::Sync() {
    for (uint32_t port_index = 0; port_index < dmx::config::max::kPorts; port_index++) {
        auto& tx_buffer = s_DmxTxBuffer[port_index];

        if (!tx_buffer.dmx.data_pending) {
            continue;
        }

        tx_buffer.dmx.data_pending = false;

        if (sv_port_state[port_index] == dmx::PortState::kTx) {
            if ((tx_buffer.output_style == dmx::OutputStyle::kDelta) && (tx_buffer.state == dmx::TxRxState::kIdle)) {
                StartDmxOutput(port_index);
            }
        }
    }
}

// DMX Receive
const uint8_t* Dmx::GetDmxChanged([[maybe_unused]] uint32_t port_index) {
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
    const auto* __restrict__ available = GetDmxAvailable(port_index);

    if (available == nullptr) {
        return nullptr;
    }

    const auto* __restrict__ src32 = reinterpret_cast<const volatile uint32_t*>(sv_rx_buffer[port_index].dmx.current.data);
    auto* __restrict__ dst32 = reinterpret_cast<volatile uint32_t*>(sv_rx_buffer[port_index].dmx.previous.data);

    if (sv_rx_buffer[port_index].dmx.current.slots_in_packet != sv_rx_buffer[port_index].dmx.previous.slots_in_packet) {
        sv_rx_buffer[port_index].dmx.previous.slots_in_packet = sv_rx_buffer[port_index].dmx.current.slots_in_packet;

        for (size_t i = 0; i < dmx::buffer::kSize / 4; ++i) {
            dst32[i] = src32[i];
        }

        return available;
    }

    bool is_changed = false;

    for (size_t i = 0; i < dmx::buffer::kSize / 4; ++i) {
        const auto kSrcValue = src32[i];
        auto dst_value = dst32[i];

        if (kSrcValue != dst_value) {
            dst32[i] = kSrcValue;
            is_changed = true;
        }
    }

    return (is_changed ? available : nullptr);
#else
    return nullptr;
#endif // !defined(CONFIG_DMX_TRANSMIT_ONLY)
}

const uint8_t* Dmx::GetDmxAvailable([[maybe_unused]] uint32_t port_index) {
    DMX_CHECK_PORT_INDEX_PTR(port_index);
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
    auto slots_in_packet = sv_rx_buffer[port_index].dmx.current.slots_in_packet;

    if ((slots_in_packet & dmx::kDmxSlotsCompleteFlag) != dmx::kDmxSlotsCompleteFlag) {
        return nullptr;
    }

    slots_in_packet &= ~dmx::kDmxSlotsCompleteFlag;
    slots_in_packet--; // Remove SC from length
    sv_rx_buffer[port_index].dmx.current.slots_in_packet = slots_in_packet;

    return const_cast<const uint8_t*>(sv_rx_buffer[port_index].dmx.current.data);
#else
    return nullptr;
#endif // !defined(CONFIG_DMX_TRANSMIT_ONLY)
}

const uint8_t* Dmx::GetDmxCurrentData(uint32_t port_index) {
    return const_cast<const uint8_t*>(sv_rx_buffer[port_index].dmx.current.data);
}

uint32_t Dmx::GetDmxUpdatesPerSecond([[maybe_unused]] uint32_t port_index) {
    DMX_CHECK_PORT_INDEX_RET(port_index, 0);
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
    return sv_rx_dmx_packets[port_index].per_second;
#else
    return 0;
#endif // !defined(CONFIG_DMX_TRANSMIT_ONLY)
}

// RDM Send Discovery Response Message
void Dmx::RdmTransmitDiscoveryRespondMessage(uint32_t port_index, const uint8_t* data, uint32_t length) {
    DMX_CHECK_PORT_INDEX_VOID(port_index);
    assert(data != nullptr);
    assert(length != 0);

    // 3.2.2 Responder Packet spacing
    timing::DelayUs(rdm::responder::kPacketSpacing, gsv_rdm_data_receive_end[port_index]);

    SetPortDirection(port_index, dmx::Direction::kOutput, false);

    const auto kUart = std::to_underlying(kDirGpio[port_index].uart);

    for (uint32_t i = 0; i < length; i++) {
        do {
            __DMB();
        } while (!gd32::UartFlagGet<USART_FLAG_TBE>(kUart));

        USART_TDATA(kUart) = USART_TDATA_TDATA & data[i];
    }

    while (!gd32::UartFlagGet<USART_FLAG_TC>(kUart)) {
        static_cast<void>(GET_BITS(USART_RDATA(kUart), 0U, 8U));
    }

    TIMER_CNT(TIMER5) = 0;
    do {
        __DMB();
    } while (TIMER_CNT(TIMER5) < rdm::responder::kDataDirectionDelay);

    SetPortDirection(port_index, dmx::Direction::kInput, true);

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
    sv_total_statistics[port_index].rdm.sent.discovery_response = sv_total_statistics[port_index].rdm.sent.discovery_response + 1;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
}

// RDM Receive
const uint8_t* Dmx::RdmReceive(uint32_t port_index) {
    DMX_CHECK_PORT_INDEX_PTR(port_index);

    if ((sv_rx_buffer[port_index].rdm.index & dmx::kRdmSlotsCompleteFlag) != dmx::kRdmSlotsCompleteFlag) {
        return nullptr;
    }

    sv_rx_buffer[port_index].rdm.index = 0;

    const auto* data = const_cast<const uint8_t*>(sv_rx_buffer[port_index].rdm.data);

    if (data[0] == E120_SC_RDM) {
        const auto* rdm_command = reinterpret_cast<const struct TRdmMessage*>(data);

        uint32_t index;
        uint16_t checksum = 0;

        for (index = 0; index < e120::kMessageLengthMin; index++) {
            checksum = static_cast<uint16_t>(checksum + data[index]);
        }

        for (; index < rdm_command->message_length; index++) {
            checksum = static_cast<uint16_t>(checksum + data[index]);
        }

        if (data[index++] == static_cast<uint8_t>(checksum >> 8)) {
            if (data[index] == static_cast<uint8_t>(checksum)) {
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
                sv_total_statistics[port_index].rdm.received.good = sv_total_statistics[port_index].rdm.received.good + 1;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
                return data;
            }
        }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
        sv_total_statistics[port_index].rdm.received.bad = sv_total_statistics[port_index].rdm.received.bad + 1;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)
        return nullptr;
    }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
    sv_total_statistics[port_index].rdm.received.discovery_response = sv_total_statistics[port_index].rdm.received.discovery_response + 1;
#endif // !defined(CONFIG_DMX_DISABLE_STATISTICS)

    return data;
}

// RDM Receive with timeout
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
const uint8_t* Dmx::RdmReceiveTimeOut(uint32_t port_index, uint16_t timeout_ms) {
    DMX_CHECK_PORT_INDEX_PTR(port_index);

    uint8_t* data_available = nullptr;
    TIMER_CNT(TIMER5) = 0;

    do {
        data_available = const_cast<uint8_t*>(RdmReceive(port_index));
        if (data_available != nullptr) {
            return data_available;
        }
    } while (TIMER_CNT(TIMER5) < timeout_ms);

    return nullptr;
}

// Explicit template instantiations
template void Dmx::SetTransmitDataWithSC<dmx::SendStyle::kDirect>(const uint32_t, const uint8_t*, uint32_t);
template void Dmx::SetTransmitDataWithSC<dmx::SendStyle::kSync>(const uint32_t, const uint8_t*, uint32_t);

template void Dmx::SetTransmitDataWithoutSC<dmx::SendStyle::kDirect>(const uint32_t, const uint8_t*, uint32_t);
template void Dmx::SetTransmitDataWithoutSC<dmx::SendStyle::kSync>(const uint32_t, const uint8_t*, uint32_t);

template void Dmx::SetSendDataInternal<0, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<0, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<0, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<0, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);

#if DMX_MAX_PORTS >= 2
template void Dmx::SetSendDataInternal<1, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<1, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<1, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<1, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
#endif // DMX_MAX_PORTS >= 2

#if DMX_MAX_PORTS >= 3
template void Dmx::SetSendDataInternal<2, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<2, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<2, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<2, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
#endif // DMX_MAX_PORTS >= 3

#if DMX_MAX_PORTS >= 4
template void Dmx::SetSendDataInternal<3, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<3, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<3, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<3, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
#endif // DMX_MAX_PORTS >= 4

#if DMX_MAX_PORTS >= 5
template void Dmx::SetSendDataInternal<4, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<4, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<4, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<4, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
#endif // DMX_MAX_PORTS >= 5

#if DMX_MAX_PORTS >= 6
template void Dmx::SetSendDataInternal<5, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<5, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<5, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<5, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
#endif // DMX_MAX_PORTS >= 6

#if DMX_MAX_PORTS >= 7
template void Dmx::SetSendDataInternal<6, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<6, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<6, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<6, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
#endif // DMX_MAX_PORTS >= 7

#if DMX_MAX_PORTS == 8
template void Dmx::SetSendDataInternal<7, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<7, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<7, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<7, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
#endif // DMX_MAX_PORTS == 8

#if !defined(CONFIG_DMX_NO_OPTIMIZE)
#pragma GCC pop_options
#endif // !defined(CONFIG_DMX_NO_OPTIMIZE)
#pragma GCC push_options
#pragma GCC optimize("Os")
// Configuration
[[gnu::noinline]]
void Dmx::SetTransmitBreakTime(uint32_t break_time) {
    s_dmx_transmit.break_time = std::max(dmx::transmit::kBreakTimeMin, break_time);
    SetTransmitPeriodTime(transmit_period_requested_);
}

[[gnu::noinline]]
uint32_t Dmx::TransmitBreakTime() const {
    return s_dmx_transmit.break_time;
}

[[gnu::noinline]]
void Dmx::SetTransmitMabTime(uint32_t mab_time) {
    s_dmx_transmit.mab_time = std::max(dmx::transmit::kMabTimeMin, mab_time);
    SetTransmitPeriodTime(transmit_period_requested_);
}

[[gnu::noinline]]
uint32_t Dmx::TransmitMabTime() const {
    return s_dmx_transmit.mab_time;
}

[[gnu::noinline]]
void Dmx::SetTransmitPeriodTime(uint32_t period) {
    transmit_period_requested_ = period;

    auto length_max = s_DmxTxBuffer[0].dmx.data[0].length;

    for (uint32_t port_index = 1; port_index < dmx::config::max::kPorts; port_index++) {
        const auto kLength = s_DmxTxBuffer[port_index].dmx.data[0].length;
        if (kLength > length_max) {
            length_max = kLength;
        }
    }

    auto package_length_micro_seconds = s_dmx_transmit.break_time + s_dmx_transmit.mab_time + (length_max * dmx::kSlotTime);

    // The GD32F4xx/GD32H7XX Timer 1 has a 32-bit counter
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
    if (package_length_micro_seconds > (UINT16_MAX - dmx::kSlotTime)) {
        s_dmx_transmit.break_time = std::min(dmx::transmit::kBreakTimeTypical, s_dmx_transmit.break_time);
        s_dmx_transmit.mab_time = dmx::transmit::kMabTimeMin;
        package_length_micro_seconds = s_dmx_transmit.break_time + s_dmx_transmit.mab_time + (length_max * dmx::kSlotTime);
    }
#endif // defined(GD32F4XX) || defined(GD32H7XX)

    if (period != 0) {
        if (period < package_length_micro_seconds) {
            transmit_period_ = std::max(dmx::transmit::kBreakToBreakTimeMin, package_length_micro_seconds + dmx::kSlotTime);
        } else {
            transmit_period_ = period;
        }
    } else {
        transmit_period_ = std::max(dmx::transmit::kBreakToBreakTimeMin, package_length_micro_seconds + dmx::kSlotTime);
    }

    s_dmx_transmit.inter_time = transmit_period_ - package_length_micro_seconds;

    DMX_DEBUG_PRINTF("period=%u, length_max=%u, transmit_period_=%u, package_length_micro_seconds=%u -> s_dmx_transmit.inter_time=%u", period, length_max, transmit_period_, package_length_micro_seconds, s_dmx_transmit.inter_time);
}

[[gnu::noinline]]
void Dmx::SetTransmitSlots(uint16_t slots) {
    if ((slots >= 2) && (slots <= dmx::kChannelsMax)) {
        transmit_slots_ = slots;

        for (uint32_t i = 0; i < dmx::config::max::kPorts; i++) {
            transmit_length_[i] = static_cast<uint32_t>(slots);
        }

        SetTransmitPeriodTime(transmit_period_requested_);
    }
}

[[gnu::noinline]]
void Dmx::SetOutputStyle(uint32_t port_index, dmx::OutputStyle output_style) {
    DMX_CHECK_PORT_INDEX_VOID(port_index);

    s_DmxTxBuffer[port_index].output_style = output_style;

    if (output_style == dmx::OutputStyle::kConstant) {
        if (!has_continuous_output_) {
            has_continuous_output_ = true;
            if (port_direction_[port_index] == dmx::Direction::kOutput) {
                StartDmxOutput(port_index);
            }
            return;
        }

        for (uint32_t index = 0; index < dmx::config::max::kPorts; index++) {
            if ((s_DmxTxBuffer[index].output_style == dmx::OutputStyle::kConstant) && (port_direction_[index] == dmx::Direction::kOutput)) {
                DataDisable(index);
            }
        }

        for (uint32_t index = 0; index < dmx::config::max::kPorts; index++) {
            if ((s_DmxTxBuffer[index].output_style == dmx::OutputStyle::kConstant) && (port_direction_[index] == dmx::Direction::kOutput)) {
                StartDmxOutput(index);
            }
        }
    } else {
        has_continuous_output_ = false;
        for (uint32_t index = 0; index < dmx::config::max::kPorts; index++) {
            if (s_DmxTxBuffer[index].output_style == dmx::OutputStyle::kConstant) {
                has_continuous_output_ = true;
                return;
            }
        }
    }
}

[[gnu::noinline]]
dmx::OutputStyle Dmx::GetOutputStyle(uint32_t port_index) const {
    DMX_CHECK_PORT_INDEX_RET(port_index, dmx::OutputStyle::kConstant);
    return s_DmxTxBuffer[port_index].output_style;
}

// Setup
static void UartDmxConfig(uint32_t usart_periph) {
    gd32::UartBegin(usart_periph, dmx::kBaudRate, gd32::kUartBits8, gd32::kUartParityNone, gd32::kUartStop2Bits);
}

static void UsartDmaConfig() {
    DMA_PARAMETER_STRUCT dma_init_struct;
    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMA1);
#if defined(GD32H7XX)
    rcu_periph_clock_enable(RCU_DMAMUX);
#endif // defined(GD32H7XX)

#if defined(DMX_USE_USART0)
    // USART 0 TX
    dma_deinit(USART0_DMAx, USART0_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_USART0_TX;
#endif // defined(GD32H7XX)
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_addr = reinterpret_cast<uint32_t>(&USART_TDATA(USART0));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(USART0_DMAx, USART0_TX_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(USART0_DMAx, USART0_TX_DMA_CHx);
    dma_memory_to_memory_disable(USART0_DMAx, USART0_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(USART0_DMAx, USART0_TX_DMA_CHx, USART0_TX_DMA_SUBPERIx);
#endif // defined(GD32F4XX)
    Gd32DmaInterruptDisable<USART0_DMAx, USART0_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
#if !defined(GD32F4XX)
    NVIC_SetPriority(DMA0_Channel3_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel3_IRQn);
#else
    NVIC_SetPriority(DMA1_Channel7_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel7_IRQn);
#endif // !defined(GD32F4XX)
#endif // defined(DMX_USE_USART0)

#if defined(DMX_USE_USART1)
    // USART 1 TX
    dma_deinit(USART1_DMAx, USART1_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_USART1_TX;
#endif // defined(GD32H7XX)
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_addr = reinterpret_cast<uint32_t>(&USART_TDATA(USART1));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(USART1_DMAx, USART1_TX_DMA_CHx, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(USART1_DMAx, USART1_TX_DMA_CHx);
    dma_memory_to_memory_disable(USART1_DMAx, USART1_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(USART1_DMAx, USART1_TX_DMA_CHx, USART1_TX_DMA_SUBPERIx);
#endif // defined(GD32F4XX)
    Gd32DmaInterruptDisable<USART1_DMAx, USART1_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
    NVIC_SetPriority(DMA0_Channel6_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel6_IRQn);
#endif // defined(DMX_USE_USART1)

#if defined(DMX_USE_USART2)
    // USART 2 TX
    dma_deinit(USART2_DMAx, USART2_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_USART2_TX;
#endif // defined(GD32H7XX)
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_addr = reinterpret_cast<uint32_t>(&USART_TDATA(USART2));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(USART2_DMAx, USART2_TX_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(USART2_DMAx, USART2_TX_DMA_CHx);
    dma_memory_to_memory_disable(USART2_DMAx, USART2_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(USART2_DMAx, USART2_TX_DMA_CHx, USART2_TX_DMA_SUBPERIx);
#endif // defined(GD32F4XX)
    Gd32DmaInterruptDisable<USART2_DMAx, USART2_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
#if defined(GD32F4XX) || defined(GD32H7XX)
    NVIC_SetPriority(DMA0_Channel3_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel3_IRQn);
#else
    NVIC_SetPriority(DMA0_Channel1_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel1_IRQn);
#endif // defined(GD32F4XX) || defined(GD32H7XX)
#endif // defined(DMX_USE_USART2)

#if defined(DMX_USE_UART3)
    // UART 3 TX
    dma_deinit(UART3_DMAx, UART3_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_UART3_TX;
#endif // defined(GD32H7XX)
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_addr = reinterpret_cast<uint32_t>(&USART_TDATA(UART3));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(UART3_DMAx, UART3_TX_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(UART3_DMAx, UART3_TX_DMA_CHx);
    dma_memory_to_memory_disable(UART3_DMAx, UART3_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(UART3_DMAx, UART3_TX_DMA_CHx, UART3_TX_DMA_SUBPERIx);
#endif // defined(GD32F4XX)
    Gd32DmaInterruptDisable<UART3_DMAx, UART3_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
#if defined(GD32F30X)
    NVIC_SetPriority(DMA1_Channel3_Channel4_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel3_Channel4_IRQn);
#elif !defined(GD32F4XX)
    NVIC_SetPriority(DMA1_Channel4_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
#else
    NVIC_SetPriority(DMA0_Channel4_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel4_IRQn);
#endif // defined(GD32F30X)
#endif // defined(DMX_USE_UART3)

#if defined(DMX_USE_UART4)
    // UART 4 TX
    dma_deinit(UART4_DMAx, UART4_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_UART4_TX;
#endif // defined(GD32H7XX)
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_addr = reinterpret_cast<uint32_t>(&USART_TDATA(UART4));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(UART4_DMAx, UART4_TX_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(UART4_DMAx, UART4_TX_DMA_CHx);
    dma_memory_to_memory_disable(UART4_DMAx, UART4_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(UART4_DMAx, UART4_TX_DMA_CHx, UART4_TX_DMA_SUBPERIx);
#endif // defined(GD32F4XX)
    Gd32DmaInterruptDisable<UART4_DMAx, UART4_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
#if !defined(GD32F4XX)
    NVIC_SetPriority(DMA1_Channel3_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);
#else
    NVIC_SetPriority(DMA0_Channel7_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel7_IRQn);
#endif // !defined(GD32F4XX)
#endif // defined(DMX_USE_UART4)

#if defined(DMX_USE_USART5)
    // USART 5 TX
    dma_deinit(USART5_DMAx, USART5_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_USART5_TX;
#endif // defined(GD32H7XX)
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F20X)
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif // defined(GD32F20X)
    dma_init_struct.periph_addr = reinterpret_cast<uint32_t>(&USART_TDATA(USART5));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F20X)
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif // defined(GD32F20X)
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(USART5_DMAx, USART5_TX_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(USART5_DMAx, USART5_TX_DMA_CHx);
    dma_memory_to_memory_disable(USART5_DMAx, USART5_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(USART5_DMAx, USART5_TX_DMA_CHx, USART5_TX_DMA_SUBPERIx);
#endif // defined(GD32F4XX)
    Gd32DmaInterruptDisable<USART5_DMAx, USART5_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
    NVIC_SetPriority(DMA1_Channel6_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel6_IRQn);
#endif // defined(DMX_USE_USART5)

#if defined(DMX_USE_UART6)
    // UART 6 TX
    dma_deinit(UART6_DMAx, UART6_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_UART6_TX;
#endif // defined(GD32H7XX)
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F20X)
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif // defined(GD32F20X)
    dma_init_struct.periph_addr = reinterpret_cast<uint32_t>(&USART_TDATA(UART6));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F20X)
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif // defined(GD32F20X)
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(UART6_DMAx, UART6_TX_DMA_CHx, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(UART6_DMAx, UART6_TX_DMA_CHx);
    dma_memory_to_memory_disable(UART6_DMAx, UART6_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(UART6_DMAx, UART6_TX_DMA_CHx, UART6_TX_DMA_SUBPERIx);
#endif // defined(GD32F4XX)
    Gd32DmaInterruptDisable<UART6_DMAx, UART4_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
#if defined(GD32F20X)
    NVIC_SetPriority(DMA1_Channel4_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
#else
    NVIC_SetPriority(DMA0_Channel1_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel1_IRQn);
#endif // defined(GD32F20X)
#endif // defined(DMX_USE_UART6)

#if defined(DMX_USE_UART7)
    // UART 7 TX
    dma_deinit(UART7_DMAx, UART7_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_UART7_TX;
#endif // defined(GD32H7XX)
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F20X)
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif // defined(GD32F20X)
    dma_init_struct.periph_addr = reinterpret_cast<uint32_t>(&USART_TDATA(UART7));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F20X)
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif // defined(GD32F20X)
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(UART7_DMAx, UART7_TX_DMA_CHx, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(UART7_DMAx, UART7_TX_DMA_CHx);
    dma_memory_to_memory_disable(UART7_DMAx, UART7_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(UART7_DMAx, UART7_TX_DMA_CHx, UART7_TX_DMA_SUBPERIx);
#endif // defined(GD32F4XX)
#if defined(GD32F20X)
    NVIC_SetPriority(DMA1_Channel3_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);
#else
    NVIC_SetPriority(DMA0_Channel0_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel0_IRQn);
#endif // defined(GD32F20X)
#endif // defined(DMX_USE_UART7)
}

static void Timer1Config() {
    rcu_periph_clock_enable(RCU_TIMER1);
    timer_deinit(TIMER1);

    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = TIMER_PSC_1MHZ;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = UINT32_MAX;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER1, &timer_initpara);

    timer_flag_clear(TIMER1, UINT32_MAX);
    timer_interrupt_flag_clear(TIMER1, UINT32_MAX);

#if defined(DMX_USE_USART0)
    timer_channel_output_mode_config(TIMER1, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
    TIMER_CH0CV(TIMER1) = UINT32_MAX;
    timer_interrupt_enable(TIMER1, TIMER_INT_CH0);
#endif // defined(DMX_USE_USART0)

#if defined(DMX_USE_USART1)
    timer_channel_output_mode_config(TIMER1, TIMER_CH_1, TIMER_OC_MODE_ACTIVE);
    TIMER_CH1CV(TIMER1) = UINT32_MAX;
    timer_interrupt_enable(TIMER1, TIMER_INT_CH1);
#endif // defined(DMX_USE_USART1)

#if defined(DMX_USE_USART2)
    timer_channel_output_mode_config(TIMER1, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
    TIMER_CH2CV(TIMER1) = UINT32_MAX;
    timer_interrupt_enable(TIMER1, TIMER_INT_CH2);
#endif // defined(DMX_USE_USART2)

#if defined(DMX_USE_UART3)
    timer_channel_output_mode_config(TIMER1, TIMER_CH_3, TIMER_OC_MODE_ACTIVE);
    TIMER_CH3CV(TIMER1) = UINT32_MAX;
    timer_interrupt_enable(TIMER1, TIMER_INT_CH3);
#endif // defined(DMX_USE_UART3)

    NVIC_SetPriority(TIMER1_IRQn, 0);
    NVIC_EnableIRQ(TIMER1_IRQn);

    timer_enable(TIMER1);
}

#if defined(DMX_USE_UART4) || defined(DMX_USE_USART5) || defined(DMX_USE_UART6) || defined(DMX_USE_UART7)
static void Timer4Config() {
    rcu_periph_clock_enable(RCU_TIMER4);
    timer_deinit(TIMER4);

    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = TIMER_PSC_1MHZ;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = UINT32_MAX;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER4, &timer_initpara);

    timer_flag_clear(TIMER4, UINT32_MAX);
    timer_interrupt_flag_clear(TIMER4, UINT32_MAX);

#if defined(DMX_USE_UART4)
    timer_channel_output_mode_config(TIMER4, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
    TIMER_CH0CV(TIMER4) = UINT32_MAX;
    timer_interrupt_enable(TIMER4, TIMER_INT_CH0);
#endif // defined(DMX_USE_UART4)

#if defined(DMX_USE_USART5)
    timer_channel_output_mode_config(TIMER4, TIMER_CH_1, TIMER_OC_MODE_ACTIVE);
    TIMER_CH1CV(TIMER4) = UINT32_MAX;
    timer_interrupt_enable(TIMER4, TIMER_INT_CH1);
#endif // defined(DMX_USE_USART5)

#if defined(DMX_USE_UART6)
    timer_channel_output_mode_config(TIMER4, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
    TIMER_CH2CV(TIMER4) = UINT32_MAX;
    timer_interrupt_enable(TIMER4, TIMER_INT_CH2);
#endif // defined(DMX_USE_UART6)

#if defined(DMX_USE_UART7)
    timer_channel_output_mode_config(TIMER4, TIMER_CH_3, TIMER_OC_MODE_ACTIVE);
    TIMER_CH3CV(TIMER4) = UINT32_MAX;
    timer_interrupt_enable(TIMER4, TIMER_INT_CH3);
#endif // defined(DMX_USE_UART7)

    NVIC_SetPriority(TIMER4_IRQn, 0);
    NVIC_EnableIRQ(TIMER4_IRQn);

    timer_enable(TIMER4);
}
#endif // defined(DMX_USE_UART4) || defined(DMX_USE_USART5) || defined(DMX_USE_UART6) || defined(DMX_USE_UART7)

Dmx::Dmx() {
    DMX_DEBUG_ENTRY();
    assert(s_this == nullptr);
    s_this = this;

    s_dmx_transmit.break_time = dmx::transmit::kBreakTimeTypical;
    s_dmx_transmit.mab_time = dmx::transmit::kMabTimeMin;
    s_dmx_transmit.inter_time = dmx::transmit::kPeriodDefault - s_dmx_transmit.break_time - s_dmx_transmit.mab_time - (dmx::kChannelsMax * dmx::kSlotTime) - dmx::kSlotTime;

    for (uint32_t port_index = 0; port_index < dmx::config::max::kPorts; port_index++) {
        transmit_length_[port_index] = dmx::kChannelsMax;
        sv_rx_buffer[port_index].state = dmx::TxRxState::kIdle;
        s_DmxTxBuffer[port_index].state = dmx::TxRxState::kIdle;

        if ((kDirGpio[port_index].port != 0) && (kDirGpio[port_index].pin != 0)) {
            Gd32GpioFsel(kDirGpio[port_index].port, kDirGpio[port_index].pin, GPIO_FSEL_OUTPUT);
            SetPortDirection(port_index, dmx::Direction::kInput, false);
        }

        SetOutputStyle(port_index, dmx::OutputStyle::kDelta);
        ClearData(port_index);
    }

    SetTransmitBreakTime(dmx::transmit::kBreakTimeTypical);
    SetTransmitMabTime(dmx::transmit::kMabTimeMin);
    SetTransmitSlots(dmx::kChannelsMax);
    SetTransmitPeriodTime(0);

    UsartDmaConfig(); // DMX Transmit
#if defined(DMX_USE_USART0) || defined(DMX_USE_USART1) || defined(DMX_USE_USART2) || defined(DMX_USE_UART3)
    Timer1Config(); // DMX Transmit -> USART0, USART1, USART2, UART3
#endif              // defined(DMX_USE_USART0) || defined(DMX_USE_USART1) || defined(DMX_USE_USART2) || defined(DMX_USE_UART3)
#if defined(DMX_USE_UART4) || defined(DMX_USE_USART5) || defined(DMX_USE_UART6) || defined(DMX_USE_UART7)
    Timer4Config(); // DMX Transmit -> UART4, USART5, UART6, UART7
#endif              // defined(DMX_USE_UART4) || defined(DMX_USE_USART5) || defined(DMX_USE_UART6) || defined(DMX_USE_UART7)

#if defined(DMX_USE_USART0) || defined(DMX_USE_USART0_RX)
    UartDmxConfig(USART0);
    NVIC_SetPriority(USART0_IRQn, 0);
    NVIC_EnableIRQ(USART0_IRQn);
#endif // defined(DMX_USE_USART0) || defined(DMX_USE_USART0_RX)
#if defined(DMX_USE_USART1) || defined(DMX_USE_USART1_RX)
    UartDmxConfig(USART1);
    NVIC_SetPriority(USART1_IRQn, 0);
    NVIC_EnableIRQ(USART1_IRQn);
#endif // defined(DMX_USE_USART1) || defined(DMX_USE_USART1_RX)
#if defined(DMX_USE_USART2) || defined(DMX_USE_USART2_RX)
    UartDmxConfig(USART2);
    NVIC_SetPriority(USART2_IRQn, 0);
    NVIC_EnableIRQ(USART2_IRQn);
#endif // defined(DMX_USE_USART2) || defined(DMX_USE_USART2_RX)
#if defined(DMX_USE_UART3) || defined(DMX_USE_UART3_RX)
    UartDmxConfig(UART3);
    NVIC_SetPriority(UART3_IRQn, 0);
    NVIC_EnableIRQ(UART3_IRQn);
#endif // defined(DMX_USE_UART3) || defined(DMX_USE_UART3_RX)
#if defined(DMX_USE_UART4) || defined(DMX_USE_UART4_RX)
    UartDmxConfig(UART4);
    NVIC_SetPriority(UART4_IRQn, 0);
    NVIC_EnableIRQ(UART4_IRQn);
#endif // defined(DMX_USE_UART4) || defined(DMX_USE_UART4_RX)
#if defined(DMX_USE_USART5) || defined(DMX_USE_USART5_RX)
    UartDmxConfig(USART5);
    NVIC_SetPriority(USART5_IRQn, 0);
    NVIC_EnableIRQ(USART5_IRQn);
#endif // defined(DMX_USE_USART5) || defined(DMX_USE_USART5_RX)
#if defined(DMX_USE_UART6) || defined(DMX_USE_UART6_RX)
    UartDmxConfig(UART6);
    NVIC_SetPriority(UART6_IRQn, 0);
    NVIC_EnableIRQ(UART6_IRQn);
#endif // defined(DMX_USE_UART6) || defined(DMX_USE_UART6_RX)
#if defined(DMX_USE_UART7) || defined(DMX_USE_UART7_RX)
    UartDmxConfig(UART7);
    NVIC_SetPriority(UART7_IRQn, 0);
    NVIC_EnableIRQ(UART7_IRQn);
#endif // defined(DMX_USE_UART7) || defined(DMX_USE_UART7_RX)

    DMX_DEBUG_EXIT();
}

#pragma GCC pop_options