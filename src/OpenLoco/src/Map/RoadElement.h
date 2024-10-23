#pragma once

#include "TileElementBase.h"

namespace OpenLoco::World
{
    struct Animation;

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
        RoadElement() = default;
        RoadElement(World::SmallZ baseZ, World::SmallZ clearZ);

        uint8_t rotation() const { return _type & 0x03; }
        void setRotation(uint8_t rotation)
        {
            _type &= ~0x3;
            _type |= rotation & 0x03;
        }
        uint8_t roadId() const { return _4 & 0xF; } // _4l
        void setRoadId(uint8_t roadId)
        {
            _4 &= ~0xF;
            _4 |= roadId & 0xF;
        }
        void setHasGhostMods(bool hasGhostMods)
        {
            _4 &= ~0x40;
            _4 |= hasGhostMods ? 0x40 : 0;
        }
        bool hasGhostMods() const { return (_4 & 0x40) != 0; }
        bool hasBridge() const { return (_4 & 0x80) != 0; }
        void setHasBridge(bool hasBridge)
        {
            _4 &= ~0x80;
            _4 |= hasBridge ? 0x80 : 0;
        }
        uint8_t unk4u() const { return (_4 & 0x30) >> 4; }
        void setUnk4u(uint8_t newUnk4u)
        {
            _4 &= ~(0x30);
            _4 |= (newUnk4u & 0x3) << 4;
        }
        uint8_t roadObjectId() const { return _5 >> 4; } // _5u
        void setRoadObjectId(uint8_t objectId)
        {
            _5 &= ~0xF0;
            _5 |= (objectId & 0xF) << 4;
        }
        uint8_t sequenceIndex() const { return _5 & 0x3; } // _5l
        void setSequenceIndex(uint8_t index)
        {
            _5 &= ~0x3;
            _5 |= index & 0x3;
        }
        uint8_t levelCrossingObjectId() const { return (_5 >> 2) & 0x3; } // _5l 0b0000_1100 NOTE: Shared with streetlight style
        void setLevelCrossingObjectId(uint8_t levelCrossingObjectId)
        {
            _5 &= ~0b1100;
            _5 |= (levelCrossingObjectId & 0x3) << 2;
        }
        uint8_t streetLightStyle() const { return (_5 >> 2) & 0x3; } // _5l 0b0000_1100 NOTE: Shared with level crossing object id 0 = no street light
        uint8_t unk6l() const { return _6 & 0xF; }
        void setUnk6l(uint8_t value)
        {
            _6 &= ~0xF;
            _6 |= value & 0xF;
        }                                          // _6u
        uint8_t bridge() const { return _6 >> 5; } // _6u
        void setBridgeObjectId(const uint8_t bridge)
        {
            _6 &= ~0xE0;
            _6 |= bridge << 5;
        }
        bool hasStationElement() const { return (_type & 0x80) != 0; }
        void setHasStationElement(bool state)
        {
            _type &= ~0x80;
            _type |= state ? 0x80 : 0;
        }
        bool hasSignalElement() const { return (_type & 0x40) != 0; } // Always false
        bool hasUnk7_10() const { return _7 & (1 << 4); }             // _7u (bit 4) level crossing related
        void setUnk7_10(bool newState)                                // _7u (bit 4) level crossing related
        {
            _7 &= ~(1 << 4);
            _7 |= newState ? (1 << 4) : 0;
        }
        bool hasLevelCrossing() const { return _7 & (1 << 5); } // _7 0b0010_0000 NOTE: if false may have street light
        void setHasLevelCrossing(bool state)
        {
            _7 &= ~(1 << 5);
            _7 |= state ? (1U << 5) : 0;
        }
        // NOTE: This is the same field as mods! NON-TRAM SPECIFIC
        bool hasUnk7_40() const { return _7 & (1U << 6); }
        // NOTE: This is the same field as mods! NON-TRAM SPECIFIC
        void setUnk7_40(bool newState)
        {
            _7 &= ~(1 << 6);
            _7 |= newState ? (1 << 6) : 0;
        }
        // NOTE: This is the same field as mods! NON-TRAM SPECIFIC
        bool hasUnk7_80() const { return _7 & (1U << 7); }
        // NOTE: This is the same field as mods! NON-TRAM SPECIFIC
        void setUnk7_80(bool newState)
        {
            _7 &= ~(1U << 7);
            _7 |= newState ? (1U << 7) : 0;
        }
        bool hasMod(uint8_t mod) const { return _7 & (1 << (mod + 6)); } // _7u (bits 6 and 7)
        uint8_t mods() const { return _7 >> 6; }                         // _7u
        void setMods(uint8_t mods)
        {
            _7 &= ~0xC0;
            _7 |= (mods & 0x3) << 6;
        }
        void setMod(uint8_t mod, bool value)
        {
            _7 &= ~(1U << (mod + 6));
            _7 |= value ? (1U << (mod + 6)) : 0;
        }
        CompanyId owner() const { return CompanyId(_7 & 0xF); } // _7l
        void setOwner(CompanyId newOwner) { _7 = (_7 & 0xF0) | (enumValue(newOwner) & 0xF); }
        bool update(const World::Pos2& loc);
    };
#pragma pack(pop)
    static_assert(sizeof(RoadElement) == kTileElementSize);

    bool updateLevelCrossingAnimation(const Animation& anim);
}
