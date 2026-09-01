/**
 * @file spi_flash_internal.h
 *
 */
/* Copyright (C) 2019-2026 by Arjan van Vught mailto:info@g32-dmx.org
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

#ifndef SPI_SPI_FLASH_INTERNAL_H_
#define SPI_SPI_FLASH_INTERNAL_H_

#include <cstdint>

namespace spi::flash {
struct Info {
    const char* name;
    uint32_t size;
    // Poll cmd - for flash erase/program
    uint8_t poll_cmd;
};
void Init();
void Transfer(uint32_t length, const uint8_t* data_out, uint8_t* data_in, uint32_t flags);

#ifdef H3
#define CONFIG_SPI_FLASH_MACRONIX
bool ProbeMacronix(struct spi::flash::Info* flash, const uint8_t* idcode);
#define CONFIG_SPI_FLASH_GIGADEVICE
bool ProbeGigadevice(struct spi::flash::Info* flash, const uint8_t* idcode);
#endif
#define CONFIG_SPI_FLASH_WINBOND
bool ProbeWinbond(struct spi::flash::Info* flash, const uint8_t* idcode);
} // namespace spi::flash

#define STATUS_WIP 0x01
#define STATUS_PEC 0x80

#define SPI_FLASH_16MB_BOUN 0x1000000

#define SPI_XFER_BEGIN 0x01 ///< Assert CS before transfer
#define SPI_XFER_END 0x02   ///< Deassert CS after transfer

#define SPI_XFER_SPEED_HZ 6000000 ///< 6MHz

#endif // SPI_SPI_FLASH_INTERNAL_H_
