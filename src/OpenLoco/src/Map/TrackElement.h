#pragma once

#include "TileElementBase.h"
#include <optional>

namespace OpenLoco::World
{
#pragma pack(push, 1)

    struct TrackElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::track;

    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        TrackElement() = default;
        TrackElement(World::SmallZ baseZ, World::SmallZ clearZ, uint8_t direction, uint8_t quarterTile, uint8_t sequenceIndex, uint8_t trackObjId, uint8_t trackId, std::optional<uint8_t> bridge, CompanyId owner, uint8_t mods);

        bool hasStationElement() const { return (_type & 0x80) != 0; }
        void setHasStationElement(bool state)
        {
            _type &= ~0x80;
            _type |= state ? 0x80 : 0;
        }
        bool hasSignal() const { return (_type & 0x40) != 0; }
        void setHasSignal(bool state)
        {
            _type &= ~0x40;
            _type |= state ? 0x40 : 0;
        }
        uint8_t unkDirection() const { return _type & 0x03; }
        void setRotation(uint8_t rotation)
        {
            _type &= ~0x3;
            _type |= rotation & 0x3;
        }
        uint8_t trackId() const { return _4 & 0x3F; } // _4
        void setTrackId(uint8_t trackId)
        {
            _4 &= ~0x3F;
            _4 |= trackId & 0x3F;
        }
        bool hasBridge() const { return (_4 & 0x80) != 0; }
        void setHasBridge(bool state)
        {
            _4 &= ~(1U << 7);
            _4 |= state ? (1U << 7) : 0;
        }
        bool hasGhostMods() const { return (_4 & 0x40) != 0; }
        void setHasGhostMods(bool state)
        {
            _4 &= ~(1U << 6);
            _4 |= state ? (1U << 6) : 0;
        }
        uint8_t trackObjectId() const { return _5 >> 4; } // _5u
        void setTrackObjectId(uint8_t trackObjectId)
        {
            _5 &= ~0xF0;
            _5 |= (trackObjectId & 0xF) << 4;
        }
        uint8_t sequenceIndex() const { return _5 & 0xF; } // _5l
        void setSequenceIndex(uint8_t index)
        {
            _5 &= ~0x0F;
            _5 |= index & 0xF;
        }
        bool hasLevelCrossing() const { return (_6 & 0x10) != 0; } // _6_10
        void setHasLevelCrossing(bool state)
        {
            _6 &= ~(1U << 4);
            _6 |= state ? (1U << 4) : 0;
        }
        uint8_t bridge() const { return _6 >> 5; } // _6u
        void setBridgeObjectId(uint8_t bridgeObjectId)
        {
            _6 &= ~0xE0;
            _6 |= (bridgeObjectId & 0x7) << 5;
        }
        CompanyId owner() const { return CompanyId(_7 & 0xF); } // _7l
        void setOwner(CompanyId newOwner) { _7 = (_7 & 0xF0) | (enumValue(newOwner) & 0xF); }
        bool hasMod(uint8_t mod) const { return _7 & (1 << (4 + mod)); } // _7u
        uint8_t mods() const { return _7 >> 4; }                         // _7u
        void setMod(uint8_t mod, bool state)
        {
            _7 &= ~(1U << (4 + mod));
            _7 |= state ? (1U << (4 + mod)) : 0;
        }
    };
#pragma pack(pop)
    static_assert(sizeof(TrackElement) == kTileElementSize);
}
