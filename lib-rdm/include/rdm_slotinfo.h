/**
 * @file rdm_slotinfo.h
 *
 */
/* Copyright (C) 2018-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef RDMSLOTINFO_H_
#define RDMSLOTINFO_H_

#include <cstdint>
#include <string_view>

namespace rdm::slotinfo {
// Appendix C: Slot Info (Normative)
// Table C-1: Slot Type
enum class SlotType : uint8_t {
    kStPrimary = 0x00,
    kStSecFine = 0x01,
    kStSecTiming = 0x02,
    kStSecSpeed = 0x03,
    kStSecControl = 0x04,
    kStSecIndex = 0x05,
    kStSecRotation = 0x06,
    kStSecIndexRotate = 0x07,
    kStSecUndefined = 0xFF,
};
// Table C-2: Slot ID Definitions
// http://tsp.esta.org/tsp/working_groups/CP/RDMextras.html
enum class SlotDefinition : uint16_t {
    kSdIntensity = 0x0001,
    kSdIntensityMaster = 0x0002,
    kSdPan = 0x0101,
    kSdTilt = 0x0102,
    kSdColorWheel = 0x0201,
    kSdColorSubCyan = 0x0202,
    kSdColorSubYellow = 0x0203,
    kSdColorSubMagenta = 0x0204,
    kSdColorAddRed = 0x0205,
    kSdColorAddGreen = 0x0206,
    kSdColorAddBlue = 0x0207,
    kSdColorCorrection = 0x0208,
    kSdColorScroll = 0x0209,
    kSdColorSemaphore = 0x0210,
    kSdColorAddAmber = 0x0211,
    kSdColorAddWhite = 0x0212,
    kSdColorAddWarmWhite = 0x0213,
    kSdColorAddCoolWhite = 0x0214,
    kSdColorSubUv = 0x0215,
    kSdColorHue = 0x0216,
    kSdColorSaturation = 0x0217,
    kSdStaticGoboWheel = 0x0301,
    kSdRotoGoboWheel = 0x0302,
    kSdPrismWheel = 0x0303,
    kSdEffectsWheel = 0x0304,
    kSdBeamSizeIris = 0x0401,
    kSdEdge = 0x0402,
    kSdFrost = 0x0403,
    kSdStrobe = 0x0404,
    kSdZoom = 0x0405,
    kSdFramingShutter = 0x0406,
    kSdShutterRotate = 0x0407,
    kSdDouser = 0x0408,
    kSdBarnDoor = 0x0409,
    kSdLampControl = 0x0501,
    kSdFixtureControl = 0x0502,
    kSdFixtureSpeed = 0x0503,
    kSdMacro = 0x0504,
    kSdPowerControl = 0x0505,
    kSdFanControl = 0x0506,
    kSdHeaterControl = 0x0507,
    kSdFountainControl = 0x0508,
    kSdUndefined = 0xFFFF,
};

struct SlotInfo {
    SlotDefinition category{SlotDefinition::kSdUndefined};
    SlotType type{SlotType::kStSecUndefined};
};

[[nodiscard]] std::string_view GetTypeText(SlotType slot_type);
[[nodiscard]] std::string_view GetCategoryText(uint16_t slot_offset, SlotDefinition slot_definition);
[[nodiscard]] std::string_view GetCategoryTextUndefined(uint16_t slot_offset) __attribute__((weak));
} // namespace rdm::slotinfo

#endif // RDMSLOTINFO_H_
