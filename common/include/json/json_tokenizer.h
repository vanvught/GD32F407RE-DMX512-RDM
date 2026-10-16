/**
 * @file json_tokenizer.h
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

#ifndef JSON_JSON_TOKENIZER_H_
#define JSON_JSON_TOKENIZER_H_

#include <string_view>

struct JsonTokenizer {
    const char* p;
    const char* end;

    constexpr explicit JsonTokenizer(std::string_view input) noexcept : p(input.data()), end(input.data() + input.size()) {}

    constexpr void SkipWhitespace() noexcept {
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
            ++p;
        }
    }

    [[nodiscard]] constexpr bool NextString(std::string_view& out) noexcept {
        SkipWhitespace();

        if (p >= end || *p != '"') {
            return false;
        }

        ++p; // Skip opening quote
        const auto* start = p;

        while (p < end) {
            if (*p == '"') {
                out = {start, static_cast<size_t>(p - start)};
                ++p; // Skip closing quote
                return true;
            }

            if (*p == '\\') {
                ++p;

                if (p >= end) {
                    return false;
                }

                // Validate JSON escape character.
                switch (*p) {
                    case '"':
                    case '\\':
                    case '/':
                    case 'b':
                    case 'f':
                    case 'n':
                    case 'r':
                    case 't':
                        break;

                    case 'u':
                        // Unicode escapes require four hex digits.
                        // Not supported by this lightweight tokenizer.
                        return false;

                    default:
                        return false;
                }
            } else if (static_cast<unsigned char>(*p) < 0x20U) {
                // Unescaped control characters are invalid in JSON strings.
                return false;
            }

            ++p;
        }

        return false;
    }

    [[nodiscard]] constexpr bool Expect(char character) noexcept {
        SkipWhitespace();

        if (p >= end || *p != character) {
            return false;
        }

        ++p;
        return true;
    }

    [[nodiscard]] constexpr bool NextValue(std::string_view& out) noexcept {
        SkipWhitespace();

        if (p >= end) {
            return false;
        }

        if (*p == '"') {
            return NextString(out);
        }

        const auto* start = p;

        while (p < end && *p != ',' && *p != '}' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
            ++p;
        }

        if (p == start) {
            return false;
        }

        out = {start, static_cast<size_t>(p - start)};
        return true;
    }
};

#endif // JSON_JSON_TOKENIZER_H_
