/**
 * @file json_jsondoc.h
 *
 */
/* Copyright (C) 2025-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef JSON_JSON_JSONDOC_H_
#define JSON_JSON_JSONDOC_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

class JsonDoc {
   public:
    explicit JsonDoc(std::span<char> buffer) noexcept : buffer_(buffer) {
        assert(buffer_.data() != nullptr);
        assert(buffer_.size() >= 3U);
        if (buffer_.empty()) {
            overflow_ = true;
            return;
        }
        buffer_[0] = '\0';
        Append('{');
    }

    JsonDoc(char* buffer, uint32_t capacity) noexcept : JsonDoc(std::span<char>{buffer, static_cast<std::size_t>(capacity)}) {}

    class KeyProxy {
       public:
        KeyProxy(JsonDoc& doc, std::string_view first, std::string_view second = {}) noexcept : doc_(doc), first_(first), second_(second) {}

        KeyProxy& operator=(std::string_view value) noexcept {
            doc_.WriteStringField(first_, second_, value);
            return *this;
        }

        KeyProxy& operator=(const char* value) noexcept {
            assert(value != nullptr);
            return *this = std::string_view{value};
        }

        KeyProxy& operator=(uint32_t value) noexcept {
            doc_.WriteNumberField(first_, second_, value);
            return *this;
        }

       private:
        JsonDoc& doc_;
        std::string_view first_;
        std::string_view second_;
    };

    [[nodiscard]] KeyProxy operator[](std::string_view key) noexcept { return {*this, key}; }

    void End() noexcept {
        if (ended_) {
            return;
        }
        ended_ = true;
        Append('}');
    }

    [[nodiscard]] uint32_t Size() const noexcept { return static_cast<uint32_t>(pos_); }

    // Only consume the output when Ok() is true after End().
    [[nodiscard]] bool Ok() const noexcept { return !overflow_ && ended_; }

   private:
    void Append(char c) noexcept {
        if (overflow_) {
            return;
        }
        if (buffer_.empty() || pos_ >= buffer_.size() - 1U) {
            overflow_ = true;
            return;
        }
        buffer_[pos_++] = c;
        buffer_[pos_] = '\0';
    }

    void Append(std::string_view text) noexcept {
        if (overflow_) {
            return;
        }
        if (buffer_.empty() || pos_ >= buffer_.size() || text.size() > buffer_.size() - 1U - pos_) {
            overflow_ = true;
            return;
        }
        if (!text.empty()) {
            std::memcpy(buffer_.data() + pos_, text.data(), text.size());
            pos_ += text.size();
        }
        buffer_[pos_] = '\0';
    }

    void BeginField(std::string_view first, std::string_view second) noexcept {
        if (ended_ || overflow_) {
            overflow_ = true;
            return;
        }
        if (!first_field_) {
            Append(',');
        }
        Append('"');
        Append(first);
        Append(second);
        Append("\":");
        first_field_ = false;
    }

    void WriteStringField(std::string_view first, std::string_view second, std::string_view value) noexcept {
        BeginField(first, second);
        Append('"');
        Append(value);
        Append('"');
    }

    void WriteNumberField(std::string_view first, std::string_view second, uint32_t value) noexcept {
        BeginField(first, second);
        char digits[10];
        char* p = digits + sizeof(digits);
        do {
            *--p = static_cast<char>('0' + (value % 10U));
            value /= 10U;
        } while (value != 0U);
        Append(std::string_view{p, static_cast<std::size_t>(digits + sizeof(digits) - p)});
    }

    std::span<char> buffer_;
    std::size_t pos_{0};
    bool first_field_{true};
    bool ended_{false};
    bool overflow_{false};
};

#endif // JSON_JSON_JSONDOC_H_
