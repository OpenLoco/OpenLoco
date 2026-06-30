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
        playingRandomAnimation = 1U << 4,
        randomAnimationQueued = 1U << 5,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(IndustryElementFlags);

    constexpr uint16_t kIndustryElement6ColourMask = 0xF800;
    constexpr uint16_t kIndustryElement6BuildingTypeMask = 0x07C0;
    constexpr uint16_t kIndustryElement6RandomAnimationTypeMask = 0x0003;
    constexpr uint16_t kIndustryElement6SectionsCompletedMask = 0x001F;
    constexpr uint16_t kIndustryElement5TileSequenceMask = 0x03;
    constexpr uint8_t kIndustryElement5SectionConstructionProgressMask = 0xE0;

#pragma pack(push, 1)

    struct IndustryElement : public TileElement
    {
        static constexpr ElementType kElementType = ElementType::industry;

    private:
        IndustryId _industryId;

        /* Field _5 data structures
         * 0b111xxxxx = construction progress of uppermost section
         * 0bxxxxxx11 = sequence number of multi-tile building
         * 0bxxx111xx = unused bits
         */
        uint8_t _5;

        /* Field _6 data structures
         * 0b11111xxxxxxxxxxx = colour
         * 0bxxxxx11111xxxxxx = building type
         * 0bxxxxxxxxxx1xxxxx = random animation is queued and will play at the next opportunity
         * 0bxxxxxxxxxxx1xxxx = random animation is currently playing
         * 0bxxxxxxxxxxxxxx11 = random animation type
         * 0bxxxxxxxxxxxx11xx = unused bits (of above)
         * 0bxxxxxxxxxx111111 = number of building sections completed
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
            _6 &= ~kIndustryElement6BuildingTypeMask;
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
            _5 &= ~kIndustryElement5TileSequenceMask;
            _5 |= index & kIndustryElement5TileSequenceMask;
        }
        // var_5_E0
        uint8_t sectionProgress() const;
        void setSectionProgress(uint8_t val);

        Colour colour() const;
        void setColour(Colour c)
        {
            _6 &= ~kIndustryElement6ColourMask;
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
            return _6 & kIndustryElement6RandomAnimationTypeMask;
        }
        void setRandomAnimationType(uint8_t type);
    };
#pragma pack(pop)
    static_assert(sizeof(IndustryElement) == kTileElementSize);

    struct Animation;
    bool updateIndustryContinuousAnimation(const Animation& anim);
    bool updateIndustryRandomAnimation(const Animation& anim);
}
