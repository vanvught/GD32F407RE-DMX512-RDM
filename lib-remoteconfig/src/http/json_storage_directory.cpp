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
#include <ctime>
#include <dirent.h>
#include <sys/stat.h>
#include <string_view>

#include "firmware/debug/debug_config.h"
#include "common/utils/utils_string.h"
#include "json/json_jsondoc.h"
#include "common/utils/utils_print.h"

void DriveSize(const char* path, uint32_t& total_bytes, uint32_t& free_bytes);

namespace json::storage {
namespace {
bool Filter(const char* name) {
    return *name == '.';
}

constexpr char kDirName[] = ".";
constexpr std::string_view kJsonNoStorage = R"({"label":"No storage"})";
} // namespace

uint32_t Directory(char* out_buffer, uint32_t out_buffer_size) {
    auto* dirp = opendir(kDirName);

    if constexpr (debug::config::kTraceEnabled) {
        perror("opendir");
    }

    if (dirp == nullptr) {
        constexpr auto kSize = kJsonNoStorage.size();
        memcpy(out_buffer, kJsonNoStorage.data(), kSize);
        return kSize;
    }

    uint32_t total_bytes{};
    uint32_t free_bytes{};
    DriveSize(kDirName, total_bytes, free_bytes);

    JsonDoc doc(out_buffer, out_buffer_size);

    doc["label"] = "storage";
    doc["capacity"] = total_bytes;
    doc["free"] = free_bytes;

    doc.BeginArray("files");

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

            struct stat buf;
            const auto kStat = stat(read_dir->d_name, &buf);

            if (kStat == 0) {
                time_t epoch_time = buf.st_mtime;
                auto* local_time = localtime(&epoch_time);
                char date_time[20];

                common::DateTimeToString(*local_time, date_time);

                doc.BeginObject();

                doc["name"] = read_dir->d_name;
                doc["size"] = static_cast<uint32_t>(buf.st_size);
                doc["date"] = date_time;

                doc.EndObject();

                if (doc.HasOverflow()) {
                    ERROR("JsonDoc overflow");
                    break;
                }
            }
        }
    } while (read_dir != nullptr);

    doc.EndArray();

    closedir(dirp);

    doc.End();

    return doc.Ok() ? doc.Size() : 0U;
}
} // namespace json::storage
#endif // DISABLE_FS
