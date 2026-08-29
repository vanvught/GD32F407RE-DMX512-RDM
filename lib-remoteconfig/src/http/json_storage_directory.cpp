#if !defined(DISABLE_FS)
/**
 * @file json_storage_directory.cpp
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

#include <cstdio>
#include <cstdint>
#include <cassert>
#include <dirent.h>

namespace json::storage {
static bool Filter(const char* name) {
    return *name == '.';
}

uint32_t Directory(char* out_buffer, uint32_t out_buffer_size) {
    const auto kBufferSize = out_buffer_size - 2U;
#if defined(__linux__) || defined(__APPLE__)
    auto* dirp = opendir("storage");
#else
    auto* dirp = opendir(".");
#endif
    perror("opendir");

    auto length = static_cast<uint32_t>(snprintf(out_buffer, kBufferSize, R"({"label":"%s","files":[)", (dirp != nullptr) ? "storage" : "No storage"));

    if (dirp != nullptr) {
        struct dirent* read_dir{};
        do {
            read_dir = readdir(dirp);
            if (read_dir != nullptr) {
                if (read_dir->d_type == DT_DIR) {
                    continue;
                }

                if (Filter(read_dir->d_name)) {
                    continue;
                }

                const auto kSize = kBufferSize - length;
                const auto kCharacters = static_cast<uint32_t>(snprintf(&out_buffer[length], kSize, "\"%s\",", read_dir->d_name));

                if (kCharacters > kSize) {
                    break;
                }

                length += kCharacters;

                if (length >= kBufferSize) {
                    break;
                }
            }
        } while (read_dir != nullptr);

        closedir(dirp);

        if (out_buffer[length - 1] == ',') {
            length--;
        }
    }

    out_buffer[length++] = ']';
    out_buffer[length++] = '}';

    assert(length <= out_buffer_size);
    return length;
}
} // namespace json::storage
#endif
