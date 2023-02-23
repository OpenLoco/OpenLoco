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
        bool hasSignal() const { return (_type & 0x40) != 0; }
        uint8_t unkDirection() const { return _type & 0x03; }
        uint8_t trackId() const { return _4 & 0x3F; } // _4
        bool hasBridge() const { return (_4 & 0x80) != 0; }
        bool hasGhostMods() const { return (_4 & 0x40) != 0; }
        uint8_t trackObjectId() const { return _5 >> 4; }  // _5u
        uint8_t sequenceIndex() const { return _5 & 0xF; } // _5l
        bool has_6_10() const { return (_6 & 0x10) != 0; }
        uint8_t bridge() const { return _6 >> 5; }              // _6u
        CompanyId owner() const { return CompanyId(_7 & 0xF); } // _7l
        void setOwner(CompanyId newOwner) { _7 = (_7 & 0xF0) | (enumValue(newOwner) & 0xF); }
        bool hasMod(uint8_t mod) const { return _7 & (1 << (4 + mod)); } // _7u
        uint8_t mods() const { return _7 >> 4; }                         // _7u
    };
#pragma pack(pop)
    static_assert(sizeof(TrackElement) == kTileElementSize);
}
