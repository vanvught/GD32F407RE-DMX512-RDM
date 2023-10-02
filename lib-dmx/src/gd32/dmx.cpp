/**
 * @file dmx.cpp
 *
 */
/* Copyright (C) 2021-2023 by Arjan van Vught mailto:info@gd32-dmx.org
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

#pragma GCC push_options
#pragma GCC optimize ("O3")
#pragma GCC optimize ("-funroll-loops")
#pragma GCC optimize ("-fprefetch-loop-arrays")

#if 0
# if defined (GD32F207RG)
#  define LOGIC_ANALYZER
#  if defined (NDEBUG)
#   undef NDEBUG
#  endif
# endif
#endif

extern "C" {
void console_error(const char *);
}

#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cassert>

#include "dmx.h"
#include "dmxconst.h"

#include "rdm.h"
#include "rdm_e120.h"

#include "gd32.h"
#include "gd32_uart.h"
#include "gd32/dmx_config.h"
#include "dmx_internal.h"

#include "logic_analyzer.h"

#include "debug.h"

#ifndef ALIGNED
# define ALIGNED __attribute__ ((aligned (4)))
#endif

#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
# define UNUSED
# else
# define UNUSED __attribute__((unused))
#endif

/**
 * https://www.gd32-dmx.org/memory.html
 */
#if defined (GD32F20X) ||defined (GD32F4XX)
# define SECTION_DMA_BUFFER					__attribute__ ((section (".dmx")))
#else
# define SECTION_DMA_BUFFER
#endif

namespace dmx {
enum class TxRxState {
	IDLE, BREAK, MAB, DMXDATA, DMXINTER, RDMDATA, CHECKSUMH, CHECKSUML, RDMDISC
};

enum class PortState {
	IDLE, TX, RX
};

struct TxDmxPacket {
	uint8_t data[dmx::buffer::SIZE];	// multiple of uint16_t
	uint16_t nLength;
};

struct TxData {
	struct TxDmxPacket dmx;
	OutputStyle outputStyle;
	volatile TxRxState State;
};

struct RxDmxPackets {
	uint32_t nPerSecond;
	uint32_t nCount;
	uint32_t nCountPrevious;
	uint16_t nTimerCounterPrevious;
};

struct RxRdmPackets {
	uint32_t nIndex;
	uint16_t nChecksum;					// This must be uint16_t
	uint16_t nDiscIndex;
};

struct RxData {
	uint8_t data[dmx::buffer::SIZE];	// multiple of uint16_t
	union {
		volatile RxRdmPackets Rdm;
		volatile Statistics Dmx;
	};
	volatile TxRxState State;
};

struct DirGpio {
	uint32_t nPort;
	uint32_t nPin;
};
}  // namespace dmx

using namespace dmx;

static constexpr DirGpio s_DirGpio[DMX_MAX_PORTS] = {
		{ config::DIR_PORT_0_GPIO_PORT, config::DIR_PORT_0_GPIO_PIN },
#if DMX_MAX_PORTS >= 2
		{ config::DIR_PORT_1_GPIO_PORT, config::DIR_PORT_1_GPIO_PIN },
#endif
#if DMX_MAX_PORTS >= 3
		{ config::DIR_PORT_2_GPIO_PORT, config::DIR_PORT_2_GPIO_PIN },
#endif
#if DMX_MAX_PORTS >= 4
		{ config::DIR_PORT_3_GPIO_PORT, config::DIR_PORT_3_GPIO_PIN },
#endif
#if DMX_MAX_PORTS >= 5
		{ config::DIR_PORT_4_GPIO_PORT, config::DIR_PORT_4_GPIO_PIN },
#endif
#if DMX_MAX_PORTS >= 6
		{ config::DIR_PORT_5_GPIO_PORT, config::DIR_PORT_5_GPIO_PIN },
#endif
#if DMX_MAX_PORTS >= 7
		{ config::DIR_PORT_6_GPIO_PORT, config::DIR_PORT_6_GPIO_PIN },
#endif
#if DMX_MAX_PORTS == 8
		{ config::DIR_PORT_7_GPIO_PORT, config::DIR_PORT_7_GPIO_PIN },
#endif
};

static volatile PortState sv_PortState[config::max::OUT] ALIGNED;

// DMX RX

static uint8_t UNUSED s_RxDmxPrevious[config::max::IN][dmx::buffer::SIZE] ALIGNED;
static volatile RxDmxPackets sv_nRxDmxPackets[config::max::IN] ALIGNED;

// RDM RX
volatile uint32_t gv_RdmDataReceiveEnd;

// DMX RDM RX

static RxData s_RxBuffer[config::max::IN] ALIGNED;

// DMX TX

static TxData s_TxBuffer[config::max::OUT] ALIGNED SECTION_DMA_BUFFER;

static uint32_t s_nDmxTransmitBreakTime { dmx::transmit::BREAK_TIME_TYPICAL };
static uint32_t s_nDmxTransmitMabTime { dmx::transmit::MAB_TIME_MIN };		///< MAB_TIME_MAX = 1000000U;
static uint32_t s_nDmxTransmitInterTime { dmx::transmit::PERIOD_DEFAULT - dmx::transmit::MAB_TIME_MIN - dmx::transmit::BREAK_TIME_TYPICAL - (dmx::max::CHANNELS * 44) - 44 };

static void irq_handler_dmx_rdm_input(const uint32_t uart, const uint32_t nPortIndex) {
	uint32_t nIndex;
	uint16_t nCounter;

	if (RESET != (USART_REG_VAL(uart, USART_FLAG_FERR) & BIT(USART_BIT_POS(USART_FLAG_FERR)))) {
		USART_REG_VAL(uart, USART_FLAG_FERR) &= ~BIT(USART_BIT_POS(USART_FLAG_FERR));
		static_cast<void>(GET_BITS(USART_DATA(uart), 0U, 8U));
		if (s_RxBuffer[nPortIndex].State == TxRxState::IDLE) {
			s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket = 0;
			s_RxBuffer[nPortIndex].State = TxRxState::BREAK;
		}
		return;
	}

	const auto data = static_cast<uint8_t>(GET_BITS(USART_DATA(uart), 0U, 8U));

	switch (s_RxBuffer[nPortIndex].State) {
	case TxRxState::IDLE:
		s_RxBuffer[nPortIndex].State = TxRxState::RDMDISC;
		s_RxBuffer[nPortIndex].data[0] = data;
		s_RxBuffer[nPortIndex].Rdm.nIndex = 1;
#if DMX_MAX_PORTS >= 5
		if (nPortIndex < 4) {
#endif
			sv_nRxDmxPackets[nPortIndex].nTimerCounterPrevious = static_cast<uint16_t>(TIMER_CNT(TIMER2));
#if DMX_MAX_PORTS >= 5
		} else {
			sv_nRxDmxPackets[nPortIndex].nTimerCounterPrevious = static_cast<uint16_t>(TIMER_CNT(TIMER3));
		}
#endif
		break;
	case TxRxState::BREAK:
		switch (data) {
		case START_CODE:
			s_RxBuffer[nPortIndex].data[0] = START_CODE;
			s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket = 1;
			sv_nRxDmxPackets[nPortIndex].nCount++;
			s_RxBuffer[nPortIndex].State = TxRxState::DMXDATA;
#if DMX_MAX_PORTS >= 5
			if (nPortIndex < 4) {
#endif
				sv_nRxDmxPackets[nPortIndex].nTimerCounterPrevious = static_cast<uint16_t>(TIMER_CNT(TIMER2));
#if DMX_MAX_PORTS >= 5
			} else {
				sv_nRxDmxPackets[nPortIndex].nTimerCounterPrevious = static_cast<uint16_t>(TIMER_CNT(TIMER3));
			}
#endif
			break;
		case E120_SC_RDM:
			s_RxBuffer[nPortIndex].data[0] = E120_SC_RDM;
			s_RxBuffer[nPortIndex].Rdm.nChecksum = E120_SC_RDM;
			s_RxBuffer[nPortIndex].Rdm.nIndex = 1;
			s_RxBuffer[nPortIndex].State = TxRxState::RDMDATA;
			break;
		default:
			s_RxBuffer[nPortIndex].State = TxRxState::IDLE;
			break;
		}
		break;
		case TxRxState::DMXDATA:
			nIndex = s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket;
			s_RxBuffer[nPortIndex].data[nIndex] = data;
			s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket++;

			if (s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket > dmx::max::CHANNELS) {
				s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket |= 0x8000;
				s_RxBuffer[nPortIndex].State = TxRxState::IDLE;
				break;
			}
#if DMX_MAX_PORTS >= 5
			if (nPortIndex < 4) {
#endif
				nCounter = static_cast<uint16_t>(TIMER_CNT(TIMER2));
#if DMX_MAX_PORTS >= 5
			} else {
				nCounter = static_cast<uint16_t>(TIMER_CNT(TIMER3));
			}
#endif
			{
				const uint16_t nDelta = nCounter - sv_nRxDmxPackets[nPortIndex].nTimerCounterPrevious;
				sv_nRxDmxPackets[nPortIndex].nTimerCounterPrevious = nCounter;
				const auto nPulse = static_cast<uint16_t>(nCounter + nDelta + 4);

				switch(nPortIndex){
				case 0:
					TIMER_CH0CV(TIMER2) = nPulse;
					break;
#if DMX_MAX_PORTS >= 2
				case 1:
					TIMER_CH1CV(TIMER2) = nPulse;
					break;
#endif
#if DMX_MAX_PORTS >= 3
				case 2:
					TIMER_CH2CV(TIMER2) = nPulse;
					break;
#endif
#if DMX_MAX_PORTS >= 4
				case 3:
					TIMER_CH3CV(TIMER2) = nPulse;
					break;
#endif
#if DMX_MAX_PORTS >= 5
				case 4:
					TIMER_CH0CV(TIMER3) = nPulse;
					break;
#endif
#if DMX_MAX_PORTS >= 6
				case 5:
					TIMER_CH1CV(TIMER3) = nPulse;
					break;
#endif
#if DMX_MAX_PORTS >= 7
				case 6:
					TIMER_CH2CV(TIMER3) = nPulse;
					break;
#endif
#if DMX_MAX_PORTS == 8
				case 7:
					TIMER_CH3CV(TIMER3) = nPulse;
					break;
#endif
				default:
					assert(0);
					__builtin_unreachable();
					break;
				}
			}
			break;
		case TxRxState::RDMDATA: {
			nIndex = s_RxBuffer[nPortIndex].Rdm.nIndex;
			s_RxBuffer[nPortIndex].data[nIndex] = data;
			s_RxBuffer[nPortIndex].Rdm.nIndex++;

			s_RxBuffer[nPortIndex].Rdm.nChecksum = static_cast<uint16_t>(s_RxBuffer[nPortIndex].Rdm.nChecksum + data);

			const auto *p = reinterpret_cast<struct TRdmMessage*>(&s_RxBuffer[nPortIndex].data[0]);

			nIndex = s_RxBuffer[nPortIndex].Rdm.nIndex;

			if ((nIndex >= 24) && (nIndex <= sizeof(struct TRdmMessage)) && (nIndex == p->message_length)) {
				s_RxBuffer[nPortIndex].State = TxRxState::CHECKSUMH;
			} else if (nIndex > sizeof(struct TRdmMessage)) {
				s_RxBuffer[nPortIndex].State = TxRxState::IDLE;
			}
		}
		break;
		case TxRxState::CHECKSUMH:
			nIndex = s_RxBuffer[nPortIndex].Rdm.nIndex;
			s_RxBuffer[nPortIndex].data[nIndex] = data;
			s_RxBuffer[nPortIndex].Rdm.nIndex++;
			s_RxBuffer[nPortIndex].Rdm.nChecksum = static_cast<uint16_t>(s_RxBuffer[nPortIndex].Rdm.nChecksum - static_cast<uint16_t>(data << 8));
			s_RxBuffer[nPortIndex].State = TxRxState::CHECKSUML;
			break;
		case TxRxState::CHECKSUML: {
			nIndex = s_RxBuffer[nPortIndex].Rdm.nIndex;
			s_RxBuffer[nPortIndex].data[nIndex] = data;
			s_RxBuffer[nPortIndex].Rdm.nIndex++;
			s_RxBuffer[nPortIndex].Rdm.nChecksum = static_cast<uint16_t>(s_RxBuffer[nPortIndex].Rdm.nChecksum - data);

			const auto *p = reinterpret_cast<struct TRdmMessage *>(&s_RxBuffer[nPortIndex].data[0]);

			if (!((s_RxBuffer[nPortIndex].Rdm.nChecksum == 0) && (p->sub_start_code == E120_SC_SUB_MESSAGE))) {
				s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket= 0; // This is correct.
			} else {
				s_RxBuffer[nPortIndex].Rdm.nIndex |= 0x4000;
				gv_RdmDataReceiveEnd = DWT->CYCCNT;
			}

			s_RxBuffer[nPortIndex].State = TxRxState::IDLE;
		}
		break;
		case TxRxState::RDMDISC:
			nIndex = s_RxBuffer[nPortIndex].Rdm.nIndex;

			if (nIndex < 24) {
				s_RxBuffer[nPortIndex].data[nIndex] = data;
				s_RxBuffer[nPortIndex].Rdm.nIndex++;
			}

#if DMX_MAX_PORTS >= 5
			if (nPortIndex < 4) {
#endif
				nCounter = static_cast<uint16_t>(TIMER_CNT(TIMER2));
#if DMX_MAX_PORTS >= 5
			} else {
				nCounter = static_cast<uint16_t>(TIMER_CNT(TIMER3));
			}
#endif
			{
				const uint16_t nDelta = nCounter - sv_nRxDmxPackets[nPortIndex].nTimerCounterPrevious;
				sv_nRxDmxPackets[nPortIndex].nTimerCounterPrevious = nCounter;
				const auto nPulse = static_cast<uint16_t>(nCounter + nDelta + 4);

				switch(nPortIndex){
				case 0:
					TIMER_CH0CV(TIMER2) = nPulse;
					break;
#if DMX_MAX_PORTS >= 2
				case 1:
					TIMER_CH1CV(TIMER2) = nPulse;
					break;
#endif
#if DMX_MAX_PORTS >= 3
				case 2:
					TIMER_CH2CV(TIMER2) = nPulse;
					break;
#endif
#if DMX_MAX_PORTS >= 4
				case 3:
					TIMER_CH3CV(TIMER2) = nPulse;
					break;
#endif
#if DMX_MAX_PORTS >= 5
				case 4:
					TIMER_CH0CV(TIMER3) = nPulse;
					break;
#endif
#if DMX_MAX_PORTS >= 6
				case 5:
					TIMER_CH1CV(TIMER3) = nPulse;
					break;
#endif
#if DMX_MAX_PORTS >= 7
				case 6:
					TIMER_CH2CV(TIMER3) = nPulse;
					break;
#endif
#if DMX_MAX_PORTS == 8
				case 7:
					TIMER_CH3CV(TIMER3) = nPulse;
					break;
#endif
				default:
					assert(0);
					__builtin_unreachable();
					break;
				}
			}
			break;
		default:
			s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket= 0; // This is correct.
			s_RxBuffer[nPortIndex].State = TxRxState::IDLE;
			break;
	}
}

extern "C" {
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
#if defined (DMX_USE_USART0)
void USART0_IRQHandler(void) {
	irq_handler_dmx_rdm_input(USART0, config::USART0_PORT);
}
#endif
#if defined (DMX_USE_USART1)
void USART1_IRQHandler(void) {
	irq_handler_dmx_rdm_input(USART1, config::USART1_PORT);
}
#endif
#if defined (DMX_USE_USART2)
void USART2_IRQHandler(void) {
	irq_handler_dmx_rdm_input(USART2, config::USART2_PORT);
}
#endif
#if defined (DMX_USE_UART3)
void UART3_IRQHandler(void) {
	irq_handler_dmx_rdm_input(UART3, config::UART3_PORT);
}
#endif
#if defined (DMX_USE_UART4)
void UART4_IRQHandler(void) {
	irq_handler_dmx_rdm_input(UART4, config::UART4_PORT);
}
#endif
#if defined (DMX_USE_USART5)
void USART5_IRQHandler(void) {
	irq_handler_dmx_rdm_input(USART5, config::USART5_PORT);
}
#endif
#if defined (DMX_USE_UART6)
void UART6_IRQHandler(void) {
	irq_handler_dmx_rdm_input(UART6, config::UART6_PORT);
}
#endif
#if defined (DMX_USE_UART7)
void UART7_IRQHandler(void) {
	irq_handler_dmx_rdm_input(UART7, config::UART7_PORT);
}
#endif
#endif
}

static void timer1_config() {
	rcu_periph_clock_enable(RCU_TIMER1);
	timer_deinit(TIMER1);

	timer_parameter_struct timer_initpara;

	timer_initpara.prescaler = TIMER_PSC_1MHZ;
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = ~0;
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(TIMER1, &timer_initpara);

	timer_flag_clear(TIMER1, ~0);
	timer_interrupt_flag_clear(TIMER1, ~0);

#if defined (DMX_USE_USART0)
	timer_channel_output_mode_config(TIMER1, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0, ~0);
#endif

#if defined (DMX_USE_USART1)
	timer_channel_output_mode_config(TIMER1, TIMER_CH_1, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_1, ~0);
#endif

#if defined (DMX_USE_USART2)
	timer_channel_output_mode_config(TIMER1, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_2, ~0);
#endif

#if defined (DMX_USE_UART3)
	timer_channel_output_mode_config(TIMER1, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_3, ~0);
#endif

	NVIC_SetPriority(TIMER1_IRQn, 0);
	NVIC_EnableIRQ(TIMER1_IRQn);

	timer_enable(TIMER1);
}

static void timer4_config() {
	rcu_periph_clock_enable(RCU_TIMER4);
	timer_deinit(TIMER4);

	timer_parameter_struct timer_initpara;

	timer_initpara.prescaler = TIMER_PSC_1MHZ;
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = ~0;
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(TIMER4, &timer_initpara);

	timer_flag_clear(TIMER4, ~0);
	timer_interrupt_flag_clear(TIMER4, ~0);

#if defined (DMX_USE_UART4)
	timer_channel_output_mode_config(TIMER4, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0, ~0);
#endif

#if defined (DMX_USE_USART5)
	timer_channel_output_mode_config(TIMER4, TIMER_CH_1, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_1, ~0);
#endif

#if defined (DMX_USE_UART6)
	timer_channel_output_mode_config(TIMER4, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2, ~0);
#endif

#if defined (DMX_USE_UART7)
	timer_channel_output_mode_config(TIMER4, TIMER_CH_3, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3, ~0);
#endif

	NVIC_SetPriority(TIMER4_IRQn, 0);
	NVIC_EnableIRQ(TIMER4_IRQn);

	timer_enable(TIMER4);
}

#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
static void timer2_config() {
	rcu_periph_clock_enable(RCU_TIMER2);
	timer_deinit(TIMER2);

	timer_parameter_struct timer_initpara;

	timer_initpara.prescaler = TIMER_PSC_1MHZ;
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = static_cast<uint32_t>(~0);
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(TIMER2, &timer_initpara);

	timer_flag_clear(TIMER2, ~0);
	timer_interrupt_flag_clear(TIMER2, ~0);

	timer_channel_output_mode_config(TIMER2, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_mode_config(TIMER2, TIMER_CH_1, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_mode_config(TIMER2, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_mode_config(TIMER2, TIMER_CH_3, TIMER_OC_MODE_ACTIVE);

	NVIC_SetPriority(TIMER2_IRQn, 1);
	NVIC_EnableIRQ(TIMER2_IRQn);

	timer_enable(TIMER2);
}

static void timer3_config() {
#if DMX_MAX_PORTS >= 5
	rcu_periph_clock_enable(RCU_TIMER3);
	timer_deinit(TIMER3);

	timer_parameter_struct timer_initpara;

	timer_initpara.prescaler = TIMER_PSC_1MHZ;
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = static_cast<uint32_t>(~0);
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(TIMER3, &timer_initpara);

	timer_flag_clear(TIMER3, ~0);
	timer_interrupt_flag_clear(TIMER3, ~0);

	timer_channel_output_mode_config(TIMER3, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_mode_config(TIMER3, TIMER_CH_1, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_mode_config(TIMER3, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_mode_config(TIMER3, TIMER_CH_3, TIMER_OC_MODE_ACTIVE);

	NVIC_SetPriority(TIMER3_IRQn, 1);
	NVIC_EnableIRQ(TIMER3_IRQn);

	timer_enable(TIMER3);
#endif
}

static void timer6_config() {
	rcu_periph_clock_enable(RCU_TIMER6);
	timer_deinit(TIMER6);

	timer_parameter_struct timer_initpara;
	timer_initpara.prescaler = TIMER_PSC_10KHZ;
	timer_initpara.period = 10000;		// 1 second
	timer_init(TIMER6, &timer_initpara);

	timer_flag_clear(TIMER6, ~0);
	timer_interrupt_flag_clear(TIMER6, ~0);

	timer_interrupt_enable(TIMER6, TIMER_INT_UP);

	NVIC_SetPriority(TIMER6_IRQn, 0);
	NVIC_EnableIRQ(TIMER6_IRQn);

	timer_enable(TIMER6);
}
#endif

static void usart_dma_config(void) {
	DMA_PARAMETER_STRUCT dma_init_struct;
	rcu_periph_clock_enable(RCU_DMA0);
	rcu_periph_clock_enable(RCU_DMA1);
	/*
	 * USART 0
	 */
#if defined (DMX_USE_USART0)
	dma_deinit(USART0_DMA, USART0_TX_DMA_CH);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if !defined (GD32F4XX)
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
	dma_init_struct.periph_addr = USART0 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if !defined (GD32F4XX)
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(USART0_DMA, USART0_TX_DMA_CH, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(USART0_DMA, USART0_TX_DMA_CH);
	dma_memory_to_memory_disable(USART0_DMA, USART0_TX_DMA_CH);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(USART0_DMA, USART0_TX_DMA_CH, USART0_TX_DMA_SUBPERIx);
# endif
	dma_interrupt_disable(USART0_DMA, USART0_TX_DMA_CH, DMA_INTERRUPT_DISABLE);
# if !defined (GD32F4XX)
	NVIC_SetPriority(DMA0_Channel3_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel3_IRQn);
# else
	NVIC_SetPriority(DMA1_Channel7_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel7_IRQn);
# endif
#endif
	/*
	 * USART 1
	 */
#if defined (DMX_USE_USART1)
	dma_deinit(USART1_DMA, USART1_TX_DMA_CH);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if !defined (GD32F4XX)
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
	dma_init_struct.periph_addr = USART1 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if !defined (GD32F4XX)
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(USART1_DMA, USART1_TX_DMA_CH, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(USART1_DMA, USART1_TX_DMA_CH);
	dma_memory_to_memory_disable(USART1_DMA, USART1_TX_DMA_CH);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(USART1_DMA, USART1_TX_DMA_CH, USART1_TX_DMA_SUBPERIx);
# endif
	dma_interrupt_disable(USART1_DMA, USART1_TX_DMA_CH, DMA_INTERRUPT_DISABLE);
	NVIC_SetPriority(DMA0_Channel6_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel6_IRQn);
#endif
	/*
	 * USART 2
	 */
#if defined (DMX_USE_USART2)
	dma_deinit(USART2_DMA, USART2_TX_DMA_CH);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
# if !defined (GD32F4XX)
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
# endif
	dma_init_struct.periph_addr = USART2 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
# if !defined (GD32F4XX)
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
# else
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
# endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(USART2_DMA, USART2_TX_DMA_CH, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(USART2_DMA, USART2_TX_DMA_CH);
	dma_memory_to_memory_disable(USART2_DMA, USART2_TX_DMA_CH);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(USART2_DMA, USART2_TX_DMA_CH, USART2_TX_DMA_SUBPERIx);
# endif
	dma_interrupt_disable(USART2_DMA, USART2_TX_DMA_CH, DMA_INTERRUPT_DISABLE);
	NVIC_SetPriority(DMA0_Channel1_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel1_IRQn);
#endif
	/*
	 * UART 3
	 */
#if defined (DMX_USE_UART3)
	dma_deinit(DMA1, DMA_CH4);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if !defined (GD32F4XX)
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
	dma_init_struct.periph_addr = UART3 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if !defined (GD32F4XX)
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(DMA1, DMA_CH4, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(DMA1, DMA_CH4);
	dma_memory_to_memory_disable(DMA1, DMA_CH4);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(UART3_DMA, UART3_TX_DMA_CH, UART3_TX_DMA_SUBPERIx);
# endif
	dma_interrupt_disable(USART0_DMA, USART0_TX_DMA_CH, DMA_INTERRUPT_DISABLE);
# if !defined (GD32F4XX)
	NVIC_SetPriority(DMA1_Channel4_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel4_IRQn);
# else
	NVIC_SetPriority(DMA0_Channel4_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel4_IRQn);
# endif
#endif
	/*
	 * UART 4
	 */
#if defined (DMX_USE_UART4)
	dma_deinit(UART4_DMA, UART4_TX_DMA_CH);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if !defined (GD32F4XX)
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
	dma_init_struct.periph_addr = UART4 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if !defined (GD32F4XX)
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(UART4_DMA, UART4_TX_DMA_CH, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(UART4_DMA, UART4_TX_DMA_CH);
	dma_memory_to_memory_disable(UART4_DMA, UART4_TX_DMA_CH);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(UART4_DMA, UART4_TX_DMA_CH, UART4_TX_DMA_SUBPERIx);
# endif
	dma_interrupt_disable(UART4_DMA, UART4_TX_DMA_CH, DMA_INTERRUPT_DISABLE);
# if defined (GD32F20X)
	NVIC_SetPriority(DMA1_Channel3_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel3_IRQn);
# else
	NVIC_SetPriority(DMA0_Channel7_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel7_IRQn);
# endif
#endif
	/*
	 * USART 5
	 */
#if defined (DMX_USE_USART5)
	dma_deinit(USART5_DMA, USART5_TX_DMA_CH);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if !defined (GD32F4XX)
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
	dma_init_struct.periph_addr = USART5 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if !defined (GD32F4XX)
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(USART5_DMA, USART5_TX_DMA_CH, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(USART5_DMA, USART5_TX_DMA_CH);
	dma_memory_to_memory_disable(USART5_DMA, USART5_TX_DMA_CH);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(USART5_DMA, USART5_TX_DMA_CH, USART5_TX_DMA_SUBPERIx);
# endif
	dma_interrupt_disable(USART5_DMA, USART5_TX_DMA_CH, DMA_INTERRUPT_DISABLE);
	NVIC_SetPriority(DMA1_Channel6_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel6_IRQn);
#endif
	/*
	 * UART 6
	 */
#if defined (DMX_USE_UART6)
	dma_deinit(UART6_DMA, UART6_TX_DMA_CH);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if !defined (GD32F4XX)
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
	dma_init_struct.periph_addr = UART6 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if !defined (GD32F4XX)
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(UART6_DMA, UART6_TX_DMA_CH, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(UART6_DMA, UART6_TX_DMA_CH);
	dma_memory_to_memory_disable(UART6_DMA, UART6_TX_DMA_CH);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(UART6_DMA, UART6_TX_DMA_CH, UART6_TX_DMA_SUBPERIx);
# endif
	dma_interrupt_disable(UART6_DMA, UART4_TX_DMA_CH, DMA_INTERRUPT_DISABLE);
# if defined (GD32F20X)
	NVIC_SetPriority(DMA1_Channel4_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel4_IRQn);
# else
	NVIC_SetPriority(DMA0_Channel1_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel1_IRQn);
# endif
#endif
	/*
	 * UART 7
	 */
#if defined (DMX_USE_UART7)
	dma_deinit(UART7_DMA, UART7_TX_DMA_CH);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if !defined (GD32F4XX)
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif
	dma_init_struct.periph_addr = UART7 + 0x04U;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if !defined (GD32F4XX)
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(UART7_DMA, UART7_TX_DMA_CH, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(UART7_DMA, UART7_TX_DMA_CH);
	dma_memory_to_memory_disable(UART7_DMA, UART7_TX_DMA_CH);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(UART7_DMA, UART7_TX_DMA_CH, UART7_TX_DMA_SUBPERIx);
# endif
# if defined (GD32F20X)
	NVIC_SetPriority(DMA1_Channel3_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel3_IRQn);
# else
	NVIC_SetPriority(DMA0_Channel0_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel0_IRQn);
# endif
#endif
}

extern "C" {
void TIMER1_IRQHandler() {
	/*
	 * USART 0
	 */
#if defined (DMX_USE_USART0)
	if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0) {
		switch (s_TxBuffer[dmx::config::USART0_PORT].State) {
		case TxRxState::DMXINTER:
			_gpio_mode_output(USART0_GPIO_PORT, USART0_TX_PIN);
			GPIO_BC(USART0_GPIO_PORT) = USART0_TX_PIN;
			s_TxBuffer[dmx::config::USART0_PORT].State = TxRxState::BREAK;
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0, TIMER_CNT(TIMER1) + s_nDmxTransmitBreakTime);
			break;
		case TxRxState::BREAK:
			_gpio_mode_af(USART0_GPIO_PORT, USART0_TX_PIN, USART0);
			s_TxBuffer[dmx::config::USART0_PORT].State = TxRxState::MAB;
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0, TIMER_CNT(TIMER1) + s_nDmxTransmitMabTime);
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(USART0_DMA, USART0_TX_DMA_CH);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(USART0_DMA, USART0_TX_DMA_CH) = dmaCHCTL;
			dma_interrupt_flag_clear(USART0_DMA, USART0_TX_DMA_CH, DMA_INTF_FTFIF);
			const auto *p = &s_TxBuffer[dmx::config::USART0_PORT].dmx;
			DMA_CHMADDR(USART0_DMA, USART0_TX_DMA_CH) = (uint32_t) p->data;
			DMA_CHCNT(USART0_DMA, USART0_TX_DMA_CH) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(USART0_DMA, USART0_TX_DMA_CH) = dmaCHCTL;
			usart_dma_transmit_config(USART0, USART_DENT_ENABLE);
		}
			break;
		default:
			break;
		}

		timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_CH0);
		return;
	}
#endif
	/*
	 * USART 1
	 */
#if defined (DMX_USE_USART1)
	if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH1) == TIMER_INT_FLAG_CH1) {
		switch (s_TxBuffer[dmx::config::USART1_PORT].State) {
		case TxRxState::DMXINTER:
			_gpio_mode_output(USART1_GPIO_PORT, USART1_TX_PIN);
			GPIO_BC(USART1_GPIO_PORT) = USART1_TX_PIN;
			s_TxBuffer[dmx::config::USART1_PORT].State = TxRxState::BREAK;
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_1, TIMER_CNT(TIMER1) + s_nDmxTransmitBreakTime);
			break;
		case TxRxState::BREAK:
			_gpio_mode_af(USART1_GPIO_PORT, USART1_TX_PIN, USART1);
			s_TxBuffer[dmx::config::USART1_PORT].State = TxRxState::MAB;
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_1, TIMER_CNT(TIMER1) + s_nDmxTransmitMabTime);
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(USART1_DMA, USART1_TX_DMA_CH);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(USART1_DMA, USART1_TX_DMA_CH) = dmaCHCTL;
			dma_interrupt_flag_clear(USART1_DMA, USART1_TX_DMA_CH, DMA_INTF_FTFIF);
			const auto *p = &s_TxBuffer[dmx::config::USART1_PORT].dmx;
			DMA_CHMADDR(USART1_DMA, USART1_TX_DMA_CH) = (uint32_t) p->data;
			DMA_CHCNT(USART1_DMA, USART1_TX_DMA_CH) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(USART1_DMA, USART1_TX_DMA_CH) = dmaCHCTL;
			usart_dma_transmit_config(USART1, USART_DENT_ENABLE);
		}
			break;
		default:
			break;
		}

		timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_CH1);
		return;
	}
#endif
	/*
	 * USART 2
	 */
#if defined (DMX_USE_USART2)
	if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH2) == TIMER_INT_FLAG_CH2) {
		switch (s_TxBuffer[dmx::config::USART2_PORT].State) {
		case TxRxState::DMXINTER:
			_gpio_mode_output(USART2_GPIO_PORT, USART2_TX_PIN);
			GPIO_BC(USART2_GPIO_PORT) = USART2_TX_PIN;
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_2, TIMER_CNT(TIMER1) + s_nDmxTransmitBreakTime);
			s_TxBuffer[dmx::config::USART2_PORT].State = TxRxState::BREAK;
			break;
		case TxRxState::BREAK:
			_gpio_mode_af(USART2_GPIO_PORT, USART2_TX_PIN, USART2);
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_2, TIMER_CNT(TIMER1) + s_nDmxTransmitMabTime);
			s_TxBuffer[dmx::config::USART2_PORT].State = TxRxState::MAB;
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(USART2_DMA, USART2_TX_DMA_CH);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(USART2_DMA, USART2_TX_DMA_CH) = dmaCHCTL;
			const auto *p = &s_TxBuffer[dmx::config::USART2_PORT].dmx;
			DMA_CHMADDR(USART2_DMA, USART2_TX_DMA_CH) = (uint32_t) p->data;
			DMA_CHCNT(USART2_DMA, USART2_TX_DMA_CH) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(USART2_DMA, USART2_TX_DMA_CH) = dmaCHCTL;
			usart_dma_transmit_config(USART2, USART_DENT_ENABLE);
			s_TxBuffer[dmx::config::USART2_PORT].State = TxRxState::DMXDATA;
		}
			break;
		case TxRxState::DMXDATA:
			console_error("DMXDATA:USART2");
			break;
		default:
			break;
		}

		timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_CH2);
		return;
	}
#endif
	/*
	 * UART 3
	 */
#if defined (DMX_USE_UART3)
	if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH3) == TIMER_INT_FLAG_CH3) {
		switch (s_TxBuffer[dmx::config::UART3_PORT].State) {
		case TxRxState::DMXINTER:
			_gpio_mode_output(UART3_GPIO_PORT, UART3_TX_PIN);
			GPIO_BC(UART3_GPIO_PORT) = UART3_TX_PIN;
			s_TxBuffer[dmx::config::UART3_PORT].State = TxRxState::BREAK;
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_3, TIMER_CNT(TIMER1) + s_nDmxTransmitBreakTime);
			break;
		case TxRxState::BREAK:
			_gpio_mode_af(UART3_GPIO_PORT, UART3_TX_PIN, UART3);
			s_TxBuffer[dmx::config::UART3_PORT].State = TxRxState::MAB;
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_3, TIMER_CNT(TIMER1) + s_nDmxTransmitMabTime);
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(UART3_DMA, UART3_TX_DMA_CH);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(UART3_DMA, UART3_TX_DMA_CH) = dmaCHCTL;
			dma_interrupt_flag_clear(UART3_DMA, UART3_TX_DMA_CH, DMA_INTF_FTFIF);
			const auto *p = &s_TxBuffer[dmx::config::UART3_PORT].dmx;
			DMA_CHMADDR(UART3_DMA, UART3_TX_DMA_CH) = (uint32_t) p->data;
			DMA_CHCNT(UART3_DMA, UART3_TX_DMA_CH) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(UART3_DMA, UART3_TX_DMA_CH) = dmaCHCTL;
			usart_dma_transmit_config(UART3, USART_DENT_ENABLE);
		}
			break;
		default:
			break;
		}

		timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_CH3);
		return;
	}
#endif
}

void TIMER4_IRQHandler() {
	/*
	 * UART 4
	 */
#if defined (DMX_USE_UART4)
	if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0) {
		switch (s_TxBuffer[dmx::config::UART4_PORT].State) {
		case TxRxState::DMXINTER:
			_gpio_mode_output(UART4_GPIO_TX_PORT, UART4_TX_PIN);
			GPIO_BC(UART4_GPIO_TX_PORT) = UART4_TX_PIN;
			s_TxBuffer[dmx::config::UART4_PORT].State = TxRxState::BREAK;
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0, TIMER_CNT(TIMER4) + s_nDmxTransmitBreakTime);
			break;
		case TxRxState::BREAK:
			_gpio_mode_af(UART4_GPIO_TX_PORT, UART4_TX_PIN, UART4);
			s_TxBuffer[dmx::config::UART4_PORT].State = TxRxState::MAB;
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0, TIMER_CNT(TIMER4) + s_nDmxTransmitMabTime);
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(UART4_DMA, UART4_TX_DMA_CH);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(UART4_DMA, UART4_TX_DMA_CH) = dmaCHCTL;
			const auto *p = &s_TxBuffer[dmx::config::UART4_PORT].dmx;
			DMA_CHMADDR(UART4_DMA, UART4_TX_DMA_CH) = (uint32_t) p->data;
			DMA_CHCNT(UART4_DMA, UART4_TX_DMA_CH) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(UART4_DMA, UART4_TX_DMA_CH) = dmaCHCTL;
			usart_dma_transmit_config(UART4, USART_DENT_ENABLE);
		}
			break;
		default:
			break;
		}

		timer_interrupt_flag_clear(TIMER4, TIMER_INT_FLAG_CH0);
		return;
	}
#endif
	/*
	 * USART 5
	 */
#if defined (DMX_USE_USART5)
	if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH1) == TIMER_INT_FLAG_CH1) {
		switch (s_TxBuffer[dmx::config::USART5_PORT].State) {
		case TxRxState::DMXINTER:
			_gpio_mode_output(USART5_GPIO_PORT, USART5_TX_PIN);
			GPIO_BC(USART5_GPIO_PORT) = USART5_TX_PIN;
			s_TxBuffer[dmx::config::USART5_PORT].State = TxRxState::BREAK;
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_1, TIMER_CNT(TIMER4) + s_nDmxTransmitBreakTime);
			break;
		case TxRxState::BREAK:
			_gpio_mode_af(USART5_GPIO_PORT, USART5_TX_PIN, USART5);
			s_TxBuffer[dmx::config::USART5_PORT].State = TxRxState::MAB;
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_1, TIMER_CNT(TIMER4) + s_nDmxTransmitMabTime);
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(USART5_DMA, USART5_TX_DMA_CH);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(USART5_DMA, USART5_TX_DMA_CH) = dmaCHCTL;
			const auto *p = &s_TxBuffer[dmx::config::USART5_PORT].dmx;
			DMA_CHMADDR(USART5_DMA, USART5_TX_DMA_CH) = (uint32_t) p->data;
			DMA_CHCNT(USART5_DMA, USART5_TX_DMA_CH) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(USART5_DMA, USART5_TX_DMA_CH) = dmaCHCTL;
			usart_dma_transmit_config(USART5, USART_DENT_ENABLE);
		}
			break;
		default:
			break;
		}

		timer_interrupt_flag_clear(TIMER4, TIMER_INT_FLAG_CH1);
		return;
	}
#endif
	/*
	 * UART 6
	 */
#if defined (DMX_USE_UART6)
	if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH2) == TIMER_INT_FLAG_CH2) {
		switch (s_TxBuffer[dmx::config::UART6_PORT].State) {
		case TxRxState::DMXINTER:
			_gpio_mode_output(UART6_GPIO_PORT, UART6_TX_PIN);
			GPIO_BC(UART6_GPIO_PORT) = UART6_TX_PIN;
			s_TxBuffer[dmx::config::UART6_PORT].State = TxRxState::BREAK;
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2, TIMER_CNT(TIMER4) + s_nDmxTransmitBreakTime);
			break;
		case TxRxState::BREAK:
			_gpio_mode_af(UART6_GPIO_PORT, UART6_TX_PIN, UART6);
			s_TxBuffer[dmx::config::UART6_PORT].State = TxRxState::MAB;
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2, TIMER_CNT(TIMER4) + s_nDmxTransmitMabTime);
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(UART6_DMA, UART6_TX_DMA_CH);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(UART6_DMA, UART6_TX_DMA_CH)= dmaCHCTL;
			dma_interrupt_flag_clear(UART6_DMA, UART6_TX_DMA_CH, DMA_INTF_FTFIF);
			const auto *p = &s_TxBuffer[dmx::config::UART6_PORT].dmx;
			DMA_CHMADDR(UART6_DMA, UART6_TX_DMA_CH) = (uint32_t) p->data;
			DMA_CHCNT(UART6_DMA, UART6_TX_DMA_CH) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(UART6_DMA, UART6_TX_DMA_CH)= dmaCHCTL;
			usart_dma_transmit_config(UART6, USART_DENT_ENABLE);
		}
			break;
		default:
			break;
		}

		timer_interrupt_flag_clear(TIMER4, TIMER_INT_FLAG_CH2);
		return;
	}
#endif
	/*
	 * UART 7
	 */
#if defined (DMX_USE_UART7)
	if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH3) == TIMER_INT_FLAG_CH3) {
		switch (s_TxBuffer[dmx::config::UART7_PORT].State) {
		case TxRxState::DMXINTER:
			_gpio_mode_output(UART7_GPIO_PORT, UART7_TX_PIN);
			GPIO_BC(UART7_GPIO_PORT) = UART7_TX_PIN;
			s_TxBuffer[dmx::config::UART7_PORT].State = TxRxState::BREAK;
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2, TIMER_CNT(TIMER4) + s_nDmxTransmitBreakTime);
			break;
		case TxRxState::BREAK:
			_gpio_mode_af(UART7_GPIO_PORT, UART7_TX_PIN, UART7);
			s_TxBuffer[dmx::config::UART7_PORT].State = TxRxState::MAB;
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2, TIMER_CNT(TIMER4) + s_nDmxTransmitMabTime);
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(UART7_DMA, UART7_TX_DMA_CH);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(UART7_DMA, UART7_TX_DMA_CH) = dmaCHCTL;
			dma_interrupt_flag_clear(UART7_DMA, UART7_TX_DMA_CH, DMA_INTF_FTFIF);
			const auto *p = &s_TxBuffer[dmx::config::UART7_PORT].dmx;
			DMA_CHMADDR(UART7_DMA, UART7_TX_DMA_CH) = (uint32_t) p->data;
			DMA_CHCNT(UART7_DMA, UART7_TX_DMA_CH) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			usart_dma_transmit_config(UART7, USART_DENT_ENABLE);
		}
			break;
		default:
			break;
		}

		timer_interrupt_flag_clear(TIMER4, TIMER_INT_FLAG_CH3);
		return;
	}
#endif
}

#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
void TIMER2_IRQHandler() {
	const auto nIntFlag = TIMER_INTF(TIMER2);

	if ((nIntFlag & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0) {
		if (s_RxBuffer[0].State == TxRxState::DMXDATA) {
			s_RxBuffer[0].State = TxRxState::IDLE;
			s_RxBuffer[0].Dmx.nSlotsInPacket |= 0x8000;
		} else if (s_RxBuffer[0].State == TxRxState::RDMDISC) {
			s_RxBuffer[0].State = TxRxState::IDLE;
			s_RxBuffer[0].Dmx.nSlotsInPacket |= 0x4000;
		}
	}
#if DMX_MAX_PORTS >= 2
	else if ((nIntFlag & TIMER_INT_FLAG_CH1) == TIMER_INT_FLAG_CH1) {
		if (s_RxBuffer[1].State == TxRxState::DMXDATA) {
			s_RxBuffer[1].State = TxRxState::IDLE;
			s_RxBuffer[1].Dmx.nSlotsInPacket |= 0x8000;
		} else if (s_RxBuffer[1].State == TxRxState::RDMDISC) {
			s_RxBuffer[1].State = TxRxState::IDLE;
			s_RxBuffer[1].Dmx.nSlotsInPacket |= 0x4000;
		}
	}
#endif
#if DMX_MAX_PORTS >= 3
	else if ((nIntFlag & TIMER_INT_FLAG_CH2) == TIMER_INT_FLAG_CH2) {
		if (s_RxBuffer[2].State == TxRxState::DMXDATA) {
			s_RxBuffer[2].State = TxRxState::IDLE;
			s_RxBuffer[2].Dmx.nSlotsInPacket |= 0x8000;
		} else if (s_RxBuffer[2].State == TxRxState::RDMDISC) {
			s_RxBuffer[2].State = TxRxState::IDLE;
			s_RxBuffer[2].Dmx.nSlotsInPacket |= 0x4000;
		}
	}
#endif
#if DMX_MAX_PORTS >= 4
	else if ((nIntFlag & TIMER_INT_FLAG_CH3) == TIMER_INT_FLAG_CH3) {
		if (s_RxBuffer[3].State == TxRxState::DMXDATA) {
			s_RxBuffer[3].State = TxRxState::IDLE;
			s_RxBuffer[3].Dmx.nSlotsInPacket |= 0x8000;
		} else if (s_RxBuffer[3].State == TxRxState::RDMDISC) {
			s_RxBuffer[3].State = TxRxState::IDLE;
			s_RxBuffer[3].Dmx.nSlotsInPacket |= 0x4000;
		}
	}
#endif
	timer_interrupt_flag_clear(TIMER2, nIntFlag);
}

void TIMER3_IRQHandler() {
	const auto nIntFlag = TIMER_INTF(TIMER3);
#if DMX_MAX_PORTS >= 5
	if ((nIntFlag & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0) {
		if (s_RxBuffer[4].State == TxRxState::DMXDATA) {
			s_RxBuffer[4].State = TxRxState::IDLE;
			s_RxBuffer[4].Dmx.nSlotsInPacket |= 0x8000;
		} else if (s_RxBuffer[4].State == TxRxState::RDMDISC) {
			s_RxBuffer[4].State = TxRxState::IDLE;
			s_RxBuffer[4].Dmx.nSlotsInPacket |= 0x4000;
		}
	}
# if DMX_MAX_PORTS >= 6
	else if ((nIntFlag & TIMER_INT_FLAG_CH1) == TIMER_INT_FLAG_CH1) {
		if (s_RxBuffer[5].State == TxRxState::DMXDATA) {
			s_RxBuffer[5].State = TxRxState::IDLE;
			s_RxBuffer[5].Dmx.nSlotsInPacket |= 0x8000;
		} else if (s_RxBuffer[5].State == TxRxState::RDMDISC) {
			s_RxBuffer[5].State = TxRxState::IDLE;
			s_RxBuffer[5].Dmx.nSlotsInPacket |= 0x4000;
		}
	}
# endif
# if DMX_MAX_PORTS >= 7
	else if ((nIntFlag & TIMER_INT_FLAG_CH2) == TIMER_INT_FLAG_CH2) {
		if (s_RxBuffer[6].State == TxRxState::DMXDATA) {
			s_RxBuffer[6].State = TxRxState::IDLE;
			s_RxBuffer[6].Dmx.nSlotsInPacket |= 0x8000;
		} else if (s_RxBuffer[6].State == TxRxState::RDMDISC) {
			s_RxBuffer[6].State = TxRxState::IDLE;
			s_RxBuffer[6].Dmx.nSlotsInPacket |= 0x4000;
		}
	}
# endif
# if DMX_MAX_PORTS == 8
	else if ((nIntFlag & TIMER_INT_FLAG_CH3) == TIMER_INT_FLAG_CH3) {
		if (s_RxBuffer[7].State == TxRxState::DMXDATA) {
			s_RxBuffer[7].State = TxRxState::IDLE;
			s_RxBuffer[7].Dmx.nSlotsInPacket |= 0x8000;
		} else if (s_RxBuffer[7].State == TxRxState::RDMDISC) {
			s_RxBuffer[7].State = TxRxState::IDLE;
			s_RxBuffer[7].Dmx.nSlotsInPacket |= 0x4000;
		}
	}
# endif
#endif
	timer_interrupt_flag_clear(TIMER3, nIntFlag);
}

void TIMER6_IRQHandler() {
	for (auto i = 0; i < DMX_MAX_PORTS; i++) {
		sv_nRxDmxPackets[i].nPerSecond = sv_nRxDmxPackets[i].nCount - sv_nRxDmxPackets[i].nCountPrevious;
		sv_nRxDmxPackets[i].nCountPrevious = sv_nRxDmxPackets[i].nCount;
	}

	timer_interrupt_flag_clear(TIMER6, TIMER_INT_FLAG_UP);
}
#endif
}

#include <stdio.h>

extern "C" {
/*
 * USART 0
 */
#if defined (DMX_USE_USART0)
# if !defined (GD32F4XX)
void DMA0_Channel3_IRQHandler() {
	if (dma_interrupt_flag_get(DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_GET) == SET) {
		dma_interrupt_disable(DMA0, DMA_CH3, DMA_INTERRUPT_DISABLE);

		if (s_TxBuffer[dmx::config::USART0_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::USART0_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0 , TIMER_CNT(TIMER1) + s_nDmxTransmitInterTime);
			s_TxBuffer[dmx::config::USART0_PORT].State = TxRxState::DMXINTER;
		}
	}

	dma_interrupt_flag_clear(DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR);
}
# else
void DMA1_Channel7_IRQHandler() {
	if (dma_interrupt_flag_get(DMA1, DMA_CH7, DMA_INTERRUPT_FLAG_GET) == SET) {
		dma_interrupt_disable(DMA1, DMA_CH7, DMA_INTERRUPT_DISABLE);

		if (s_TxBuffer[dmx::config::USART0_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::USART0_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0 , TIMER_CNT(TIMER1) + s_nDmxTransmitInterTime);
			s_TxBuffer[dmx::config::USART0_PORT].State = TxRxState::DMXINTER;
		}
	}

	dma_interrupt_flag_clear(DMA1, DMA_CH7, DMA_INTERRUPT_FLAG_CLEAR);
}
# endif
#endif
/*
 * USART 1
 */
#if defined (DMX_USE_USART1)
void DMA0_Channel6_IRQHandler() {
	if (dma_interrupt_flag_get(DMA0, DMA_CH6, DMA_INTERRUPT_FLAG_GET) == SET) {
		dma_interrupt_disable(DMA0, DMA_CH6, DMA_INTERRUPT_DISABLE);

		if (s_TxBuffer[dmx::config::USART1_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::USART1_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_1 , TIMER_CNT(TIMER1) + s_nDmxTransmitInterTime);
			s_TxBuffer[dmx::config::USART1_PORT].State = TxRxState::DMXINTER;
		}
	}

	dma_interrupt_flag_clear(DMA0, DMA_CH6, DMA_INTERRUPT_FLAG_CLEAR);
}
#endif
/*
 * USART 2
 */
#if defined (DMX_USE_USART2)
# if !defined (GD32F4XX)
void DMA0_Channel1_IRQHandler() {
	if (dma_interrupt_flag_get(DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_GET) == SET) {
		dma_interrupt_disable(DMA0, DMA_CH1, DMA_INTERRUPT_DISABLE);

		usart_flag_clear(USART2, USART_FLAG_TC);

		if (s_TxBuffer[dmx::config::USART2_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::USART2_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_2 , TIMER_CNT(TIMER1) + s_nDmxTransmitInterTime);
			s_TxBuffer[dmx::config::USART2_PORT].State = TxRxState::DMXINTER;
		}
	}

	dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_CLEAR);
}
# else
void DMA0_Channel3_IRQHandler() {
	if (dma_interrupt_flag_get(DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_GET) == SET) {
		dma_interrupt_disable(DMA0, DMA_CH3, DMA_INTERRUPT_DISABLE);

		if (s_TxBuffer[dmx::config::USART2_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::USART2_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_2 , TIMER_CNT(TIMER1) + s_nDmxTransmitInterTime);
			s_TxBuffer[dmx::config::USART2_PORT].State = TxRxState::DMXINTER;
		}
	}

	dma_interrupt_flag_clear(DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR);
}
# endif
#endif
/*
 * UART 3
 */
#if defined (DMX_USE_UART3)
# if !defined (GD32F4XX)
void DMA1_Channel4_IRQHandler() {
	if (dma_interrupt_flag_get(DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_GET) == SET) {
		dma_interrupt_disable(DMA1, DMA_CH4, DMA_INTERRUPT_DISABLE);

		if (s_TxBuffer[dmx::config::UART3_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART3_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_3 , TIMER_CNT(TIMER1) + s_nDmxTransmitInterTime);
			s_TxBuffer[dmx::config::UART3_PORT].State = TxRxState::DMXINTER;
		}
	}

	dma_interrupt_flag_clear(DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_CLEAR);
}
# endif
# if defined (GD32F4XX)
void DMA0_Channel4_IRQHandler() {
	if (dma_interrupt_flag_get(DMA0, DMA_CH4, DMA_INTERRUPT_FLAG_GET) == SET) {
		dma_interrupt_disable(DMA0, DMA_CH4, DMA_INTERRUPT_DISABLE);

		if (s_TxBuffer[dmx::config::UART3_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART3_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_3 , TIMER_CNT(TIMER4) + s_nDmxTransmitInterTime);
			s_TxBuffer[dmx::config::UART3_PORT].State = TxRxState::DMXINTER;
		}
	}

	dma_interrupt_flag_clear(DMA0, DMA_CH4, DMA_INTERRUPT_FLAG_CLEAR);
}
# endif
#endif
/*
 * UART 4
 */
#if defined (DMX_USE_UART4)
# if defined (GD32F20X)
void DMA1_Channel3_IRQHandler() {
	if (dma_interrupt_flag_get(DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_GET) == SET) {
		dma_interrupt_disable(DMA1, DMA_CH3, DMA_INTERRUPT_DISABLE);

		if (s_TxBuffer[dmx::config::UART4_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART4_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0 , TIMER_CNT(TIMER4) + s_nDmxTransmitInterTime);
			s_TxBuffer[dmx::config::UART4_PORT].State = TxRxState::DMXINTER;
		}
	}

	dma_interrupt_flag_clear(DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR);
}
# endif
# if defined (GD32F4XX)
void DMA0_Channel7_IRQHandler() {
	if (dma_interrupt_flag_get(DMA0, DMA_CH7, DMA_INTERRUPT_FLAG_GET) == SET) {
		dma_interrupt_disable(DMA0, DMA_CH7, DMA_INTERRUPT_DISABLE);

		if (s_TxBuffer[dmx::config::UART4_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART4_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0 , TIMER_CNT(TIMER4) + s_nDmxTransmitInterTime);
			s_TxBuffer[dmx::config::UART4_PORT].State = TxRxState::DMXINTER;
		}
	}

	dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INTERRUPT_FLAG_CLEAR);
}
# endif
#endif
/*
 * USART 5
 */
#if defined (DMX_USE_USART5)
void DMA1_Channel6_IRQHandler() {
	if (dma_interrupt_flag_get(DMA1, DMA_CH6, DMA_INTERRUPT_FLAG_GET) == SET) {
		dma_interrupt_disable(DMA1, DMA_CH6, DMA_INTERRUPT_DISABLE);

		if (s_TxBuffer[dmx::config::USART5_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::USART5_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_1 , TIMER_CNT(TIMER4) + s_nDmxTransmitInterTime);
			s_TxBuffer[dmx::config::USART5_PORT].State = TxRxState::DMXINTER;
		}
	}

	dma_interrupt_flag_clear(DMA1, DMA_CH6, DMA_INTERRUPT_FLAG_CLEAR);
}
#endif
/*
 * UART 6
 */
#if defined (DMX_USE_UART6)
# if defined (GD32F20X)
void DMA1_Channel4_IRQHandler() {
	if (dma_interrupt_flag_get(DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_GET) == SET) {
		dma_interrupt_disable(DMA1, DMA_CH4, DMA_INTERRUPT_DISABLE);

		if (s_TxBuffer[dmx::config::UART6_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART6_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2 , TIMER_CNT(TIMER4) + s_nDmxTransmitInterTime);
			s_TxBuffer[dmx::config::UART6_PORT].State = TxRxState::DMXINTER;
		}
	}

	dma_interrupt_flag_clear(DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_CLEAR);
}
# endif
# if defined (GD32F4XX)
void DMA0_Channel1_IRQHandler() {
	if (dma_interrupt_flag_get(DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_GET) == SET) {
		dma_interrupt_disable(DMA0, DMA_CH1, DMA_INTERRUPT_DISABLE);

		if (s_TxBuffer[dmx::config::UART6_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART6_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2 , TIMER_CNT(TIMER4) + s_nDmxTransmitInterTime);
			s_TxBuffer[dmx::config::UART6_PORT].State = TxRxState::DMXINTER;
		}
	}

	dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_CLEAR);
}
# endif
#endif
/*
 * UART 7
 */
#if defined (DMX_USE_UART7)
# if defined (GD32F20X)
void DMA1_Channel3_IRQHandler() {
	if (dma_interrupt_flag_get(DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_GET) == SET) {
		dma_interrupt_disable(DMA1, DMA_CH3, DMA_INTERRUPT_DISABLE);

		if (s_TxBuffer[dmx::config::UART7_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART7_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3 , TIMER_CNT(TIMER4) + s_nDmxTransmitInterTime);
			s_TxBuffer[dmx::config::UART7_PORT].State = TxRxState::DMXINTER;
		}
	}

	dma_interrupt_flag_clear(DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR);
}
# endif
# if defined (GD32F4XX)
void DMA0_Channel0_IRQHandler() {
	if (dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INTERRUPT_FLAG_GET) == SET) {
		dma_interrupt_disable(DMA0, DMA_CH0, DMA_INTERRUPT_DISABLE);

		if (s_TxBuffer[dmx::config::UART7_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART7_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3 , TIMER_CNT(TIMER4) + s_nDmxTransmitInterTime);
			s_TxBuffer[dmx::config::UART7_PORT].State = TxRxState::DMXINTER;
		}
	}

	dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INTERRUPT_FLAG_CLEAR);
}
# endif
#endif
}

static void uart_dmx_config(uint32_t usart_periph) {
	gd32_uart_begin(usart_periph, 250000U, GD32_UART_BITS_8, GD32_UART_PARITY_NONE, GD32_UART_STOP_2BITS);
}

Dmx *Dmx::s_pThis = nullptr;

Dmx::Dmx() {
	DEBUG_ENTRY

	assert(s_pThis == nullptr);
	s_pThis = this;

	for (auto i = 0; i < DMX_MAX_PORTS; i++) {
#if !defined (GD32F4XX)
		gpio_init(s_DirGpio[i].nPort, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, s_DirGpio[i].nPin);
#else
		gpio_mode_set(s_DirGpio[i].nPort, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, s_DirGpio[i].nPin);
		gpio_output_options_set(s_DirGpio[i].nPort, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, s_DirGpio[i].nPin);
		gpio_af_set(s_DirGpio[i].nPort, GPIO_AF_0, s_DirGpio[i].nPin);
#endif
		m_nDmxTransmissionLength[i] = dmx::max::CHANNELS;
		SetPortDirection(i, PortDirection::INP, false);
		s_RxBuffer[i].State = TxRxState::IDLE;
		s_TxBuffer[i].State = TxRxState::IDLE;
		s_TxBuffer[i].outputStyle = dmx::OutputStyle::DELTA;
		ClearData(i);
	}

	usart_dma_config();	// DMX Transmit
#if defined (DMX_USE_USART0) || defined (DMX_USE_USART1) || defined (DMX_USE_USART2) || defined (DMX_USE_UART3)
	timer1_config();	// DMX Transmit -> USART0, USART1, USART2, UART3
#endif
#if defined (DMX_USE_UART4) || defined (DMX_USE_USART5) ||  defined (DMX_USE_UART6) || defined (DMX_USE_UART7)
	timer4_config();	// DMX Transmit -> UART4, USART5, UART6, UART7
#endif
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
	timer2_config();	// DMX Receive	-> Slot time-out Port 0,1,2,3
	timer3_config();	// DMX Receive	-> Slot time-out Port 4,5,6,7
	timer6_config();	// DMX Receive	-> Updates per second
#endif

#if defined (DMX_USE_USART0)
	uart_dmx_config(USART0);
	NVIC_EnableIRQ(USART0_IRQn);
#endif
#if defined (DMX_USE_USART1)
	uart_dmx_config(USART1);
	NVIC_EnableIRQ(USART1_IRQn);
#endif
#if defined (DMX_USE_USART2)
	uart_dmx_config(USART2);
	NVIC_EnableIRQ(USART2_IRQn);
#endif
#if defined (DMX_USE_UART3)
	uart_dmx_config(UART3);
	NVIC_EnableIRQ(UART3_IRQn);
#endif
#if defined (DMX_USE_UART4)
	uart_dmx_config(UART4);
	NVIC_EnableIRQ(UART4_IRQn);
#endif
#if defined (DMX_USE_USART5)
	uart_dmx_config(USART5);
	NVIC_EnableIRQ(USART5_IRQn);
#endif
#if defined (DMX_USE_UART6)
	uart_dmx_config(UART6);
	NVIC_EnableIRQ(UART6_IRQn);
#endif
#if defined (DMX_USE_UART7)
	uart_dmx_config(UART7);
	NVIC_EnableIRQ(UART7_IRQn);
#endif

	logic_analyzer::init();
	DEBUG_EXIT
}

void Dmx::SetPortDirection(const uint32_t nPortIndex, PortDirection portDirection, bool bEnableData) {
//	DEBUG_PRINTF("nPortIndex=%u %s %c", nPortIndex, portDirection == PortDirection::INP ? "Input" : "Output", bEnableData ? 'Y' : 'N');
	assert(nPortIndex < DMX_MAX_PORTS);

	const auto nUart = _port_to_uart(nPortIndex);

	if (m_dmxPortDirection[nPortIndex] != portDirection) {
		m_dmxPortDirection[nPortIndex] = portDirection;

		StopData(nUart, nPortIndex);

		if (portDirection == PortDirection::OUTP) {
			GPIO_BOP(s_DirGpio[nPortIndex].nPort) = s_DirGpio[nPortIndex].nPin;
		} else if (portDirection == PortDirection::INP) {
			GPIO_BC(s_DirGpio[nPortIndex].nPort) = s_DirGpio[nPortIndex].nPin;
		} else {
			assert(0);
		}
	} else if (!bEnableData) {
		StopData(nUart, nPortIndex);
	}

	if (bEnableData) {
		StartData(nUart, nPortIndex);
	}
}

void Dmx::ClearData(const uint32_t nPortIndex) {
	assert(nPortIndex < DMX_MAX_PORTS);

	auto *p = &s_TxBuffer[nPortIndex];
	auto *p16 =	reinterpret_cast<uint16_t*>(p->dmx.data);

	for (auto i = 0; i < dmx::buffer::SIZE / 2; i++) {
		*p16++ = 0;
	}

	p->dmx.nLength = 513; // Including START Code
}

void Dmx::StartDmxOutput(const uint32_t nPortIndex) {
	logic_analyzer::ch2_set();

	assert(nPortIndex < dmx::config::max::OUT);
	const auto nUart = _port_to_uart(nPortIndex);

	/*
	 * USART_FLAG_TC is set after power on.
	 * The flag is cleared by DMA interrupt when maximum slots - 1 are transmitted.
	 */

	//TODO Do we need a timeout just to be safe?
	while (SET != usart_flag_get(nUart, USART_FLAG_TC))
		;

	switch (nUart) {
	/* TIMER 1 */
#if defined (DMX_USE_USART0)
	case USART0:
		_gpio_mode_output(USART0_GPIO_PORT, USART0_TX_PIN);
		GPIO_BC(USART0_GPIO_PORT) = USART0_TX_PIN;
		timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0, TIMER_CNT(TIMER1) + s_nDmxTransmitBreakTime);
		timer_interrupt_enable(TIMER1, TIMER_INT_CH0);
		break;
#endif
#if defined (DMX_USE_USART1)
	case USART1:
		_gpio_mode_output(USART1_GPIO_PORT, USART1_TX_PIN);
		GPIO_BC(USART1_GPIO_PORT) = USART1_TX_PIN;
		timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_1, TIMER_CNT(TIMER1) + s_nDmxTransmitBreakTime);
		timer_interrupt_enable(TIMER1, TIMER_INT_CH1);
		break;
#endif
#if defined (DMX_USE_USART2)
	case USART2:
		_gpio_mode_output(USART2_GPIO_PORT, USART2_TX_PIN);
		GPIO_BC(USART2_GPIO_PORT) = USART2_TX_PIN;
		timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_2, TIMER_CNT(TIMER1) + s_nDmxTransmitBreakTime);
		timer_interrupt_enable(TIMER1, TIMER_INT_CH2);
		break;
#endif
#if defined (DMX_USE_UART3)
	case UART3:
		_gpio_mode_output(UART3_GPIO_PORT, UART3_TX_PIN);
		GPIO_BC(UART3_GPIO_PORT) = UART3_TX_PIN;
		timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_3, TIMER_CNT(TIMER1) + s_nDmxTransmitBreakTime);
		timer_interrupt_enable(TIMER1, TIMER_INT_CH3);
		break;
#endif
		/* TIMER 4 */
#if defined (DMX_USE_UART4)
	case UART4:
		_gpio_mode_output(UART4_GPIO_TX_PORT, UART4_TX_PIN);
		GPIO_BC(UART4_GPIO_TX_PORT) = UART4_TX_PIN;
		timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0, TIMER_CNT(TIMER4) + s_nDmxTransmitBreakTime);
		timer_interrupt_enable(TIMER4, TIMER_INT_CH0);
		break;
#endif
#if defined (DMX_USE_USART5)
	case USART5:
		_gpio_mode_output(USART5_GPIO_PORT, USART5_TX_PIN);
		GPIO_BC(USART5_GPIO_PORT) = USART5_TX_PIN;
		timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_1, TIMER_CNT(TIMER4) + s_nDmxTransmitBreakTime);
		timer_interrupt_enable(TIMER4, TIMER_INT_CH1);
		break;
#endif
#if defined (DMX_USE_UART6)
	case UART6:
		_gpio_mode_output(UART6_GPIO_PORT, UART6_TX_PIN);
		GPIO_BC(UART6_GPIO_PORT) = UART6_TX_PIN;
		timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2, TIMER_CNT(TIMER4) + s_nDmxTransmitBreakTime);
		timer_interrupt_enable(TIMER4, TIMER_INT_CH2);
		break;
#endif
#if defined (DMX_USE_UART7)
	case UART7:
		_gpio_mode_output(UART7_GPIO_PORT, UART7_TX_PIN);
		GPIO_BC(UART7_GPIO_PORT) = UART7_TX_PIN;
		timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3, TIMER_CNT(TIMER4) + s_nDmxTransmitBreakTime);
		timer_interrupt_enable(TIMER4, TIMER_INT_CH3);
		break;
#endif
	default:
		assert(0);
		break;
	}

	s_TxBuffer[nPortIndex].State = TxRxState::BREAK;

	logic_analyzer::ch2_clear();
}


void Dmx::StopData(const uint32_t nUart, const uint32_t nPortIndex) {
	assert(nPortIndex < DMX_MAX_PORTS);

	if (sv_PortState[nPortIndex] == PortState::IDLE) {
		return;
	}

	if (m_dmxPortDirection[nPortIndex] == PortDirection::OUTP) {
		do {
			if (s_TxBuffer[nPortIndex].State == dmx::TxRxState::DMXINTER) {
				usart_flag_clear(nUart, USART_FLAG_TC);
				while (SET != usart_flag_get(nUart, USART_FLAG_TC))
					;
				s_TxBuffer[nPortIndex].State = dmx::TxRxState::IDLE;
			}
		} while (s_TxBuffer[nPortIndex].State != dmx::TxRxState::IDLE);
	} else if (m_dmxPortDirection[nPortIndex] == PortDirection::INP) {
		usart_interrupt_disable(nUart, USART_INT_RBNE);
		s_RxBuffer[nPortIndex].State = TxRxState::IDLE;

#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
		switch (nPortIndex) {
		case 0:
			TIMER_DMAINTEN(TIMER2) &= (~(uint32_t) TIMER_INT_CH0);
			break;
#if DMX_MAX_PORTS >= 2
		case 1:
			TIMER_DMAINTEN(TIMER2) &= (~(uint32_t) TIMER_INT_CH1);
			break;
#endif
#if DMX_MAX_PORTS >= 3
		case 2:
			TIMER_DMAINTEN(TIMER2) &= (~(uint32_t) TIMER_INT_CH2);
			break;
#endif
#if DMX_MAX_PORTS >= 4
		case 3:
			TIMER_DMAINTEN(TIMER2) &= (~(uint32_t) TIMER_INT_CH3);
			break;
#endif
#if DMX_MAX_PORTS >= 5
		case 4:
			TIMER_DMAINTEN(TIMER3) &= (~(uint32_t) TIMER_INT_CH0);
			break;
#endif
#if DMX_MAX_PORTS >= 6
		case 5:
			TIMER_DMAINTEN(TIMER3) &= (~(uint32_t) TIMER_INT_CH1);
			break;
#endif
#if DMX_MAX_PORTS >= 7
		case 6:
			TIMER_DMAINTEN(TIMER3) &= (~(uint32_t) TIMER_INT_CH2);
			break;
#endif
#if DMX_MAX_PORTS == 8
		case 7:
			TIMER_DMAINTEN(TIMER3) &= (~(uint32_t) TIMER_INT_CH3);
			break;
#endif
		default:
			assert(0);
			__builtin_unreachable();
			break;
		}
#endif
	} else {
		assert(0);
		__builtin_unreachable();
	}

	sv_PortState[nPortIndex] = PortState::IDLE;
}

// DMX Send

void Dmx::SetDmxBreakTime(uint32_t nBreakTime) {
	s_nDmxTransmitBreakTime = std::max(transmit::BREAK_TIME_MIN, nBreakTime);
	SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
}

void Dmx::SetDmxMabTime(uint32_t nMabTime) {
	s_nDmxTransmitMabTime = std::max(transmit::MAB_TIME_MIN, nMabTime);
	SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
}

void Dmx::SetDmxPeriodTime(uint32_t nPeriod) {
	m_nDmxTransmitPeriodRequested = nPeriod;

	auto nLengthMax = s_TxBuffer[0].dmx.nLength;

	for (uint32_t nPortIndex = 1; nPortIndex < config::max::OUT; nPortIndex++) {
		const auto nLength = s_TxBuffer[nPortIndex].dmx.nLength;
		if (nLength > nLengthMax) {
			nLengthMax = nLength;
		}
	}

	auto nPackageLengthMicroSeconds = s_nDmxTransmitBreakTime + s_nDmxTransmitMabTime + (nLengthMax * 44U);

	// The GD32F4xx Timer 1 has a 32-bit counter
#if ! defined(GD32F4XX)
	if (nPackageLengthMicroSeconds > (static_cast<uint16_t>(~0) - 44U)) {
		s_nDmxTransmitBreakTime = std::min(transmit::BREAK_TIME_TYPICAL, s_nDmxTransmitBreakTime);
		s_nDmxTransmitMabTime = transmit::MAB_TIME_MIN;
		nPackageLengthMicroSeconds = s_nDmxTransmitBreakTime + s_nDmxTransmitMabTime + (nLengthMax * 44U);
	}
#endif

	uint32_t nDmxTransmitPeriod;

	if (nPeriod != 0) {
		if (nPeriod < nPackageLengthMicroSeconds) {
			nDmxTransmitPeriod = std::max(transmit::BREAK_TO_BREAK_TIME_MIN, nPackageLengthMicroSeconds + 44U);
		} else {
			nDmxTransmitPeriod = nPeriod;
		}
	} else {
		nDmxTransmitPeriod = std::max(transmit::BREAK_TO_BREAK_TIME_MIN, nPackageLengthMicroSeconds + 44U);
	}

	s_nDmxTransmitInterTime = nDmxTransmitPeriod - nPackageLengthMicroSeconds;

	DEBUG_PRINTF("nPeriod=%u, nLengthMax=%u, s_nDmxTransmitPeriod=%u, nPackageLengthMicroSeconds=%u -> s_nDmxTransmitInterTime=%u", nPeriod, nLengthMax, nDmxTransmitPeriod, nPackageLengthMicroSeconds, s_nDmxTransmitInterTime);
}

void Dmx::SetDmxSlots(uint16_t nSlots) {
	if ((nSlots >= 2) && (nSlots <= dmx::max::CHANNELS)) {
		m_nDmxTransmitSlots = nSlots;

		for (uint32_t i = 0; i < config::max::OUT; i++) {
			m_nDmxTransmissionLength[i] = std::min(m_nDmxTransmissionLength[i], static_cast<uint32_t>(nSlots));
		}

		SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
	}
}

void Dmx::SetOutputStyle(const uint32_t nPortIndex, const dmx::OutputStyle outputStyle) {
	assert(nPortIndex < dmx::config::max::OUT);

	s_TxBuffer[nPortIndex].outputStyle = outputStyle;
}

dmx::OutputStyle Dmx::GetOutputStyle(const uint32_t nPortIndex) const {
	assert(nPortIndex < dmx::config::max::OUT);
	return s_TxBuffer[nPortIndex].outputStyle;
}

void Dmx::SetSendData(const uint32_t nPortIndex, const uint8_t *pData, uint32_t nLength) {
	assert(nPortIndex < DMX_MAX_PORTS);

	auto *p = &s_TxBuffer[nPortIndex];
	auto *pDst = p->dmx.data;

	nLength = std::min(nLength, static_cast<uint32_t>(m_nDmxTransmitSlots));
	p->dmx.nLength = static_cast<uint16_t>(nLength + 1);

	memcpy(pDst, pData,  nLength);

	if (nLength != m_nDmxTransmissionLength[nPortIndex]) {
		m_nDmxTransmissionLength[nPortIndex] = nLength;
		SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
	}
}

void Dmx::SetSendDataWithoutSC(const uint32_t nPortIndex, const uint8_t *pData, uint32_t nLength) {
	logic_analyzer::ch0_set();

	assert(nPortIndex < DMX_MAX_PORTS);

	auto *p = &s_TxBuffer[nPortIndex];
	auto *pDst = p->dmx.data;

	nLength = std::min(nLength, static_cast<uint32_t>(m_nDmxTransmitSlots));
	p->dmx.nLength = static_cast<uint16_t>(nLength + 1);

	pDst[0] = START_CODE;
	memcpy(&pDst[1], pData,  nLength);

	if (nLength != m_nDmxTransmissionLength[nPortIndex]) {
		m_nDmxTransmissionLength[nPortIndex] = nLength;
		SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
	}

	logic_analyzer::ch0_clear();
}

void Dmx::Blackout() {
	DEBUG_ENTRY

	for (uint32_t nPortIndex = 0; nPortIndex < DMX_MAX_PORTS; nPortIndex++) {
		if (m_dmxPortDirection[nPortIndex] == dmx::PortDirection::OUTP) {
			const auto nUart = _port_to_uart(nPortIndex);

			StopData(nUart, nPortIndex);
			ClearData(nPortIndex);
			StartData(nUart, nPortIndex);
		}
	}

	DEBUG_EXIT
}

void Dmx::FullOn() {
	DEBUG_ENTRY

	for (uint32_t nPortIndex = 0; nPortIndex < DMX_MAX_PORTS; nPortIndex++) {
		if (m_dmxPortDirection[nPortIndex] == dmx::PortDirection::OUTP) {
			const auto nUart = _port_to_uart(nPortIndex);

			StopData(nUart, nPortIndex);

			auto *p = &s_TxBuffer[nPortIndex];
			auto *p16 = reinterpret_cast<uint16_t*>(p->dmx.data);

			for (auto i = 0; i < dmx::buffer::SIZE / 2; i++) {
				*p16++ = UINT16_MAX;
			}

			p->dmx.data[0] = dmx::START_CODE;
			p->dmx.nLength = 513;

			StartData(nUart, nPortIndex);
		}
	}

	DEBUG_EXIT
}

void Dmx::StartOutput(const uint32_t nPortIndex) {
	if ((sv_PortState[nPortIndex] == dmx::PortState::TX)
			&& (s_TxBuffer[nPortIndex].outputStyle == dmx::OutputStyle::DELTA)
			&& (s_TxBuffer[nPortIndex].State == dmx::TxRxState::IDLE)) {

		StartDmxOutput(nPortIndex);
	}
}

static bool s_doForce;

void Dmx::SetOutput(const bool doForce) {
	logic_analyzer::ch1_set();

	if (doForce) {
		s_doForce = true;
		for (uint32_t nPortIndex = 0; nPortIndex < dmx::config::max::OUT; nPortIndex++) {
			if ((sv_PortState[nPortIndex] == dmx::PortState::TX)
					&& (s_TxBuffer[nPortIndex].outputStyle == dmx::OutputStyle::CONTINOUS)) {
				StopData(_port_to_uart(nPortIndex), nPortIndex);
			}
		}

		logic_analyzer::ch1_clear();
		return;
	}

	for (uint32_t nPortIndex = 0; nPortIndex < dmx::config::max::OUT; nPortIndex++) {
		if (sv_PortState[nPortIndex] == dmx::PortState::TX) {
			const auto b = ((s_TxBuffer[nPortIndex].outputStyle == dmx::OutputStyle::CONTINOUS) && s_doForce);
			if (b || ((s_TxBuffer[nPortIndex].outputStyle == dmx::OutputStyle::DELTA) && (s_TxBuffer[nPortIndex].State == dmx::TxRxState::IDLE))) {
				StartDmxOutput(nPortIndex);
			}
		}
	}

	s_doForce = false;
	logic_analyzer::ch1_clear();
}

void Dmx::StartData(const uint32_t nUart, const uint32_t nPortIndex) {
//	DEBUG_PRINTF("sv_PortState[%u]=%u", nPortIndex, sv_PortState[nPortIndex]);
	assert(nPortIndex < DMX_MAX_PORTS);
	assert(sv_PortState[nPortIndex] == PortState::IDLE);

	if (m_dmxPortDirection[nPortIndex] == dmx::PortDirection::OUTP) {
		sv_PortState[nPortIndex] = PortState::TX;
		if (s_TxBuffer[nPortIndex].outputStyle == dmx::OutputStyle::CONTINOUS) {
			StartDmxOutput(nPortIndex);
		}
	} else if (m_dmxPortDirection[nPortIndex] == dmx::PortDirection::INP) {
		s_RxBuffer[nPortIndex].State = TxRxState::IDLE;

		while (RESET == usart_flag_get(nUart, USART_FLAG_TBE))
			;

		usart_interrupt_flag_clear(nUart, USART_INT_FLAG_RBNE);
		usart_interrupt_enable(nUart, USART_INT_RBNE);

		sv_PortState[nPortIndex] = PortState::RX;

#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
		switch (nPortIndex) {
		case 0:
			TIMER_DMAINTEN(TIMER2) |= (uint32_t) TIMER_INT_CH0;
			break;
#if DMX_MAX_PORTS >= 2
		case 1:
			TIMER_DMAINTEN(TIMER2) |= (uint32_t) TIMER_INT_CH1;
			break;
#endif
#if DMX_MAX_PORTS >= 3
		case 2:
			TIMER_DMAINTEN(TIMER2) |= (uint32_t) TIMER_INT_CH2;
			break;
#endif
#if DMX_MAX_PORTS >= 4
		case 3:
			TIMER_DMAINTEN(TIMER2) |= (uint32_t) TIMER_INT_CH3;
			break;
#endif
#if DMX_MAX_PORTS >= 5
		case 4:
			TIMER_DMAINTEN(TIMER3) |= (uint32_t) TIMER_INT_CH0;
			break;
#endif
#if DMX_MAX_PORTS >= 6
		case 5:
			TIMER_DMAINTEN(TIMER3) |= (uint32_t) TIMER_INT_CH1;
			break;
#endif
#if DMX_MAX_PORTS >= 7
		case 6:
			TIMER_DMAINTEN(TIMER3) |= (uint32_t) TIMER_INT_CH2;
			break;
#endif
#if DMX_MAX_PORTS == 8
		case 7:
			TIMER_DMAINTEN(TIMER3) |= (uint32_t) TIMER_INT_CH3;
			break;
#endif
		default:
			assert(0);
			__builtin_unreachable();
			break;
		}
#endif
	} else {
		assert(0);
		__builtin_unreachable();
	}
}


// DMX Receive

const uint8_t* Dmx::GetDmxChanged(UNUSED uint32_t nPortIndex) {
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
	const auto *p = GetDmxAvailable(nPortIndex);

	if (p == nullptr) {
		return nullptr;
	}

	const auto *pSrc16 = reinterpret_cast<const uint16_t*>(p);
	auto *pDst16 = reinterpret_cast<uint16_t *>(&s_RxDmxPrevious[nPortIndex][0]);

	auto isChanged = false;

	for (auto i = 0; i < buffer::SIZE / 2; i++) {
		if (*pDst16 != *pSrc16) {
			*pDst16 = *pSrc16;
			isChanged = true;
		}
		pDst16++;
		pSrc16++;
	}

	return (isChanged ? p : nullptr);
#else
	return nullptr;
#endif
}

const uint8_t *Dmx::GetDmxAvailable(UNUSED uint32_t nPortIndex)  {
	assert(nPortIndex < DMX_MAX_PORTS);
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
	if ((s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket & 0x8000) != 0x8000) {
		return nullptr;
	}

	s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket &= ~0x8000;
	s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket--;	// Remove SC from length

	return s_RxBuffer[nPortIndex].data;
#else
	return nullptr;
#endif
}

const uint8_t* Dmx::GetDmxCurrentData(const uint32_t nPortIndex) {
	return s_RxBuffer[nPortIndex].data;
}

uint32_t Dmx::GetDmxUpdatesPerSecond(UNUSED uint32_t nPortIndex) {
	assert(nPortIndex < DMX_MAX_PORTS);
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
	return sv_nRxDmxPackets[nPortIndex].nPerSecond;
#else
	return 0;
#endif
}

uint32_t GetDmxReceivedCount(UNUSED uint32_t nPortIndex) {
	assert(nPortIndex < DMX_MAX_PORTS);
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
	return sv_nRxDmxPackets[nPortIndex].nCount;
#else
	return 0;
#endif
}

// RDM Send

void Dmx::RdmSendRaw(const uint32_t nPortIndex, const uint8_t* pRdmData, uint32_t nLength) {
	assert(nPortIndex < DMX_MAX_PORTS);
	assert(pRdmData != nullptr);
	assert(nLength != 0);

	const auto nUart = _port_to_uart(nPortIndex);

	while (SET != usart_flag_get(nUart, USART_FLAG_TC))
		;

	switch (nUart) {
#if defined (DMX_USE_USART0)
	case USART0:
		_gpio_mode_output(USART0_GPIO_PORT, USART0_TX_PIN);
		GPIO_BC(USART0_GPIO_PORT) = USART0_TX_PIN;
		break;
#endif
#if defined (DMX_USE_USART1)
	case USART1:
		_gpio_mode_output(USART1_GPIO_PORT, USART1_TX_PIN);
		GPIO_BC(USART1_GPIO_PORT) = USART1_TX_PIN;
		break;
#endif
#if defined (DMX_USE_USART2)
	case USART2:
		_gpio_mode_output(USART2_GPIO_PORT, USART2_TX_PIN);
		GPIO_BC(USART2_GPIO_PORT) = USART2_TX_PIN;
		break;
#endif
#if defined (DMX_USE_UART3)
	case UART3:
		_gpio_mode_output(UART3_GPIO_PORT, UART3_TX_PIN);
		GPIO_BC(UART3_GPIO_PORT) = UART3_TX_PIN;
		break;
#endif
#if defined (DMX_USE_UART4)
	case UART4:
		_gpio_mode_output(UART4_GPIO_TX_PORT, UART4_TX_PIN);
		GPIO_BC(UART4_GPIO_TX_PORT) = UART4_TX_PIN;
		break;
#endif
#if defined (DMX_USE_USART5)
	case USART5:
		_gpio_mode_output(USART5_GPIO_PORT, USART5_TX_PIN);
		GPIO_BC(USART5_GPIO_PORT) = USART5_TX_PIN;
		break;
#endif
#if defined (DMX_USE_UART6)
	case UART6:
		_gpio_mode_output(UART6_GPIO_PORT, UART6_TX_PIN);
		GPIO_BC(UART6_GPIO_PORT) = UART6_TX_PIN;
		break;
#endif
#if defined (DMX_USE_UART7)
	case UART7:
		_gpio_mode_output(UART7_GPIO_PORT, UART7_TX_PIN);
		GPIO_BC(UART7_GPIO_PORT) = UART7_TX_PIN;
		break;
#endif
	default:
		assert(0);
		__builtin_unreachable();
		break;
	}

	udelay(RDM_TRANSMIT_BREAK_TIME);

	switch (nUart) {
#if defined (DMX_USE_USART0)
	case USART0:
		_gpio_mode_af(USART0_GPIO_PORT, USART0_TX_PIN, USART0);
		break;
#endif
#if defined (DMX_USE_USART1)
	case USART1:
		_gpio_mode_af(USART1_GPIO_PORT, USART1_TX_PIN, USART1);
		break;
#endif
#if defined (DMX_USE_USART2)
	case USART2:
		_gpio_mode_af(USART2_GPIO_PORT, USART2_TX_PIN, USART2);
		break;
#endif
#if defined (DMX_USE_UART3)
	case UART3:
		_gpio_mode_af(UART3_GPIO_PORT, UART3_TX_PIN, UART3);
		break;
#endif
#if defined (DMX_USE_UART4)
	case UART4:
		_gpio_mode_af(UART4_GPIO_TX_PORT, UART4_TX_PIN, UART4);
		break;
#endif
#if defined (DMX_USE_USART5)
	case USART5:
		_gpio_mode_af(USART5_GPIO_PORT, USART5_TX_PIN, USART5);
		break;
#endif
#if defined (DMX_USE_UART6)
	case UART6:
		_gpio_mode_af(UART6_GPIO_PORT, UART6_TX_PIN, UART6);
		break;
#endif
#if defined (DMX_USE_UART7)
	case UART7:
		_gpio_mode_af(UART7_GPIO_PORT, UART7_TX_PIN, UART7);
		break;
#endif
	default:
		assert(0);
		__builtin_unreachable();
		break;
	}

	udelay(RDM_TRANSMIT_MAB_TIME);

	for (uint32_t i = 0; i < nLength; i++) {
		while (RESET == usart_flag_get(nUart, USART_FLAG_TBE))
			;
		USART_DATA(nUart) = static_cast<uint16_t>(USART_DATA_DATA & pRdmData[i]);
	}

	while (SET != usart_flag_get(nUart, USART_FLAG_TC)) {
		static_cast<void>(GET_BITS(USART_DATA(nUart), 0U, 8U));
	}
}

// RDM Receive

const uint8_t *Dmx::RdmReceive(const uint32_t nPortIndex) {
	assert(nPortIndex < DMX_MAX_PORTS);

	if ((s_RxBuffer[nPortIndex].Rdm.nIndex & 0x4000) != 0x4000) {
		return nullptr;
	}

	s_RxBuffer[nPortIndex].Dmx.nSlotsInPacket = 0; // This is correct.
	return s_RxBuffer[nPortIndex].data;
}

const uint8_t *Dmx::RdmReceiveTimeOut(const uint32_t nPortIndex, uint16_t nTimeOut) {
	assert(nPortIndex < DMX_MAX_PORTS);

	uint8_t *p = nullptr;
	TIMER_CNT(TIMER5) = 0;

	do {
		if ((p = const_cast<uint8_t*>(RdmReceive(nPortIndex))) != nullptr) {
			return p;
		}
	} while (TIMER_CNT(TIMER5) < nTimeOut);

	return nullptr;
}
