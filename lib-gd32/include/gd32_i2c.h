/**
 * @file gd32_i2c.h
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

#ifndef GD32_I2C_H_
#define GD32_I2C_H_

#include <cstdint>

namespace gd32
{
inline constexpr uint32_t kI2CNormalSpeed = 100000;
inline constexpr uint32_t kI2CFullSpeed = 400000;
} // namespace gd32

typedef enum GD32_I2C_RC
{
    GD32_I2C_OK = 0,
    GD32_I2C_NOK,
    GD32_I2C_NACK,
    GD32_I2C_NOK_LA,
    GD32_I2C_NOK_TOUT
} gd32_i2c_rc_t;

void Gd32I2cBegin();
void Gd32I2cSetBaudrate(uint32_t baudrate);
void Gd32I2cSetAddress(uint8_t address);
uint8_t Gd32I2cWrite(const char* buffer, uint32_t length);
uint8_t Gd32I2cWrite(uint8_t address, const char* buffer, uint32_t length);
uint8_t Gd32I2cRead(char* buffer, uint32_t length);
uint8_t Gd32I2cRead(uint8_t address, char* buffer, uint32_t length);
bool Gd32I2cIsConnected(uint8_t address, uint32_t baudrate = gd32::kI2CNormalSpeed);
void Gd32I2cWriteReg(uint8_t reg, uint8_t value);
void Gd32I2cWriteReg(uint8_t address, uint8_t reg, uint8_t value);
void Gd32I2cReadReg(uint8_t reg, uint8_t& value);
void Gd32I2cReadReg(uint8_t address, uint8_t reg, uint8_t& value);

#if defined(CONFIG_ENABLE_I2C1)
void Gd32I2c1Begin();
void Gd32I2c1SetBaudrate(uint32_t baudrate);
void Gd32I2c1SetAddress(uint8_t address);
uint8_t Gd32I2c1Write(const char* buffer, uint32_t length);
uint8_t Gd32I2c1Write(uint8_t address, const char* buffer, uint32_t length);
uint8_t Gd32I2c1Read(char* buffer, uint32_t length);
uint8_t Gd32I2c1Read(uint8_t address, char* buffer, uint32_t length);
bool Gd32I2c1IsConnected(uint8_t address, uint32_t baudrate = gd32::kI2CNormalSpeed);
void Gd32I2c1WriteReg(uint8_t reg, uint8_t value);
void Gd32I2c1WriteReg(uint8_t address, uint8_t reg, uint8_t value);
void Gd32I2c1ReadReg(uint8_t reg, uint8_t& value);
#endif

#endif  // GD32_I2C_H_
