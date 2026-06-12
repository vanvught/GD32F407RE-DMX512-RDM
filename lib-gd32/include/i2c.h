/**
 * @file i2c.h
 *
 */
/* Copyright (C) 2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef I2C_H_
#define I2C_H_

#include <cstdint>

#include "gd32_i2c.h"
#include "timing.h"

namespace i2c {
inline constexpr uint32_t kNormalSpeed = gd32::kI2CNormalSpeed;
inline constexpr uint32_t kFullSpeed = gd32::kI2CFullSpeed;
  
struct ReturnCode {
    static constexpr uint8_t kOk = GD32_I2C_OK;
    static constexpr uint8_t kNok = GD32_I2C_NOK;
    static constexpr uint8_t kNack = GD32_I2C_NACK;
    static constexpr uint8_t kNokLa = GD32_I2C_NOK_LA;
    static constexpr uint8_t kNokTout = GD32_I2C_NOK_TOUT;
};

inline void Begin() {
    Gd32I2cBegin();
}

inline void SetBaudrate(uint32_t baudrate) {
    Gd32I2cSetBaudrate(baudrate);
}

inline void SetAddress(uint8_t address) {
    Gd32I2cSetAddress(address);
}

inline uint8_t Write(const char* buffer, uint32_t length) {
    return Gd32I2cWrite(buffer, length);
}

inline uint8_t Write(uint8_t address, const char* buffer, uint32_t length) {
    return Gd32I2cWrite(address, buffer, length);
}

inline void Write(uint8_t data) {
    const char kBuffer[] = {static_cast<char>(data)};
    Write(kBuffer, 1);
}

inline uint8_t Read(char* buffer, uint32_t length) {
    return Gd32I2cRead(buffer, length);
}

inline uint8_t Read(uint8_t address, char* buffer, uint32_t length) {
    return Gd32I2cRead(address, buffer, length);
}

inline uint16_t Read16() {
    char buf[2] = {0};
    Read(buf, 2);
    return static_cast<uint16_t>(static_cast<uint16_t>(buf[0]) << 8 | static_cast<uint16_t>(buf[1]));
}

inline bool IsConnected(uint8_t address, uint32_t baudrate = kNormalSpeed) {
    return Gd32I2cIsConnected(address, baudrate);
}

inline void WriteReg(uint8_t reg, uint8_t value) {
    Gd32I2cWriteReg(reg, value);
}

inline void WriteReg(uint8_t reg, uint16_t value) {
    Gd32I2cWriteReg(reg, value);
}

inline void WriteReg(uint8_t address, uint8_t reg, uint8_t value) {
    Gd32I2cWriteReg(address, reg, value);
}

inline void ReadReg(uint8_t reg, uint8_t& value) {
    Gd32I2cReadReg(reg, value);
}

inline void ReadReg(uint8_t address, uint8_t reg, uint8_t& value) {
    Gd32I2cReadReg(address, reg, value);
}

inline uint16_t ReadRegister16(uint8_t reg) {
    const char kBuffer[] = {static_cast<char>(reg)};
    i2c::Write(&kBuffer[0], 1);
    return Read16();
}

inline uint16_t ReadRegister16DelayUs(uint8_t reg, uint32_t delay_us) {
    const char kBuffer[] = {static_cast<char>(reg)};
    i2c::Write(&kBuffer[0], 1);
    timing::DelayUs(delay_us);
    return Read16();
}
} // namespace i2c

#endif // I2C_H_
