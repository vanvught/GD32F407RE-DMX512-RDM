/**
 * @file dmx.cpp
 */
/* Copyright (C) 2021-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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
#endif

#pragma GCC push_options
#pragma GCC optimize("O3")
#pragma GCC optimize("-fprefetch-loop-arrays")

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <cassert>

#include "dmx.h"
#include "dmxconst.h"
#include "rdm.h"
#include "rdm_e120.h"
#include "gd32.h"
#include "gd32_dma.h"
#include "gd32_uart.h"
#include "dmx_internal.h"
#include "dmx/dmx_config.h"
#include "gd32/dmx_assert.h"
#include "gd32/dmx_dma_check.h" // Do not reorder/move
#if defined(LOGIC_ANALYZER)
#include "logic_analyzer.h"
#endif
#include "firmware/debug/debug_debug.h"

extern struct HwTimersSeconds g_Seconds;

namespace dmx
{
enum class TxRxState
{
    kIdle,
    kBreak,
    kMab,
    kDmxdata,
    kDmxinter,
    kRdmdata,
    kChecksumh,
    kChecksuml,
    kRdmdisc
};

enum class PortState
{
    kIdle,
    kTx,
    kRx
};

struct TxDmxDataPacket
{
    uint8_t data[dmx::buffer::SIZE]; // multiple of uint32_t
    uint32_t length;
};

struct TxDmxPacket
{
    TxDmxDataPacket data[2];
    uint32_t write_index;
    uint32_t read_index;
    bool data_pending;
};

struct TxData
{
    TxDmxPacket dmx;
    OutputStyle output_style ALIGNED;
    volatile TxRxState state;
};

struct DmxTransmit
{
    uint32_t break_time;
    uint32_t mab_time;
    uint32_t inter_time;
};

struct RxDmxPackets
{
    uint32_t per_second;
    uint32_t count;
    uint32_t count_previous;
};

struct RxDmxData
{
    uint8_t data[dmx::buffer::SIZE] ALIGNED; // multiple of uint32_t
    uint32_t slots_in_packet;
};

struct RxData
{
    struct Dmx
    {
        volatile RxDmxData current;
        RxDmxData previous;
    } dmx ALIGNED;
    struct Rdm
    {
        volatile uint8_t data[sizeof(struct TRdmMessage)] ALIGNED;
        volatile uint32_t index;
    } rdm ALIGNED;
    volatile TxRxState state;
} ALIGNED;

struct DirGpio
{
    uint32_t port;
    uint32_t pin;
};
} // namespace dmx

static constexpr dmx::DirGpio kDirGpio[DMX_MAX_PORTS] = {
    {dmx::config::DIR_PORT_0_GPIO_PORT, dmx::config::DIR_PORT_0_GPIO_PIN},
#if DMX_MAX_PORTS >= 2
    {dmx::config::DIR_PORT_1_GPIO_PORT, dmx::config::DIR_PORT_1_GPIO_PIN},
#endif
#if DMX_MAX_PORTS >= 3
    {dmx::config::DIR_PORT_2_GPIO_PORT, dmx::config::DIR_PORT_2_GPIO_PIN},
#endif
#if DMX_MAX_PORTS >= 4
    {dmx::config::DIR_PORT_3_GPIO_PORT, dmx::config::DIR_PORT_3_GPIO_PIN},
#endif
#if DMX_MAX_PORTS >= 5
    {dmx::config::DIR_PORT_4_GPIO_PORT, dmx::config::DIR_PORT_4_GPIO_PIN},
#endif
#if DMX_MAX_PORTS >= 6
    {dmx::config::DIR_PORT_5_GPIO_PORT, dmx::config::DIR_PORT_5_GPIO_PIN},
#endif
#if DMX_MAX_PORTS >= 7
    {dmx::config::DIR_PORT_6_GPIO_PORT, dmx::config::DIR_PORT_6_GPIO_PIN},
#endif
#if DMX_MAX_PORTS == 8
    {dmx::config::DIR_PORT_7_GPIO_PORT, dmx::config::DIR_PORT_7_GPIO_PIN},
#endif
};

static volatile dmx::PortState sv_port_state[dmx::config::max::PORTS] ALIGNED;

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
static volatile dmx::TotalStatistics sv_total_statistics[dmx::config::max::PORTS] ALIGNED;
#endif

// DMX RX

static volatile dmx::RxDmxPackets sv_rx_dmx_packets[dmx::config::max::PORTS] ALIGNED;

// RDM RX
volatile uint32_t gsv_RdmDataReceiveEnd;

// DMX RDM RX

static volatile dmx::RxData sv_rx_buffer[dmx::config::max::PORTS] ALIGNED;

// DMX TX

static dmx::TxData s_TxBuffer[dmx::config::max::PORTS] ALIGNED SECTION_DMA_BUFFER;
static dmx::DmxTransmit s_dmx_transmit;

template <uint32_t uart, uint32_t port_index> void IrqHandlerDmxRdmInput()
{
    auto& rx_buffer = sv_rx_buffer[port_index];
    const auto kIsFlagIdleFrame = (USART_REG_VAL(uart, USART_FLAG_IDLE) & BIT(USART_BIT_POS(USART_FLAG_IDLE))) == BIT(USART_BIT_POS(USART_FLAG_IDLE));
    /*
     * Software can clear this bit by reading the USART_STAT and USART_DATA registers one by one.
     */
    if (kIsFlagIdleFrame)
    {
        static_cast<void>(GET_BITS(USART_RDATA(uart), 0U, 8U));

        if (rx_buffer.state == dmx::TxRxState::kDmxdata)
        {
            rx_buffer.state = dmx::TxRxState::kIdle;
            rx_buffer.dmx.current.slots_in_packet |= 0x8000;

            return;
        }

        if (rx_buffer.state == dmx::TxRxState::kRdmdisc)
        {
            rx_buffer.state = dmx::TxRxState::kIdle;
            rx_buffer.rdm.index |= 0x4000;

            return;
        }

        return;
    }

    const auto kIsFlagFrameError = (USART_REG_VAL(uart, USART_FLAG_FERR) & BIT(USART_BIT_POS(USART_FLAG_FERR))) == BIT(USART_BIT_POS(USART_FLAG_FERR));
    /*
     * Software can clear this bit by reading the USART_STAT and USART_DATA registers one by one.
     */
    if (kIsFlagFrameError)
    {
        static_cast<void>(GET_BITS(USART_RDATA(uart), 0U, 8U));

        if (rx_buffer.state == dmx::TxRxState::kIdle)
        {
            rx_buffer.state = dmx::TxRxState::kBreak;
        }

        return;
    }

    const auto kData = static_cast<uint8_t>(GET_BITS(USART_RDATA(uart), 0U, 8U));

    switch (rx_buffer.state)
    {
        case dmx::TxRxState::kIdle:
            rx_buffer.state = dmx::TxRxState::kRdmdisc;
            rx_buffer.rdm.data[0] = kData;
            rx_buffer.rdm.index = 1;
            break;
        case dmx::TxRxState::kBreak:
            switch (kData)
            {
                case dmx::kStartCode:
                    rx_buffer.dmx.current.data[0] = dmx::kStartCode;
                    rx_buffer.dmx.current.slots_in_packet = 1;
                    sv_rx_dmx_packets[port_index].count = sv_rx_dmx_packets[port_index].count + 1;
                    rx_buffer.state = dmx::TxRxState::kDmxdata;
                    break;
                case E120_SC_RDM:
                    rx_buffer.rdm.data[0] = E120_SC_RDM;
                    rx_buffer.rdm.index = 1;
                    rx_buffer.state = dmx::TxRxState::kRdmdata;
                    break;
                default:
                    rx_buffer.dmx.current.slots_in_packet = 0;
                    rx_buffer.rdm.index = 0;
                    rx_buffer.state = dmx::TxRxState::kIdle;
                    break;
            }
            break;
        case dmx::TxRxState::kDmxdata:
        {
            auto index = rx_buffer.dmx.current.slots_in_packet;
            rx_buffer.dmx.current.data[index] = kData;
            index++;
            rx_buffer.dmx.current.slots_in_packet = index;

            if (index > dmx::kChannelsMax)
            {
                index |= 0x8000;
                rx_buffer.dmx.current.slots_in_packet = index;
                rx_buffer.state = dmx::TxRxState::kIdle;
                break;
            }
        }
        break;
        case dmx::TxRxState::kRdmdata:
        {
            auto index = rx_buffer.rdm.index;
            rx_buffer.rdm.data[index] = kData;
            index++;
            rx_buffer.rdm.index = index;

            const auto* p = reinterpret_cast<volatile struct TRdmMessage*>(&rx_buffer.rdm.data[0]);

            if ((index >= 24) && (index <= sizeof(struct TRdmMessage)) && (index == p->message_length))
            {
                rx_buffer.state = dmx::TxRxState::kChecksumh;
            }
            else if (index > sizeof(struct TRdmMessage))
            {
                rx_buffer.state = dmx::TxRxState::kIdle;
            }
        }
        break;
        case dmx::TxRxState::kChecksumh:
        {
            auto index = rx_buffer.rdm.index;
            rx_buffer.rdm.data[index] = kData;
            index++;
            rx_buffer.rdm.index = index;
            rx_buffer.state = dmx::TxRxState::kChecksuml;
        }
        break;
        case dmx::TxRxState::kChecksuml:
        {
            auto index = rx_buffer.rdm.index;
            rx_buffer.rdm.data[index] = kData;
            index |= 0x4000;
            rx_buffer.rdm.index = index;
            rx_buffer.state = dmx::TxRxState::kIdle;
            gsv_RdmDataReceiveEnd = DWT->CYCCNT;
        }
        break;
        case dmx::TxRxState::kRdmdisc:
        {
            auto index = rx_buffer.rdm.index;

            if (index < 24)
            {
                rx_buffer.rdm.data[index] = kData;
                index++;
                rx_buffer.rdm.index = index;
            }
        }
        break;
        default:
            rx_buffer.dmx.current.slots_in_packet = 0;
            rx_buffer.rdm.index = 0;
            rx_buffer.state = dmx::TxRxState::kIdle;
            break;
    }
}

extern "C"
{
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
#if defined(DMX_USE_USART0)
    void USART0_IRQHandler()
    {
        IrqHandlerDmxRdmInput<USART0, dmx::config::USART0_PORT>();
    }
#endif
#if defined(DMX_USE_USART1)
    void USART1_IRQHandler()
    {
        IrqHandlerDmxRdmInput<USART1, dmx::config::USART1_PORT>();
    }
#endif
#if defined(DMX_USE_USART2)
    void USART2_IRQHandler()
    {
        IrqHandlerDmxRdmInput<USART2, dmx::config::USART2_PORT>();
    }
#endif
#if defined(DMX_USE_UART3)
    void UART3_IRQHandler()
    {
        IrqHandlerDmxRdmInput<UART3, dmx::config::UART3_PORT>();
    }
#endif
#if defined(DMX_USE_UART4)
    void UART4_IRQHandler()
    {
        IrqHandlerDmxRdmInput<UART4, dmx::config::UART4_PORT>();
    }
#endif
#if defined(DMX_USE_USART5)
    void USART5_IRQHandler()
    {
        IrqHandlerDmxRdmInput<USART5, dmx::config::USART5_PORT>();
    }
#endif
#if defined(DMX_USE_UART6)
    void UART6_IRQHandler()
    {
        IrqHandlerDmxRdmInput<UART6, dmx::config::UART6_PORT>();
    }
#endif
#if defined(DMX_USE_UART7)
    void UART7_IRQHandler()
    {
        IrqHandlerDmxRdmInput<UART7, dmx::config::UART7_PORT>();
    }
#endif
#endif
}

static void Timer1Config()
{
    rcu_periph_clock_enable(RCU_TIMER1);
    timer_deinit(TIMER1);

    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = TIMER_PSC_1MHZ;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = ~0;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER1, &timer_initpara);

    timer_flag_clear(TIMER1, ~0);
    timer_interrupt_flag_clear(TIMER1, ~0);

#if defined(DMX_USE_USART0)
    timer_channel_output_mode_config(TIMER1, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
    timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0, ~0);
    timer_interrupt_enable(TIMER1, TIMER_INT_CH0);
#endif /* DMX_USE_USART0 */

#if defined(DMX_USE_USART1)
    timer_channel_output_mode_config(TIMER1, TIMER_CH_1, TIMER_OC_MODE_ACTIVE);
    timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_1, ~0);
    timer_interrupt_enable(TIMER1, TIMER_INT_CH1);
#endif /* DMX_USE_USART1 */

#if defined(DMX_USE_USART2)
    timer_channel_output_mode_config(TIMER1, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
    timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_2, ~0);
    timer_interrupt_enable(TIMER1, TIMER_INT_CH2);
#endif /* DMX_USE_USART2 */

#if defined(DMX_USE_UART3)
    timer_channel_output_mode_config(TIMER1, TIMER_CH_3, TIMER_OC_MODE_ACTIVE);
    timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_3, ~0);
    timer_interrupt_enable(TIMER1, TIMER_INT_CH3);
#endif /* DMX_USE_UART3 */

    NVIC_SetPriority(TIMER1_IRQn, 0);
    NVIC_EnableIRQ(TIMER1_IRQn);

    timer_enable(TIMER1);
}

#if defined(DMX_USE_UART4) || defined(DMX_USE_USART5) || defined(DMX_USE_UART6) || defined(DMX_USE_UART7)
static void Timer4Config()
{
    rcu_periph_clock_enable(RCU_TIMER4);
    timer_deinit(TIMER4);

    timer_parameter_struct timer_initpara;
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = TIMER_PSC_1MHZ;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = ~0;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER4, &timer_initpara);

    timer_flag_clear(TIMER4, ~0);
    timer_interrupt_flag_clear(TIMER4, ~0);

#if defined(DMX_USE_UART4)
    timer_channel_output_mode_config(TIMER4, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
    timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0, ~0);
    timer_interrupt_enable(TIMER4, TIMER_INT_CH0);
#endif /* DMX_USE_UART4 */

#if defined(DMX_USE_USART5)
    timer_channel_output_mode_config(TIMER4, TIMER_CH_1, TIMER_OC_MODE_ACTIVE);
    timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_1, ~0);
    timer_interrupt_enable(TIMER4, TIMER_INT_CH1);
#endif /* DMX_USE_USART5 */

#if defined(DMX_USE_UART6)
    timer_channel_output_mode_config(TIMER4, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
    timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2, ~0);
    timer_interrupt_enable(TIMER4, TIMER_INT_CH2);
#endif /* DMX_USE_UART6 */

#if defined(DMX_USE_UART7)
    timer_channel_output_mode_config(TIMER4, TIMER_CH_3, TIMER_OC_MODE_ACTIVE);
    timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3, ~0);
    timer_interrupt_enable(TIMER4, TIMER_INT_CH3);
#endif /* DMX_USE_UART7 */

    NVIC_SetPriority(TIMER4_IRQn, 0);
    NVIC_EnableIRQ(TIMER4_IRQn);

    timer_enable(TIMER4);
}
#endif

static void UsartDmaConfig()
{
    DMA_PARAMETER_STRUCT dma_init_struct;
    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMA1);
#if defined(GD32H7XX)
    rcu_periph_clock_enable(RCU_DMAMUX);
#endif
#if defined(DMX_USE_USART0)
    /*
     * USART 0 TX
     */
    dma_deinit(USART0_DMAx, USART0_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_USART0_TX;
#endif
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
#if defined(GD32H7XX)
    dma_init_struct.periph_addr = (uint32_t)&USART_TDATA(USART0);
#else
    dma_init_struct.periph_addr = USART0 + 0x04U;
#endif
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(USART0_DMAx, USART0_TX_DMA_CHx, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(USART0_DMAx, USART0_TX_DMA_CHx);
    dma_memory_to_memory_disable(USART0_DMAx, USART0_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(USART0_DMAx, USART0_TX_DMA_CHx, USART0_TX_DMA_SUBPERIx);
#endif
    Gd32DmaInterruptDisable<USART0_DMAx, USART0_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
#if !defined(GD32F4XX)
    NVIC_SetPriority(DMA0_Channel3_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel3_IRQn);
#else
    NVIC_SetPriority(DMA1_Channel7_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel7_IRQn);
#endif
#endif /* DMX_USE_USART0 */
#if defined(DMX_USE_USART1)
    /*
     * USART 1 TX
     */
    dma_deinit(USART1_DMAx, USART1_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_USART1_TX;
#endif
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
#if defined(GD32H7XX)
    dma_init_struct.periph_addr = (uint32_t)&USART_TDATA(USART1);
#else
    dma_init_struct.periph_addr = USART1 + 0x04U;
#endif
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(USART1_DMAx, USART1_TX_DMA_CHx, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(USART1_DMAx, USART1_TX_DMA_CHx);
    dma_memory_to_memory_disable(USART1_DMAx, USART1_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(USART1_DMAx, USART1_TX_DMA_CHx, USART1_TX_DMA_SUBPERIx);
#endif
    Gd32DmaInterruptDisable<USART1_DMAx, USART1_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
    NVIC_SetPriority(DMA0_Channel6_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel6_IRQn);
#endif /* DMX_USE_USART1 */
#if defined(DMX_USE_USART2)
    /*
     * USART 2 TX
     */
    dma_deinit(USART2_DMAx, USART2_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_USART2_TX;
#endif
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
    dma_init_struct.periph_addr = (uint32_t)&USART_TDATA(USART2);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(USART2_DMAx, USART2_TX_DMA_CHx, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(USART2_DMAx, USART2_TX_DMA_CHx);
    dma_memory_to_memory_disable(USART2_DMAx, USART2_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(USART2_DMAx, USART2_TX_DMA_CHx, USART2_TX_DMA_SUBPERIx);
#endif
    Gd32DmaInterruptDisable<USART2_DMAx, USART2_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
#if defined(GD32F4XX) || defined(GD32H7XX)
    NVIC_SetPriority(DMA0_Channel3_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel3_IRQn);
#else
    NVIC_SetPriority(DMA0_Channel1_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel1_IRQn);
#endif
#endif /* DMX_USE_USART2 */
#if defined(DMX_USE_UART3)
    /*
     * UART 3 TX
     */
    dma_deinit(UART3_DMAx, UART3_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_UART3_TX;
#endif
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
#if defined(GD32H7XX)
    dma_init_struct.periph_addr = (uint32_t)&USART_TDATA(UART3);
#else
    dma_init_struct.periph_addr = UART3 + 0x04U;
#endif
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(UART3_DMAx, UART3_TX_DMA_CHx, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(UART3_DMAx, UART3_TX_DMA_CHx);
    dma_memory_to_memory_disable(UART3_DMAx, UART3_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(UART3_DMAx, UART3_TX_DMA_CHx, UART3_TX_DMA_SUBPERIx);
#endif
    Gd32DmaInterruptDisable<UART3_DMAx, UART3_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
#if !defined(GD32F4XX)
    NVIC_SetPriority(DMA1_Channel4_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
#else
    NVIC_SetPriority(DMA0_Channel4_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel4_IRQn);
#endif
#endif /* DMX_USE_UART3 */
#if defined(DMX_USE_UART4)
    /*
     * UART 4 TX
     */
    dma_deinit(UART4_DMAx, UART4_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_UART4_TX;
#endif
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
#if defined(GD32H7XX)
    dma_init_struct.periph_addr = (uint32_t)&USART_TDATA(UART4);
#else
    dma_init_struct.periph_addr = UART4 + 0x04U;
#endif
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(UART4_DMAx, UART4_TX_DMA_CHx, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(UART4_DMAx, UART4_TX_DMA_CHx);
    dma_memory_to_memory_disable(UART4_DMAx, UART4_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(UART4_DMAx, UART4_TX_DMA_CHx, UART4_TX_DMA_SUBPERIx);
#endif
    Gd32DmaInterruptDisable<UART4_DMAx, UART4_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
#if !defined(GD32F4XX)
    NVIC_SetPriority(DMA1_Channel3_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);
#else
    NVIC_SetPriority(DMA0_Channel7_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel7_IRQn);
#endif
#endif /* DMX_USE_UART4 */
#if defined(DMX_USE_USART5)
    /*
     * USART 5 TX
     */
    dma_deinit(USART5_DMAx, USART5_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_USART5_TX;
#endif
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F20X)
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
#if defined(GD32H7XX)
    dma_init_struct.periph_addr = (uint32_t)&USART_TDATA(USART5);
#else
    dma_init_struct.periph_addr = USART5 + 0x04U;
#endif
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F20X)
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(USART5_DMAx, USART5_TX_DMA_CHx, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(USART5_DMAx, USART5_TX_DMA_CHx);
    dma_memory_to_memory_disable(USART5_DMAx, USART5_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(USART5_DMAx, USART5_TX_DMA_CHx, USART5_TX_DMA_SUBPERIx);
#endif
    Gd32DmaInterruptDisable<USART5_DMAx, USART5_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
    NVIC_SetPriority(DMA1_Channel6_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel6_IRQn);
#endif /* DMX_USE_USART5 */
#if defined(DMX_USE_UART6)
    /*
     * UART 6 TX
     */
    dma_deinit(UART6_DMAx, UART6_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_UART6_TX;
#endif
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F20X)
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
#if defined(GD32H7XX)
    dma_init_struct.periph_addr = (uint32_t)&USART_TDATA(UART6);
#else
    dma_init_struct.periph_addr = UART6 + 0x04U;
#endif
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F20X)
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(UART6_DMAx, UART6_TX_DMA_CHx, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(UART6_DMAx, UART6_TX_DMA_CHx);
    dma_memory_to_memory_disable(UART6_DMAx, UART6_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(UART6_DMAx, UART6_TX_DMA_CHx, UART6_TX_DMA_SUBPERIx);
#endif
    Gd32DmaInterruptDisable<UART6_DMAx, UART4_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
#if defined(GD32F20X)
    NVIC_SetPriority(DMA1_Channel4_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
#else
    NVIC_SetPriority(DMA0_Channel1_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel1_IRQn);
#endif
#endif /* DMX_USE_UART6 */
#if defined(DMX_USE_UART7)
    /*
     * UART 7 TX
     */
    dma_deinit(UART7_DMAx, UART7_TX_DMA_CHx);
#if defined(GD32H7XX)
    dma_init_struct.request = DMA_REQUEST_UART7_TX;
#endif
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F20X)
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
#if defined(GD32H7XX)
    dma_init_struct.periph_addr = (uint32_t)&USART_TDATA(UART7);
#else
    dma_init_struct.periph_addr = UART7 + 0x04U;
#endif
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F20X)
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(UART7_DMAx, UART7_TX_DMA_CHx, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(UART7_DMAx, UART7_TX_DMA_CHx);
    dma_memory_to_memory_disable(UART7_DMAx, UART7_TX_DMA_CHx);
#if defined(GD32F4XX)
    dma_channel_subperipheral_select(UART7_DMAx, UART7_TX_DMA_CHx, UART7_TX_DMA_SUBPERIx);
#endif
#if defined(GD32F20X)
    NVIC_SetPriority(DMA1_Channel3_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);
#else
    NVIC_SetPriority(DMA0_Channel0_IRQn, 1);
    NVIC_EnableIRQ(DMA0_Channel0_IRQn);
#endif
#endif /* DMX_USE_UART7 */
}

template <uint32_t UsartPeripheral, uint32_t DmaController, dma_channel_enum DmaChannel, typename TxBufferType> void DmaRestartTx(TxBufferType& tx_buffer)
{
    if (tx_buffer.dmx.read_index != tx_buffer.dmx.write_index)
    {
        tx_buffer.dmx.read_index ^= 1;
    }

    const auto* p = &tx_buffer.dmx.data[tx_buffer.dmx.read_index];

    auto dma_chctl = DMA_CHCTL(DmaController, DmaChannel);

    // Disable channel
    dma_chctl &= ~DMA_CHXCTL_CHEN;
    DMA_CHCTL(DmaController, DmaChannel) = dma_chctl;

    // Clear transfer complete interrupt
    Gd32DmaInterruptFlagClear<DmaController, DmaChannel, DMA_INTF_FTFIF>();

    // Configure transfer
    DMA_CHMADDR(DmaController, DmaChannel) = reinterpret_cast<uint32_t>(p->data);
    DMA_CHCNT(DmaController, DmaChannel) = (p->length & DMA_CHXCNT_CNT);

    // Re-enable channel and interrupt
    dma_chctl |= DMA_CHXCTL_CHEN | DMA_INTERRUPT_ENABLE;
    DMA_CHCTL(DmaController, DmaChannel) = dma_chctl;

    // Enable USART DMA transmission
    USART_CTL2(UsartPeripheral) |= USART_TRANSMIT_DMA_ENABLE;
}

#define DMA_RESTART_TX(PORT_INDEX, USARTx, DMAx, CHx) DmaRestartTx<USARTx, DMAx, CHx>(s_TxBuffer[PORT_INDEX])

extern "C"
{
    void TIMER1_IRQHandler()
    {
        /*
         * USART 0
         */
#if defined(DMX_USE_USART0)
        if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0)
        {
            switch (s_TxBuffer[dmx::config::USART0_PORT].state)
            {
                case dmx::TxRxState::kDmxinter:
                    Gd32GpioModeOutput<USART0_GPIOx, USART0_TX_GPIO_PINx>();
                    GPIO_BC(USART0_GPIOx) = USART0_TX_GPIO_PINx;
                    s_TxBuffer[dmx::config::USART0_PORT].state = dmx::TxRxState::kBreak;
                    TIMER_CH0CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
                    break;
                case dmx::TxRxState::kBreak:
                    Gd32GpioModeAf<USART0_GPIOx, USART0_TX_GPIO_PINx, USART0>();
                    s_TxBuffer[dmx::config::USART0_PORT].state = dmx::TxRxState::kMab;
                    TIMER_CH0CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.mab_time;
                    break;
                case dmx::TxRxState::kMab:
                {
                    DMA_RESTART_TX(dmx::config::USART0_PORT, USART0, USART0_DMAx, USART0_TX_DMA_CHx);
                }
                break;
                default:
                    [[unlikely]] break;
            }

            TIMER_INTF(TIMER1) = static_cast<uint32_t>(~TIMER_INT_FLAG_CH0);
        }
#endif
        /*
         * USART 1
         */
#if defined(DMX_USE_USART1)
        if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH1) == TIMER_INT_FLAG_CH1)
        {
            switch (s_TxBuffer[dmx::config::USART1_PORT].state)
            {
                case dmx::TxRxState::kDmxinter:
                    Gd32GpioModeOutput<USART1_GPIOx, USART1_TX_GPIO_PINx>();
                    GPIO_BC(USART1_GPIOx) = USART1_TX_GPIO_PINx;
                    s_TxBuffer[dmx::config::USART1_PORT].state = dmx::TxRxState::kBreak;
                    TIMER_CH1CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
                    break;
                case dmx::TxRxState::kBreak:
                    Gd32GpioModeAf<USART1_GPIOx, USART1_TX_GPIO_PINx, USART1>();
                    s_TxBuffer[dmx::config::USART1_PORT].state = dmx::TxRxState::kMab;
                    TIMER_CH1CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.mab_time;
                    break;
                case dmx::TxRxState::kMab:
                {
                    DMA_RESTART_TX(dmx::config::USART1_PORT, USART1, USART1_DMAx, USART1_TX_DMA_CHx);
                }
                break;
                default:
                    [[unlikely]] break;
            }

            TIMER_INTF(TIMER1) = static_cast<uint32_t>(~TIMER_INT_FLAG_CH1);
        }
#endif
        /*
         * USART 2
         */
#if defined(DMX_USE_USART2)
        if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH2) == TIMER_INT_FLAG_CH2)
        {
            switch (s_TxBuffer[dmx::config::USART2_PORT].state)
            {
                case dmx::TxRxState::kDmxinter:
                    Gd32GpioModeOutput<USART2_GPIOx, USART2_TX_GPIO_PINx>();
                    GPIO_BC(USART2_GPIOx) = USART2_TX_GPIO_PINx;
                    s_TxBuffer[dmx::config::USART2_PORT].state = dmx::TxRxState::kBreak;
                    TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
                    break;
                case dmx::TxRxState::kBreak:
                    Gd32GpioModeAf<USART2_GPIOx, USART2_TX_GPIO_PINx, USART2>();
                    s_TxBuffer[dmx::config::USART2_PORT].state = dmx::TxRxState::kMab;
                    TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.mab_time;
                    break;
                case dmx::TxRxState::kMab:
                {
                    DMA_RESTART_TX(dmx::config::USART2_PORT, USART2, USART2_DMAx, USART2_TX_DMA_CHx);
                }
                break;
                default:
                    [[unlikely]] break;
            }

            TIMER_INTF(TIMER1) = static_cast<uint32_t>(~TIMER_INT_FLAG_CH2);
        }
#endif
        /*
         * UART 3
         */
#if defined(DMX_USE_UART3)
        if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH3) == TIMER_INT_FLAG_CH3)
        {
            switch (s_TxBuffer[dmx::config::UART3_PORT].state)
            {
                case dmx::TxRxState::kDmxinter:
                    Gd32GpioModeOutput<UART3_GPIOx, UART3_TX_GPIO_PINx>();
                    GPIO_BC(UART3_GPIOx) = UART3_TX_GPIO_PINx;
                    s_TxBuffer[dmx::config::UART3_PORT].state = dmx::TxRxState::kBreak;
                    TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
                    break;
                case dmx::TxRxState::kBreak:
                    Gd32GpioModeAf<UART3_GPIOx, UART3_TX_GPIO_PINx, UART3>();
                    s_TxBuffer[dmx::config::UART3_PORT].state = dmx::TxRxState::kMab;
                    TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.mab_time;
                    break;
                case dmx::TxRxState::kMab:
                {
                    DMA_RESTART_TX(dmx::config::UART3_PORT, UART3, UART3_DMAx, UART3_TX_DMA_CHx);
                }
                break;
                default:
                    [[unlikely]] break;
            }

            TIMER_INTF(TIMER1) = static_cast<uint32_t>(~TIMER_INT_FLAG_CH3);
        }
#endif
        // Clear all remaining interrupt flags (safety measure)
        TIMER_INTF(TIMER1) = static_cast<uint32_t>(~0);
    }

    void TIMER4_IRQHandler()
    {
        /*
         * UART 4
         */
#if defined(DMX_USE_UART4)
        if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0)
        {
            switch (s_TxBuffer[dmx::config::UART4_PORT].state)
            {
                case dmx::TxRxState::kDmxinter:
                    Gd32GpioModeOutput<UART4_TX_GPIOx, UART4_TX_GPIO_PINx>();
                    GPIO_BC(UART4_TX_GPIOx) = UART4_TX_GPIO_PINx;
                    s_TxBuffer[dmx::config::UART4_PORT].state = dmx::TxRxState::kBreak;
                    TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
                    break;
                case dmx::TxRxState::kBreak:
                    Gd32GpioModeAf<UART4_TX_GPIOx, UART4_TX_GPIO_PINx, UART4>();
                    s_TxBuffer[dmx::config::UART4_PORT].state = dmx::TxRxState::kMab;
                    TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.mab_time;
                    break;
                case dmx::TxRxState::kMab:
                {
                    DMA_RESTART_TX(dmx::config::UART4_PORT, UART4, UART4_DMAx, UART4_TX_DMA_CHx);
                }
                break;
                default:
                    break;
            }

            TIMER_INTF(TIMER4) = static_cast<uint32_t>(~TIMER_INT_FLAG_CH0);
        }
#endif
        /*
         * USART 5
         */
#if defined(DMX_USE_USART5)
        if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH1) == TIMER_INT_FLAG_CH1)
        {
            switch (s_TxBuffer[dmx::config::USART5_PORT].state)
            {
                case dmx::TxRxState::kDmxinter:
                    Gd32GpioModeOutput<USART5_GPIOx, USART5_TX_GPIO_PINx>();
                    GPIO_BC(USART5_GPIOx) = USART5_TX_GPIO_PINx;
                    s_TxBuffer[dmx::config::USART5_PORT].state = dmx::TxRxState::kBreak;
                    TIMER_CH1CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
                    break;
                case dmx::TxRxState::kBreak:
                    Gd32GpioModeAf<USART5_GPIOx, USART5_TX_GPIO_PINx, USART5>();
                    s_TxBuffer[dmx::config::USART5_PORT].state = dmx::TxRxState::kMab;
                    TIMER_CH1CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.mab_time;
                    break;
                case dmx::TxRxState::kMab:
                {
                    DMA_RESTART_TX(dmx::config::USART5_PORT, USART5, USART5_DMAx, USART5_TX_DMA_CHx);
                }
                break;
                default:
                    break;
            }

            TIMER_INTF(TIMER4) = static_cast<uint32_t>(~TIMER_INT_FLAG_CH1);
        }
#endif
        /*
         * UART 6
         */
#if defined(DMX_USE_UART6)
        if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH2) == TIMER_INT_FLAG_CH2)
        {
            switch (s_TxBuffer[dmx::config::UART6_PORT].state)
            {
                case dmx::TxRxState::kDmxinter:
                    Gd32GpioModeOutput<UART6_GPIOx, UART6_TX_GPIO_PINx>();
                    GPIO_BC(UART6_GPIOx) = UART6_TX_GPIO_PINx;
                    s_TxBuffer[dmx::config::UART6_PORT].state = dmx::TxRxState::kBreak;
                    TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
                    break;
                case dmx::TxRxState::kBreak:
                    Gd32GpioModeAf<UART6_GPIOx, UART6_TX_GPIO_PINx, UART6>();
                    s_TxBuffer[dmx::config::UART6_PORT].state = dmx::TxRxState::kMab;
                    TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.mab_time;
                    break;
                case dmx::TxRxState::kMab:
                {
                    DMA_RESTART_TX(dmx::config::UART6_PORT, UART6, UART6_DMAx, UART6_TX_DMA_CHx);
                }
                break;
                default:
                    break;
            }

            TIMER_INTF(TIMER4) = static_cast<uint32_t>(~TIMER_INT_FLAG_CH2);
        }
#endif
        /*
         * UART 7
         */
#if defined(DMX_USE_UART7)
        if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH3) == TIMER_INT_FLAG_CH3)
        {
            switch (s_TxBuffer[dmx::config::UART7_PORT].state)
            {
                case dmx::TxRxState::kDmxinter:
                    Gd32GpioModeOutput<UART7_GPIOx, UART7_TX_GPIO_PINx>();
                    GPIO_BC(UART7_GPIOx) = UART7_TX_GPIO_PINx;
                    s_TxBuffer[dmx::config::UART7_PORT].state = dmx::TxRxState::kBreak;
                    TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
                    break;
                case dmx::TxRxState::kBreak:
                    Gd32GpioModeAf<UART7_GPIOx, UART7_TX_GPIO_PINx, UART7>();
                    s_TxBuffer[dmx::config::UART7_PORT].state = dmx::TxRxState::kMab;
                    TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.mab_time;
                    break;
                case dmx::TxRxState::kMab:
                {
                    DMA_RESTART_TX(dmx::config::UART7_PORT, UART7, UART7_DMAx, UART7_TX_DMA_CHx);
                }
                break;
                default:
                    break;
            }

            TIMER_INTF(TIMER4) = static_cast<uint32_t>(~TIMER_INT_FLAG_CH3);
        }
#endif
        // Clear all remaining interrupt flags (safety measure)
        TIMER_INTF(TIMER4) = static_cast<uint32_t>(~0);
    }

    void TIMER6_IRQHandler()
    {
        const auto kIntFlag = TIMER_INTF(TIMER6);

        if ((kIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP)
        {
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
            for (uint32_t i = 0; i < DMX_MAX_PORTS; i++)
            {
                auto& packet = sv_rx_dmx_packets[i];
                packet.per_second = packet.count - packet.count_previous;
                packet.count_previous = packet.count;
            }
#endif
            g_Seconds.nUptime = g_Seconds.nUptime + 1;
        }

        // Clear all remaining interrupt flags (safety measure)
        TIMER_INTF(TIMER6) = static_cast<uint32_t>(~kIntFlag);
    }
}

extern "C"
{
/*
 * USART 0
 */
#if defined(DMX_USE_USART0)
#if defined(GD32F4XX) || defined(GD32H7XX)
    void DMA1_Channel7_IRQHandler()
    {
        if (Gd32DmaInterruptFlagGet<DMA1, DMA_CH7, DMA_INTERRUPT_FLAG_GET>())
        {
            Gd32DmaInterruptDisable<DMA1, DMA_CH7, DMA_INTERRUPT_DISABLE>();

            if (s_TxBuffer[dmx::config::USART0_PORT].output_style == dmx::OutputStyle::kDelta)
            {
                s_TxBuffer[dmx::config::USART0_PORT].state = dmx::TxRxState::kIdle;
            }
            else
            {
                timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0, TIMER_CNT(TIMER1) + s_dmx_transmit.inter_time);
                s_TxBuffer[dmx::config::USART0_PORT].state = dmx::TxRxState::kDmxinter;
            }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            sv_total_statistics[dmx::config::USART0_PORT].dmx.sent++;
#endif
        }

        Gd32DmaInterruptFlagClear<DMA1, DMA_CH7, DMA_INTERRUPT_FLAG_CLEAR>();
    }
#else
    void DMA0_Channel3_IRQHandler()
    {
        if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_GET>())
        {
            Gd32DmaInterruptDisable<DMA0, DMA_CH3, DMA_INTERRUPT_DISABLE>();

            if (s_TxBuffer[dmx::config::USART0_PORT].output_style == dmx::OutputStyle::kDelta)
            {
                s_TxBuffer[dmx::config::USART0_PORT].state = dmx::TxRxState::kIdle;
            }
            else
            {
                timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0, TIMER_CNT(TIMER1) + s_dmx_transmit.inter_time);
                s_TxBuffer[dmx::config::USART0_PORT].state = dmx::TxRxState::kDmxinter;
            }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[dmx::config::USART0_PORT].dmx.sent + 1;
            sv_total_statistics[dmx::config::USART0_PORT].dmx.sent = kSent;
#endif
        }

        Gd32DmaInterruptFlagClear<DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR>();
    }
#endif
#endif
/*
 * USART 1
 */
#if defined(DMX_USE_USART1)
    void DMA0_Channel6_IRQHandler()
    {
        if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH6, DMA_INTERRUPT_FLAG_GET>())
        {
            Gd32DmaInterruptDisable<DMA0, DMA_CH6, DMA_INTERRUPT_DISABLE>();

            if (s_TxBuffer[dmx::config::USART1_PORT].output_style == dmx::OutputStyle::kDelta)
            {
                s_TxBuffer[dmx::config::USART1_PORT].state = dmx::TxRxState::kIdle;
            }
            else
            {
                timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_1, TIMER_CNT(TIMER1) + s_dmx_transmit.inter_time);
                s_TxBuffer[dmx::config::USART1_PORT].state = dmx::TxRxState::kDmxinter;
            }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            sv_total_statistics[dmx::config::USART1_PORT].dmx.sent++;
#endif
        }

        Gd32DmaInterruptFlagClear<DMA0, DMA_CH6, DMA_INTERRUPT_FLAG_CLEAR>();
    }
#endif
/*
 * USART 2
 */
#if defined(DMX_USE_USART2)
#if defined(GD32F4XX) || defined(GD32H7XX)
    void DMA0_Channel3_IRQHandler()
    {
        if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_GET>())
        {
            Gd32DmaInterruptDisable<DMA0, DMA_CH3, DMA_INTERRUPT_DISABLE>();

            if (s_TxBuffer[dmx::config::USART2_PORT].output_style == dmx::OutputStyle::kDelta)
            {
                s_TxBuffer[dmx::config::USART2_PORT].state = dmx::TxRxState::kIdle;
            }
            else
            {
                timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_2, TIMER_CNT(TIMER1) + s_dmx_transmit.inter_time);
                s_TxBuffer[dmx::config::USART2_PORT].state = dmx::TxRxState::kDmxinter;
            }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            sv_total_statistics[dmx::config::USART2_PORT].dmx.sent++;
#endif
        }

        Gd32DmaInterruptFlagClear<DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR>();
    }
#else
    void DMA0_Channel1_IRQHandler()
    {
        if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_GET>())
        {
            Gd32DmaInterruptDisable<DMA0, DMA_CH1, DMA_INTERRUPT_DISABLE>();

            if (s_TxBuffer[dmx::config::USART2_PORT].output_style == dmx::OutputStyle::kDelta)
            {
                s_TxBuffer[dmx::config::USART2_PORT].state = dmx::TxRxState::kIdle;
            }
            else
            {
                timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_2, TIMER_CNT(TIMER1) + s_dmx_transmit.inter_time);
                s_TxBuffer[dmx::config::USART2_PORT].state = dmx::TxRxState::kDmxinter;
            }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[dmx::config::USART2_PORT].dmx.sent + 1;
            sv_total_statistics[dmx::config::USART2_PORT].dmx.sent = kSent;
#endif
        }

        Gd32DmaInterruptFlagClear<DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_CLEAR>();
    }
#endif
#endif
/*
 * UART 3
 */
#if defined(DMX_USE_UART3)
#if defined(GD32F4XX) || defined(GD32H7XX)
    void DMA0_Channel4_IRQHandler()
    {
        if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH4, DMA_INTERRUPT_FLAG_GET>())
        {
            Gd32DmaInterruptDisable<DMA0, DMA_CH4, DMA_INTERRUPT_DISABLE>();

            if (s_TxBuffer[dmx::config::UART3_PORT].output_style == dmx::OutputStyle::kDelta)
            {
                s_TxBuffer[dmx::config::UART3_PORT].state = dmx::TxRxState::kIdle;
            }
            else
            {
                timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_3, TIMER_CNT(TIMER1) + s_dmx_transmit.inter_time);
                s_TxBuffer[dmx::config::UART3_PORT].state = dmx::TxRxState::kDmxinter;
            }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            sv_total_statistics[dmx::config::UART3_PORT].dmx.sent++;
#endif
        }

        Gd32DmaInterruptFlagClear<DMA0, DMA_CH4, DMA_INTERRUPT_FLAG_CLEAR>();
    }
#else
    void DMA1_Channel4_IRQHandler()
    {
        if (Gd32DmaInterruptFlagGet<DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_GET>())
        {
            Gd32DmaInterruptDisable<DMA1, DMA_CH4, DMA_INTERRUPT_DISABLE>();

            if (s_TxBuffer[dmx::config::UART3_PORT].output_style == dmx::OutputStyle::kDelta)
            {
                s_TxBuffer[dmx::config::UART3_PORT].state = dmx::TxRxState::kIdle;
            }
            else
            {
                timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_3, TIMER_CNT(TIMER1) + s_dmx_transmit.inter_time);
                s_TxBuffer[dmx::config::UART3_PORT].state = dmx::TxRxState::kDmxinter;
            }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            sv_total_statistics[dmx::config::UART3_PORT].dmx.sent++;
#endif
        }

        Gd32DmaInterruptFlagClear<DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_CLEAR>();
    }
#endif
#endif
/*
 * UART 4
 */
#if defined(DMX_USE_UART4)
#if defined(GD32F20X)
    void DMA1_Channel3_IRQHandler()
    {
        if (Gd32DmaInterruptFlagGet<DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_GET>())
        {
            Gd32DmaInterruptDisable<DMA1, DMA_CH3, DMA_INTERRUPT_DISABLE>();

            if (s_TxBuffer[dmx::config::UART4_PORT].output_style == dmx::OutputStyle::kDelta)
            {
                s_TxBuffer[dmx::config::UART4_PORT].state = dmx::TxRxState::kIdle;
            }
            else
            {
                timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0, TIMER_CNT(TIMER4) + s_dmx_transmit.inter_time);
                s_TxBuffer[dmx::config::UART4_PORT].state = dmx::TxRxState::kDmxinter;
            }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[dmx::config::UART4_PORT].dmx.sent + 1;
            sv_total_statistics[dmx::config::UART4_PORT].dmx.sent = kSent;
#endif
        }

        Gd32DmaInterruptFlagClear<DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR>();
    }
#endif
#if defined(GD32F4XX) || defined(GD32H7XX)
    void DMA0_Channel7_IRQHandler()
    {
        if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH7, DMA_INTERRUPT_FLAG_GET>())
        {
            Gd32DmaInterruptDisable<DMA0, DMA_CH7, DMA_INTERRUPT_DISABLE>();

            if (s_TxBuffer[dmx::config::UART4_PORT].output_style == dmx::OutputStyle::kDelta)
            {
                s_TxBuffer[dmx::config::UART4_PORT].state = dmx::TxRxState::kIdle;
            }
            else
            {
                timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0, TIMER_CNT(TIMER4) + s_dmx_transmit.inter_time);
                s_TxBuffer[dmx::config::UART4_PORT].state = dmx::TxRxState::kDmxinter;
            }
        }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
        sv_total_statistics[dmx::config::UART4_PORT].dmx.sent++;
#endif

        Gd32DmaInterruptFlagClear<DMA0, DMA_CH7, DMA_INTERRUPT_FLAG_CLEAR>();
    }
#endif
#endif
/*
 * USART 5
 */
#if defined(DMX_USE_USART5)
    void DMA1_Channel6_IRQHandler()
    {
        if (Gd32DmaInterruptFlagGet<DMA1, DMA_CH6, DMA_INTERRUPT_FLAG_GET>())
        {
            Gd32DmaInterruptDisable<DMA1, DMA_CH6, DMA_INTERRUPT_DISABLE>();

            if (s_TxBuffer[dmx::config::USART5_PORT].output_style == dmx::OutputStyle::kDelta)
            {
                s_TxBuffer[dmx::config::USART5_PORT].state = dmx::TxRxState::kIdle;
            }
            else
            {
                timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_1, TIMER_CNT(TIMER4) + s_dmx_transmit.inter_time);
                s_TxBuffer[dmx::config::USART5_PORT].state = dmx::TxRxState::kDmxinter;
            }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto kSent = sv_total_statistics[dmx::config::USART5_PORT].dmx.sent + 1;
            sv_total_statistics[dmx::config::USART5_PORT].dmx.sent = kSent;
#endif
        }

        Gd32DmaInterruptFlagClear<DMA1, DMA_CH6, DMA_INTERRUPT_FLAG_CLEAR>();
    }
#endif
/*
 * UART 6
 */
#if defined(DMX_USE_UART6)
#if defined(GD32F20X)
    void DMA1_Channel4_IRQHandler()
    {
        if (Gd32DmaInterruptFlagGet<DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_GET>())
        {
            Gd32DmaInterruptDisable<DMA1, DMA_CH4, DMA_INTERRUPT_DISABLE>();

            if (s_TxBuffer[dmx::config::UART6_PORT].output_style == dmx::OutputStyle::kDelta)
            {
                s_TxBuffer[dmx::config::UART6_PORT].state = dmx::TxRxState::kIdle;
            }
            else
            {
                timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2, TIMER_CNT(TIMER4) + s_dmx_transmit.inter_time);
                s_TxBuffer[dmx::config::UART6_PORT].state = dmx::TxRxState::kDmxinter;
            }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            const auto sent = sv_total_statistics[dmx::config::UART6_PORT].dmx.sent + 1;
            sv_total_statistics[dmx::config::UART6_PORT].dmx.sent = sent;
#endif
        }

        Gd32DmaInterruptFlagClear<DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_CLEAR>();
    }
#endif
#if defined(GD32F4XX) || defined(GD32H7XX)
    void DMA0_Channel1_IRQHandler()
    {
        if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_GET>())
        {
            Gd32DmaInterruptDisable<DMA0, DMA_CH1, DMA_INTERRUPT_DISABLE>();

            if (s_TxBuffer[dmx::config::UART6_PORT].output_style == dmx::OutputStyle::kDelta)
            {
                s_TxBuffer[dmx::config::UART6_PORT].state = dmx::TxRxState::kIdle;
            }
            else
            {
                timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2, TIMER_CNT(TIMER4) + s_dmx_transmit.inter_time);
                s_TxBuffer[dmx::config::UART6_PORT].state = dmx::TxRxState::kDmxinter;
            }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            sv_total_statistics[dmx::config::UART6_PORT].dmx.sent++;
#endif
        }

        Gd32DmaInterruptFlagClear<DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_CLEAR>();
    }
#endif
#endif
/*
 * UART 7
 */
#if defined(DMX_USE_UART7)
#if defined(GD32F20X)
    void DMA1_Channel3_IRQHandler()
    {
        if (Gd32DmaInterruptFlagGet<DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_GET>())
        {
            Gd32DmaInterruptDisable<DMA1, DMA_CH3, DMA_INTERRUPT_DISABLE>();

            if (s_TxBuffer[dmx::config::UART7_PORT].output_style == dmx::OutputStyle::kDelta)
            {
                s_TxBuffer[dmx::config::UART7_PORT].state = dmx::TxRxState::kIdle;
            }
            else
            {
                timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3, TIMER_CNT(TIMER4) + s_dmx_transmit.inter_time);
                s_TxBuffer[dmx::config::UART7_PORT].state = dmx::TxRxState::kDmxinter;
            }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            sv_total_statistics[dmx::config::UART7_PORT].dmx.sent++;
#endif
        }

        Gd32DmaInterruptFlagClear<DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR>();
    }
#endif
#if defined(GD32F4XX) || defined(GD32H7XX)
    void DMA0_Channel0_IRQHandler()
    {
        if (Gd32DmaInterruptFlagGet<DMA0, DMA_CH0, DMA_INTERRUPT_FLAG_GET>())
        {
            Gd32DmaInterruptDisable<DMA0, DMA_CH0, DMA_INTERRUPT_DISABLE>();

            if (s_TxBuffer[dmx::config::UART7_PORT].output_style == dmx::OutputStyle::kDelta)
            {
                s_TxBuffer[dmx::config::UART7_PORT].state = dmx::TxRxState::kIdle;
            }
            else
            {
                timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3, TIMER_CNT(TIMER4) + s_dmx_transmit.inter_time);
                s_TxBuffer[dmx::config::UART7_PORT].state = dmx::TxRxState::kDmxinter;
            }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
            sv_total_statistics[dmx::config::UART7_PORT].dmx.sent++;
#endif
        }

        Gd32DmaInterruptFlagClear<DMA0, DMA_CH0, DMA_INTERRUPT_FLAG_CLEAR>();
    }
#endif
#endif
}

[[gnu::noinline]]
void Dmx::SetPortDirection(uint32_t port_index, dmx::PortDirection port_direction, bool enable_data)
{
    DEBUG_PRINTF("port_index=%u", port_index);

    DMX_CHECK_PORT_INDEX_VOID(port_index);

    if (m_dmxPortDirection[port_index] != port_direction)
    {
        m_dmxPortDirection[port_index] = port_direction;

        StopData(port_index);

        if (port_direction == dmx::PortDirection::kOutput)
        {
            GPIO_BOP(kDirGpio[port_index].port) = kDirGpio[port_index].pin;
        }
        else if (port_direction == dmx::PortDirection::kInput)
        {
            GPIO_BC(kDirGpio[port_index].port) = kDirGpio[port_index].pin;
        }
        else
        {
            assert(0);
        }
    }
    else if (!enable_data)
    {
        StopData(port_index);
    }

    if (enable_data)
    {
        StartData(port_index);
    }
}

void Dmx::ClearData(uint32_t port_index)
{
    assert(port_index < dmx::config::max::PORTS);

    auto* p = &s_TxBuffer[port_index].dmx.data[0];
    p->length = 513; // Including START Code
    __builtin_memset(p->data, 0, dmx::buffer::SIZE);
}

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
volatile dmx::TotalStatistics& Dmx::GetTotalStatistics(uint32_t port_index)
{
    sv_total_statistics[port_index].dmx.received = sv_rx_dmx_packets[port_index].count;
    return sv_total_statistics[port_index];
}
#endif

template <uint32_t port_index, uint32_t nUart> static void StartDmxOutput()
{
    DEBUG_PRINTF("port_index=%u, nUart=%u", port_index, nUart);
    /*
     * USART_FLAG_TC is set after power on.
     * The flag is cleared by DMA interrupt when maximum slots - 1 are transmitted.
     */

    // TODO(a): Do we need a timeout just to be safe?
    while (SET != usart_flag_get(nUart, USART_FLAG_TC));

    switch (nUart)
    {
        /* TIMER 1 */
#if defined(DMX_USE_USART0)
        case USART0:
            Gd32GpioModeOutput<USART0_GPIOx, USART0_TX_GPIO_PINx>();
            GPIO_BC(USART0_GPIOx) = USART0_TX_GPIO_PINx;
            TIMER_CH0CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
            s_TxBuffer[dmx::config::USART0_PORT].state = dmx::TxRxState::kBreak;
            return;
            break;
#endif
#if defined(DMX_USE_USART1)
        case USART1:
            Gd32GpioModeOutput<USART1_GPIOx, USART1_TX_GPIO_PINx>();
            GPIO_BC(USART1_GPIOx) = USART1_TX_GPIO_PINx;
            TIMER_CH1CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
            s_TxBuffer[dmx::config::USART1_PORT].state = dmx::TxRxState::kBreak;
            return;
            break;
#endif
#if defined(DMX_USE_USART2)
        case USART2:
            Gd32GpioModeOutput<USART2_GPIOx, USART2_TX_GPIO_PINx>();
            GPIO_BC(USART2_GPIOx) = USART2_TX_GPIO_PINx;
            TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
            s_TxBuffer[dmx::config::USART2_PORT].state = dmx::TxRxState::kBreak;
            return;
            break;
#endif
#if defined(DMX_USE_UART3)
        case UART3:
            Gd32GpioModeOutput<UART3_GPIOx, UART3_TX_GPIO_PINx>();
            GPIO_BC(UART3_GPIOx) = UART3_TX_GPIO_PINx;
            TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + s_dmx_transmit.break_time;
            s_TxBuffer[dmx::config::UART3_PORT].state = dmx::TxRxState::kBreak;
            return;
            break;
#endif
            /* TIMER 4 */
#if defined(DMX_USE_UART4)
        case UART4:
            Gd32GpioModeOutput<UART4_TX_GPIOx, UART4_TX_GPIO_PINx>();
            GPIO_BC(UART4_TX_GPIOx) = UART4_TX_GPIO_PINx;
            TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
            s_TxBuffer[dmx::config::UART4_PORT].state = dmx::TxRxState::kBreak;
            return;
            break;
#endif
#if defined(DMX_USE_USART5)
        case USART5:
            Gd32GpioModeOutput<USART5_GPIOx, USART5_TX_GPIO_PINx>();
            GPIO_BC(USART5_GPIOx) = USART5_TX_GPIO_PINx;
            TIMER_CH1CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
            s_TxBuffer[dmx::config::USART5_PORT].state = dmx::TxRxState::kBreak;
            return;
            break;
#endif
#if defined(DMX_USE_UART6)
        case UART6:
            Gd32GpioModeOutput<UART6_GPIOx, UART6_TX_GPIO_PINx>();
            GPIO_BC(UART6_GPIOx) = UART6_TX_GPIO_PINx;
            TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
            s_TxBuffer[dmx::config::UART6_PORT].state = dmx::TxRxState::kBreak;
            return;
            break;
#endif
#if defined(DMX_USE_UART7)
        case UART7:
            Gd32GpioModeOutput<UART7_GPIOx, UART7_TX_GPIO_PINx>();
            GPIO_BC(UART7_GPIOx) = UART7_TX_GPIO_PINx;
            TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + s_dmx_transmit.break_time;
            s_TxBuffer[dmx::config::UART7_PORT].state = dmx::TxRxState::kBreak;
            return;
            break;
#endif
        default:
            [[unlikely]] assert(0);

            break;
    }

    assert(0);
}

template <uint32_t portIndex> static void StartDmxOutputPort()
{
    if constexpr (portIndex < dmx::config::max::PORTS)
    {
        StartDmxOutput<portIndex, DmxPortToUart(portIndex)>();
    }
}

void Dmx::StartDmxOutput(uint32_t port_index)
{
    assert(port_index < dmx::config::max::PORTS);

    switch (port_index)
    {
        case 0:
            StartDmxOutputPort<0>();
            break;
#if DMX_MAX_PORTS >= 2
        case 1:
            StartDmxOutputPort<1>();
            break;
#endif
#if DMX_MAX_PORTS >= 3
        case 2:
            StartDmxOutputPort<2>();
            break;
#endif
#if DMX_MAX_PORTS >= 4
        case 3:
            StartDmxOutputPort<3>();
            break;
#endif
#if DMX_MAX_PORTS >= 5
        case 4:
            StartDmxOutputPort<4>();
            break;
#endif
#if DMX_MAX_PORTS >= 6
        case 5:
            StartDmxOutputPort<5>();
            break;
#endif
#if DMX_MAX_PORTS >= 7
        case 6:
            StartDmxOutputPort<6>();
            break;
#endif
#if DMX_MAX_PORTS >= 8
        case 7:
            StartDmxOutputPort<7>();
            break;
#endif
        default:
            [[unlikely]] break;
    }
}

void Dmx::StopData(uint32_t port_index)
{
    DMX_CHECK_PORT_INDEX_VOID(port_index);

    if (sv_port_state[port_index] == dmx::PortState::kIdle)
    {
        return;
    }

    sv_port_state[port_index] = dmx::PortState::kIdle;

    const auto kUart = DmxPortToUart(port_index);

    if (m_dmxPortDirection[port_index] == dmx::PortDirection::kOutput)
    {
        do
        {
            if (s_TxBuffer[port_index].state == dmx::TxRxState::kDmxinter)
            {
                Gd32UsartFlagClear<USART_FLAG_TC>(kUart);
                do
                {
                    __DMB();
                } while (!Gd32UsartFlagGet<USART_FLAG_TC>(kUart));

                s_TxBuffer[port_index].state = dmx::TxRxState::kIdle;
            }
        } while (s_TxBuffer[port_index].state != dmx::TxRxState::kIdle);

        return;
    }

    if (m_dmxPortDirection[port_index] == dmx::PortDirection::kInput)
    {
        Gd32UsartInterruptDisable<USART_INT_RBNE>(kUart);
        Gd32UsartInterruptDisable<USART_INT_FLAG_IDLE>(kUart);
        sv_rx_buffer[port_index].state = dmx::TxRxState::kIdle;
        return;
    }

    assert(0);
}

// DMX Send
template <uint32_t portIndex, bool hasStartCode, dmx::SendStyle dmxSendStyle> void Dmx::SetSendDataInternal(const uint8_t* data, uint32_t length)
{
    DMX_CHECK_PORT_INDEX_VOID(portIndex);

    auto& tx_buffer = s_TxBuffer[portIndex];
    const auto kHasDataPending = tx_buffer.dmx.read_index != tx_buffer.dmx.write_index;

    if (!kHasDataPending)
    {
        // No pending data — switch to the other buffer
        tx_buffer.dmx.write_index ^= 1;
    }

    const auto kWriteIndex = tx_buffer.dmx.write_index;

    auto* dst_data = tx_buffer.dmx.data[kWriteIndex].data;

    const auto kCappedLength = (length < m_nDmxTransmitSlots) ? length : m_nDmxTransmitSlots;
    tx_buffer.dmx.data[kWriteIndex].length = kCappedLength + 1;

    tx_buffer.dmx.data_pending = true;

    if constexpr (hasStartCode)
    {
        memcpy(dst_data, data, kCappedLength);
    }
    else
    {
        dst_data[0] = dmx::kStartCode;
        memcpy(&dst_data[1], data, kCappedLength);
    }

    if (kCappedLength != m_nDmxTransmissionLength[portIndex])
    {
        m_nDmxTransmissionLength[portIndex] = kCappedLength;
        SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
    }

    if constexpr (dmxSendStyle == dmx::SendStyle::kDirect)
    {
        StartOutput(portIndex);
    }
}

void Dmx::Blackout()
{
    DEBUG_ENTRY();

    for (uint32_t port_index = 0; port_index < DMX_MAX_PORTS; port_index++)
    {
        if (m_dmxPortDirection[port_index] == dmx::PortDirection::kOutput)
        {
            StopData(port_index);
            ClearData(port_index);
            StartData(port_index);
        }
    }

    DEBUG_EXIT();
}

void Dmx::FullOn()
{
    DEBUG_ENTRY();

    for (uint32_t port_index = 0; port_index < DMX_MAX_PORTS; port_index++)
    {
        if (m_dmxPortDirection[port_index] == dmx::PortDirection::kOutput)
        {
            StopData(port_index);

            auto* __restrict__ p = &s_TxBuffer[port_index].dmx.data[0];
            auto* p32 = reinterpret_cast<uint32_t*>(p->data);

            for (auto i = 0; i < dmx::buffer::SIZE / 4; i++)
            {
                *p32++ = UINT32_MAX;
            }

            p->data[0] = dmx::kStartCode;
            p->length = 513;

            StartData(port_index);
        }
    }

    DEBUG_EXIT();
}

void Dmx::StartOutput(uint32_t port_index)
{
    DMX_CHECK_PORT_INDEX_VOID(port_index);

    if ((sv_port_state[port_index] == dmx::PortState::kTx) && (s_TxBuffer[port_index].output_style == dmx::OutputStyle::kDelta) && (s_TxBuffer[port_index].state == dmx::TxRxState::kIdle))
    {
        StartDmxOutput(port_index);
    }
}

void Dmx::Sync()
{
    for (uint32_t port_index = 0; port_index < dmx::config::max::PORTS; port_index++)
    {
        auto& tx_buffer = s_TxBuffer[port_index];

        if (!tx_buffer.dmx.data_pending)
        {
            continue;
        }

        tx_buffer.dmx.data_pending = false;

        if (sv_port_state[port_index] == dmx::PortState::kTx)
        {
            if ((tx_buffer.output_style == dmx::OutputStyle::kDelta) && (tx_buffer.state == dmx::TxRxState::kIdle))
            {
                StartDmxOutput(port_index);
            }
        }
    }
}

void Dmx::StartData(uint32_t port_index)
{
    DEBUG_PRINTF("port_index=%u", port_index);
    DMX_CHECK_PORT_INDEX_VOID(port_index);
    assert(sv_port_state[port_index] == dmx::PortState::kIdle);

    if (m_dmxPortDirection[port_index] == dmx::PortDirection::kOutput)
    {
        sv_port_state[port_index] = dmx::PortState::kTx;
        SetOutputStyle(port_index, GetOutputStyle(port_index));
        return;
    }

    if (m_dmxPortDirection[port_index] == dmx::PortDirection::kInput)
    {
        sv_rx_buffer[port_index].state = dmx::TxRxState::kIdle;

        const auto kUart = DmxPortToUart(port_index);

        do
        {
            __DMB();
        } while (!Gd32UsartFlagGet<USART_FLAG_TBE>(kUart));

        Gd32UsartInterruptFlagClear<USART_INT_FLAG_RBNE>(kUart);
        Gd32UsartInterruptFlagClear<USART_INT_FLAG_IDLE>(kUart);
        Gd32UsartInterruptEnable<USART_INT_RBNE>(kUart);
        Gd32UsartInterruptEnable<USART_INT_FLAG_IDLE>(kUart);

        sv_port_state[port_index] = dmx::PortState::kRx;
        return;
    }

    assert(0);
}

// DMX Receive

const uint8_t* Dmx::GetDmxChanged(uint32_t port_index)
{
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
    const auto* __restrict__ p = GetDmxAvailable(port_index);

    if (p == nullptr)
    {
        return nullptr;
    }

    const auto* __restrict__ src32 = reinterpret_cast<const volatile uint32_t*>(sv_rx_buffer[port_index].dmx.current.data);
    auto* __restrict__ dst32 = reinterpret_cast<volatile uint32_t*>(sv_rx_buffer[port_index].dmx.previous.data);

    if (sv_rx_buffer[port_index].dmx.current.slots_in_packet != sv_rx_buffer[port_index].dmx.previous.slots_in_packet)
    {
        sv_rx_buffer[port_index].dmx.previous.slots_in_packet = sv_rx_buffer[port_index].dmx.current.slots_in_packet;

        for (size_t i = 0; i < dmx::buffer::SIZE / 4; ++i)
        {
            dst32[i] = src32[i];
        }

        return p;
    }

    bool is_changed = false;

    for (size_t i = 0; i < dmx::buffer::SIZE / 4; ++i)
    {
        const auto kSrcValue = src32[i];
        auto dst_value = dst32[i];

        if (kSrcValue != dst_value)
        {
            dst32[i] = kSrcValue;
            is_changed = true;
        }
    }

    return (is_changed ? p : nullptr);
#else
    return nullptr;
#endif
}

const uint8_t* Dmx::GetDmxAvailable([[maybe_unused]] uint32_t port_index)
{
    DMX_CHECK_PORT_INDEX_PTR(port_index);
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
    auto slots_in_packet = sv_rx_buffer[port_index].dmx.current.slots_in_packet;

    if ((slots_in_packet & 0x8000) != 0x8000)
    {
        return nullptr;
    }

    slots_in_packet &= ~0x8000;
    slots_in_packet--; // Remove SC from length
    sv_rx_buffer[port_index].dmx.current.slots_in_packet = slots_in_packet;

    return const_cast<const uint8_t*>(sv_rx_buffer[port_index].dmx.current.data);
#else
    return nullptr;
#endif
}

const uint8_t* Dmx::GetDmxCurrentData(uint32_t port_index)
{
    return const_cast<const uint8_t*>(sv_rx_buffer[port_index].dmx.current.data);
}

uint32_t Dmx::GetDmxUpdatesPerSecond([[maybe_unused]] uint32_t port_index)
{
    DMX_CHECK_PORT_INDEX_RET(port_index, 0);
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
    return sv_rx_dmx_packets[port_index].per_second;
#else
    return 0;
#endif
}

// RDM Send

template <uint32_t port_index, uint32_t nUart> static void RdmSendRawImpl(const uint8_t* const kData, uint32_t length)
{
    DMX_CHECK_PORT_INDEX_VOID(port_index);
    assert(kData != nullptr);
    assert(length != 0);

    switch (nUart)
    {
#if defined(DMX_USE_USART0)
        case USART0:
            Gd32GpioModeOutput<USART0_GPIOx, USART0_TX_GPIO_PINx>();
            GPIO_BC(USART0_GPIOx) = USART0_TX_GPIO_PINx;
            break;
#endif
#if defined(DMX_USE_USART1)
        case USART1:
            Gd32GpioModeOutput<USART1_GPIOx, USART1_TX_GPIO_PINx>();
            GPIO_BC(USART1_GPIOx) = USART1_TX_GPIO_PINx;
            break;
#endif
#if defined(DMX_USE_USART2)
        case USART2:
            Gd32GpioModeOutput<USART2_GPIOx, USART2_TX_GPIO_PINx>();
            GPIO_BC(USART2_GPIOx) = USART2_TX_GPIO_PINx;
            break;
#endif
#if defined(DMX_USE_UART3)
        case UART3:
            Gd32GpioModeOutput<UART3_GPIOx, UART3_TX_GPIO_PINx>();
            GPIO_BC(UART3_GPIOx) = UART3_TX_GPIO_PINx;
            break;
#endif
#if defined(DMX_USE_UART4)
        case UART4:
            Gd32GpioModeOutput<UART4_TX_GPIOx, UART4_TX_GPIO_PINx>();
            GPIO_BC(UART4_TX_GPIOx) = UART4_TX_GPIO_PINx;
            break;
#endif
#if defined(DMX_USE_USART5)
        case USART5:
            Gd32GpioModeOutput<USART5_GPIOx, USART5_TX_GPIO_PINx>();
            GPIO_BC(USART5_GPIOx) = USART5_TX_GPIO_PINx;
            break;
#endif
#if defined(DMX_USE_UART6)
        case UART6:
            Gd32GpioModeOutput<UART6_GPIOx, UART6_TX_GPIO_PINx>();
            GPIO_BC(UART6_GPIOx) = UART6_TX_GPIO_PINx;
            break;
#endif
#if defined(DMX_USE_UART7)
        case UART7:
            Gd32GpioModeOutput<UART7_GPIOx, UART7_TX_GPIO_PINx>();
            GPIO_BC(UART7_GPIOx) = UART7_TX_GPIO_PINx;
            break;
#endif
        default:
            [[unlikely]] assert(0);

            break;
    }

    TIMER_CNT(TIMER5) = 0;
    do
    {
        __DMB();
    } while (TIMER_CNT(TIMER5) < RDM_TRANSMIT_BREAK_TIME);

    switch (nUart)
    {
#if defined(DMX_USE_USART0)
        case USART0:
            Gd32GpioModeAf<USART0_GPIOx, USART0_TX_GPIO_PINx, USART0>();
            break;
#endif
#if defined(DMX_USE_USART1)
        case USART1:
            Gd32GpioModeAf<USART1_GPIOx, USART1_TX_GPIO_PINx, USART1>();
            break;
#endif
#if defined(DMX_USE_USART2)
        case USART2:
            Gd32GpioModeAf<USART2_GPIOx, USART2_TX_GPIO_PINx, USART2>();
            break;
#endif
#if defined(DMX_USE_UART3)
        case UART3:
            Gd32GpioModeAf<UART3_GPIOx, UART3_TX_GPIO_PINx, UART3>();
            break;
#endif
#if defined(DMX_USE_UART4)
        case UART4:
            Gd32GpioModeAf<UART4_TX_GPIOx, UART4_TX_GPIO_PINx, UART4>();
            break;
#endif
#if defined(DMX_USE_USART5)
        case USART5:
            Gd32GpioModeAf<USART5_GPIOx, USART5_TX_GPIO_PINx, USART5>();
            break;
#endif
#if defined(DMX_USE_UART6)
        case UART6:
            Gd32GpioModeAf<UART6_GPIOx, UART6_TX_GPIO_PINx, UART6>();
            break;
#endif
#if defined(DMX_USE_UART7)
        case UART7:
            Gd32GpioModeAf<UART7_GPIOx, UART7_TX_GPIO_PINx, UART7>();
            break;
#endif
        default:
            [[unlikely]] assert(0);

            break;
    }

    TIMER_CNT(TIMER5) = 0;
    do
    {
        __DMB();
    } while (TIMER_CNT(TIMER5) < RDM_TRANSMIT_MAB_TIME);

    for (uint32_t i = 0; i < length; i++)
    {
        do
        {
            __DMB();
        } while (!Gd32UsartFlagGet<USART_FLAG_TBE>(nUart));

        USART_TDATA(nUart) = USART_TDATA_TDATA & kData[i];
    }

    while (!Gd32UsartFlagGet<USART_FLAG_TC>(nUart))
    {
        static_cast<void>(GET_BITS(USART_RDATA(nUart), 0U, 8U));
    }

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
    sv_total_statistics[port_index].rdm.sent.classes = sv_total_statistics[port_index].rdm.sent.classes + 1;
#endif
}

void Dmx::RdmSendRaw(uint32_t port_index, const uint8_t* data, uint32_t length)
{
    DMX_CHECK_PORT_INDEX_VOID(port_index);

    switch (port_index)
    {
        case 0:
            RdmSendRawImpl<0, DmxPortToUart(0)>(data, length);
            break;
#if DMX_MAX_PORTS >= 2
        case 1:
            RdmSendRawImpl<1, DmxPortToUart(1)>(data, length);
            break;
#endif
#if DMX_MAX_PORTS >= 3
        case 2:
            RdmSendRawImpl<2, DmxPortToUart(2)>(data, length);
            break;
#endif
#if DMX_MAX_PORTS >= 4
        case 3:
            RdmSendRawImpl<3, DmxPortToUart(3)>(data, length);
            break;
#endif
#if DMX_MAX_PORTS >= 5
        case 4:
            RdmSendRawImpl<4, DmxPortToUart(4)>(data, length);
            break;
#endif
#if DMX_MAX_PORTS >= 6
        case 5:
            RdmSendRawImpl<5, DmxPortToUart(5)>(data, length);
            break;
#endif
#if DMX_MAX_PORTS >= 7
        case 6:
            RdmSendRawImpl<6, DmxPortToUart(6)>(data, length);
            break;
#endif
#if DMX_MAX_PORTS == 8
        case 7:
            RdmSendRawImpl<7, DmxPortToUart(7)>(data, length);
            break;
#endif
        default:
            [[unlikely]] break;
    }
}

void Dmx::RdmSendDiscoveryRespondMessage(uint32_t port_index, const uint8_t* data, uint32_t length)
{
    DMX_CHECK_PORT_INDEX_VOID(port_index);
    assert(data != nullptr);
    assert(length != 0);

    // 3.2.2 Responder Packet spacing
    udelay(RDM_RESPONDER_PACKET_SPACING, gsv_RdmDataReceiveEnd);

    SetPortDirection(port_index, dmx::PortDirection::kOutput, false);

    const auto kUart = DmxPortToUart(port_index);

    for (uint32_t i = 0; i < length; i++)
    {
        do
        {
            __DMB();
        } while (!Gd32UsartFlagGet<USART_FLAG_TBE>(kUart));

        USART_TDATA(kUart) = USART_TDATA_TDATA & data[i];
    }

    while (!Gd32UsartFlagGet<USART_FLAG_TC>(kUart))
    {
        static_cast<void>(GET_BITS(USART_RDATA(kUart), 0U, 8U));
    }

    TIMER_CNT(TIMER5) = 0;
    do
    {
        __DMB();
    } while (TIMER_CNT(TIMER5) < RDM_RESPONDER_DATA_DIRECTION_DELAY);

    SetPortDirection(port_index, dmx::PortDirection::kInput, true);

#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
    sv_total_statistics[port_index].rdm.sent.discovery_response = sv_total_statistics[port_index].rdm.sent.discovery_response + 1;
#endif
}

// RDM Receive

const uint8_t* Dmx::RdmReceive(uint32_t port_index)
{
    DMX_CHECK_PORT_INDEX_PTR(port_index);

    if ((sv_rx_buffer[port_index].rdm.index & 0x4000) != 0x4000)
    {
        return nullptr;
    }

    sv_rx_buffer[port_index].rdm.index = 0;

    const auto* p = const_cast<const uint8_t*>(sv_rx_buffer[port_index].rdm.data);

    if (p[0] == E120_SC_RDM)
    {
        const auto* rdm_command = reinterpret_cast<const struct TRdmMessage*>(p);

        uint32_t i;
        uint16_t checksum = 0;

        for (i = 0; i < 24; i++)
        {
            checksum = static_cast<uint16_t>(checksum + p[i]);
        }

        for (; i < rdm_command->message_length; i++)
        {
            checksum = static_cast<uint16_t>(checksum + p[i]);
        }

        if (p[i++] == static_cast<uint8_t>(checksum >> 8))
        {
            if (p[i] == static_cast<uint8_t>(checksum))
            {
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
                sv_total_statistics[port_index].rdm.received.good = sv_total_statistics[port_index].rdm.received.good + 1;
#endif
                return p;
            }
        }
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
        sv_total_statistics[port_index].rdm.received.bad = sv_total_statistics[port_index].rdm.received.bad + 1;
#endif
        return nullptr;
    }
    else
    {
#if !defined(CONFIG_DMX_DISABLE_STATISTICS)
        sv_total_statistics[port_index].rdm.received.discovery_response = sv_total_statistics[port_index].rdm.received.discovery_response + 1;
#endif
    }

    return p;
}

const uint8_t* Dmx::RdmReceiveTimeOut(uint32_t port_index, uint16_t time_out)
{
    DMX_CHECK_PORT_INDEX_PTR(port_index);

    uint8_t* p = nullptr;
    TIMER_CNT(TIMER5) = 0;

    do
    {
        if ((p = const_cast<uint8_t*>(RdmReceive(port_index))) != nullptr)
        {
            return p;
        }
    } while (TIMER_CNT(TIMER5) < time_out);

    return nullptr;
}

// Explicit template instantiations
template void Dmx::SetSendData<dmx::SendStyle::kDirect>(const uint32_t, const uint8_t*, uint32_t);
template void Dmx::SetSendData<dmx::SendStyle::kSync>(const uint32_t, const uint8_t*, uint32_t);

template void Dmx::SetSendDataWithoutSC<dmx::SendStyle::kDirect>(const uint32_t, const uint8_t*, uint32_t);
template void Dmx::SetSendDataWithoutSC<dmx::SendStyle::kSync>(const uint32_t, const uint8_t*, uint32_t);

template void Dmx::SetSendDataInternal<0, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<0, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<0, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<0, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);

#if DMX_MAX_PORTS >= 2
template void Dmx::SetSendDataInternal<1, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<1, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<1, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<1, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
#endif

#if DMX_MAX_PORTS >= 3
template void Dmx::SetSendDataInternal<2, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<2, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<2, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<2, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
#endif

#if DMX_MAX_PORTS >= 4
template void Dmx::SetSendDataInternal<3, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<3, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<3, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<3, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
#endif

#if DMX_MAX_PORTS >= 5
template void Dmx::SetSendDataInternal<4, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<4, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<4, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<4, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
#endif

#if DMX_MAX_PORTS >= 6
template void Dmx::SetSendDataInternal<5, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<5, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<5, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<5, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
#endif

#if DMX_MAX_PORTS >= 7
template void Dmx::SetSendDataInternal<6, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<6, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<6, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<6, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
#endif

#if DMX_MAX_PORTS == 8
template void Dmx::SetSendDataInternal<7, true, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<7, true, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<7, false, dmx::SendStyle::kDirect>(const uint8_t*, uint32_t);
template void Dmx::SetSendDataInternal<7, false, dmx::SendStyle::kSync>(const uint8_t*, uint32_t);
#endif

#pragma GCC push_options
#pragma GCC optimize("Os")

[[gnu::noinline]]
void Dmx::SetDmxBreakTime(uint32_t break_time)
{
    s_dmx_transmit.break_time = std::max(dmx::transmit::kBreakTimeMin, break_time);
    SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
}

[[gnu::noinline]]
uint32_t Dmx::GetDmxBreakTime() const
{
    return s_dmx_transmit.break_time;
}

[[gnu::noinline]]
void Dmx::SetDmxMabTime(uint32_t mab_time)
{
    s_dmx_transmit.mab_time = std::max(dmx::transmit::kMabTimeMin, mab_time);
    SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
}

[[gnu::noinline]]
uint32_t Dmx::GetDmxMabTime() const
{
    return s_dmx_transmit.mab_time;
}

[[gnu::noinline]]
void Dmx::SetDmxPeriodTime(uint32_t period)
{
    m_nDmxTransmitPeriodRequested = period;

    auto length_max = s_TxBuffer[0].dmx.data[0].length;

    for (uint32_t port_index = 1; port_index < dmx::config::max::PORTS; port_index++)
    {
        const auto kLength = s_TxBuffer[port_index].dmx.data[0].length;
        if (kLength > length_max)
        {
            length_max = kLength;
        }
    }

    auto package_length_micro_seconds = s_dmx_transmit.break_time + s_dmx_transmit.mab_time + (length_max * 44U);

    // The GD32F4xx/GD32H7XX Timer 1 has a 32-bit counter
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
    if (package_length_micro_seconds > (static_cast<uint16_t>(~0) - 44U))
    {
        s_dmx_transmit.break_time = std::min(dmx::transmit::kBreakTimeTypical, s_dmx_transmit.break_time);
        s_dmx_transmit.mab_time = dmx::transmit::kMabTimeMin;
        package_length_micro_seconds = s_dmx_transmit.break_time + s_dmx_transmit.mab_time + (length_max * 44U);
    }
#endif

    if (period != 0)
    {
        if (period < package_length_micro_seconds)
        {
            m_nDmxTransmitPeriod = std::max(dmx::transmit::kBreakToBreakTimeMin, package_length_micro_seconds + 44U);
        }
        else
        {
            m_nDmxTransmitPeriod = period;
        }
    }
    else
    {
        m_nDmxTransmitPeriod = std::max(dmx::transmit::kBreakToBreakTimeMin, package_length_micro_seconds + 44U);
    }

    s_dmx_transmit.inter_time = m_nDmxTransmitPeriod - package_length_micro_seconds;

    DEBUG_PRINTF("period=%u, nLengthMax=%u, m_nDmxTransmitPeriod=%u, nPackageLengthMicroSeconds=%u -> s_dmx_transmit.inter_time=%u", period, length_max, m_nDmxTransmitPeriod, package_length_micro_seconds, s_dmx_transmit.inter_time);
}

[[gnu::noinline]]
void Dmx::SetDmxSlots(uint16_t slots)
{
    if ((slots >= 2) && (slots <= dmx::kChannelsMax))
    {
        m_nDmxTransmitSlots = slots;

        for (uint32_t i = 0; i < dmx::config::max::PORTS; i++)
        {
            m_nDmxTransmissionLength[i] = std::min(m_nDmxTransmissionLength[i], static_cast<uint32_t>(slots));
        }

        SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
    }
}

[[gnu::noinline]]
void Dmx::SetOutputStyle(uint32_t port_index, dmx::OutputStyle output_style)
{
    DMX_CHECK_PORT_INDEX_VOID(port_index);

    s_TxBuffer[port_index].output_style = output_style;

    if (output_style == dmx::OutputStyle::kConstant)
    {
        if (!m_bHasContinuosOutput)
        {
            m_bHasContinuosOutput = true;
            if (m_dmxPortDirection[port_index] == dmx::PortDirection::kOutput)
            {
                StartDmxOutput(port_index);
            }
            return;
        }

        for (uint32_t index = 0; index < dmx::config::max::PORTS; index++)
        {
            if ((s_TxBuffer[index].output_style == dmx::OutputStyle::kConstant) && (m_dmxPortDirection[index] == dmx::PortDirection::kOutput))
            {
                StopData(index);
            }
        }

        for (uint32_t index = 0; index < dmx::config::max::PORTS; index++)
        {
            if ((s_TxBuffer[index].output_style == dmx::OutputStyle::kConstant) && (m_dmxPortDirection[index] == dmx::PortDirection::kOutput))
            {
                StartDmxOutput(index);
            }
        }
    }
    else
    {
        m_bHasContinuosOutput = false;
        for (uint32_t index = 0; index < dmx::config::max::PORTS; index++)
        {
            if (s_TxBuffer[index].output_style == dmx::OutputStyle::kConstant)
            {
                m_bHasContinuosOutput = true;
                return;
            }
        }
    }
}

[[gnu::noinline]]
dmx::OutputStyle Dmx::GetOutputStyle(uint32_t port_index) const
{
    DMX_CHECK_PORT_INDEX_RET(port_index, dmx::OutputStyle::kConstant);
    return s_TxBuffer[port_index].output_style;
}

static void UartDmxConfig(uint32_t usart_periph)
{
    Gd32UartBegin(usart_periph, 250000U, gd32::kUartBits8, gd32::kUartParityNone, gd32::kUartStop2Bits);
}

Dmx::Dmx()
{
    DEBUG_ENTRY();

    assert(s_this == nullptr);
    s_this = this;

    s_dmx_transmit.break_time = dmx::transmit::kBreakTimeTypical;
    s_dmx_transmit.mab_time = dmx::transmit::kMabTimeMin;
    s_dmx_transmit.inter_time = dmx::transmit::kPeriodDefault - s_dmx_transmit.break_time - s_dmx_transmit.mab_time - (dmx::kChannelsMax * 44) - 44;

    for (auto port_index = 0; port_index < DMX_MAX_PORTS; port_index++)
    {
#if defined(GPIO_INIT)
        gpio_init(kDirGpio[port_index].port, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, kDirGpio[port_index].pin);
#else
        gpio_mode_set(kDirGpio[port_index].port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, kDirGpio[port_index].pin);
        gpio_output_options_set(kDirGpio[port_index].port, GPIO_OTYPE_PP, GPIO_OSPEED, kDirGpio[port_index].pin);
#endif
        m_nDmxTransmissionLength[port_index] = dmx::kChannelsMax;
        sv_rx_buffer[port_index].state = dmx::TxRxState::kIdle;
        s_TxBuffer[port_index].state = dmx::TxRxState::kIdle;
        SetPortDirection(port_index, dmx::PortDirection::kInput, false);
        SetOutputStyle(port_index, dmx::OutputStyle::kDelta);
        ClearData(port_index);
    }

    UsartDmaConfig(); // DMX Transmit
#if defined(DMX_USE_USART0) || defined(DMX_USE_USART1) || defined(DMX_USE_USART2) || defined(DMX_USE_UART3)
    Timer1Config(); // DMX Transmit -> USART0, USART1, USART2, UART3
#endif
#if defined(DMX_USE_UART4) || defined(DMX_USE_USART5) || defined(DMX_USE_UART6) || defined(DMX_USE_UART7)
    Timer4Config(); // DMX Transmit -> UART4, USART5, UART6, UART7
#endif

#if defined(DMX_USE_USART0)
    UartDmxConfig(USART0);
    NVIC_SetPriority(USART0_IRQn, 0);
    NVIC_EnableIRQ(USART0_IRQn);
#endif
#if defined(DMX_USE_USART1)
    UartDmxConfig(USART1);
    NVIC_SetPriority(USART1_IRQn, 0);
    NVIC_EnableIRQ(USART1_IRQn);
#endif
#if defined(DMX_USE_USART2)
    UartDmxConfig(USART2);
    NVIC_SetPriority(USART2_IRQn, 0);
    NVIC_EnableIRQ(USART2_IRQn);
#endif
#if defined(DMX_USE_UART3)
    UartDmxConfig(UART3);
    NVIC_SetPriority(UART3_IRQn, 0);
    NVIC_EnableIRQ(UART3_IRQn);
#endif
#if defined(DMX_USE_UART4)
    UartDmxConfig(UART4);
    NVIC_SetPriority(UART4_IRQn, 0);
    NVIC_EnableIRQ(UART4_IRQn);
#endif
#if defined(DMX_USE_USART5)
    UartDmxConfig(USART5);
    NVIC_SetPriority(USART5_IRQn, 0);
    NVIC_EnableIRQ(USART5_IRQn);
#endif
#if defined(DMX_USE_UART6)
    UartDmxConfig(UART6);
    NVIC_SetPriority(UART6_IRQn, 0);
    NVIC_EnableIRQ(UART6_IRQn);
#endif
#if defined(DMX_USE_UART7)
    UartDmxConfig(UART7);
    NVIC_SetPriority(UART7_IRQn, 0);
    NVIC_EnableIRQ(UART7_IRQn);
#endif

    DEBUG_EXIT();
}