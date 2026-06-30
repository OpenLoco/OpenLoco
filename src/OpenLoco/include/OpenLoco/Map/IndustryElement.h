#pragma once

#include "OpenLoco/Core/EnumFlags.hpp"
#include "TileElement.h"

namespace OpenLoco
{
    struct Industry;
}

namespace OpenLoco::World
{
    enum class IndustryElementFlags : uint16_t
    {
        none = 0U,
        randomAnimationTypeMask = 0x3U << 0,
        playingRandomAnimation = 1U << 4,
        randomAnimationQueued = 1U << 5,
        buildingTypeMask = 0x1FU << 6,
        colourMask = 0x1FU << 11,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(IndustryElementFlags);
#pragma pack(push, 1)

    struct IndustryElement : public TileElement
    {
        static constexpr ElementType kElementType = ElementType::industry;

    private:
        IndustryId _industryId;
        uint8_t _5;

        /* Field _6 data structures
         * 0b11111xxxxxxxxxxx = colour
         * 0bxxxxxx11111xxxxx = building type
         * 0bxxxxxxxxxxx1xxxx = random animation is queued and will play at the next opportunity
         * 0bxxxxxxxxxxxx1xxx = random animation is currently playing
         * 0bxxxxxxxxxxxxxx11 = random animation type
         * 0bxxxxx1xxxxxxx1xx = unused bits
         * 0bxxxxxxxxxx111111 = number of building sections
         */
        uint16_t _6;

    public:
        // _4
        IndustryId industryId() const { return _industryId; }
        void setIndustryId(const IndustryId id) { _industryId = id; }
        Industry* industry() const;
        // var_6_07C0
        uint8_t buildingType() const;
        void setBuildingType(uint8_t type)
        {
            _6 &= ~0x7C0;
            _6 |= type << 6;
        }
        uint8_t rotation() const { return _0 & 0x3; }
        void setRotation(const uint8_t rotation)
        {
            _0 &= ~0x3;
            _0 |= rotation & 0x3;
        }
        // var_5_03
        uint8_t sequenceIndex() const;
        void setSequenceIndex(const uint8_t index)
        {
            _5 &= ~0x3;
            _5 |= index & 0x3;
        }
        // var_5_E0
        uint8_t sectionProgress() const;
        void setSectionProgress(uint8_t val);

        Colour var_6_F800() const;
        void setColour(Colour c)
        {
            _6 &= ~0xF800;
            _6 |= enumValue(c) << 11;
        }

        uint8_t sectionsCompleted() const;
        void setSectionsCompleted(uint8_t val);

        bool isConstructed() const { return _0 & 0x80; }
        void setIsConstructed(bool val);

        bool update(const World::Pos2& loc);
        constexpr bool hasFlags(IndustryElementFlags flagsToTest) const
        {
            return (static_cast<IndustryElementFlags>(_6) & flagsToTest) == flagsToTest;
        }
        void setFlags(IndustryElementFlags flagsToSet)
        {
            _6 |= enumValue(flagsToSet);
        }
        void unsetFlags(IndustryElementFlags flagsToUnset)
        {
            _6 &= ~enumValue(flagsToUnset);
        };

        constexpr uint8_t randomAnimationType() const
        {
            return _6 & enumValue(IndustryElementFlags::randomAnimationTypeMask);
        }
        void setRandomAnimationType(uint8_t type);
    };
#pragma pack(pop)
    static_assert(sizeof(IndustryElement) == kTileElementSize);

    struct Animation;
    bool updateIndustryContinuousAnimation(const Animation& anim);
    bool updateIndustryRandomAnimation(const Animation& anim);
}
