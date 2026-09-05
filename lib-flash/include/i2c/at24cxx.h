/**
 * @file  at24cxx.h
 * @brief I2C interface for AT24Cxx EEPROM devices.
 *
 */
/* Copyright (C) 2022-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef I2C_AT24CXX_H_
#define I2C_AT24CXX_H_

#include <cstdint>
#include <span>
#include <cstring>

#include "i2c.h"
#include "common/utils/utils_math.h"

namespace at24cxx {
static constexpr uint8_t kI2CAddress = 0x50;

struct ATTypes {
    static constexpr uint32_t kAT24LC512 = 65536;
    static constexpr uint32_t kAT24LC256 = 32768;
    static constexpr uint32_t kAT24LC128 = 16384;
    static constexpr uint32_t kAT24LC64 = 8192;
    static constexpr uint32_t kAT24LC32 = 4096;
    static constexpr uint32_t kAT24LC16 = 2048;
    static constexpr uint32_t kAT24LC08 = 1024;
    static constexpr uint32_t kAT24LC04 = 512;
    static constexpr uint32_t kAT24LC02 = 256;
    static constexpr uint32_t kAT24LC01 = 128;
};
} // namespace at24cxx

template <uint32_t kType>
class AT24Cxx {
    static constexpr bool IsValidType() {
        return kType == at24cxx::ATTypes::kAT24LC512 ||
               kType == at24cxx::ATTypes::kAT24LC256 ||
               kType == at24cxx::ATTypes::kAT24LC128 || 
               kType == at24cxx::ATTypes::kAT24LC64 || 
               kType == at24cxx::ATTypes::kAT24LC32 || 
               kType == at24cxx::ATTypes::kAT24LC16 ||
               kType == at24cxx::ATTypes::kAT24LC08 || 
               kType == at24cxx::ATTypes::kAT24LC04 || 
               kType == at24cxx::ATTypes::kAT24LC02 || 
               kType == at24cxx::ATTypes::kAT24LC01;
    }

   public:
    explicit AT24Cxx(uint8_t device_address) : address_(device_address) {
        static_assert(IsValidType(), "Invalid type specified for AT24Cxx.");
        connected_ = i2c::IsConnected(address_, i2c::kFullSpeed);
    }

    [[nodiscard]] bool IsConnected() const { return connected_; }

    [[nodiscard]] uint8_t GetAddress() const { return address_; }

    constexpr uint32_t GetSize() { return kType; }

    constexpr uint32_t GetPageSize() {
        if constexpr (kType <= at24cxx::ATTypes::kAT24LC02) {
            return 8;
        }
        if constexpr (kType <= at24cxx::ATTypes::kAT24LC16) {
            return 16;
        }
        if constexpr (kType <= at24cxx::ATTypes::kAT24LC64) {
            return 32;
        }
        if constexpr (kType <= at24cxx::ATTypes::kAT24LC256) {
            return 64;
        }
        return 128;
    }

    void Write(uint32_t memory_address, uint8_t data) {
        if (!connected_) {
            return;
        }

        i2c::SetAddress(address_);

        while (!AckRead()) {
        }

        if constexpr (kIsAddressSizeTwoWords) {
            const char kBuffer[] = {static_cast<char>(memory_address >> 8), static_cast<char>(memory_address & 0xFF), static_cast<char>(data)};
            i2c::Write(kBuffer, (sizeof(kBuffer) / sizeof(kBuffer[0])));
        } else {
            const char kBuffer[] = {static_cast<char>(memory_address & 0xFF), static_cast<char>(data)};
            i2c::SetAddress(address_ | ((memory_address >> 8) & 0x7));
            i2c::Write(kBuffer, (sizeof(kBuffer) / sizeof(kBuffer[0])));
        }
    }

    void Write(uint32_t memory_address, std::span<const uint8_t> data) {
        if (!connected_) {
            return;
        }

        char buffer[128];

        i2c::SetAddress(address_);

        while (!data.empty()) {
            while (!AckRead()) {
            }

            const auto kOffsetPage = memory_address % GetPageSize();
            uint32_t count;

            if constexpr (kIsAddressSizeTwoWords) {
                count = common::Min(common::Min(static_cast<uint32_t>(data.size()), GetPageSize() - 2), GetPageSize() - kOffsetPage);

                buffer[0] = static_cast<char>(memory_address >> 8);
                buffer[1] = static_cast<char>(memory_address & 0xFF);

                memcpy(&buffer[2], data.data(), count);
                i2c::Write(buffer, 2 + count);
            } else {
                count = common::Min(common::Min(static_cast<uint32_t>(data.size()), GetPageSize() - 1), GetPageSize() - kOffsetPage);

                buffer[0] = static_cast<char>(memory_address & 0xFF);
                memcpy(&buffer[1], data.data(), count);

                i2c::SetAddress(address_ | ((memory_address >> 8) & 0x7));
                i2c::Write(buffer, 1 + count);
            }

            memory_address += count;
            data = data.subspan(count);
        }
    }

    uint8_t Read(uint32_t memory_address) {
        if (!connected_) {
            return 0;
        }

        i2c::SetAddress(address_);

        while (!AckRead());

        if constexpr (kIsAddressSizeTwoWords) {
            char buffer[] = {static_cast<char>(memory_address >> 8), static_cast<char>(memory_address & 0xFF)};
            i2c::Write(buffer, sizeof(buffer) / sizeof(buffer[0]));
        } else {
            const char kBuffer[] = {static_cast<char>(memory_address & 0xFF)};
            i2c::SetAddress(address_ | ((memory_address >> 8) & 0x7));
            i2c::Write(kBuffer, sizeof(kBuffer) / sizeof(kBuffer[0]));
        }

        char character;
        i2c::Read(&character, 1);
        return static_cast<uint8_t>(character);
    }

    uint8_t Read(uint32_t memory_address, std::span<uint8_t> data) {
        if (!connected_) {
            return 1;
        }

        i2c::SetAddress(address_);

        while (!AckRead()) {
        }

        if constexpr (kIsAddressSizeTwoWords) {
            const char kBuffer[] = {static_cast<char>(memory_address >> 8), static_cast<char>(memory_address & 0xFF)};

            i2c::Write(kBuffer, sizeof(kBuffer));
        } else {
            const char kBuffer[] = {static_cast<char>(memory_address & 0xFF)};

            i2c::SetAddress(address_ | ((memory_address >> 8) & 0x7));
            i2c::Write(kBuffer, sizeof(kBuffer));
        }

        return i2c::Read(reinterpret_cast<char*>(data.data()), static_cast<uint32_t>(data.size()));
    }

   private:
    bool AckRead() {
        char character;
        return i2c::Read(&character, 1) == 0;
    }

    // Determines if the memory address size is 2 bytes.
    static constexpr bool kIsAddressSizeTwoWords = kType > at24cxx::ATTypes::kAT24LC16;

    uint8_t address_;
    bool connected_{false};
};

class AT24C02 : public AT24Cxx<at24cxx::ATTypes::kAT24LC02> {
   public:
    AT24C02() : AT24Cxx(at24cxx::kI2CAddress) {}
};

class AT24C04 : public AT24Cxx<at24cxx::ATTypes::kAT24LC04> {
   public:
    AT24C04() : AT24Cxx(at24cxx::kI2CAddress) {}
};

class AT24C16 : public AT24Cxx<at24cxx::ATTypes::kAT24LC16> {
   public:
    AT24C16() : AT24Cxx(at24cxx::kI2CAddress) {}
};

class AT24C32 : public AT24Cxx<at24cxx::ATTypes::kAT24LC32> {
   public:
    explicit AT24C32(uint8_t index) : AT24Cxx(at24cxx::kI2CAddress + (index & 0x7)) {}
};

#endif // I2C_AT24CXX_H_
