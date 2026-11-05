/**
 * @file rdm_slotinfo.cpp
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

#include <cstdint>
#include <string_view>

#include "rdm_slotinfo.h"
#include "rdm_e120.h"

namespace rdm::slotinfo {
// Appendix C: Slot Info (Normative)
namespace {
struct SlotTypeEntry {
    rdm::slotinfo::SlotType id;
    std::string_view description;
};

constexpr SlotTypeEntry kTableC1[] = {
    {.id = SlotType::kStPrimary, .description = "Slot directly controls parameter (represents Coarse for 16-bit parameters)"},
    {.id = SlotType::kStSecFine, .description = "Fine, for 16-bit parameters"},
    {.id = SlotType::kStSecTiming, .description = "Slot sets timing value for associated parameter"},
    {.id = SlotType::kStSecSpeed, .description = "Slot sets speed/velocity for associated parameter"},
    {.id = SlotType::kStSecControl, .description = "Slot provides control/mode info for parameter"},
    {.id = SlotType::kStSecIndex, .description = "Slot sets index position for associated parameter"},
    {.id = SlotType::kStSecRotation, .description = "Slot sets rotation speed for associated parameter"},
    {.id = SlotType::kStSecIndexRotate, .description = "Combined index/rotation control"},
    {.id = SlotType::kStSecUndefined, .description = "Undefined secondary type"}, //
};

struct SlotDefinitionEntry {
    rdm::slotinfo::SlotDefinition id;
    std::string_view description;
};

constexpr SlotDefinitionEntry kTableC2[] = {
    // Intensity Functions
    {.id = SlotDefinition::kSdIntensity, .description = "Intensity"},
    {.id = SlotDefinition::kSdIntensityMaster, .description = "Intensity Master"},
    // Movement Functions
    {.id = SlotDefinition::kSdPan, .description = "Pan"},
    {.id = SlotDefinition::kSdTilt, .description = "Tilt"},
    // Color Functions
    {.id = SlotDefinition::kSdColorWheel, .description = "Color Wheel"},
    {.id = SlotDefinition::kSdColorSubCyan, .description = "Subtractive Color Mixer – Cyan/Blue"},
    {.id = SlotDefinition::kSdColorSubYellow, .description = "Subtractive Color Mixer – Yellow/Amber"},
    {.id = SlotDefinition::kSdColorSubMagenta, .description = "Subtractive Color Mixer - Magenta"},
    {.id = SlotDefinition::kSdColorAddRed, .description = "Additive Color Mixer - Red"},
    {.id = SlotDefinition::kSdColorAddGreen, .description = "Additive Color Mixer - Green"},
    {.id = SlotDefinition::kSdColorAddBlue, .description = "Additive Color Mixer - Blue"},
    {.id = SlotDefinition::kSdColorCorrection, .description = "Color Temperature Correction"},
    {.id = SlotDefinition::kSdColorScroll, .description = "Color Scroll feature"},
    {.id = SlotDefinition::kSdColorSemaphore, .description = "Color Semaphore feature"},
    {.id = SlotDefinition::kSdColorAddAmber, .description = "Additive Color Mixer - Amber"},
    {.id = SlotDefinition::kSdColorAddWhite, .description = "Additive Color Mixer - White"},
    {.id = SlotDefinition::kSdColorAddWarmWhite, .description = "Additive Color Mixer - Warm White"},
    {.id = SlotDefinition::kSdColorAddCoolWhite, .description = "Additive Color Mixer - Cool White"},
    {.id = SlotDefinition::kSdColorSubUv, .description = "Subtractive Color Mixer - UV"},
    {.id = SlotDefinition::kSdColorHue, .description = "Hue"},
    {.id = SlotDefinition::kSdColorSaturation, .description = "Saturation"},
    // Image Functions
    {.id = SlotDefinition::kSdStaticGoboWheel, .description = "Static gobo wheel"},
    {.id = SlotDefinition::kSdRotoGoboWheel, .description = "Rotating gobo wheel"},
    {.id = SlotDefinition::kSdPrismWheel, .description = "Prism wheel"},
    {.id = SlotDefinition::kSdEffectsWheel, .description = "Effects wheel"},
    // Beam Functions
    {.id = SlotDefinition::kSdBeamSizeIris, .description = "Beam size iris"},
    {.id = SlotDefinition::kSdEdge, .description = "Edge/Lens focus"},
    {.id = SlotDefinition::kSdFrost, .description = "Frost/Diffusion"},
    {.id = SlotDefinition::kSdStrobe, .description = "Strobe/Shutter"},
    {.id = SlotDefinition::kSdZoom, .description = "Zoom lens"},
    {.id = SlotDefinition::kSdFramingShutter, .description = "Framing shutter"},
    {.id = SlotDefinition::kSdShutterRotate, .description = "Framing shutter rotation"},
    {.id = SlotDefinition::kSdDouser, .description = "Douser"},
    {.id = SlotDefinition::kSdBarnDoor, .description = "Barn Door"},
    // Control Functions
    {.id = SlotDefinition::kSdLampControl, .description = "Lamp control functions"},
    {.id = SlotDefinition::kSdFixtureControl, .description = "Fixture control channel"},
    {.id = SlotDefinition::kSdFixtureSpeed, .description = "Overall speed / timing parameter"},
    //
    {.id = SlotDefinition::kSdMacro, .description = "Macro control"},
    {.id = SlotDefinition::kSdPowerControl, .description = "Relay or power control"},
    {.id = SlotDefinition::kSdFanControl, .description = "Fan control"},
    {.id = SlotDefinition::kSdHeaterControl, .description = "Heater control"},
    {.id = SlotDefinition::kSdFountainControl, .description = "Fountain water pump control "},
    // Undefined
    {.id = SlotDefinition::kSdUndefined, .description = "No definition"}, //
};
} // namespace
std::string_view GetTypeText(SlotType slot_type) {
    for (const auto& entry : kTableC1) {
        if (slot_type == entry.id) {
            return entry.description;
        }
    }

    return "Undefined secondary type";
}

std::string_view GetCategoryText(uint16_t slot_offset, SlotDefinition slot_definition) {
    if (slot_definition == SlotDefinition::kSdUndefined) {
        return GetCategoryTextUndefined(slot_offset);
    }

    uint32_t low = 0;
    uint32_t high = std::size(kTableC2);

    while (low < high) {
        const auto kMid = low + ((high - low) / 2);

        if (kTableC2[kMid].id < slot_definition) {
            low = kMid + 1;
        } else {
            high = kMid;
        }
    }

    if (low < std::size(kTableC2) && kTableC2[low].id == slot_definition) {
        return kTableC2[low].description;
    }

    return {};
}

std::string_view GetCategoryTextUndefined([[maybe_unused]] uint16_t slot_offset) {
    return "No definition";
}
} // namespace rdm::slotinfo