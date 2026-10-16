/**
 * @file json_key.h
 *
 */
/* Copyright (C) 2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef JSON_JSON_KEY_H_
#define JSON_JSON_KEY_H_

#include <cstdint>
#include <string_view>

#include "common/utils/utils_hash.h"

namespace json {

using SimpleSetter = void (*)(std::string_view);
using KeyedSetter = void (*)(std::string_view, std::string_view);

struct KeyDefinition {
    const char* name;
    uint8_t length;
    uint32_t hash;
};

struct Key {
    const KeyDefinition* definition;

    union {
        SimpleSetter set_simple;
        KeyedSetter set_keyed;
    };

    enum { kSimple, kKeyed } type;

    [[nodiscard]] constexpr std::string_view GetName() const noexcept { return {definition->name, definition->length}; }

    [[nodiscard]] constexpr uint32_t GetHash() const noexcept { return definition->hash; }
};

constexpr Key MakeKey(SimpleSetter set, const KeyDefinition& key) noexcept {
    return Key{.definition = &key, .set_simple = set, .type = Key::kSimple};
}

constexpr Key MakeKey(KeyedSetter set, const KeyDefinition& key) noexcept {
    return Key{.definition = &key, .set_keyed = set, .type = Key::kKeyed};
}

template <std::size_t kN>
constexpr KeyDefinition MakeKey(const char (&name)[kN]) noexcept {
    static_assert(kN > 1);
    static_assert(kN - 1 <= UINT8_MAX);

    return {
        .name = name,
        .length = static_cast<uint8_t>(kN - 1),
        .hash = Fnv1a32(std::string_view{name, kN - 1}),
    };
}
} // namespace json

#endif // JSON_JSON_KEY_H_
