/**
 * @file gd32_i2c.h
 *
 */
/* Copyright (C) 2021-2022 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef GD32_I2C_H_
#define GD32_I2C_H_

#include <stdint.h>

typedef enum GD32_I2C_BAUDRATE {
	GD32_I2C_NORMAL_SPEED = 100000,
	GD32_I2C_FULL_SPEED = 400000
} gd32_i2c_baudrate_t;

typedef enum GD32_I2C_RC {
	GD32_I2C_OK = 0,
	GD32_I2C_NOK,
	GD32_I2C_NACK,
	GD32_I2C_NOK_LA,
	GD32_I2C_NOK_TOUT
} gd32_i2c_rc_t;

#ifdef __cplusplus
extern "C" {
#endif

void gd32_i2c_begin(void);
void gd32_i2c_end(void);
extern uint8_t gd32_i2c_write(const char *pBuffer, uint32_t nLength);
uint8_t gd32_i2c_read(char *pBuffer, uint32_t nLength);
void gd32_i2c_set_baudrate(uint32_t nBaudrate);
void gd32_i2c_set_address(uint8_t nAddress);

#ifdef __cplusplus
}
#endif

#endif /* GD32_I2C_H_ */
