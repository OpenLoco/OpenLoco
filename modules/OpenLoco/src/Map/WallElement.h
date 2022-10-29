#pragma once

#include "TileElementBase.h"

namespace OpenLoco::Map
{
#pragma pack(push, 1)
    struct WallElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::wall;

    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t wallObjectId() const { return _4; }      // _4
        uint8_t rotation() const { return _type & 0x3; } // _0
    };
#pragma pack(pop)
    static_assert(sizeof(WallElement) == kTileElementSize);
}
