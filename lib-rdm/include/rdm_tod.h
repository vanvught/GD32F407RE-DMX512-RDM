/**
 * @file rdm_tod.h
 *
 */
/* Copyright (C) 2017-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef RDMTOD_H_
#define RDMTOD_H_

#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>

#include "rdm_debug.h"
#include "rdmconst.h"
#include "common/utils/utils_hex.h"

namespace rdm {
inline constexpr uint32_t kUidStringLength = 13;

inline void FormatUid(uint64_t uid, char (&buffer)[kUidStringLength + 1]) noexcept {
    for (uint32_t i = 0; i < 12; ++i) {
        const auto kShift = (11U - i) * 4U;
        const auto kDigit = static_cast<uint32_t>((uid >> kShift) & 0xFU);
        const auto kPos = i < 4U ? i : i + 1U;

        buffer[kPos] = common::hex::ToCharLowercase(kDigit);
    }

    buffer[4] = ':';
    buffer[kUidStringLength] = '\0';
}

inline void FormatUid(const uint8_t* uid, char (&buffer)[kUidStringLength + 1]) noexcept {
    for (uint32_t i = 0; i < rdm::kUidSize; ++i) {
        const auto kPos = (2U * i) + (i >= 2U ? 1U : 0U);

        buffer[kPos] = common::hex::ToCharLowercase(uid[i] >> 4U);
        buffer[kPos + 1U] = common::hex::ToCharLowercase(uid[i]);
    }

    buffer[4] = ':';
    buffer[kUidStringLength] = '\0';
}

inline void PrintUid(const uint8_t* uid) {
    char buffer[kUidStringLength + 1];
    FormatUid(uid, buffer);
    printf("%s", buffer);
}

class Tod {
   public:
#ifndef RDM_DISCOVERY_TOD_TABLE_SIZE
#define RDM_DISCOVERY_TOD_TABLE_SIZE 200U
#endif // RDM_DISCOVERY_TOD_TABLE_SIZE
    static constexpr uint32_t kTableSize = RDM_DISCOVERY_TOD_TABLE_SIZE;
    static_assert(kTableSize > 0, "RDM TOD table must not be empty");
    static constexpr uint32_t kMutesTableSize = (kTableSize + 31) / 32;
    static constexpr uint32_t kInvalidEntry = UINT32_MAX;

    Tod() noexcept {
        for (auto& tod : tod_) {
            memcpy(&tod, rdm::kUidAll, rdm::kUidSize);
        }
    }

    ~Tod() = default;

    void Reset() {
        for (uint32_t i = 0; i < entries_; i++) {
            memcpy(&tod_[i], rdm::kUidAll, rdm::kUidSize);
        }

        entries_ = 0;
        saved_index_ = kInvalidEntry;

        for (uint32_t& mute : mutes_) {
            mute = 0;
        }
    }

    [[nodiscard]] uint32_t FindIndex(const uint8_t* uid) const noexcept {
        for (uint32_t index = 0; index < entries_; index++) {
            if (memcmp(tod_[index].uid, uid, rdm::kUidSize) == 0) {
                return index;
            }
        }

        return kInvalidEntry;
    }

    bool AddUid(const uint8_t* uid) noexcept {
        if (entries_ >= kTableSize) {
            return false;
        }

        if (FindIndex(uid) != kInvalidEntry) {
            return false;
        }

        memcpy(tod_[entries_++].uid, uid, rdm::kUidSize);
        return true;
    }

    [[nodiscard]] uint32_t UidCount() const noexcept { return entries_; }

    bool CopyUidEntry(uint32_t index, uint8_t uid[rdm::kUidSize]) const noexcept {
        if (index >= entries_) {
            memcpy(uid, rdm::kUidAll, rdm::kUidSize);
            return false;
        }

        memcpy(uid, &tod_[index], rdm::kUidSize);
        return true;
    }

    void Copy(uint8_t* table) const noexcept {
        assert(table != nullptr);
        static_assert(sizeof(Uid) == rdm::kUidSize);
        memcpy(table, tod_, entries_ * sizeof(Uid));
    }

    bool Delete(const uint8_t* uid) {
        uint32_t entry = 0;

        for (; entry < entries_; ++entry) {
            if (memcmp(tod_[entry].uid, uid, rdm::kUidSize) == 0) {
                break;
            }
        }

        if (entry == entries_) {
            return false;
        }

        for (uint32_t i = entry; i + 1 < entries_; i++) {
            tod_[i] = tod_[i + 1];

            // Move the mute state with its UID.
            const auto kNext = i + 1U;
            const bool kMuted = (mutes_[kNext / 32U] & (uint32_t{1} << (kNext % 32U))) != 0;
            const auto kMask = uint32_t{1} << (i % 32U);
            auto& mute = mutes_[i / 32U];

            if (kMuted) {
                mute |= kMask;
            } else {
                mute &= ~kMask;
            }
        }

        entries_--;
        memcpy(tod_[entries_].uid, rdm::kUidAll, rdm::kUidSize);

        // Clear the mute bit belonging to the now-unused slot.
        mutes_[entries_ / 32U] &= ~(uint32_t{1} << (entries_ % 32U));

        // Adjust the saved index.
        if (saved_index_ == entry) {
            saved_index_ = kInvalidEntry;
        } else if (saved_index_ != kInvalidEntry && saved_index_ > entry) {
            --saved_index_;
        }

        return true;
    }

    bool Exist(const uint8_t* uid) noexcept {
        saved_index_ = FindIndex(uid);
        return saved_index_ != kInvalidEntry;
    }

    [[nodiscard]] const uint8_t* Next() noexcept {
        if (entries_ == 0) {
            saved_index_ = kInvalidEntry;
            return nullptr;
        }

        if (saved_index_ == kInvalidEntry || saved_index_ >= entries_ - 1) {
            saved_index_ = 0;
        } else {
            ++saved_index_;
        }

        return tod_[saved_index_].uid;
    }

    void Mute() {
        if (saved_index_ == kInvalidEntry) {
            return;
        }

        const auto kIndex = saved_index_ / 32U;
        const auto kShift = saved_index_ % 32U;

        mutes_[kIndex] |= (uint32_t{1} << kShift);
    }

    void UnMute() {
        if (saved_index_ >= entries_) {
            return;
        }

        const auto kIndex = saved_index_ / 32U;
        const auto kShift = saved_index_ % 32U;

        mutes_[kIndex] &= ~(uint32_t{1} << kShift);
    }

    void UnMuteAll() {
        for (uint32_t& mute : mutes_) {
            mute = 0;
        }
    }

    [[nodiscard]] bool IsMuted() const noexcept {
        if (saved_index_ >= entries_) {
            return true;
        }

        const auto kIndex = saved_index_ / 32U;
        const auto kShift = saved_index_ % 32U;

        return (mutes_[kIndex] & (uint32_t{1} << kShift)) != 0;
    }

    void Dump([[maybe_unused]] uint32_t count) const {
        if constexpr (kDebugRdmDiscoveryEnabled) {
            if (count > kTableSize) {
                count = kTableSize;
            }

            printf("[%u]\n", static_cast<unsigned int>(count));
            for (uint32_t i = 0; i < count; i++) {
                printf("%.2x%.2x:%.2x%.2x%.2x%.2x\n", tod_[i].uid[0], tod_[i].uid[1], tod_[i].uid[2], tod_[i].uid[3], tod_[i].uid[4], tod_[i].uid[5]);
            }
        }
    }

    void Dump() const {
        if constexpr (kDebugRdmDiscoveryEnabled) {
            Dump(entries_);
        }
    }

   private:
    uint32_t entries_{0};
    uint32_t saved_index_{kInvalidEntry};
    uint32_t mutes_[kMutesTableSize]{};
    struct Uid {
        uint8_t uid[rdm::kUidSize];
    };
    Uid tod_[kTableSize];
};
} // namespace rdm

#endif // RDMTOD_H_
