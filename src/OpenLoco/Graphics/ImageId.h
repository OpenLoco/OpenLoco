#include "Colour.h"

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
    private:
        // clang-format off
        static constexpr uint32_t kMaskIndex       = 0b00000000000001111111111111111111;
        static constexpr uint32_t kMaskRemap       = 0b00000111111110000000000000000000;
        static constexpr uint32_t kMaskPrimary     = 0b00000000111110000000000000000000;
        static constexpr uint32_t kMaskSecondary   = 0b00011111000000000000000000000000;
        static constexpr uint32_t kFlagPrimary     = 0b00100000000000000000000000000000;
        static constexpr uint32_t kFlagBlend       = 0b01000000000000000000000000000000;
        static constexpr uint32_t kFlagSecondary   = 0b10000000000000000000000000000000;
        static constexpr uint32_t kShiftRemap      = 19;
        static constexpr uint32_t kShiftPrimary    = 19;
        static constexpr uint32_t kShiftSecondary  = 24;
        static constexpr uint32_t kIndexUndefined  = 0b00000000000001111111111111111111;
        static constexpr uint32_t kValueUndefined  = kIndexUndefined;
        // clang-format on

        // NONE = No remap
        // BLENDED = No source copy, remap destination only (glass)
        // PRIMARY | BLENDED = Destination is blended with source (water)
        // PRIMARY = Remap with palette id (first 32 are colour palettes)
        // PRIMARY | SECONDARY = Remap with primary and secondary colours

        static constexpr ImageIndex kImageIndexUndefined = std::numeric_limits<ImageIndex>::max();
        uint32_t _index = kImageIndexUndefined;

    public:
        static ImageId fromUInt32(uint32_t value)
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

        uint32_t toUInt32() const
        {
            return _index;
        }

        bool hasValue() const
        {
            return getIndex() != kImageIndexUndefined;
        }

        bool hasPrimary() const
        {
            return _index & kFlagPrimary;
        }

        bool hasSecondary() const
        {
            return _index & kFlagSecondary;
        }

        bool isRemap() const
        {
            return (_index & kFlagPrimary) && !(_index & kFlagSecondary);
        }

        bool isBlended() const
        {
            return _index & kFlagBlend;
        }

        ImageIndex getIndex() const
        {
            return _index & kMaskIndex;
        }

        ExtColour getRemap() const
        {
            return static_cast<ExtColour>((_index & kMaskRemap) >> kShiftRemap);
        }

        Colour getPrimary() const
        {
            return static_cast<Colour>((_index & kMaskPrimary) >> kShiftPrimary);
        }

        Colour getSecondary() const
        {
            return static_cast<Colour>((_index & kMaskSecondary) >> kShiftSecondary);
        }

        constexpr ImageId withIndex(ImageIndex index) const
        {
            ImageId result = *this;
            result._index = index;
            return result;
        }

        constexpr ImageId withIndexOffset(ImageIndex offset) const
        {
            ImageId result = *this;
            result._index += offset;
            return result;
        }

        constexpr ImageId withRemap(ExtColour paletteId) const
        {
            ImageId result = *this;
            result._index &= ~(kMaskPrimary | kMaskSecondary | kFlagSecondary | kFlagBlend);
            result._index |= enumValue(paletteId) << kShiftRemap;
            result._index |= kFlagPrimary;
            return result;
        }

        constexpr ImageId withPrimary(Colour colour) const
        {
            ImageId result = *this;
            result._index &= ~kMaskPrimary;
            result._index |= enumValue(colour) << kShiftPrimary;
            result._index |= kFlagPrimary;
            return result;
        }

        constexpr ImageId withSecondary(Colour colour) const
        {
            ImageId result = *this;
            result._index &= ~kMaskSecondary;
            result._index |= enumValue(colour) << kShiftSecondary;
            result._index |= kFlagSecondary;
            return result;
        }

        ImageId withTransparancy(ExtColour colour) const
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
