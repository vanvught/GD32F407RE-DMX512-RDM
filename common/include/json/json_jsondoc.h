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
    enum class Root : uint8_t { kObject, kArray };

    explicit JsonDoc(std::span<char> buffer, Root root = Root::kObject) noexcept : buffer_(buffer) {
        assert(buffer_.data() != nullptr);
        assert(buffer_.size() >= 3U);

        if (buffer_.empty()) {
            overflow_ = true;
            return;
        }

        buffer_[0] = '\0';
        container_[0] = root == Root::kArray ? Container::kArray : Container::kObject;
        Append(root == Root::kArray ? '[' : '{');
    }

    JsonDoc(char* buffer, uint32_t capacity, Root root = Root::kObject) noexcept
        : JsonDoc(std::span<char>{buffer, static_cast<std::size_t>(capacity)}, root) {}

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

    // Named object inside an object.
    void BeginObject(std::string_view key) noexcept {
        if (!CanPush() || container_[depth_] != Container::kObject) {
            overflow_ = true;
            return;
        }

        BeginField(key, {});
        Append('{');
        Push(Container::kObject);
    }

    // Anonymous object inside an array.
    void BeginObject() noexcept {
        if (!CanPush() || container_[depth_] != Container::kArray) {
            overflow_ = true;
            return;
        }

        BeginElement();
        Append('{');
        Push(Container::kObject);
    }

    void EndObject() noexcept {
        if (ended_ || overflow_ || depth_ == 0U || container_[depth_] != Container::kObject) {
            overflow_ = true;
            return;
        }

        Append('}');
        if (!overflow_) {
            --depth_;
        }
    }

    // Named array inside an object.
    void BeginArray(std::string_view key) noexcept {
        if (!CanPush() || container_[depth_] != Container::kObject) {
            overflow_ = true;
            return;
        }

        BeginField(key, {});
        Append('[');
        Push(Container::kArray);
    }

    void EndArray() noexcept {
        if (ended_ || overflow_ || depth_ == 0U || container_[depth_] != Container::kArray) {
            overflow_ = true;
            return;
        }

        Append(']');
        if (!overflow_) {
            --depth_;
        }
    }

    void End() noexcept {
        if (ended_) {
            return;
        }

        if (depth_ != 0U || overflow_) {
            overflow_ = true;
            return;
        }

        Append(container_[0] == Container::kArray ? ']' : '}');
        if (!overflow_) {
            ended_ = true;
        }
    }

    [[nodiscard]] uint32_t Size() const noexcept { return static_cast<uint32_t>(pos_); }

    [[nodiscard]] bool Ok() const noexcept { return !overflow_ && ended_; }

    [[nodiscard]] bool HasOverflow() const noexcept { return overflow_; }

   private:
    enum class Container : uint8_t { kObject, kArray };

    // Root object + three additional nesting levels.
    static constexpr uint32_t kMaxDepth = 4U;

    [[nodiscard]] bool CanPush() const noexcept { return !ended_ && !overflow_ && depth_ + 1U < kMaxDepth; }

    void Push(Container type) noexcept {
        if (!overflow_) {
            ++depth_;
            container_[depth_] = type;
            first_field_[depth_] = true;
        }
    }

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

    void AppendEscaped(std::string_view value) noexcept {
        constexpr char kHex[] = "0123456789abcdef";

        for (unsigned char c : value) {
            if (c == '"' || c == '\\') {
                Append('\\');
                Append(static_cast<char>(c));
            } else if (c < 0x20U) {
                char escaped[] = {'\\', 'u', '0', '0', kHex[c >> 4U], kHex[c & 0x0FU]};
                Append(std::string_view{escaped, sizeof(escaped)});
            } else {
                Append(static_cast<char>(c));
            }

            if (overflow_) {
                return;
            }
        }
    }

    void BeginElement() noexcept {
        if (ended_ || overflow_ || container_[depth_] != Container::kArray) {
            overflow_ = true;
            return;
        }

        if (!first_field_[depth_]) {
            Append(',');
        }

        if (!overflow_) {
            first_field_[depth_] = false;
        }
    }

    void BeginField(std::string_view first, std::string_view second) noexcept {
        if (ended_ || overflow_ || container_[depth_] != Container::kObject) {
            overflow_ = true;
            return;
        }

        if (!first_field_[depth_]) {
            Append(',');
        }

        Append('"');
        Append(first);
        Append(second);
        Append("\":");

        if (!overflow_) {
            first_field_[depth_] = false;
        }
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
            *--p = static_cast<char>('0' + value % 10U);
            value /= 10U;
        } while (value != 0U);

        Append(std::string_view{p, static_cast<std::size_t>(digits + sizeof(digits) - p)});
    }

    std::span<char> buffer_;
    std::size_t pos_{0};

    bool first_field_[kMaxDepth]{true};
    Container container_[kMaxDepth]{};
    uint32_t depth_{0};

    bool ended_{false};
    bool overflow_{false};
};

#endif // JSON_JSON_JSONDOC_H_
