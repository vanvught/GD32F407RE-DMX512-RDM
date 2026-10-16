/**
 * @file json_parser.h
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

#ifndef JSON_JSON_PARSER_H_
#define JSON_JSON_PARSER_H_

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "common/utils/utils_hash.h"
#include "json/json_key.h"
#include "json/json_tokenizer.h"

inline void ParseJsonWithTable(std::string_view input, const json::Key* keys, size_t key_count) {
    JsonTokenizer tok(input);
    tok.SkipWhitespace();

    if (tok.p >= tok.end || *tok.p != '{') {
        return;
    }

    ++tok.p;

    while (tok.p < tok.end) {
        std::string_view json_key;

        if (!tok.NextString(json_key)) {
            break;
        }

        if (!tok.Expect(':')) {
            break;
        }

        std::string_view value;

        if (!tok.NextValue(value)) {
            break;
        }

        const auto kHash = Fnv1a32(json_key);

        for (size_t i = 0; i < key_count; ++i) {
            const auto& key = keys[i];

            if (key.GetHash() != kHash || key.GetName() != json_key) {
                continue;
            }

            if (key.type == json::Key::kSimple) {
                key.set_simple(value);
            } else {
                key.set_keyed(json_key, value);
            }

            break;
        }

        tok.SkipWhitespace();

        if (tok.p < tok.end && *tok.p == ',') {
            ++tok.p;
        } else if (tok.p < tok.end && *tok.p == '}') {
            break;
        } else {
            break;
        }
    }
}

template <size_t kN>
inline void ParseJsonWithTable(std::string_view input, const json::Key (&keys)[kN]) {
    ParseJsonWithTable(input, keys, kN);
}

// Temporary compatibility overload.
template <size_t kN>
inline void ParseJsonWithTable(const char* buffer, size_t size, const json::Key (&keys)[kN]) {
    ParseJsonWithTable(std::string_view{buffer, size}, keys);
}

#endif // JSON_JSON_PARSER_H_
