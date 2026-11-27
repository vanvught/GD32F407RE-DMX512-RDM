/**
 * @file uart0.cpp
 *
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

#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>

#include "gd32_uart.h"
#if defined(CONFIG_USART0_ENABLE_TX_DMA) || defined(CONFIG_USART0_ENABLE_RX_DMA)
#include "gd32_dma.h"
#endif            // defined(CONFIG_USART0_ENABLE_TX_DMA) || defined(CONFIG_USART0_ENABLE_RX_DMA)
#include "gd32.h" // IWYU pragma: keep

namespace uart0 {
namespace {
constexpr uint32_t kPrintfBufferSize =
#ifndef CONFIG_USART0_PRINTF_BUFFER_SIZE
    128;
#else
    CONFIG_USART0_PRINTF_BUFFER_SIZE;
#endif

char s_printf_buffer[kPrintfBufferSize];
} // namespace

#ifdef CONFIG_USART0_ENABLE_TX_DMA
namespace {
constexpr uint32_t kTxDmaBufferSize =
#ifndef CONFIG_USART0_TX_DMA_BUFFER_SIZE
    256;
#else
    CONFIG_USART0_TX_DMA_BUFFER_SIZE;
#endif

uint8_t s_tx_buffer[kTxDmaBufferSize]{};
uint8_t dma_buffer_tx[kTxDmaBufferSize]{};

uint32_t s_tx_count{0};
} // namespace
#endif // CONFIG_USART0_ENABLE_TX_DMA

#ifdef CONFIG_USART0_ENABLE_RX_DMA
namespace {
constexpr uint32_t kRxDmaBufferSize =
#ifndef CONFIG_USART0_RX_DMA_BUFFER_SIZE
    128;
#else
    CONFIG_USART0_RX_DMA_BUFFER_SIZE;
#endif

char s_rx_buffer[kRxDmaBufferSize];

struct RxCount {
    uint16_t rx;
    uint16_t tx;
};

volatile RxCount sv_rx_count;

volatile uint32_t sv_receive_flag;
constexpr uint32_t kSizeRxBuffer = sizeof(s_rx_buffer);
} // namespace

extern "C" void USART0_IRQHandler() {
    if (RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE)) {
        usart_data_receive(USART0);

        sv_rx_count.rx = static_cast<uint16_t>(kSizeRxBuffer - (dma_transfer_number_get(USART0_DMAx, USART0_RX_DMA_CHx)));
        sv_receive_flag = 1;

        auto chtl = DMA_CHCTL(USART0_DMAx, USART0_RX_DMA_CHx);
        chtl &= ~DMA_CHXCTL_CHEN;
        DMA_CHCTL(USART0_DMAx, USART0_RX_DMA_CHx) = chtl;
        Gd32DmaInterruptFlagClear<USART0_DMAx, USART0_RX_DMA_CHx, DMA_FLAG_FTF>(); // Needed for GD32F4xx
        DMA_CHCNT(USART0_DMAx, USART0_RX_DMA_CHx) = kSizeRxBuffer;
        chtl |= DMA_CHXCTL_CHEN;
        DMA_CHCTL(USART0_DMAx, USART0_RX_DMA_CHx) = chtl;
    }
}
#endif // CONFIG_USART0_ENABLE_RX_DMA

void Init() {
    gd32::UartBegin(USART0, 115200U, gd32::kUartBits8, gd32::kUartParityNone, gd32::kUartStop1Bit);
#if defined(CONFIG_USART0_ENABLE_TX_DMA) || defined(CONFIG_USART0_ENABLE_RX_DMA)
    // DMA
#ifdef GD32H7XX
    rcu_periph_clock_enable(RCU_DMAMUX);
#endif // GD32H7XX

    rcu_periph_clock_enable(USART0_RCU_DMAx);

    DMA_PARAMETER_STRUCT dma_init_struct;
#ifdef CONFIG_USART0_ENABLE_TX_DMA
    dma_deinit(USART0_DMAx, USART0_TX_DMA_CHx);
    dma_struct_para_init(&dma_init_struct);
#ifdef GD32H7XX
    dma_init_struct.request = DMA_REQUEST_USART0_TX;
#endif // GD32H7XX
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_addr = reinterpret_cast<uint32_t>(&USART_DATA(USART0));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init(USART0_DMAx, USART0_TX_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(USART0_DMAx, USART0_TX_DMA_CHx);
#ifdef GD32F4XX
    dma_channel_subperipheral_select(USART0_DMAx, USART0_TX_DMA_CHx, USART0_TX_DMA_SUBPERIx);
#endif // GD32F4XX
    // USART
    usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_ENABLE);
#endif // CONFIG_USART0_ENABLE_TX_DMA

#ifdef CONFIG_USART0_ENABLE_RX_DMA
    dma_deinit(USART0_DMAx, USART0_RX_DMA_CHx);
    dma_struct_para_init(&dma_init_struct);
#ifdef GD32H7XX
    dma_init_struct.request = DMA_REQUEST_USART0_RX;
#endif // GD32H7XX
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
#if defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.memory0_addr = reinterpret_cast<uint32_t>(s_rx_buffer);
#else
    dma_init_struct.memory_addr = reinterpret_cast<uint32_t>(s_rx_buffer);
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
#else
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.number = kSizeRxBuffer;
    dma_init_struct.periph_addr = reinterpret_cast<uint32_t>(&USART_DATA(USART0));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
#if defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.periph_memory_width = DMA_PERIPHERAL_WIDTH_8BIT;
#else
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
#endif // defined(GD32F4XX) || defined(GD32H7XX)
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init(USART0_DMAx, USART0_RX_DMA_CHx, &dma_init_struct);
    dma_circulation_disable(USART0_DMAx, USART0_RX_DMA_CHx);
#ifdef GD32F4XX
    dma_channel_subperipheral_select(USART0_DMAx, USART0_RX_DMA_CHx, USART0_RX_DMA_SUBPERIx);
#endif // GD32F4XX
    dma_channel_enable(USART0_DMAx, USART0_RX_DMA_CHx);
    // USART
    usart_dma_receive_config(USART0, USART_RECEIVE_DMA_ENABLE);
    // NVIC
    NVIC_SetPriority(USART0_IRQn, 3);
    NVIC_EnableIRQ(USART0_IRQn);
    usart_interrupt_enable(USART0, USART_INT_IDLE);
#endif // CONFIG_USART0_ENABLE_RX_DMA

#ifdef CONFIG_USART0_ENABLE_TX_DMA
    s_tx_count = 0;
#endif
#endif // defined(CONFIG_USART0_ENABLE_TX_DMA) || defined(CONFIG_USART0_ENABLE_RX_DMA)
}

#ifdef CONFIG_USART0_ENABLE_TX_DMA
namespace {
void WaitDma() {
    while (DMA_CHCNT(USART0_DMAx, USART0_TX_DMA_CHx) > 0) {
    }
}
} // namespace

void WriteDma(const void* data, uint32_t size) {
    assert(data != nullptr);
    assert(size != 0);
    assert(size <= DMA_CHXCNT_CNT);

    if (size == 0) {
        return;
    }

    WaitDma();

    while (!gd32::UartFlagGet<USART_FLAG_TC>(USART0)) {
    }

    auto dma_chctl = DMA_CHCTL(USART0_DMAx, USART0_TX_DMA_CHx);
    dma_chctl &= ~DMA_CHXCTL_CHEN;
    DMA_CHCTL(USART0_DMAx, USART0_TX_DMA_CHx) = dma_chctl;
    DMA_CHMADDR(USART0_DMAx, USART0_TX_DMA_CHx) = reinterpret_cast<uint32_t>(data);
    Gd32DmaInterruptFlagClear<USART0_DMAx, USART0_TX_DMA_CHx, DMA_FLAG_FTF>(); // Needed for GD32F4xx
    DMA_CHCNT(USART0_DMAx, USART0_TX_DMA_CHx) = size;
    dma_chctl |= DMA_CHXCTL_CHEN;
    DMA_CHCTL(USART0_DMAx, USART0_TX_DMA_CHx) = dma_chctl;
}

namespace {
void TxBufferDrain() {
    if (s_tx_count == 0) {
        return;
    }

    WaitDma();

    std::memcpy(dma_buffer_tx, s_tx_buffer, s_tx_count);

    const auto kSize = s_tx_count;
    s_tx_count = 0;

    WriteDma(dma_buffer_tx, kSize);
}

void TxBufferPut(uint8_t data) {
    s_tx_buffer[s_tx_count++] = data;

    if (s_tx_count == kTxDmaBufferSize) {
        TxBufferDrain();
    }
}
} // namespace
#endif // CONFIG_USART0_ENABLE_TX_DMA

void PutChar(int character) {
#ifdef CONFIG_USART0_ENABLE_TX_DMA
    if (character == '\n') {
        TxBufferPut(static_cast<uint8_t>('\r'));
        TxBufferPut(static_cast<uint8_t>('\n'));
        TxBufferDrain();
        return;
    }

    TxBufferPut(static_cast<uint8_t>(character));
#else

    if (character == '\n') {
        while (!gd32::UartFlagGet<USART_FLAG_TBE>(USART0)) {
        }
        USART_TDATA(USART0) = static_cast<uint16_t>(USART_TDATA_TDATA & static_cast<uint8_t>('\r'));
    }

    while (!gd32::UartFlagGet<USART_FLAG_TBE>(USART0)) {
    }
    USART_TDATA(USART0) = static_cast<uint16_t>(USART_TDATA_TDATA & static_cast<uint8_t>(character));
#endif
}

int Printf(const char* fmt, ...) {
    va_list arp;

    va_start(arp, fmt);

    const auto kSize = vsnprintf(s_printf_buffer, sizeof(s_printf_buffer), fmt, arp);
    s_printf_buffer[sizeof(s_printf_buffer) - 1] = '\0';

    va_end(arp);

    char* string_buffer = s_printf_buffer;

    while (*string_buffer != '\0') {
        PutChar(*string_buffer++);
    }

    return kSize;
}

void Puts(const char* string) {
    while (*string != '\0') {
        PutChar(*string++);
    }

    PutChar('\n');
}

#ifdef CONFIG_USART0_ENABLE_RX_DMA
int GetChar() {
    if (sv_receive_flag != 1) [[unlikely]] {
        return EOF;
    }

    if (sv_rx_count.tx < sv_rx_count.rx) {
        int c = static_cast<int>(s_rx_buffer[sv_rx_count.tx]);
        sv_rx_count.tx = sv_rx_count.tx + 1;
        return c;
    }

    sv_receive_flag = 0;
    sv_rx_count.tx = 0;
    sv_rx_count.rx = 0;

    return EOF;
}
#else
int GetChar() {
    if (!gd32::UartFlagGet<USART_FLAG_RBNE>(USART0)) [[likely]] {
        return EOF;
    }

    const auto kChar = static_cast<int>(USART_RDATA(USART0));

#ifdef UART0_ECHO
    PutChar(kChar);
#endif // UART0_ECHO

    return kChar;
}
#endif // CONFIG_USART0_ENABLE_RX_DMA
} // namespace uart0
