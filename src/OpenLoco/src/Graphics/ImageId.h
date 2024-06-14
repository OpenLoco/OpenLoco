#pragma once

#include "Colour.h"
#include <cassert>

namespace OpenLoco
{

    using ImageIndex = uint32_t;

    /**
     * Represents a specific image from a catalogue such as G1, object etc. with remap
     * colours and flags.
     *
     * This is currently all stored as a single 32-bit integer, but will allow easy
     * extension to 64-bits or higher so that more images can be used.
     */
    struct ImageId
    {
        // clang-format off
        static constexpr uint32_t kIndexUndefined   = 0b00000000000001111111111111111111;
    private:
        static constexpr uint32_t kMaskIndex        = 0b00000000000001111111111111111111;
        static constexpr uint32_t kMaskRemap        = 0b00000011111110000000000000000000;
        static constexpr uint32_t kMaskTranslucent  = 0b00000111111110000000000000000000;
        static constexpr uint32_t kMaskPrimary      = 0b00000000111110000000000000000000;
        static constexpr uint32_t kMaskSecondary    = 0b00011111000000000000000000000000;
        static constexpr uint32_t kMaskNoiseMask    = 0b00011100000000000000000000000000;
        static constexpr uint32_t kFlagPrimary      = 0b00100000000000000000000000000000;
        static constexpr uint32_t kFlagBlend        = 0b01000000000000000000000000000000;
        static constexpr uint32_t kFlagSecondary    = 0b10000000000000000000000000000000;
        static constexpr uint32_t kShiftRemap       = 19;
        static constexpr uint32_t kShiftPrimary     = 19;
        static constexpr uint32_t kShiftTranslucent = 19;
        static constexpr uint32_t kShiftSecondary   = 24;
        static constexpr uint32_t kShiftNoiseMask   = 26;
        static constexpr uint32_t kValueUndefined   = kIndexUndefined;
        // clang-format on

        // Noise mask can be used with NONE and PRIMARY
        // NONE = No remap
        // BLENDED = No source copy, remap destination only (glass)
        // PRIMARY | BLENDED = Destination is blended with source (water)
        // PRIMARY = Remap with palette id (first 32 are colour palettes)
        // PRIMARY | SECONDARY = Remap with primary and secondary colours

        static constexpr ImageIndex kImageIndexUndefined = std::numeric_limits<ImageIndex>::max();
        uint32_t _index = kImageIndexUndefined;

    public:
        [[nodiscard]] static ImageId fromUInt32(uint32_t value)
        {
            ImageId result;
            result._index = value;
            return result;
        }

        ImageId() = default;

        explicit constexpr ImageId(ImageIndex index)
            : _index(index == kIndexUndefined ? kImageIndexUndefined : index)
        {
        }

        constexpr ImageId(uint32_t index, ExtColour palette)
            : ImageId(ImageId(index).withRemap(palette))
        {
        }

        constexpr ImageId(uint32_t index, Colour primaryColour)
            : ImageId(ImageId(index).withPrimary(primaryColour))
        {
        }

        constexpr ImageId(uint32_t index, Colour primaryColour, Colour secondaryColour)
            : ImageId(ImageId(index).withPrimary(primaryColour).withSecondary(secondaryColour))
        {
        }

        constexpr ImageId(uint32_t index, ColourScheme scheme)
            : ImageId(index, scheme.primary, scheme.secondary)
        {
        }

        [[nodiscard]] constexpr uint32_t toUInt32() const
        {
            return _index;
        }

        [[nodiscard]] constexpr bool hasValue() const
        {
            return getIndex() != kImageIndexUndefined;
        }

        [[nodiscard]] constexpr bool hasPrimary() const
        {
            return _index & kFlagPrimary;
        }

        [[nodiscard]] constexpr bool hasSecondary() const
        {
            return _index & kFlagSecondary;
        }

        [[nodiscard]] constexpr bool hasNoiseMask() const
        {
            return !isBlended() && !hasSecondary() && (getNoiseMask() != 0);
        }

        [[nodiscard]] constexpr bool isRemap() const
        {
            return hasPrimary() && !hasSecondary() && !isBlended();
        }

        [[nodiscard]] constexpr bool isBlended() const
        {
            return _index & kFlagBlend;
        }

        [[nodiscard]] constexpr ImageIndex getIndex() const
        {
            return _index & kMaskIndex;
        }

        [[nodiscard]] constexpr ExtColour getTranslucency() const
        {
            return static_cast<ExtColour>((_index & kMaskTranslucent) >> kShiftTranslucent);
        }

        [[nodiscard]] constexpr ExtColour getRemap() const
        {
            return static_cast<ExtColour>((_index & kMaskRemap) >> kShiftRemap);
        }

        [[nodiscard]] constexpr Colour getPrimary() const
        {
            return static_cast<Colour>((_index & kMaskPrimary) >> kShiftPrimary);
        }

        [[nodiscard]] constexpr Colour getSecondary() const
        {
            return static_cast<Colour>((_index & kMaskSecondary) >> kShiftSecondary);
        }

        [[nodiscard]] constexpr uint8_t getNoiseMask() const
        {
            return (_index & kMaskNoiseMask) >> kShiftNoiseMask;
        }

        [[nodiscard]] constexpr ImageId withIndex(ImageIndex index) const
        {
            ImageId result = *this;
            result._index &= ~kMaskIndex;
            result._index |= index;
            return result;
        }

        [[nodiscard]] constexpr ImageId withIndexOffset(ImageIndex offset) const
        {
            ImageId result = *this;
            result._index += offset;
            return result;
        }

        [[nodiscard]] constexpr ImageId withRemap(ExtColour paletteId) const
        {
            ImageId result = *this;
            assert(enumValue(paletteId) <= 0x7F); // If larger then it eats into noiseMask
            result._index &= ~(kMaskRemap | kFlagSecondary | kFlagBlend);
            result._index |= enumValue(paletteId) << kShiftRemap;
            result._index |= kFlagPrimary;
            return result;
        }

        // Can be used withRemap or withPrimary or with None
        [[nodiscard]] constexpr ImageId withNoiseMask(uint8_t noise) const
        {
            ImageId result = *this;
            result._index &= ~(kMaskNoiseMask | kFlagSecondary | kFlagBlend);
            result._index |= noise << kShiftNoiseMask;
            return result;
        }

        [[nodiscard]] constexpr ImageId withPrimary(Colour colour) const
        {
            ImageId result = *this;
            result._index &= ~kMaskPrimary;
            result._index |= enumValue(colour) << kShiftPrimary;
            result._index |= kFlagPrimary;
            return result;
        }

        [[nodiscard]] constexpr ImageId withSecondary(Colour colour) const
        {
            ImageId result = *this;
            result._index &= ~kMaskSecondary;
            result._index |= enumValue(colour) << kShiftSecondary;
            result._index |= kFlagSecondary;
            return result;
        }

        [[nodiscard]] constexpr ImageId withTranslucency(ExtColour colour) const
        {
            ImageId result = *this;
            result._index &= ~(kMaskPrimary | kMaskSecondary | kFlagSecondary | kFlagPrimary);
            result._index |= enumValue(colour) << kShiftRemap;
            result._index |= kFlagBlend;
            return result;
        }
    };
    static_assert(sizeof(ImageId) == 4);
}
