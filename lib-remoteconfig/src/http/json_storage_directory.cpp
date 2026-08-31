#ifndef DISABLE_FS
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
#include <cstring>
#include <cassert>
#include <dirent.h>
#include <sys/stat.h>

#include "common/utils/utils_string.h"
#include "firmware/debug/debug_config.h"

void DriveSize(const char* path, uint32_t& total_bytes, uint32_t& free_bytes);

namespace json::storage {
namespace {
bool Filter(const char* name) {
    return *name == '.';
}

constexpr char kDirName[] = ".";
constexpr char kJsonNoStorage[] = R"({"label":"No storage"})";
} // namespace

uint32_t Directory(char* out_buffer, uint32_t out_buffer_size) {
    const auto kBufferSize = out_buffer_size - 2U;

    auto* dirp = opendir(kDirName);

    if constexpr (debug::config::kTraceEnabled) {
        perror("opendir");
    }

    if (dirp == nullptr) {
        const auto kSize = common::ConstStrLen(kJsonNoStorage);
        memcpy(out_buffer, kJsonNoStorage, kSize);
        return kSize;
    }

    uint32_t total_bytes{};
    uint32_t free_bytes{};
    DriveSize(kDirName, total_bytes, free_bytes);

    auto length = static_cast<uint32_t>(snprintf(out_buffer, kBufferSize, 
      R"({"label":"storage","capacity":%u,"free":%u,"files":[)", 
        static_cast<unsigned>(total_bytes), 
        static_cast<unsigned>(free_bytes)));

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

            struct stat buf;
            const auto kStat = stat(read_dir->d_name, &buf);

            if (kStat == 0) {
                time_t epoch_time = buf.st_mtime;
                auto* local_time = localtime(&epoch_time);
                const auto kCharacters = static_cast<uint32_t>(snprintf(&out_buffer[length], kSize, 
                  R"({"name":"%s","size":%u,"date":"%d-%.2d-%.2dT%.2d:%.2d:%.2d"},)", 
                  read_dir->d_name, 
                  static_cast<unsigned>(buf.st_size),                                          
                  1900 + local_time->tm_year, 
                  1 + local_time->tm_mon, 
                  local_time->tm_mday, 
                  local_time->tm_hour, 
                  local_time->tm_min, 
                  local_time->tm_sec));

                if (kCharacters > kSize) {
                    break;
                }

                length += kCharacters;

                if (length >= kBufferSize) {
                    break;
                }
            }
        }
    } while (read_dir != nullptr);

    closedir(dirp);

    if (out_buffer[length - 1] == ',') {
        length--;
    }

    out_buffer[length++] = ']';
    out_buffer[length++] = '}';

    assert(length <= out_buffer_size);
    return length;
}
} // namespace json::storage
#endif // DISABLE_FS
