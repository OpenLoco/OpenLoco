#pragma once

#include "OpenLoco/Core/EnumFlags.hpp"
#include "TileElement.h"

namespace OpenLoco
{
    struct Industry;
}

namespace OpenLoco::World
{
    // Field 0:  C?TT'TTRR
    // C : Is Constructed
    // T : Element type
    // R : Rotation
    // ? : Unused
    constexpr uint8_t kIndustryElement0RotationMask = 0b0000'0011;
    constexpr uint8_t kIndustryElement0Constructed = (1 << 7);

    // Field 5: PPP?'??SS
    // P : Section construction progress
    // S : Sequence index
    // ? : Unused
    constexpr uint8_t kIndustryElement5TileSequenceMask = 0b0000'0011;
    constexpr uint8_t kIndustryElement5SectionConstructionProgressMask = 0b1110'0000;

    // Field 6: CCCC'CBBB'BBPA'??TT or CCCC'CBBB'BBSS'SSSS
    // C : Colour
    // B : Building type
    // P : Random animation playing
    // A : Random animation available
    // T : Random animation type
    // S : Section construct completed
    // ? : Unused
    constexpr uint16_t kIndustryElement6SectionsCompletedMask = 0b0000'0000'0011'1111;
    constexpr uint16_t kIndustryElement6RandomAnimationTypeMask = 0b0000'0000'0000'0011;
    constexpr uint16_t kIndustryElement6RandomAnimationAvailable = (1 << 4);
    constexpr uint16_t kIndustryElement6RandomAnimationPlaying = (1 << 5);
    constexpr uint16_t kIndustryElement6BuildingTypeMask = 0b0000'0111'1100'0000;
    constexpr uint16_t kIndustryElement6ColourMask = 0b1111'1000'0000'0000;

#pragma pack(push, 1)

    struct IndustryElement : public TileElement
    {
        static constexpr ElementType kElementType = ElementType::industry;

    private:
        IndustryId _industryId;
        uint8_t _5;
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
        uint8_t rotation() const { return _0 & kIndustryElement0RotationMask; }
        void setRotation(const uint8_t rotation)
        {
            _0 &= ~kIndustryElement0RotationMask;
            _0 |= rotation & kIndustryElement0RotationMask;
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

        bool isConstructed() const { return _0 & kIndustryElement0Constructed; }
        void setIsConstructed(bool val);

        bool update(const World::Pos2& loc);

        bool randomAnimationPlaying() const { return _6 & kIndustryElement6RandomAnimationPlaying; }
        void setRandomAnimationPlaying(bool val);

        bool randomAnimationAvailable() const { return _6 & kIndustryElement6RandomAnimationAvailable; }
        void setRandomAnimationAvailable(bool val);

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
