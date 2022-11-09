#pragma once

#include "TileElementBase.h"

namespace OpenLoco::Map
{
#pragma pack(push, 1)
    struct RoadElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::road;

    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t unkDirection() const { return _type & 0x03; }
        uint8_t roadId() const { return _4 & 0xF; } // _4l
        bool hasBridge() const { return (_4 & 0x80) != 0; }
        uint8_t unk4u() const { return (_4 & 0x30) >> 4; }
        void setUnk4u(uint8_t newUnk4u)
        {
            _4 &= ~(0x30);
            _4 |= (newUnk4u & 0x3) << 4;
        }
        uint8_t roadObjectId() const { return _5 >> 4; }   // _5u
        uint8_t sequenceIndex() const { return _5 & 0x3; } // _5l
        uint8_t bridge() const { return _6 >> 5; }         // _6u
        bool hasStationElement() const { return (_type & 0x80) != 0; }
        bool hasUnk7_10() const { return _7 & (1 << 4); } // _7u (bit 4) level crossing related
        void setUnk7_10(bool newState)                    // _7u (bit 4) level crossing related
        {
            _7 &= ~(1 << 4);
            _7 |= newState ? (1 << 4) : 0;
        }
        bool hasLevelCrossing() const { return _7 & (1 << 5); }
        void setUnk7_40(bool newState) // NOTE: This is the same field as mods!
        {
            _7 &= ~(1 << 6);
            _7 |= newState ? (1 << 6) : 0;
        }
        bool hasMod(uint8_t mod) const { return _7 & (1 << (mod + 6)); } // _7u (bits 6 and 7)
        uint8_t mods() const { return _7 >> 6; }                         // _7u
        CompanyId owner() const { return CompanyId(_7 & 0xF); }          // _7l
        void setOwner(CompanyId newOwner) { _7 = (_7 & 0xF0) | (enumValue(newOwner) & 0xF); }
        bool update(const Map::Pos2& loc);
    };
#pragma pack(pop)
    static_assert(sizeof(RoadElement) == kTileElementSize);
}
