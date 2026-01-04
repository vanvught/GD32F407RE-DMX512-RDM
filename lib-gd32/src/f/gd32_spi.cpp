/**
 * @file gd32_spi.cpp
 *
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

#include <cstdint>
#include <cassert>

#include "gd32_spi.h"
#include "gd32_gpio.h"
#include "gd32.h"

static uint8_t s_cs = GD32_SPI_CS0;

static void SetCsHigh()
{
    if (s_cs == GD32_SPI_CS0)
    {
        GPIO_BOP(SPI_NSS_GPIOx) = SPI_NSS_GPIO_PINx;
    }
}

static void SetCsLow()
{
    if (s_cs == GD32_SPI_CS0)
    {
        GPIO_BC(SPI_NSS_GPIOx) = SPI_NSS_GPIO_PINx;
    }
}

static uint8_t SpiWriteRead(uint8_t byte)
{
    while (RESET == (SPI_STAT(SPI_PERIPH) & SPI_FLAG_TBE));

    SPI_DATA(SPI_PERIPH) = static_cast<uint32_t>(byte);

    while (RESET == (SPI_STAT(SPI_PERIPH) & SPI_FLAG_RBNE));

    return static_cast<uint8_t>(SPI_DATA(SPI_PERIPH));
}

static void RcuConfig()
{
    rcu_periph_clock_enable(SPI_RCU_SPIx);
    rcu_periph_clock_enable(SPI_RCU_GPIOx);
    rcu_periph_clock_enable(SPI_NSS_RCU_GPIOx);

#if defined(GPIO_INIT)
    rcu_periph_clock_enable(RCU_AF);
#endif
}

static void GpioConfig()
{
#if defined(GPIO_INIT)
#if defined(SPI_REMAP_GPIO)
    gpio_pin_remap_config(SPI_REMAP_GPIO, ENABLE);
    if constexpr (SPI_PERIPH == SPI0)
    {
        gpio_pin_remap_config(GPIO_SWJ_DISABLE_REMAP, ENABLE);
    }
#else
    if constexpr (SPI_PERIPH == SPI2)
    {
        gpio_pin_remap_config(GPIO_SWJ_DISABLE_REMAP, ENABLE);
    }
#endif
    gpio_init(SPI_GPIOx, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, SPI_SCK_GPIO_PINx | SPI_MOSI_GPIO_PINx);
    gpio_init(SPI_GPIOx, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, SPI_MISO_GPIO_PINx);
    gpio_init(SPI_NSS_GPIOx, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, SPI_NSS_GPIO_PINx);
#else
    gpio_af_set(SPI_GPIOx, SPI_GPIO_AFx, SPI_SCK_GPIO_PINx | SPI_MISO_GPIO_PINx | SPI_MOSI_GPIO_PINx);
    gpio_mode_set(SPI_GPIOx, GPIO_MODE_AF, GPIO_PUPD_NONE, SPI_SCK_GPIO_PINx | SPI_MISO_GPIO_PINx | SPI_MOSI_GPIO_PINx);
    gpio_output_options_set(SPI_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SPI_SCK_GPIO_PINx | SPI_MOSI_GPIO_PINx);

    gpio_mode_set(SPI_NSS_GPIOx, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SPI_NSS_GPIO_PINx);
    gpio_output_options_set(SPI_NSS_GPIOx, GPIO_OTYPE_PP, GPIO_OSPEED, SPI_NSS_GPIO_PINx);
#endif

    SetCsHigh();
}

static void SpiConfig()
{
    spi_disable(SPI_PERIPH);
    spi_i2s_deinit(SPI_PERIPH);

    spi_parameter_struct spi_init_struct;
    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.prescale = SPI_PSC_64;
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init(SPI_PERIPH, &spi_init_struct);

    spi_enable(SPI_PERIPH);
}

/*
 * Public API's
 */

void Gd32SpiBegin()
{
    RcuConfig();
    GpioConfig();
    SpiConfig();
}

void Gd32SpiEnd()
{
    spi_disable(SPI_PERIPH);
#if defined(GPIO_INIT)
    gpio_init(SPI_GPIOx, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ, SPI_SCK_GPIO_PINx | SPI_MISO_GPIO_PINx | SPI_MOSI_GPIO_PINx);
    gpio_init(SPI_NSS_GPIOx, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ, SPI_NSS_GPIO_PINx);
#else
    gpio_mode_set(SPI_GPIOx, GPIO_MODE_INPUT, GPIO_PUPD_NONE, SPI_SCK_GPIO_PINx | SPI_MISO_GPIO_PINx | SPI_MOSI_GPIO_PINx);
    gpio_mode_set(SPI_NSS_GPIOx, GPIO_MODE_INPUT, GPIO_PUPD_NONE, SPI_NSS_GPIO_PINx);
#endif
}

void Gd32SpiSetSpeedHz(uint32_t speed_hz)
{
    assert(speed_hz != 0);

    uint32_t div;

    if (SPI_PERIPH == SPI0)
    {
        div = APB2_CLOCK_FREQ / speed_hz; /* PCLK2 when using SPI0  */
    }
    else
    {
        div = APB1_CLOCK_FREQ / speed_hz; /* PCLK1 when using SPI1 and SPI2 */
    }

    uint32_t ctl0 = SPI_CTL0(SPI_PERIPH);
    ctl0 &= ~CTL0_PSC(7);

    if (div <= 2)
    {
        ctl0 |= SPI_PSC_2;
    }
    else if (div <= 4)
    {
        ctl0 |= SPI_PSC_4;
    }
    else if (div <= 8)
    {
        ctl0 |= SPI_PSC_8;
    }
    else if (div <= 16)
    {
        ctl0 |= SPI_PSC_16;
    }
    else if (div <= 32)
    {
        ctl0 |= SPI_PSC_32;
    }
    else if (div <= 64)
    {
        ctl0 |= SPI_PSC_64;
    }
    else if (div <= 128)
    {
        ctl0 |= SPI_PSC_128;
    }
    else
    {
        ctl0 |= SPI_PSC_256;
    }

    spi_disable(SPI_PERIPH);
    SPI_CTL0(SPI_PERIPH) = ctl0;
    spi_enable(SPI_PERIPH);
}

void Gd32SpiSetDataMode(uint8_t mode)
{
    uint32_t ctl0 = SPI_CTL0(SPI_PERIPH);
    ctl0 &= ~0x3;
    ctl0 |= (mode & 0x3);

    spi_disable(SPI_PERIPH);
    SPI_CTL0(SPI_PERIPH) = ctl0;
    spi_enable(SPI_PERIPH);
}

void Gd32SpiChipSelect(uint8_t chip_select)
{
    chip_select = chip_select;

    if (chip_select == GD32_SPI_CS0)
    {
        spi_nss_output_enable(SPI_PERIPH);
    }
    else
    {
        spi_nss_output_disable(SPI_PERIPH);
    }
}

void Gd32SpiTransfernb(const char* tx_buffer, char* rx_buffer, uint32_t data_length)
{
    assert(tx_buffer != nullptr);
    assert(rx_buffer != nullptr);

    SetCsLow();

    while (data_length-- > 0)
    {
        *rx_buffer = SpiWriteRead(static_cast<uint8_t>(*tx_buffer));
        rx_buffer++;
        tx_buffer++;
    }

    SetCsHigh();
}

void Gd32SpiTransfern(char* tx_buffer, uint32_t data_length)
{
    Gd32SpiTransfernb(tx_buffer, tx_buffer, data_length);
}

void Gd32SpiWrite(uint16_t data)
{
    SetCsLow();

    SpiWriteRead(static_cast<uint8_t>(data >> 8));
    SpiWriteRead(static_cast<uint8_t>(data & 0xFF));

    SetCsHigh();
}

void Gd32SpiWritenb(const char* tx_buffer, uint32_t data_length)
{
    assert(tx_buffer != nullptr);

    SetCsLow();

    while (data_length-- > 0)
    {
        SpiWriteRead(static_cast<uint8_t>(*tx_buffer));
        tx_buffer++;
    }

    SetCsHigh();
}

#if defined(SPI_BITBANG_SCK_GPIO_PINx)
/*
 * bitbang support
 * Note: /CS is handled by the user application
 */

void __attribute__((cold)) Gd32BitbangSpiBegin()
{
    Gd32GpioFsel(SPI_BITBANG_SCK_GPIOx, SPI_BITBANG_SCK_GPIO_PINx, GPIO_FSEL_OUTPUT);
    Gd32GpioFsel(SPI_BITBANG_MOSI_GPIOx, SPI_BITBANG_MOSI_GPIO_PINx, GPIO_FSEL_OUTPUT);
    Gd32GpioFsel(SPI_BITBANG_MISO_GPIOx, SPI_BITBANG_MISO_GPIO_PINx, GPIO_FSEL_INPUT);
}

static inline void BitbangSpiWrite(char c)
{
    for (uint32_t mask = (1U << 7); mask != 0; mask = (mask >> 1U))
    {
        if (c & mask)
        {
            GPIO_BOP(SPI_BITBANG_MOSI_GPIOx) = SPI_BITBANG_MOSI_GPIO_PINx;
        }
        else
        {
            GPIO_BC(SPI_BITBANG_MOSI_GPIOx) = SPI_BITBANG_MOSI_GPIO_PINx;
        }

        __ISB();
        GPIO_BOP(SPI_BITBANG_SCK_GPIOx) = SPI_BITBANG_SCK_GPIO_PINx;
        __ISB();
        GPIO_BC(SPI_BITBANG_SCK_GPIOx) = SPI_BITBANG_SCK_GPIO_PINx;
    }
}

static inline char BitbangSpiWriteRead(char c)
{
    char r = 0;

    for (uint32_t mask = (1U << 7); mask != 0; mask = (mask >> 1U))
    {
        if (c & mask)
        {
            GPIO_BOP(SPI_BITBANG_MOSI_GPIOx) = SPI_BITBANG_MOSI_GPIO_PINx;
        }
        else
        {
            GPIO_BC(SPI_BITBANG_MOSI_GPIOx) = SPI_BITBANG_MOSI_GPIO_PINx;
        }

        __ISB();
        GPIO_BOP(SPI_BITBANG_SCK_GPIOx) = SPI_BITBANG_SCK_GPIO_PINx;

        if ((GPIO_ISTAT(SPI_BITBANG_MISO_GPIOx) & SPI_BITBANG_MISO_GPIO_PINx) == SPI_BITBANG_MISO_GPIO_PINx)
        {
            r |= (mask);
        }

        __ISB();
        GPIO_BC(SPI_BITBANG_SCK_GPIOx) = SPI_BITBANG_SCK_GPIO_PINx;
    }

    return r;
}

void Gd32BitbangSpiWritenb(const char* tx_buffer, uint32_t data_length)
{
    assert(tx_buffer != nullptr);
    assert(data_length != 0);

    for (uint32_t i = 0; i < data_length; i++)
    {
        BitbangSpiWrite(tx_buffer[i]);
    }
}

void Gd32BitbangSpiTransfernb(const char* tx_buffer, char* rx_buffer, uint32_t data_length)
{
    assert(tx_buffer != nullptr);
    assert(rx_buffer != nullptr);
    assert(data_length != 0);

    while (data_length-- > 0)
    {
        *rx_buffer = BitbangSpiWriteRead(static_cast<uint8_t>(*tx_buffer));
        rx_buffer++;
        tx_buffer++;
    }
}
#endif
