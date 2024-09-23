/**
 * @file dmx.cpp
 */
/* Copyright (C) 2021-2024 by Arjan van Vught mailto:info@gd32-dmx.org
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

#if defined (CONFIG_TIMER6_HAVE_NO_IRQ_HANDLER)
# error
#endif

#pragma GCC push_options
#pragma GCC optimize ("O3")
#pragma GCC optimize ("-fprefetch-loop-arrays")

#include <cstdint>
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
#include "gd32/dmx_config.h"
#include "dmx_internal.h"

#include "logic_analyzer.h"

#include "debug.h"

extern struct HwTimersSeconds g_Seconds;

namespace dmx {
enum class TxRxState {
	IDLE, BREAK, MAB, DMXDATA, DMXINTER, RDMDATA, CHECKSUMH, CHECKSUML, RDMDISC
};

enum class PortState {
	IDLE, TX, RX
};

struct TxDmxPacket {
	uint8_t data[dmx::buffer::SIZE];	// multiple of uint32_t
	uint32_t nLength;
	bool bDataPending;
};

struct TxData {
	TxDmxPacket dmx;
	OutputStyle outputStyle ALIGNED;
	volatile TxRxState State;
};

struct DmxTransmit {
	uint32_t nBreakTime;
	uint32_t nMabTime;
	uint32_t nInterTime;
};

struct RxDmxPackets {
	uint32_t nPerSecond;
	uint32_t nCount;
	uint32_t nCountPrevious;
};

struct RxDmxData {
	uint8_t data[dmx::buffer::SIZE] ALIGNED;	// multiple of uint32_t
	uint32_t nSlotsInPacket;
};

struct RxData {
	struct {
		volatile RxDmxData current;
		RxDmxData previous;
	} Dmx ALIGNED;
	struct {
		volatile uint8_t data[sizeof(struct TRdmMessage)] ALIGNED;
		volatile uint32_t nIndex;
	} Rdm ALIGNED;
	volatile TxRxState State;
} ALIGNED;

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

static volatile PortState sv_PortState[dmx::config::max::PORTS] ALIGNED;

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
static volatile dmx::TotalStatistics sv_TotalStatistics[dmx::config::max::PORTS] ALIGNED;
#endif

// DMX RX

static volatile RxDmxPackets sv_nRxDmxPackets[dmx::config::max::PORTS] ALIGNED;

// RDM RX
volatile uint32_t gsv_RdmDataReceiveEnd;

// DMX RDM RX

static RxData sv_RxBuffer[dmx::config::max::PORTS] ALIGNED;

// DMX TX

static TxData s_TxBuffer[dmx::config::max::PORTS] ALIGNED SECTION_DMA_BUFFER;
static DmxTransmit s_nDmxTransmit;

template<uint32_t uart, uint32_t nPortIndex>
void irq_handler_dmx_rdm_input() {
	const auto isFlagIdleFrame = (USART_REG_VAL(uart, USART_FLAG_IDLE) & BIT(USART_BIT_POS(USART_FLAG_IDLE))) == BIT(USART_BIT_POS(USART_FLAG_IDLE));
	/*
	 * Software can clear this bit by reading the USART_STAT and USART_DATA registers one by one.
	 */
	if (isFlagIdleFrame) {
		static_cast<void>(GET_BITS(USART_RDATA(uart), 0U, 8U));

		if (sv_RxBuffer[nPortIndex].State == TxRxState::DMXDATA) {
			sv_RxBuffer[nPortIndex].State = TxRxState::IDLE;
			sv_RxBuffer[nPortIndex].Dmx.current.nSlotsInPacket |= 0x8000;

			return;
		}

		if (sv_RxBuffer[0].State == TxRxState::RDMDISC) {
			sv_RxBuffer[nPortIndex].State = TxRxState::IDLE;
			sv_RxBuffer[nPortIndex].Rdm.nIndex |= 0x4000;

			return;
		}

		return;
	}

	const auto isFlagFrameError = (USART_REG_VAL(uart, USART_FLAG_FERR) & BIT(USART_BIT_POS(USART_FLAG_FERR))) == BIT(USART_BIT_POS(USART_FLAG_FERR));
	/*
	 * Software can clear this bit by reading the USART_STAT and USART_DATA registers one by one.
	 */
	if (isFlagFrameError) {
		static_cast<void>(GET_BITS(USART_RDATA(uart), 0U, 8U));

		if (sv_RxBuffer[nPortIndex].State == TxRxState::IDLE) {
			sv_RxBuffer[nPortIndex].State = TxRxState::BREAK;
		}

		return;
	}

	const auto data = static_cast<uint8_t>(GET_BITS(USART_RDATA(uart), 0U, 8U));

	switch (sv_RxBuffer[nPortIndex].State) {
	case TxRxState::IDLE:
		sv_RxBuffer[nPortIndex].State = TxRxState::RDMDISC;
		sv_RxBuffer[nPortIndex].Rdm.data[0] = data;
		sv_RxBuffer[nPortIndex].Rdm.nIndex = 1;
		break;
	case TxRxState::BREAK:
		switch (data) {
		case START_CODE:
			sv_RxBuffer[nPortIndex].Dmx.current.data[0] = START_CODE;
			sv_RxBuffer[nPortIndex].Dmx.current.nSlotsInPacket = 1;
			sv_nRxDmxPackets[nPortIndex].nCount++;
			sv_RxBuffer[nPortIndex].State = TxRxState::DMXDATA;
			break;
		case E120_SC_RDM:
			sv_RxBuffer[nPortIndex].Rdm.data[0] = E120_SC_RDM;
			sv_RxBuffer[nPortIndex].Rdm.nIndex = 1;
			sv_RxBuffer[nPortIndex].State = TxRxState::RDMDATA;
			break;
		default:
			sv_RxBuffer[nPortIndex].Dmx.current.nSlotsInPacket = 0;
			sv_RxBuffer[nPortIndex].Rdm.nIndex = 0;
			sv_RxBuffer[nPortIndex].State = TxRxState::IDLE;
			break;
		}
		break;
	case TxRxState::DMXDATA: {
		auto nIndex = sv_RxBuffer[nPortIndex].Dmx.current.nSlotsInPacket;
		sv_RxBuffer[nPortIndex].Dmx.current.data[nIndex] = data;
		nIndex++;
		sv_RxBuffer[nPortIndex].Dmx.current.nSlotsInPacket = nIndex;

		if (nIndex > dmx::max::CHANNELS) {
			nIndex |= 0x8000;
			sv_RxBuffer[nPortIndex].Dmx.current.nSlotsInPacket = nIndex;
			sv_RxBuffer[nPortIndex].State = TxRxState::IDLE;
			break;
		}
	}
		break;
	case TxRxState::RDMDATA: {
		auto nIndex = sv_RxBuffer[nPortIndex].Rdm.nIndex;
		sv_RxBuffer[nPortIndex].Rdm.data[nIndex] = data;
		nIndex++;
		sv_RxBuffer[nPortIndex].Rdm.nIndex = nIndex;

		const auto *p = reinterpret_cast<volatile struct TRdmMessage*>(&sv_RxBuffer[nPortIndex].Rdm.data[0]);

		if ((nIndex >= 24) && (nIndex <= sizeof(struct TRdmMessage)) && (nIndex == p->message_length)) {
			sv_RxBuffer[nPortIndex].State = TxRxState::CHECKSUMH;
		} else if (nIndex > sizeof(struct TRdmMessage)) {
			sv_RxBuffer[nPortIndex].State = TxRxState::IDLE;
		}
	}
		break;
	case TxRxState::CHECKSUMH: {
		auto nIndex = sv_RxBuffer[nPortIndex].Rdm.nIndex;
		sv_RxBuffer[nPortIndex].Rdm.data[nIndex] = data;
		nIndex++;
		sv_RxBuffer[nPortIndex].Rdm.nIndex = nIndex;
		sv_RxBuffer[nPortIndex].State = TxRxState::CHECKSUML;
	}
		break;
	case TxRxState::CHECKSUML: {
		auto nIndex = sv_RxBuffer[nPortIndex].Rdm.nIndex;
		sv_RxBuffer[nPortIndex].Rdm.data[nIndex] = data;
		nIndex |= 0x4000;
		sv_RxBuffer[nPortIndex].Rdm.nIndex = nIndex;
		sv_RxBuffer[nPortIndex].State = TxRxState::IDLE;
		gsv_RdmDataReceiveEnd = DWT->CYCCNT;
	}
		break;
	case TxRxState::RDMDISC: {
		auto nIndex = sv_RxBuffer[nPortIndex].Rdm.nIndex;

		if (nIndex < 24) {
			sv_RxBuffer[nPortIndex].Rdm.data[nIndex] = data;
			nIndex++;
			sv_RxBuffer[nPortIndex].Rdm.nIndex = nIndex;
		}
	}
		break;
	default:
		sv_RxBuffer[nPortIndex].Dmx.current.nSlotsInPacket = 0;
		sv_RxBuffer[nPortIndex].Rdm.nIndex = 0;
		sv_RxBuffer[nPortIndex].State = TxRxState::IDLE;
		break;
	}
}

extern "C" {
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
#if defined (DMX_USE_USART0)
void USART0_IRQHandler(void) {
	irq_handler_dmx_rdm_input<USART0, config::USART0_PORT>();
}
#endif
#if defined (DMX_USE_USART1)
void USART1_IRQHandler(void) {
	irq_handler_dmx_rdm_input<USART1, config::USART1_PORT>();
}
#endif
#if defined (DMX_USE_USART2)
void USART2_IRQHandler(void) {
	irq_handler_dmx_rdm_input<USART2, config::USART2_PORT>();
}
#endif
#if defined (DMX_USE_UART3)
void UART3_IRQHandler(void) {
	irq_handler_dmx_rdm_input<UART3, config::UART3_PORT>();
}
#endif
#if defined (DMX_USE_UART4)
void UART4_IRQHandler(void) {
	irq_handler_dmx_rdm_input<UART4, config::UART4_PORT>();
}
#endif
#if defined (DMX_USE_USART5)
void USART5_IRQHandler(void) {
	irq_handler_dmx_rdm_input<USART5, config::USART5_PORT>();
}
#endif
#if defined (DMX_USE_UART6)
void UART6_IRQHandler(void) {
	irq_handler_dmx_rdm_input<UART6, config::UART6_PORT>();
}
#endif
#if defined (DMX_USE_UART7)
void UART7_IRQHandler(void) {
	irq_handler_dmx_rdm_input<UART7, config::UART7_PORT>();
}
#endif
#endif
}

static void timer1_config() {
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

#if defined (DMX_USE_USART0)
	timer_channel_output_mode_config(TIMER1, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0, ~0);
	timer_interrupt_enable(TIMER1, TIMER_INT_CH0);
#endif /* DMX_USE_USART0 */

#if defined (DMX_USE_USART1)
	timer_channel_output_mode_config(TIMER1, TIMER_CH_1, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_1, ~0);
	timer_interrupt_enable(TIMER1, TIMER_INT_CH1);
#endif /* DMX_USE_USART1 */

#if defined (DMX_USE_USART2)
	timer_channel_output_mode_config(TIMER1, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_2, ~0);
	timer_interrupt_enable(TIMER1, TIMER_INT_CH2);
#endif /* DMX_USE_USART2 */

#if defined (DMX_USE_UART3)
	timer_channel_output_mode_config(TIMER1, TIMER_CH_3, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_3, ~0);
	timer_interrupt_enable(TIMER1, TIMER_INT_CH3);
#endif /* DMX_USE_UART3 */

	NVIC_SetPriority(TIMER1_IRQn, 0);
	NVIC_EnableIRQ(TIMER1_IRQn);

	timer_enable(TIMER1);
}

static void timer4_config() {
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

#if defined (DMX_USE_UART4)
	timer_channel_output_mode_config(TIMER4, TIMER_CH_0, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0, ~0);
	timer_interrupt_enable(TIMER4, TIMER_INT_CH0);
#endif /* DMX_USE_UART4 */

#if defined (DMX_USE_USART5)
	timer_channel_output_mode_config(TIMER4, TIMER_CH_1, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_1, ~0);
	timer_interrupt_enable(TIMER4, TIMER_INT_CH1);
#endif /* DMX_USE_USART5 */

#if defined (DMX_USE_UART6)
	timer_channel_output_mode_config(TIMER4, TIMER_CH_2, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2, ~0);
	timer_interrupt_enable(TIMER4, TIMER_INT_CH2);
#endif /* DMX_USE_UART6 */

#if defined (DMX_USE_UART7)
	timer_channel_output_mode_config(TIMER4, TIMER_CH_3, TIMER_OC_MODE_ACTIVE);
	timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3, ~0);
	timer_interrupt_enable(TIMER4, TIMER_INT_CH3);
#endif /* DMX_USE_UART7 */

	NVIC_SetPriority(TIMER4_IRQn, 0);
	NVIC_EnableIRQ(TIMER4_IRQn);

	timer_enable(TIMER4);
}

static void usart_dma_config(void) {
	DMA_PARAMETER_STRUCT dma_init_struct;
	rcu_periph_clock_enable(RCU_DMA0);
	rcu_periph_clock_enable(RCU_DMA1);
#if defined (GD32H7XX)
	rcu_periph_clock_enable(RCU_DMAMUX);
#endif
#if defined (DMX_USE_USART0)
	/*
	 * USART 0 TX
	 */
	dma_deinit(USART0_DMAx, USART0_TX_DMA_CHx);
# if defined (GD32H7XX)
	dma_init_struct.request = DMA_REQUEST_USART0_TX;
# endif
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
# if defined (GD32F4XX) || defined (GD32H7XX)
# else
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
# endif
# if defined (GD32H7XX)
	dma_init_struct.periph_addr = (uint32_t) &USART_TDATA(USART0);
# else
	dma_init_struct.periph_addr = USART0 + 0x04U;
# endif
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
# if defined (GD32F4XX) || defined (GD32H7XX)
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
# else
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
# endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(USART0_DMAx, USART0_TX_DMA_CHx, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(USART0_DMAx, USART0_TX_DMA_CHx);
	dma_memory_to_memory_disable(USART0_DMAx, USART0_TX_DMA_CHx);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(USART0_DMAx, USART0_TX_DMA_CHx, USART0_TX_DMA_SUBPERIx);
# endif
	gd32_dma_interrupt_disable<USART0_DMAx, USART0_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
# if !defined (GD32F4XX)
	NVIC_SetPriority(DMA0_Channel3_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel3_IRQn);
# else
	NVIC_SetPriority(DMA1_Channel7_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel7_IRQn);
# endif
#endif /* DMX_USE_USART0 */
#if defined (DMX_USE_USART1)
	/*
	 * USART 1 TX
	 */
	dma_deinit(USART1_DMAx, USART1_TX_DMA_CHx);
# if defined (GD32H7XX)
	dma_init_struct.request = DMA_REQUEST_USART1_TX;
# endif
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
# if defined (GD32F4XX) || defined (GD32H7XX)
# else
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
# endif
# if defined (GD32H7XX)
	dma_init_struct.periph_addr = (uint32_t) &USART_TDATA(USART1);
# else
	dma_init_struct.periph_addr = USART1 + 0x04U;
# endif
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
# if defined (GD32F4XX) || defined (GD32H7XX)
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
# else
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
# endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(USART1_DMAx, USART1_TX_DMA_CHx, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(USART1_DMAx, USART1_TX_DMA_CHx);
	dma_memory_to_memory_disable(USART1_DMAx, USART1_TX_DMA_CHx);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(USART1_DMAx, USART1_TX_DMA_CHx, USART1_TX_DMA_SUBPERIx);
# endif
	gd32_dma_interrupt_disable<USART1_DMAx, USART1_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
	NVIC_SetPriority(DMA0_Channel6_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel6_IRQn);
#endif /* DMX_USE_USART1 */
#if defined (DMX_USE_USART2)
	/*
	 * USART 2 TX
	 */
	dma_deinit(USART2_DMAx, USART2_TX_DMA_CHx);
# if defined (GD32H7XX)
	dma_init_struct.request = DMA_REQUEST_USART2_TX;
# endif
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
# if defined (GD32F4XX) || defined (GD32H7XX)
# else
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
# endif
//# if defined (GD32H7XX)
	dma_init_struct.periph_addr = (uint32_t) &USART_TDATA(USART2);
//# else
//	dma_init_struct.periph_addr = USART2 + 0x04U;
//# endif
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
# if defined (GD32F4XX) || defined (GD32H7XX)
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
# else
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
# endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(USART2_DMAx, USART2_TX_DMA_CHx, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(USART2_DMAx, USART2_TX_DMA_CHx);
	dma_memory_to_memory_disable(USART2_DMAx, USART2_TX_DMA_CHx);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(USART2_DMAx, USART2_TX_DMA_CHx, USART2_TX_DMA_SUBPERIx);
# endif
	gd32_dma_interrupt_disable<USART2_DMAx, USART2_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
#if defined (GD32F4XX) || defined (GD32H7XX)
	NVIC_SetPriority(DMA0_Channel3_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel3_IRQn);
# else
	NVIC_SetPriority(DMA0_Channel1_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel1_IRQn);
# endif
#endif /* DMX_USE_USART2 */
#if defined (DMX_USE_UART3)
	/*
	 * UART 3 TX
	 */
	dma_deinit(UART3_DMAx, UART3_TX_DMA_CHx);
# if defined (GD32H7XX)
	dma_init_struct.request = DMA_REQUEST_UART3_TX;
# endif
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
# if defined (GD32F4XX) || defined (GD32H7XX)
# else
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
# endif
# if defined (GD32H7XX)
	dma_init_struct.periph_addr = (uint32_t) &USART_TDATA(UART3);
# else
	dma_init_struct.periph_addr = UART3 + 0x04U;
# endif
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
# if defined (GD32F4XX) || defined (GD32H7XX)
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
# else
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
# endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(UART3_DMAx, UART3_TX_DMA_CHx, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(UART3_DMAx, UART3_TX_DMA_CHx);
	dma_memory_to_memory_disable(UART3_DMAx, UART3_TX_DMA_CHx);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(UART3_DMAx, UART3_TX_DMA_CHx, UART3_TX_DMA_SUBPERIx);
# endif
	gd32_dma_interrupt_disable<UART3_DMAx, UART3_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
# if !defined (GD32F4XX)
	NVIC_SetPriority(DMA1_Channel4_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel4_IRQn);
# else
	NVIC_SetPriority(DMA0_Channel4_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel4_IRQn);
# endif
#endif /* DMX_USE_UART3 */
#if defined (DMX_USE_UART4)
	/*
	 * UART 4 TX
	 */
	dma_deinit(UART4_DMAx, UART4_TX_DMA_CHx);
# if defined (GD32H7XX)
	dma_init_struct.request = DMA_REQUEST_UART4_TX;
# endif
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
# if defined (GD32F4XX) || defined (GD32H7XX)
# else
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
# endif
# if defined (GD32H7XX)
	dma_init_struct.periph_addr = (uint32_t) &USART_TDATA(UART4);
# else
	dma_init_struct.periph_addr = UART4 + 0x04U;
# endif
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined (GD32F4XX) || defined (GD32H7XX)
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(UART4_DMAx, UART4_TX_DMA_CHx, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(UART4_DMAx, UART4_TX_DMA_CHx);
	dma_memory_to_memory_disable(UART4_DMAx, UART4_TX_DMA_CHx);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(UART4_DMAx, UART4_TX_DMA_CHx, UART4_TX_DMA_SUBPERIx);
# endif
	gd32_dma_interrupt_disable<UART4_DMAx, UART4_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
# if !defined (GD32F4XX)
	NVIC_SetPriority(DMA1_Channel3_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel3_IRQn);
# else
	NVIC_SetPriority(DMA0_Channel7_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel7_IRQn);
# endif
#endif /* DMX_USE_UART4 */
#if defined (DMX_USE_USART5)
	/*
	 * USART 5 TX
	 */
	dma_deinit(USART5_DMAx, USART5_TX_DMA_CHx);
# if defined (GD32H7XX)
	dma_init_struct.request = DMA_REQUEST_USART5_TX;
# endif
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
# if defined (GD32F20X)
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
# endif
# if defined (GD32H7XX)
	dma_init_struct.periph_addr = (uint32_t) &USART_TDATA(USART5);
# else
	dma_init_struct.periph_addr = USART5 + 0x04U;
# endif
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined (GD32F20X)
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
# else
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(USART5_DMAx, USART5_TX_DMA_CHx, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(USART5_DMAx, USART5_TX_DMA_CHx);
	dma_memory_to_memory_disable(USART5_DMAx, USART5_TX_DMA_CHx);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(USART5_DMAx, USART5_TX_DMA_CHx, USART5_TX_DMA_SUBPERIx);
# endif
	gd32_dma_interrupt_disable<USART5_DMAx, USART5_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
	NVIC_SetPriority(DMA1_Channel6_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel6_IRQn);
#endif /* DMX_USE_USART5 */
#if defined (DMX_USE_UART6)
	/*
	 * UART 6 TX
	 */
	dma_deinit(UART6_DMAx, UART6_TX_DMA_CHx);
# if defined (GD32H7XX)
	dma_init_struct.request = DMA_REQUEST_UART6_TX;
# endif
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
# if defined (GD32F20X)
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
# endif
# if defined (GD32H7XX)
	dma_init_struct.periph_addr = (uint32_t) &USART_TDATA(UART6);
# else
	dma_init_struct.periph_addr = UART6 + 0x04U;
# endif
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
# if defined (GD32F20X)
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
# else
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
# endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(UART6_DMAx, UART6_TX_DMA_CHx, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(UART6_DMAx, UART6_TX_DMA_CHx);
	dma_memory_to_memory_disable(UART6_DMAx, UART6_TX_DMA_CHx);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(UART6_DMAx, UART6_TX_DMA_CHx, UART6_TX_DMA_SUBPERIx);
# endif
	gd32_dma_interrupt_disable<UART6_DMAx, UART4_TX_DMA_CHx, DMA_INTERRUPT_DISABLE>();
# if defined (GD32F20X)
	NVIC_SetPriority(DMA1_Channel4_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel4_IRQn);
# else
	NVIC_SetPriority(DMA0_Channel1_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel1_IRQn);
# endif
#endif /* DMX_USE_UART6 */
#if defined (DMX_USE_UART7)
	/*
	 * UART 7 TX
	 */
	dma_deinit(UART7_DMAx, UART7_TX_DMA_CHx);
# if defined (GD32H7XX)
	dma_init_struct.request = DMA_REQUEST_UART7_TX;
# endif
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
# if defined (GD32F20X)
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
# endif
# if defined (GD32H7XX)
	dma_init_struct.periph_addr = (uint32_t) &USART_TDATA(UART7);
# else
	dma_init_struct.periph_addr = UART7 + 0x04U;
# endif
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
# if defined (GD32F20X)
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
# else
	dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
# endif
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init(UART7_DMAx, UART7_TX_DMA_CHx, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(UART7_DMAx, UART7_TX_DMA_CHx);
	dma_memory_to_memory_disable(UART7_DMAx, UART7_TX_DMA_CHx);
# if defined (GD32F4XX)
	dma_channel_subperipheral_select(UART7_DMAx, UART7_TX_DMA_CHx, UART7_TX_DMA_SUBPERIx);
# endif
#if defined (GD32F20X)
	NVIC_SetPriority(DMA1_Channel3_IRQn, 1);
	NVIC_EnableIRQ(DMA1_Channel3_IRQn);
# else
	NVIC_SetPriority(DMA0_Channel0_IRQn, 1);
	NVIC_EnableIRQ(DMA0_Channel0_IRQn);
# endif
#endif /* DMX_USE_UART7 */
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
			gd32_gpio_mode_output<USART0_GPIOx, USART0_TX_GPIO_PINx>();
			GPIO_BC(USART0_GPIOx) = USART0_TX_GPIO_PINx;
			s_TxBuffer[dmx::config::USART0_PORT].State = TxRxState::BREAK;
			TIMER_CH0CV(TIMER1) =  TIMER_CNT(TIMER1) + s_nDmxTransmit.nBreakTime;
			break;
		case TxRxState::BREAK:
			gd32_gpio_mode_af<USART0_GPIOx, USART0_TX_GPIO_PINx, USART0>();
			s_TxBuffer[dmx::config::USART0_PORT].State = TxRxState::MAB;
			TIMER_CH0CV(TIMER1) =  TIMER_CNT(TIMER1) + s_nDmxTransmit.nMabTime;
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(USART0_DMAx, USART0_TX_DMA_CHx);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(USART0_DMAx, USART0_TX_DMA_CHx) = dmaCHCTL;
			gd32_dma_interrupt_flag_clear<USART0_DMAx, USART0_TX_DMA_CHx, DMA_INTF_FTFIF>();
			const auto *p = &s_TxBuffer[dmx::config::USART0_PORT].dmx;
			DMA_CHMADDR(USART0_DMAx, USART0_TX_DMA_CHx) = (uint32_t) p->data;
			DMA_CHCNT(USART0_DMAx, USART0_TX_DMA_CHx) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(USART0_DMAx, USART0_TX_DMA_CHx) = dmaCHCTL;
			USART_CTL2(USART0) |= USART_TRANSMIT_DMA_ENABLE;
		}
		break;
		default:
			break;
		}

		TIMER_INTF(TIMER1) = static_cast<uint32_t>(~TIMER_INT_FLAG_CH0);
	}
#endif
	/*
	 * USART 1
	 */
#if defined (DMX_USE_USART1)
	if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH1) == TIMER_INT_FLAG_CH1) {
		switch (s_TxBuffer[dmx::config::USART1_PORT].State) {
		case TxRxState::DMXINTER:
			gd32_gpio_mode_output<USART1_GPIOx, USART1_TX_GPIO_PINx>();
			GPIO_BC(USART1_GPIOx) = USART1_TX_GPIO_PINx;
			s_TxBuffer[dmx::config::USART1_PORT].State = TxRxState::BREAK;
			TIMER_CH1CV(TIMER1) = TIMER_CNT(TIMER1) + s_nDmxTransmit.nBreakTime;
			break;
		case TxRxState::BREAK:
			gd32_gpio_mode_af<USART1_GPIOx, USART1_TX_GPIO_PINx, USART1>();
			s_TxBuffer[dmx::config::USART1_PORT].State = TxRxState::MAB;
			TIMER_CH1CV(TIMER1) =  TIMER_CNT(TIMER1) + s_nDmxTransmit.nMabTime;
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(USART1_DMAx, USART1_TX_DMA_CHx);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(USART1_DMAx, USART1_TX_DMA_CHx) = dmaCHCTL;
			gd32_dma_interrupt_flag_clear<USART1_DMAx, USART1_TX_DMA_CHx, DMA_INTF_FTFIF>();
			const auto *p = &s_TxBuffer[dmx::config::USART1_PORT].dmx;
			DMA_CHMADDR(USART1_DMAx, USART1_TX_DMA_CHx) = (uint32_t) p->data;
			DMA_CHCNT(USART1_DMAx, USART1_TX_DMA_CHx) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(USART1_DMAx, USART1_TX_DMA_CHx) = dmaCHCTL;
			USART_CTL2(USART1) |= USART_TRANSMIT_DMA_ENABLE;
		}
		break;
		default:
			break;
		}

		TIMER_INTF(TIMER1) = static_cast<uint32_t>(~TIMER_INT_FLAG_CH1);
	}
#endif
	/*
	 * USART 2
	 */
#if defined (DMX_USE_USART2)
	if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH2) == TIMER_INT_FLAG_CH2) {
		switch (s_TxBuffer[dmx::config::USART2_PORT].State) {
		case TxRxState::DMXINTER:
			gd32_gpio_mode_output<USART2_GPIOx, USART2_TX_GPIO_PINx>();
			GPIO_BC(USART2_GPIOx) = USART2_TX_GPIO_PINx;
			s_TxBuffer[dmx::config::USART2_PORT].State = TxRxState::BREAK;
			TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + s_nDmxTransmit.nBreakTime;
			break;
		case TxRxState::BREAK:
			gd32_gpio_mode_af<USART2_GPIOx, USART2_TX_GPIO_PINx, USART2>();
			s_TxBuffer[dmx::config::USART2_PORT].State = TxRxState::MAB;
			TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + s_nDmxTransmit.nMabTime;
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(USART2_DMAx, USART2_TX_DMA_CHx);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(USART2_DMAx, USART2_TX_DMA_CHx) = dmaCHCTL;
			gd32_dma_interrupt_flag_clear<USART2_DMAx, USART2_TX_DMA_CHx, DMA_INTF_FTFIF>();
			const auto *p = &s_TxBuffer[dmx::config::USART2_PORT].dmx;
			DMA_CHMADDR(USART2_DMAx, USART2_TX_DMA_CHx) = (uint32_t) p->data;
			DMA_CHCNT(USART2_DMAx, USART2_TX_DMA_CHx) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(USART2_DMAx, USART2_TX_DMA_CHx) = dmaCHCTL;
			USART_CTL2(USART2) |= USART_TRANSMIT_DMA_ENABLE;
		}
		break;
		default:
			break;
		}

		TIMER_INTF(TIMER1) = static_cast<uint32_t>(~TIMER_INT_FLAG_CH2);
	}
#endif
	/*
	 * UART 3
	 */
#if defined (DMX_USE_UART3)
	if ((TIMER_INTF(TIMER1) & TIMER_INT_FLAG_CH3) == TIMER_INT_FLAG_CH3) {
		switch (s_TxBuffer[dmx::config::UART3_PORT].State) {
		case TxRxState::DMXINTER:
			gd32_gpio_mode_output<UART3_GPIOx, UART3_TX_GPIO_PINx>();
			GPIO_BC(UART3_GPIOx) = UART3_TX_GPIO_PINx;
			s_TxBuffer[dmx::config::UART3_PORT].State = TxRxState::BREAK;
			TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + s_nDmxTransmit.nBreakTime;
			break;
		case TxRxState::BREAK:
			gd32_gpio_mode_af<UART3_GPIOx, UART3_TX_GPIO_PINx, UART3>();
			s_TxBuffer[dmx::config::UART3_PORT].State = TxRxState::MAB;
			TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + s_nDmxTransmit.nMabTime;
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(UART3_DMAx, UART3_TX_DMA_CHx);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(UART3_DMAx, UART3_TX_DMA_CHx) = dmaCHCTL;
			gd32_dma_interrupt_flag_clear<UART3_DMAx, UART3_TX_DMA_CHx, DMA_INTF_FTFIF>();
			const auto *p = &s_TxBuffer[dmx::config::UART3_PORT].dmx;
			DMA_CHMADDR(UART3_DMAx, UART3_TX_DMA_CHx) = (uint32_t) p->data;
			DMA_CHCNT(UART3_DMAx, UART3_TX_DMA_CHx) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(UART3_DMAx, UART3_TX_DMA_CHx) = dmaCHCTL;
			USART_CTL2(UART3) |= USART_TRANSMIT_DMA_ENABLE;
		}
		break;
		default:
			break;
		}

		TIMER_INTF(TIMER1) = static_cast<uint32_t>(~TIMER_INT_FLAG_CH3);
	}
#endif
	// Clear all remaining interrupt flags (safety measure)
	TIMER_INTF(TIMER1) = static_cast<uint32_t>(~0);
}

void TIMER4_IRQHandler() {
	/*
	 * UART 4
	 */
#if defined (DMX_USE_UART4)
	if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH0) == TIMER_INT_FLAG_CH0) {
		switch (s_TxBuffer[dmx::config::UART4_PORT].State) {
		case TxRxState::DMXINTER:
			gd32_gpio_mode_output<UART4_TX_GPIOx, UART4_TX_GPIO_PINx>();
			GPIO_BC(UART4_TX_GPIOx) = UART4_TX_GPIO_PINx;
			s_TxBuffer[dmx::config::UART4_PORT].State = TxRxState::BREAK;
			TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + s_nDmxTransmit.nBreakTime;
			break;
		case TxRxState::BREAK:
			gd32_gpio_mode_af<UART4_TX_GPIOx, UART4_TX_GPIO_PINx, UART4>();
			s_TxBuffer[dmx::config::UART4_PORT].State = TxRxState::MAB;
			TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + s_nDmxTransmit.nMabTime;
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(UART4_DMAx, UART4_TX_DMA_CHx);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(UART4_DMAx, UART4_TX_DMA_CHx) = dmaCHCTL;
			gd32_dma_interrupt_flag_clear<UART4_DMAx, UART4_TX_DMA_CHx, DMA_INTF_FTFIF>();
			const auto *p = &s_TxBuffer[dmx::config::UART4_PORT].dmx;
			DMA_CHMADDR(UART4_DMAx, UART4_TX_DMA_CHx) = (uint32_t) p->data;
			DMA_CHCNT(UART4_DMAx, UART4_TX_DMA_CHx) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(UART4_DMAx, UART4_TX_DMA_CHx) = dmaCHCTL;
			USART_CTL2(UART4) |= USART_TRANSMIT_DMA_ENABLE;
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
#if defined (DMX_USE_USART5)
	if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH1) == TIMER_INT_FLAG_CH1) {
		switch (s_TxBuffer[dmx::config::USART5_PORT].State) {
		case TxRxState::DMXINTER:
			gd32_gpio_mode_output<USART5_GPIOx, USART5_TX_GPIO_PINx>();
			GPIO_BC(USART5_GPIOx) = USART5_TX_GPIO_PINx;
			s_TxBuffer[dmx::config::USART5_PORT].State = TxRxState::BREAK;
			TIMER_CH1CV(TIMER4) = TIMER_CNT(TIMER4) + s_nDmxTransmit.nBreakTime;
			break;
		case TxRxState::BREAK:
			gd32_gpio_mode_af<USART5_GPIOx, USART5_TX_GPIO_PINx, USART5>();
			s_TxBuffer[dmx::config::USART5_PORT].State = TxRxState::MAB;
			TIMER_CH1CV(TIMER4) = TIMER_CNT(TIMER4) + s_nDmxTransmit.nMabTime;
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(USART5_DMAx, USART5_TX_DMA_CHx);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(USART5_DMAx, USART5_TX_DMA_CHx) = dmaCHCTL;
			gd32_dma_interrupt_flag_clear<USART5_DMAx, USART5_TX_DMA_CHx, DMA_INTF_FTFIF>();
			const auto *p = &s_TxBuffer[dmx::config::USART5_PORT].dmx;
			DMA_CHMADDR(USART5_DMAx, USART5_TX_DMA_CHx) = (uint32_t) p->data;
			DMA_CHCNT(USART5_DMAx, USART5_TX_DMA_CHx) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(USART5_DMAx, USART5_TX_DMA_CHx) = dmaCHCTL;
			USART_CTL2(USART5) |= USART_TRANSMIT_DMA_ENABLE;
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
#if defined (DMX_USE_UART6)
	if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH2) == TIMER_INT_FLAG_CH2) {
		switch (s_TxBuffer[dmx::config::UART6_PORT].State) {
		case TxRxState::DMXINTER:
			gd32_gpio_mode_output<UART6_GPIOx, UART6_TX_GPIO_PINx>();
			GPIO_BC(UART6_GPIOx) = UART6_TX_GPIO_PINx;
			s_TxBuffer[dmx::config::UART6_PORT].State = TxRxState::BREAK;
			TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + s_nDmxTransmit.nBreakTime;
			break;
		case TxRxState::BREAK:
			gd32_gpio_mode_af<UART6_GPIOx, UART6_TX_GPIO_PINx, UART6>();
			s_TxBuffer[dmx::config::UART6_PORT].State = TxRxState::MAB;
			TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + s_nDmxTransmit.nMabTime;
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(UART6_DMAx, UART6_TX_DMA_CHx);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(UART6_DMAx, UART6_TX_DMA_CHx)= dmaCHCTL;
			gd32_dma_interrupt_flag_clear<UART6_DMAx, UART6_TX_DMA_CHx, DMA_INTF_FTFIF>();
			const auto *p = &s_TxBuffer[dmx::config::UART6_PORT].dmx;
			DMA_CHMADDR(UART6_DMAx, UART6_TX_DMA_CHx) = (uint32_t) p->data;
			DMA_CHCNT(UART6_DMAx, UART6_TX_DMA_CHx) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(UART6_DMAx, UART6_TX_DMA_CHx)= dmaCHCTL;
			USART_CTL2(UART6) |= USART_TRANSMIT_DMA_ENABLE;
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
#if defined (DMX_USE_UART7)
	if ((TIMER_INTF(TIMER4) & TIMER_INT_FLAG_CH3) == TIMER_INT_FLAG_CH3) {
		switch (s_TxBuffer[dmx::config::UART7_PORT].State) {
		case TxRxState::DMXINTER:
			gd32_gpio_mode_output<UART7_GPIOx, UART7_TX_GPIO_PINx>();
			GPIO_BC(UART7_GPIOx) = UART7_TX_GPIO_PINx;
			s_TxBuffer[dmx::config::UART7_PORT].State = TxRxState::BREAK;
			TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + s_nDmxTransmit.nBreakTime;
			break;
		case TxRxState::BREAK:
			gd32_gpio_mode_af<UART7_GPIOx, UART7_TX_GPIO_PINx, UART7>();
			s_TxBuffer[dmx::config::UART7_PORT].State = TxRxState::MAB;
			TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + s_nDmxTransmit.nMabTime;
			break;
		case TxRxState::MAB: {
			uint32_t dmaCHCTL = DMA_CHCTL(UART7_DMAx, UART7_TX_DMA_CHx);
			dmaCHCTL &= ~DMA_CHXCTL_CHEN;
			DMA_CHCTL(UART7_DMAx, UART7_TX_DMA_CHx) = dmaCHCTL;
			gd32_dma_interrupt_flag_clear<UART7_DMAx, UART7_TX_DMA_CHx, DMA_INTF_FTFIF>();
			const auto *p = &s_TxBuffer[dmx::config::UART7_PORT].dmx;
			DMA_CHMADDR(UART7_DMAx, UART7_TX_DMA_CHx) = (uint32_t) p->data;
			DMA_CHCNT(UART7_DMAx, UART7_TX_DMA_CHx) = (p->nLength & DMA_CHXCNT_CNT);
			dmaCHCTL |= DMA_CHXCTL_CHEN;
			dmaCHCTL |= DMA_INTERRUPT_ENABLE;
			DMA_CHCTL(UART7_DMAx, UART7_TX_DMA_CHx)= dmaCHCTL;
			USART_CTL2(UART7) |= USART_TRANSMIT_DMA_ENABLE;
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

void TIMER6_IRQHandler() {
	const auto nIntFlag = TIMER_INTF(TIMER6);

	if ((nIntFlag & TIMER_INT_FLAG_UP) == TIMER_INT_FLAG_UP) {
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
		for (uint32_t i = 0; i < DMX_MAX_PORTS; i++) {
			auto &packet = sv_nRxDmxPackets[i];
			packet.nPerSecond = sv_nRxDmxPackets[i].nCount - packet.nCountPrevious;
			packet.nCountPrevious = packet.nCount;
		}
#endif
		g_Seconds.nUptime++;
	}

	// Clear all remaining interrupt flags (safety measure)
	TIMER_INTF(TIMER6) = static_cast<uint32_t>(~nIntFlag);
}
}

extern "C" {
/*
 * USART 0
 */
#if defined (DMX_USE_USART0)
# if defined (GD32F4XX) || defined (GD32H7XX)
void DMA1_Channel7_IRQHandler() {
	if (gd32_dma_interrupt_flag_get<DMA1, DMA_CH7, DMA_INTERRUPT_FLAG_GET>()) {
		gd32_dma_interrupt_disable<DMA1, DMA_CH7, DMA_INTERRUPT_DISABLE>();

		if (s_TxBuffer[dmx::config::USART0_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::USART0_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0 , TIMER_CNT(TIMER1) + s_nDmxTransmit.nInterTime);
			s_TxBuffer[dmx::config::USART0_PORT].State = TxRxState::DMXINTER;
		}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[dmx::config::USART0_PORT].Dmx.Sent++;
#endif
	}

	gd32_dma_interrupt_flag_clear<DMA1, DMA_CH7, DMA_INTERRUPT_FLAG_CLEAR>();
}
# else
void DMA0_Channel3_IRQHandler() {
	if (gd32_dma_interrupt_flag_get<DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_GET>()) {
		gd32_dma_interrupt_disable<DMA0, DMA_CH3, DMA_INTERRUPT_DISABLE>();

		if (s_TxBuffer[dmx::config::USART0_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::USART0_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_0 , TIMER_CNT(TIMER1) + s_nDmxTransmit.nInterTime);
			s_TxBuffer[dmx::config::USART0_PORT].State = TxRxState::DMXINTER;
		}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[dmx::config::USART0_PORT].Dmx.Sent++;
#endif
	}

	gd32_dma_interrupt_flag_clear<DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR>();
}
# endif
#endif
/*
 * USART 1
 */
#if defined (DMX_USE_USART1)
void DMA0_Channel6_IRQHandler() {
	if (gd32_dma_interrupt_flag_get<DMA0, DMA_CH6, DMA_INTERRUPT_FLAG_GET>()) {
		gd32_dma_interrupt_disable<DMA0, DMA_CH6, DMA_INTERRUPT_DISABLE>();

		if (s_TxBuffer[dmx::config::USART1_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::USART1_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_1 , TIMER_CNT(TIMER1) + s_nDmxTransmit.nInterTime);
			s_TxBuffer[dmx::config::USART1_PORT].State = TxRxState::DMXINTER;
		}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[dmx::config::USART1_PORT].Dmx.Sent++;
#endif
	}

	gd32_dma_interrupt_flag_clear<DMA0, DMA_CH6, DMA_INTERRUPT_FLAG_CLEAR>();
}
#endif
/*
 * USART 2
 */
#if defined (DMX_USE_USART2)
# if defined (GD32F4XX) || defined (GD32H7XX)
void DMA0_Channel3_IRQHandler() {
	if (gd32_dma_interrupt_flag_get<DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_GET>()) {
		gd32_dma_interrupt_disable<DMA0, DMA_CH3, DMA_INTERRUPT_DISABLE>();

		if (s_TxBuffer[dmx::config::USART2_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::USART2_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_2 , TIMER_CNT(TIMER1) + s_nDmxTransmit.nInterTime);
			s_TxBuffer[dmx::config::USART2_PORT].State = TxRxState::DMXINTER;
		}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[dmx::config::USART2_PORT].Dmx.Sent++;
#endif
	}

	gd32_dma_interrupt_flag_clear<DMA0, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR>();
}
# else
void DMA0_Channel1_IRQHandler() {
	if (gd32_dma_interrupt_flag_get<DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_GET>()) {
		gd32_dma_interrupt_disable<DMA0, DMA_CH1, DMA_INTERRUPT_DISABLE>();

		if (s_TxBuffer[dmx::config::USART2_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::USART2_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_2 , TIMER_CNT(TIMER1) + s_nDmxTransmit.nInterTime);
			s_TxBuffer[dmx::config::USART2_PORT].State = TxRxState::DMXINTER;
		}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[dmx::config::USART2_PORT].Dmx.Sent++;
#endif
	}

	gd32_dma_interrupt_flag_clear<DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_CLEAR>();
}
# endif
#endif
/*
 * UART 3
 */
#if defined (DMX_USE_UART3)
# if defined (GD32F4XX) || defined (GD32H7XX)
void DMA0_Channel4_IRQHandler() {
	if (gd32_dma_interrupt_flag_get<DMA0, DMA_CH4, DMA_INTERRUPT_FLAG_GET>()) {
		gd32_dma_interrupt_disable<DMA0, DMA_CH4, DMA_INTERRUPT_DISABLE>();

		if (s_TxBuffer[dmx::config::UART3_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART3_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3 , TIMER_CNT(TIMER4) + s_nDmxTransmit.nInterTime);
			s_TxBuffer[dmx::config::UART3_PORT].State = TxRxState::DMXINTER;
		}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[dmx::config::UART3_PORT].Dmx.Sent++;
#endif
	}

	gd32_dma_interrupt_flag_clear<DMA0, DMA_CH4, DMA_INTERRUPT_FLAG_CLEAR>();
}
# else
void DMA1_Channel4_IRQHandler() {
	if (gd32_dma_interrupt_flag_get<DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_GET>()) {
		gd32_dma_interrupt_disable<DMA1, DMA_CH4, DMA_INTERRUPT_DISABLE>();

		if (s_TxBuffer[dmx::config::UART3_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART3_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER1, TIMER_CH_3 , TIMER_CNT(TIMER1) + s_nDmxTransmit.nInterTime);
			s_TxBuffer[dmx::config::UART3_PORT].State = TxRxState::DMXINTER;
		}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[dmx::config::UART3_PORT].Dmx.Sent++;
#endif
	}

	gd32_dma_interrupt_flag_clear<DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_CLEAR>();
}
# endif
#endif
/*
 * UART 4
 */
#if defined (DMX_USE_UART4)
# if defined (GD32F20X)
void DMA1_Channel3_IRQHandler() {
	if (gd32_dma_interrupt_flag_get<DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_GET>()) {
		gd32_dma_interrupt_disable<DMA1, DMA_CH3, DMA_INTERRUPT_DISABLE>();

		if (s_TxBuffer[dmx::config::UART4_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART4_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0 , TIMER_CNT(TIMER4) + s_nDmxTransmit.nInterTime);
			s_TxBuffer[dmx::config::UART4_PORT].State = TxRxState::DMXINTER;
		}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[dmx::config::UART4_PORT].Dmx.Sent++;
#endif
	}

	gd32_dma_interrupt_flag_clear<DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR>();
}
# endif
# if defined (GD32F4XX) || defined (GD32H7XX)
void DMA0_Channel7_IRQHandler() {
	if (gd32_dma_interrupt_flag_get<DMA0, DMA_CH7, DMA_INTERRUPT_FLAG_GET>()) {
		gd32_dma_interrupt_disable<DMA0, DMA_CH7, DMA_INTERRUPT_DISABLE>();

		if (s_TxBuffer[dmx::config::UART4_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART4_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_0 , TIMER_CNT(TIMER4) + s_nDmxTransmit.nInterTime);
			s_TxBuffer[dmx::config::UART4_PORT].State = TxRxState::DMXINTER;
		}
	}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
	sv_TotalStatistics[dmx::config::UART4_PORT].Dmx.Sent++;
#endif

	gd32_dma_interrupt_flag_clear<DMA0, DMA_CH7, DMA_INTERRUPT_FLAG_CLEAR>();
}
# endif
#endif
/*
 * USART 5
 */
#if defined (DMX_USE_USART5)
void DMA1_Channel6_IRQHandler() {
	if (gd32_dma_interrupt_flag_get<DMA1, DMA_CH6, DMA_INTERRUPT_FLAG_GET>()) {
		gd32_dma_interrupt_disable<DMA1, DMA_CH6, DMA_INTERRUPT_DISABLE>();

		if (s_TxBuffer[dmx::config::USART5_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::USART5_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_1 , TIMER_CNT(TIMER4) + s_nDmxTransmit.nInterTime);
			s_TxBuffer[dmx::config::USART5_PORT].State = TxRxState::DMXINTER;
		}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[dmx::config::USART5_PORT].Dmx.Sent++;
#endif
	}

	gd32_dma_interrupt_flag_clear<DMA1, DMA_CH6, DMA_INTERRUPT_FLAG_CLEAR>();
}
#endif
/*
 * UART 6
 */
#if defined (DMX_USE_UART6)
# if defined (GD32F20X)
void DMA1_Channel4_IRQHandler() {
	if (gd32_dma_interrupt_flag_get<DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_GET>()) {
		gd32_dma_interrupt_disable<DMA1, DMA_CH4, DMA_INTERRUPT_DISABLE>();

		if (s_TxBuffer[dmx::config::UART6_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART6_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2 , TIMER_CNT(TIMER4) + s_nDmxTransmit.nInterTime);
			s_TxBuffer[dmx::config::UART6_PORT].State = TxRxState::DMXINTER;
		}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[dmx::config::UART6_PORT].Dmx.Sent++;
#endif
	}

	gd32_dma_interrupt_flag_clear<DMA1, DMA_CH4, DMA_INTERRUPT_FLAG_CLEAR>();
}
# endif
# if defined (GD32F4XX) || defined (GD32H7XX)
void DMA0_Channel1_IRQHandler() {
	if (gd32_dma_interrupt_flag_get<DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_GET>()) {
		gd32_dma_interrupt_disable<DMA0, DMA_CH1, DMA_INTERRUPT_DISABLE>();

		if (s_TxBuffer[dmx::config::UART6_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART6_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_2 , TIMER_CNT(TIMER4) + s_nDmxTransmit.nInterTime);
			s_TxBuffer[dmx::config::UART6_PORT].State = TxRxState::DMXINTER;
		}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[dmx::config::UART6_PORT].Dmx.Sent++;
#endif
	}

	gd32_dma_interrupt_flag_clear<DMA0, DMA_CH1, DMA_INTERRUPT_FLAG_CLEAR>();
}
# endif
#endif
/*
 * UART 7
 */
#if defined (DMX_USE_UART7)
# if defined (GD32F20X)
void DMA1_Channel3_IRQHandler() {
	if (gd32_dma_interrupt_flag_get<DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_GET>()) {
		gd32_dma_interrupt_disable<DMA1, DMA_CH3, DMA_INTERRUPT_DISABLE>();

		if (s_TxBuffer[dmx::config::UART7_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART7_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3 , TIMER_CNT(TIMER4) + s_nDmxTransmit.nInterTime);
			s_TxBuffer[dmx::config::UART7_PORT].State = TxRxState::DMXINTER;
		}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[dmx::config::UART7_PORT].Dmx.Sent++;
#endif
	}

	gd32_dma_interrupt_flag_clear<DMA1, DMA_CH3, DMA_INTERRUPT_FLAG_CLEAR>();
}
# endif
# if defined (GD32F4XX) || defined (GD32H7XX)
void DMA0_Channel0_IRQHandler() {
	if (gd32_dma_interrupt_flag_get<DMA0, DMA_CH0, DMA_INTERRUPT_FLAG_GET>()) {
		gd32_dma_interrupt_disable<DMA0, DMA_CH0, DMA_INTERRUPT_DISABLE>();

		if (s_TxBuffer[dmx::config::UART7_PORT].outputStyle == dmx::OutputStyle::DELTA) {
			s_TxBuffer[dmx::config::UART7_PORT].State = TxRxState::IDLE;
		} else {
			timer_channel_output_pulse_value_config(TIMER4, TIMER_CH_3 , TIMER_CNT(TIMER4) + s_nDmxTransmit.nInterTime);
			s_TxBuffer[dmx::config::UART7_PORT].State = TxRxState::DMXINTER;
		}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[dmx::config::UART7_PORT].Dmx.Sent++;
#endif
	}

	gd32_dma_interrupt_flag_clear<DMA0, DMA_CH0, DMA_INTERRUPT_FLAG_CLEAR>();
}
# endif
#endif
}

static void uart_dmx_config(const uint32_t usart_periph) {
	gd32_uart_begin(usart_periph, 250000U, GD32_UART_BITS_8, GD32_UART_PARITY_NONE, GD32_UART_STOP_2BITS);
}

Dmx *Dmx::s_pThis;

Dmx::Dmx() {
	DEBUG_ENTRY

	assert(s_pThis == nullptr);
	s_pThis = this;

	s_nDmxTransmit.nBreakTime = dmx::transmit::BREAK_TIME_TYPICAL;
	s_nDmxTransmit.nMabTime = dmx::transmit::MAB_TIME_MIN;
	s_nDmxTransmit.nInterTime = dmx::transmit::PERIOD_DEFAULT - s_nDmxTransmit.nBreakTime - s_nDmxTransmit.nMabTime - (dmx::max::CHANNELS * 44) - 44;

	for (auto i = 0; i < DMX_MAX_PORTS; i++) {
#if defined (GPIO_INIT)
		gpio_init(s_DirGpio[i].nPort, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, s_DirGpio[i].nPin);
#else
		gpio_mode_set(s_DirGpio[i].nPort, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, s_DirGpio[i].nPin);
		gpio_output_options_set(s_DirGpio[i].nPort, GPIO_OTYPE_PP, GPIO_OSPEED, s_DirGpio[i].nPin);
#endif
		m_nDmxTransmissionLength[i] = dmx::max::CHANNELS;
		SetPortDirection(i, PortDirection::INP, false);
		sv_RxBuffer[i].State = TxRxState::IDLE;
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

#if defined (DMX_USE_USART0)
	uart_dmx_config(USART0);
	NVIC_SetPriority(USART0_IRQn, 0);
	NVIC_EnableIRQ(USART0_IRQn);
#endif
#if defined (DMX_USE_USART1)
	uart_dmx_config(USART1);
	NVIC_SetPriority(USART1_IRQn, 0);
	NVIC_EnableIRQ(USART1_IRQn);
#endif
#if defined (DMX_USE_USART2)
	uart_dmx_config(USART2);
	NVIC_SetPriority(USART2_IRQn, 0);
	NVIC_EnableIRQ(USART2_IRQn);
#endif
#if defined (DMX_USE_UART3)
	uart_dmx_config(UART3);
	NVIC_SetPriority(UART3_IRQn, 0);
	NVIC_EnableIRQ(UART3_IRQn);
#endif
#if defined (DMX_USE_UART4)
	uart_dmx_config(UART4);
	NVIC_SetPriority(UART4_IRQn, 0);
	NVIC_EnableIRQ(UART4_IRQn);
#endif
#if defined (DMX_USE_USART5)
	uart_dmx_config(USART5);
	NVIC_SetPriority(USART5_IRQn, 0);
	NVIC_EnableIRQ(USART5_IRQn);
#endif
#if defined (DMX_USE_UART6)
	uart_dmx_config(UART6);
	NVIC_SetPriority(UART6_IRQn, 0);
	NVIC_EnableIRQ(UART6_IRQn);
#endif
#if defined (DMX_USE_UART7)
	uart_dmx_config(UART7);
	NVIC_SetPriority(UART7_IRQn, 0);
	NVIC_EnableIRQ(UART7_IRQn);
#endif

	DEBUG_EXIT
}

void Dmx::SetPortDirection(const uint32_t nPortIndex, PortDirection portDirection, bool bEnableData) {
	assert(nPortIndex < dmx::config::max::PORTS);

	if (m_dmxPortDirection[nPortIndex] != portDirection) {
		m_dmxPortDirection[nPortIndex] = portDirection;

		StopData(nPortIndex);

		if (portDirection == PortDirection::OUTP) {
			GPIO_BOP(s_DirGpio[nPortIndex].nPort) = s_DirGpio[nPortIndex].nPin;
		} else if (portDirection == PortDirection::INP) {
			GPIO_BC(s_DirGpio[nPortIndex].nPort) = s_DirGpio[nPortIndex].nPin;
		} else {
			assert(0);
		}
	} else if (!bEnableData) {
		StopData(nPortIndex);
	}

	if (bEnableData) {
		StartData(nPortIndex);
	}
}

void Dmx::ClearData(const uint32_t nPortIndex) {
	assert(nPortIndex < dmx::config::max::PORTS);

	auto *p = &s_TxBuffer[nPortIndex];
	p->dmx.nLength = 513; // Including START Code
	 __builtin_memset(p->dmx.data, 0, dmx::buffer::SIZE);
}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
volatile dmx::TotalStatistics& Dmx::GetTotalStatistics(const uint32_t nPortIndex) {
	sv_TotalStatistics[nPortIndex].Dmx.Received = sv_nRxDmxPackets[nPortIndex].nCount;
	return sv_TotalStatistics[nPortIndex];
}
#endif

template <uint32_t nPortIndex, uint32_t nUart>
static void start_dmx_output() {
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
		gd32_gpio_mode_output<USART0_GPIOx, USART0_TX_GPIO_PINx>();
		GPIO_BC(USART0_GPIOx) = USART0_TX_GPIO_PINx;
		TIMER_CH0CV(TIMER1) = TIMER_CNT(TIMER1) + s_nDmxTransmit.nBreakTime;
		s_TxBuffer[dmx::config::USART0_PORT].State = TxRxState::BREAK;
		return;
		break;
#endif
#if defined (DMX_USE_USART1)
	case USART1:
		gd32_gpio_mode_output<USART1_GPIOx, USART1_TX_GPIO_PINx>();
		GPIO_BC(USART1_GPIOx) = USART1_TX_GPIO_PINx;
		TIMER_CH1CV(TIMER1) = TIMER_CNT(TIMER1) + s_nDmxTransmit.nBreakTime;
		s_TxBuffer[dmx::config::USART1_PORT].State = TxRxState::BREAK;
		return;
		break;
#endif
#if defined (DMX_USE_USART2)
	case USART2:
		gd32_gpio_mode_output<USART2_GPIOx, USART2_TX_GPIO_PINx>();
		GPIO_BC(USART2_GPIOx) = USART2_TX_GPIO_PINx;
		TIMER_CH2CV(TIMER1) = TIMER_CNT(TIMER1) + s_nDmxTransmit.nBreakTime;
		s_TxBuffer[dmx::config::USART2_PORT].State = TxRxState::BREAK;
		return;
		break;
#endif
#if defined (DMX_USE_UART3)
	case UART3:
		gd32_gpio_mode_output<UART3_GPIOx, UART3_TX_GPIO_PINx>();
		GPIO_BC(UART3_GPIOx) = UART3_TX_GPIO_PINx;
		TIMER_CH3CV(TIMER1) = TIMER_CNT(TIMER1) + s_nDmxTransmit.nBreakTime;
		s_TxBuffer[dmx::config::UART3_PORT].State = TxRxState::BREAK;
		return;
		break;
#endif
		/* TIMER 4 */
#if defined (DMX_USE_UART4)
	case UART4:
		gd32_gpio_mode_output<UART4_TX_GPIOx, UART4_TX_GPIO_PINx>();
		GPIO_BC(UART4_TX_GPIOx) = UART4_TX_GPIO_PINx;
		TIMER_CH0CV(TIMER4) = TIMER_CNT(TIMER4) + s_nDmxTransmit.nBreakTime;
		s_TxBuffer[dmx::config::UART4_PORT].State = TxRxState::BREAK;
		return;
		break;
#endif
#if defined (DMX_USE_USART5)
	case USART5:
		gd32_gpio_mode_output<USART5_GPIOx, USART5_TX_GPIO_PINx>();
		GPIO_BC(USART5_GPIOx) = USART5_TX_GPIO_PINx;
		TIMER_CH1CV(TIMER4) = TIMER_CNT(TIMER4) + s_nDmxTransmit.nBreakTime;
		s_TxBuffer[dmx::config::USART5_PORT].State = TxRxState::BREAK;
		return;
		break;
#endif
#if defined (DMX_USE_UART6)
	case UART6:
		gd32_gpio_mode_output<UART6_GPIOx, UART6_TX_GPIO_PINx>();
		GPIO_BC(UART6_GPIOx) = UART6_TX_GPIO_PINx;
		TIMER_CH2CV(TIMER4) = TIMER_CNT(TIMER4) + s_nDmxTransmit.nBreakTime;
		s_TxBuffer[dmx::config::UART6_PORT].State = TxRxState::BREAK;
		return;
		break;
#endif
#if defined (DMX_USE_UART7)
	case UART7:
		gd32_gpio_mode_output<UART7_GPIOx, UART7_TX_GPIO_PINx>();
		GPIO_BC(UART7_GPIOx) = UART7_TX_GPIO_PINx;
		TIMER_CH3CV(TIMER4) = TIMER_CNT(TIMER4) + s_nDmxTransmit.nBreakTime;
		s_TxBuffer[dmx::config::UART7_PORT].State = TxRxState::BREAK;
		return;
		break;
#endif
	default:
		assert(0);
		__builtin_unreachable();
		break;
	}

	assert(0);
	__builtin_unreachable();
}

template <uint32_t portIndex>
static void start_dmx_output_port() {
    if constexpr (portIndex < dmx::config::max::PORTS) {
        start_dmx_output<portIndex, dmx_port_to_uart(portIndex)>();
    }
}

void Dmx::StartDmxOutput(const uint32_t nPortIndex) {
	assert(nPortIndex < dmx::config::max::PORTS);

	switch (nPortIndex) {
	case 0:
		start_dmx_output_port<0>();
		break;
	case 1:
		start_dmx_output_port<1>();
		break;
	case 2:
		start_dmx_output_port<2>();
		break;
	case 3:
		start_dmx_output_port<3>();
		break;
	case 4:
		start_dmx_output_port<4>();
		break;
	case 5:
		start_dmx_output_port<5>();
		break;
	case 6:
		start_dmx_output_port<6>();
		break;
	case 7:
		start_dmx_output_port<7>();
		break;
	default:
		break;
	}
}

void Dmx::StopData(const uint32_t nPortIndex) {
	assert(nPortIndex < dmx::config::max::PORTS);

	if (sv_PortState[nPortIndex] == PortState::IDLE) {
		return;
	}

	sv_PortState[nPortIndex] = PortState::IDLE;

	const auto nUart = dmx_port_to_uart(nPortIndex);

	if (m_dmxPortDirection[nPortIndex] == PortDirection::OUTP) {
		do {
			if (s_TxBuffer[nPortIndex].State == dmx::TxRxState::DMXINTER) {
				gd32_usart_flag_clear<USART_FLAG_TC>(nUart);
				do {
					__DMB();
				} while (!gd32_usart_flag_get<USART_FLAG_TC>(nUart));

				s_TxBuffer[nPortIndex].State = dmx::TxRxState::IDLE;
			}
		} while (s_TxBuffer[nPortIndex].State != dmx::TxRxState::IDLE);

		return;
	}

	if (m_dmxPortDirection[nPortIndex] == PortDirection::INP) {
		gd32_usart_interrupt_disable<USART_INT_RBNE>(nUart);
		gd32_usart_interrupt_disable<USART_INT_FLAG_IDLE>(nUart);
		sv_RxBuffer[nPortIndex].State = TxRxState::IDLE;
		return;
	}

	assert(0);
	__builtin_unreachable();
}

// DMX Send

void Dmx::SetDmxBreakTime(uint32_t nBreakTime) {
	s_nDmxTransmit.nBreakTime = std::max(transmit::BREAK_TIME_MIN, nBreakTime);
	SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
}

uint32_t Dmx::GetDmxBreakTime() const {
	return s_nDmxTransmit.nBreakTime;
}

void Dmx::SetDmxMabTime(uint32_t nMabTime) {
	s_nDmxTransmit.nMabTime = std::max(transmit::MAB_TIME_MIN, nMabTime);
	SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
}

uint32_t Dmx::GetDmxMabTime() const {
	return s_nDmxTransmit.nMabTime;
}

void Dmx::SetDmxPeriodTime(uint32_t nPeriod) {
	m_nDmxTransmitPeriodRequested = nPeriod;

	auto nLengthMax = s_TxBuffer[0].dmx.nLength;

	for (uint32_t nPortIndex = 1; nPortIndex < dmx::config::max::PORTS; nPortIndex++) {
		const auto nLength = s_TxBuffer[nPortIndex].dmx.nLength;
		if (nLength > nLengthMax) {
			nLengthMax = nLength;
		}
	}

	auto nPackageLengthMicroSeconds = s_nDmxTransmit.nBreakTime + s_nDmxTransmit.nMabTime + (nLengthMax * 44U);

	// The GD32F4xx/GD32H7XX Timer 1 has a 32-bit counter
#if  defined(GD32F4XX) || defined (GD32H7XX)
#else
	if (nPackageLengthMicroSeconds > (static_cast<uint16_t>(~0) - 44U)) {
		s_nDmxTransmit.nBreakTime = std::min(transmit::BREAK_TIME_TYPICAL, s_nDmxTransmit.nBreakTime);
		s_nDmxTransmit.nMabTime = transmit::MAB_TIME_MIN;
		nPackageLengthMicroSeconds = s_nDmxTransmit.nBreakTime + s_nDmxTransmit.nMabTime + (nLengthMax * 44U);
	}
#endif

	if (nPeriod != 0) {
		if (nPeriod < nPackageLengthMicroSeconds) {
			m_nDmxTransmitPeriod = std::max(transmit::BREAK_TO_BREAK_TIME_MIN, nPackageLengthMicroSeconds + 44U);
		} else {
			m_nDmxTransmitPeriod = nPeriod;
		}
	} else {
		m_nDmxTransmitPeriod = std::max(transmit::BREAK_TO_BREAK_TIME_MIN, nPackageLengthMicroSeconds + 44U);
	}

	s_nDmxTransmit.nInterTime = m_nDmxTransmitPeriod - nPackageLengthMicroSeconds;

	DEBUG_PRINTF("nPeriod=%u, nLengthMax=%u, m_nDmxTransmitPeriod=%u, nPackageLengthMicroSeconds=%u -> s_nDmxTransmit.nInterTime=%u", nPeriod, nLengthMax, m_nDmxTransmitPeriod, nPackageLengthMicroSeconds, s_nDmxTransmit.nInterTime);
}

void Dmx::SetDmxSlots(uint16_t nSlots) {
	if ((nSlots >= 2) && (nSlots <= dmx::max::CHANNELS)) {
		m_nDmxTransmitSlots = nSlots;

		for (uint32_t i = 0; i < dmx::config::max::PORTS; i++) {
			m_nDmxTransmissionLength[i] = std::min(m_nDmxTransmissionLength[i], static_cast<uint32_t>(nSlots));
		}

		SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
	}
}

void Dmx::SetOutputStyle(const uint32_t nPortIndex, const dmx::OutputStyle outputStyle) {
	assert(nPortIndex < dmx::config::max::PORTS);
	s_TxBuffer[nPortIndex].outputStyle = outputStyle;

	if (outputStyle == dmx::OutputStyle::CONTINOUS) {
		if (!m_bHasContinuosOutput) {
			m_bHasContinuosOutput = true;
			if (m_dmxPortDirection[nPortIndex] == dmx::PortDirection::OUTP) {
				StartDmxOutput(nPortIndex);
			}
			return;
		}

		for (uint32_t nIndex = 0; nIndex < dmx::config::max::PORTS; nIndex++) {
			if ((s_TxBuffer[nIndex].outputStyle == dmx::OutputStyle::CONTINOUS) && (m_dmxPortDirection[nIndex] == dmx::PortDirection::OUTP)) {
				StopData(nIndex);
			}
		}

		for (uint32_t nIndex = 0; nIndex < dmx::config::max::PORTS; nIndex++) {
			if ((s_TxBuffer[nIndex].outputStyle == dmx::OutputStyle::CONTINOUS) && (m_dmxPortDirection[nIndex] == dmx::PortDirection::OUTP)) {
				StartDmxOutput(nIndex);
			}
		}
	} else {
		m_bHasContinuosOutput = false;
		for (uint32_t nIndex = 0; nIndex < dmx::config::max::PORTS; nIndex++) {
			if (s_TxBuffer[nIndex].outputStyle == dmx::OutputStyle::CONTINOUS) {
				m_bHasContinuosOutput = true;
				return;
			}
		}
	}
}

dmx::OutputStyle Dmx::GetOutputStyle(const uint32_t nPortIndex) const {
	assert(nPortIndex < dmx::config::max::PORTS);
	return s_TxBuffer[nPortIndex].outputStyle;
}

void Dmx::SetSendData(const uint32_t nPortIndex, const uint8_t *pData, uint32_t nLength) {
	assert(nPortIndex < dmx::config::max::PORTS);

	auto &p = s_TxBuffer[nPortIndex];
	auto *pDst = p.dmx.data;

	nLength = std::min(nLength, static_cast<uint32_t>(m_nDmxTransmitSlots));
	p.dmx.nLength = nLength + 1;

	memcpy(pDst, pData, nLength);

	if (nLength != m_nDmxTransmissionLength[nPortIndex]) {
		m_nDmxTransmissionLength[nPortIndex] = nLength;
		SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
	}
}

void Dmx::SetSendDataWithoutSC(const uint32_t nPortIndex, const uint8_t *pData, uint32_t nLength) {
	assert(nPortIndex < dmx::config::max::PORTS);

	auto &p = s_TxBuffer[nPortIndex];
	auto *pDst = p.dmx.data;

	nLength = std::min(nLength, static_cast<uint32_t>(m_nDmxTransmitSlots));
	p.dmx.nLength = nLength + 1;
	p.dmx.bDataPending = true;

	pDst[0] = START_CODE;
	memcpy(&pDst[1], pData, nLength);

	if (nLength != m_nDmxTransmissionLength[nPortIndex]) {
		m_nDmxTransmissionLength[nPortIndex] = nLength;
		SetDmxPeriodTime(m_nDmxTransmitPeriodRequested);
	}
}

void Dmx::Blackout() {
	DEBUG_ENTRY

	for (uint32_t nPortIndex = 0; nPortIndex < DMX_MAX_PORTS; nPortIndex++) {
		if (m_dmxPortDirection[nPortIndex] == dmx::PortDirection::OUTP) {
			StopData(nPortIndex);
			ClearData(nPortIndex);
			StartData(nPortIndex);
		}
	}

	DEBUG_EXIT
}

void Dmx::FullOn() {
	DEBUG_ENTRY

	for (uint32_t nPortIndex = 0; nPortIndex < DMX_MAX_PORTS; nPortIndex++) {
		if (m_dmxPortDirection[nPortIndex] == dmx::PortDirection::OUTP) {
			StopData(nPortIndex);

			auto * __restrict__ p = &s_TxBuffer[nPortIndex];
			auto *p32 = reinterpret_cast<uint32_t *>(p->dmx.data);

			for (auto i = 0; i < dmx::buffer::SIZE / 4; i++) {
				*p32++ = UINT32_MAX;
			}

			p->dmx.data[0] = dmx::START_CODE;
			p->dmx.nLength = 513;

			StartData(nPortIndex);
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

void Dmx::Sync() {
	for (uint32_t nPortIndex = 0; nPortIndex < dmx::config::max::PORTS; nPortIndex++) {
		auto &txBuffer = s_TxBuffer[nPortIndex];

		if (!txBuffer.dmx.bDataPending) {
			continue;
		}

		txBuffer.dmx.bDataPending = false;

		if ((sv_PortState[nPortIndex] == dmx::PortState::TX)) {
			if ((txBuffer.outputStyle == dmx::OutputStyle::DELTA) && (txBuffer.State == dmx::TxRxState::IDLE)) {
				StartDmxOutput(nPortIndex);
			}
		}
	}
}

void Dmx::StartData(const uint32_t nPortIndex) {
	assert(nPortIndex < dmx::config::max::PORTS);
	assert(sv_PortState[nPortIndex] == PortState::IDLE);

	if (m_dmxPortDirection[nPortIndex] == dmx::PortDirection::OUTP) {
		sv_PortState[nPortIndex] = PortState::TX;
		return;
	}

	if (m_dmxPortDirection[nPortIndex] == dmx::PortDirection::INP) {
		sv_RxBuffer[nPortIndex].State = TxRxState::IDLE;

		const auto nUart = dmx_port_to_uart(nPortIndex);

		do {
			__DMB();
		} while (!gd32_usart_flag_get<USART_FLAG_TBE>(nUart));

		gd32_usart_interrupt_flag_clear<USART_INT_FLAG_RBNE>(nUart);
		gd32_usart_interrupt_flag_clear<USART_INT_FLAG_IDLE>(nUart);
		gd32_usart_interrupt_enable<USART_INT_RBNE>(nUart);
		gd32_usart_interrupt_enable<USART_INT_FLAG_IDLE>(nUart);

		sv_PortState[nPortIndex] = PortState::RX;
		return;
	}

	assert(0);
	__builtin_unreachable();
}


// DMX Receive

const uint8_t *Dmx::GetDmxChanged(const uint32_t nPortIndex) {
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
	const auto * __restrict__ p = GetDmxAvailable(nPortIndex);

	if (p == nullptr) {
		return nullptr;
	}

	const auto * __restrict__ pSrc32 = reinterpret_cast<const volatile uint32_t *>(sv_RxBuffer[nPortIndex].Dmx.current.data);
	auto * __restrict__ pDst32 = reinterpret_cast<uint32_t *>(sv_RxBuffer[nPortIndex].Dmx.previous.data);

	if (sv_RxBuffer[nPortIndex].Dmx.current.nSlotsInPacket != sv_RxBuffer[nPortIndex].Dmx.previous.nSlotsInPacket) {
		sv_RxBuffer[nPortIndex].Dmx.previous.nSlotsInPacket = sv_RxBuffer[nPortIndex].Dmx.current.nSlotsInPacket;

		for (size_t i = 0; i < buffer::SIZE / 4; ++i) {
		    pDst32[i] = pSrc32[i];
		}

		return p;
	}

	bool isChanged = false;

	for (size_t i = 0; i < buffer::SIZE / 4; ++i) {
	    const auto srcVal = pSrc32[i];
	    auto dstVal = pDst32[i];

	    if (srcVal != dstVal) {
	        pDst32[i] = srcVal;
	        isChanged = true;
	    }
	}

	return (isChanged ? p : nullptr);
#else
	return nullptr;
#endif
}

const uint8_t *Dmx::GetDmxAvailable([[maybe_unused]] const uint32_t nPortIndex)  {
	assert(nPortIndex < dmx::config::max::PORTS);
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
	auto nSlotsInPacket = sv_RxBuffer[nPortIndex].Dmx.current.nSlotsInPacket;

	if ((nSlotsInPacket & 0x8000) != 0x8000) {
		return nullptr;
	}

	nSlotsInPacket &= ~0x8000;
	nSlotsInPacket--;	// Remove SC from length
	sv_RxBuffer[nPortIndex].Dmx.current.nSlotsInPacket = nSlotsInPacket;

	return const_cast<const uint8_t *>(sv_RxBuffer[nPortIndex].Dmx.current.data);
#else
	return nullptr;
#endif
}

const uint8_t *Dmx::GetDmxCurrentData(const uint32_t nPortIndex) {
	return const_cast<const uint8_t *>(sv_RxBuffer[nPortIndex].Dmx.current.data);
}

uint32_t Dmx::GetDmxUpdatesPerSecond([[maybe_unused]] uint32_t nPortIndex) {
	assert(nPortIndex < dmx::config::max::PORTS);
#if !defined(CONFIG_DMX_TRANSMIT_ONLY)
	return sv_nRxDmxPackets[nPortIndex].nPerSecond;
#else
	return 0;
#endif
}

// RDM Send

template <uint32_t nPortIndex, uint32_t nUart>
static void rdm_send_raw(const uint8_t *const pRdmData, const uint32_t nLength) {
	assert(nPortIndex < dmx::config::max::PORTS);
	assert(pRdmData != nullptr);
	assert(nLength != 0);

	switch (nUart) {
#if defined (DMX_USE_USART0)
	case USART0:
		gd32_gpio_mode_output<USART0_GPIOx, USART0_TX_GPIO_PINx>();
		GPIO_BC(USART0_GPIOx) = USART0_TX_GPIO_PINx;
		break;
#endif
#if defined (DMX_USE_USART1)
	case USART1:
		gd32_gpio_mode_output<USART1_GPIOx, USART1_TX_GPIO_PINx>();
		GPIO_BC(USART1_GPIOx) = USART1_TX_GPIO_PINx;
		break;
#endif
#if defined (DMX_USE_USART2)
	case USART2:
		gd32_gpio_mode_output<USART2_GPIOx, USART2_TX_GPIO_PINx>();
		GPIO_BC(USART2_GPIOx) = USART2_TX_GPIO_PINx;
		break;
#endif
#if defined (DMX_USE_UART3)
	case UART3:
		gd32_gpio_mode_output<UART3_GPIOx, UART3_TX_GPIO_PINx>();
		GPIO_BC(UART3_GPIOx) = UART3_TX_GPIO_PINx;
		break;
#endif
#if defined (DMX_USE_UART4)
	case UART4:
		gd32_gpio_mode_output<UART4_TX_GPIOx, UART4_TX_GPIO_PINx>();
		GPIO_BC(UART4_TX_GPIOx) = UART4_TX_GPIO_PINx;
		break;
#endif
#if defined (DMX_USE_USART5)
	case USART5:
		gd32_gpio_mode_output<USART5_GPIOx, USART5_TX_GPIO_PINx>();
		GPIO_BC(USART5_GPIOx) = USART5_TX_GPIO_PINx;
		break;
#endif
#if defined (DMX_USE_UART6)
	case UART6:
		gd32_gpio_mode_output<UART6_GPIOx, UART6_TX_GPIO_PINx>();
		GPIO_BC(UART6_GPIOx) = UART6_TX_GPIO_PINx;
		break;
#endif
#if defined (DMX_USE_UART7)
	case UART7:
		gd32_gpio_mode_output<UART7_GPIOx, UART7_TX_GPIO_PINx>();
		GPIO_BC(UART7_GPIOx) = UART7_TX_GPIO_PINx;
		break;
#endif
	default:
		assert(0);
		__builtin_unreachable();
		break;
	}

	TIMER_CNT(TIMER5) = 0;
	do {
		__DMB();
	} while (TIMER_CNT(TIMER5) < RDM_TRANSMIT_BREAK_TIME);

	switch (nUart) {
#if defined (DMX_USE_USART0)
	case USART0:
		gd32_gpio_mode_af<USART0_GPIOx, USART0_TX_GPIO_PINx, USART0>();
		break;
#endif
#if defined (DMX_USE_USART1)
	case USART1:
		gd32_gpio_mode_af<USART1_GPIOx, USART1_TX_GPIO_PINx, USART1>();
		break;
#endif
#if defined (DMX_USE_USART2)
	case USART2:
		gd32_gpio_mode_af<USART2_GPIOx, USART2_TX_GPIO_PINx, USART2>();
		break;
#endif
#if defined (DMX_USE_UART3)
	case UART3:
		gd32_gpio_mode_af<UART3_GPIOx, UART3_TX_GPIO_PINx, UART3>();
		break;
#endif
#if defined (DMX_USE_UART4)
	case UART4:
		gd32_gpio_mode_af<UART4_TX_GPIOx, UART4_TX_GPIO_PINx, UART4>();
		break;
#endif
#if defined (DMX_USE_USART5)
	case USART5:
		gd32_gpio_mode_af<USART5_GPIOx, USART5_TX_GPIO_PINx, USART5>();
		break;
#endif
#if defined (DMX_USE_UART6)
	case UART6:
		gd32_gpio_mode_af<UART6_GPIOx, UART6_TX_GPIO_PINx, UART6>();
		break;
#endif
#if defined (DMX_USE_UART7)
	case UART7:
		gd32_gpio_mode_af<UART7_GPIOx, UART7_TX_GPIO_PINx, UART7>();
		break;
#endif
	default:
		assert(0);
		__builtin_unreachable();
		break;
	}

	TIMER_CNT(TIMER5) = 0;
	do {
		__DMB();
	} while (TIMER_CNT(TIMER5) < RDM_TRANSMIT_MAB_TIME);

	for (uint32_t i = 0; i < nLength; i++) {
		do {
			__DMB();
		} while (!gd32_usart_flag_get<USART_FLAG_TBE>(nUart));

		USART_TDATA(nUart) = USART_TDATA_TDATA & pRdmData[i];
	}

	while (!gd32_usart_flag_get<USART_FLAG_TC>(nUart)) {
		static_cast<void>(GET_BITS(USART_RDATA(nUart), 0U, 8U));
	}

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
	sv_TotalStatistics[nPortIndex].Rdm.Sent.Class++;
#endif
}

void Dmx::RdmSendRaw(const uint32_t nPortIndex, const uint8_t *pRdmData, uint32_t nLength) {
	switch (nPortIndex) {
	case 0:
		rdm_send_raw<0, dmx_port_to_uart(0)>(pRdmData, nLength);
		break;
#if DMX_MAX_PORTS >= 2
	case 1:
		rdm_send_raw<1, dmx_port_to_uart(1)>(pRdmData, nLength);
		break;
#endif
#if DMX_MAX_PORTS >= 3
	case 2:
		rdm_send_raw<2, dmx_port_to_uart(2)>(pRdmData, nLength);
		break;
#endif
#if DMX_MAX_PORTS >= 4
	case 3:
		rdm_send_raw<3, dmx_port_to_uart(3)>(pRdmData, nLength);
		break;
#endif
#if DMX_MAX_PORTS >= 5
	case 4:
		rdm_send_raw<4, dmx_port_to_uart(4)>(pRdmData, nLength);
		break;
#endif
#if DMX_MAX_PORTS >= 6
	case 5:
		rdm_send_raw<5, dmx_port_to_uart(5)>(pRdmData, nLength);
		break;
#endif
#if DMX_MAX_PORTS >= 7
	case 6:
		rdm_send_raw<6, dmx_port_to_uart(6)>(pRdmData, nLength);
		break;
#endif
#if DMX_MAX_PORTS == 8
	case 7:
		rdm_send_raw<7, dmx_port_to_uart(7)>(pRdmData, nLength);
		break;
#endif
	default:
		break;
	}
}

void Dmx::RdmSendDiscoveryRespondMessage(uint32_t nPortIndex, const uint8_t *pRdmData, uint32_t nLength) {
	assert(nPortIndex < dmx::config::max::PORTS);
	assert(pRdmData != nullptr);
	assert(nLength != 0);

	// 3.2.2 Responder Packet spacing
	udelay(RDM_RESPONDER_PACKET_SPACING, gsv_RdmDataReceiveEnd);

	SetPortDirection(nPortIndex, dmx::PortDirection::OUTP, false);

	const auto nUart = dmx_port_to_uart(nPortIndex);

	for (uint32_t i = 0; i < nLength; i++) {
		do {
			__DMB();
		} while (!gd32_usart_flag_get<USART_FLAG_TBE>(nUart));

		USART_TDATA(nUart) = USART_TDATA_TDATA & pRdmData[i];
	}

	while (!gd32_usart_flag_get<USART_FLAG_TC>(nUart)) {
		static_cast<void>(GET_BITS(USART_RDATA(nUart), 0U, 8U));
	}

	TIMER_CNT(TIMER5) = 0;
	do {
		__DMB();
	} while (TIMER_CNT(TIMER5) < RDM_RESPONDER_DATA_DIRECTION_DELAY);


	SetPortDirection(nPortIndex, dmx::PortDirection::INP, true);

#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
	sv_TotalStatistics[nPortIndex].Rdm.Sent.DiscoveryResponse++;
#endif
}

// RDM Receive

const uint8_t *Dmx::RdmReceive(const uint32_t nPortIndex) {
	assert(nPortIndex < dmx::config::max::PORTS);

	if ((sv_RxBuffer[nPortIndex].Rdm.nIndex & 0x4000) != 0x4000) {
		return nullptr;
	}

	sv_RxBuffer[nPortIndex].Rdm.nIndex = 0;

	const auto *p = const_cast<const uint8_t *>(sv_RxBuffer[nPortIndex].Rdm.data);

	if (p[0] == E120_SC_RDM) {
		const auto *pRdmCommand = reinterpret_cast<const struct TRdmMessage *>(p);

		uint32_t i;
		uint16_t nChecksum = 0;

		for (i = 0; i < 24; i++) {
			nChecksum = static_cast<uint16_t>(nChecksum + p[i]);
		}

		for (; i < pRdmCommand->message_length; i++) {
			nChecksum = static_cast<uint16_t>(nChecksum + p[i]);
		}

		if (p[i++] == static_cast<uint8_t>(nChecksum >> 8)) {
			if (p[i] == static_cast<uint8_t>(nChecksum)) {
#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
				sv_TotalStatistics[nPortIndex].Rdm.Received.Good++;
#endif
				return p;
			}
		}
#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[nPortIndex].Rdm.Received.Bad++;
#endif
		return nullptr;
	} else {
#if !defined (CONFIG_DMX_DISABLE_STATISTICS)
		sv_TotalStatistics[nPortIndex].Rdm.Received.DiscoveryResponse++;
#endif
	}

	return p;
}

const uint8_t *Dmx::RdmReceiveTimeOut(const uint32_t nPortIndex, uint16_t nTimeOut) {
	assert(nPortIndex < dmx::config::max::PORTS);

	uint8_t *p = nullptr;
	TIMER_CNT(TIMER5) = 0;

	do {
		if ((p = const_cast<uint8_t *>(RdmReceive(nPortIndex))) != nullptr) {
			return p;
		}
	} while (TIMER_CNT(TIMER5) < nTimeOut);

	return nullptr;
}
