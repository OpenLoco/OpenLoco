#pragma once

#include "Graphics/Colour.h"
#include "TileElementBase.h"

namespace OpenLoco::World
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
        uint8_t wallObjectId() const { return _4; } // _4
        void setWallObjectId(uint8_t id)
        {
            _4 = id;
        }
        uint8_t rotation() const { return _type & 0x3; } // _0
        void setRotation(uint8_t rotation)
        {
            _type = (_type & 0x3C) | (rotation & 0x3);
        }
        void setSlopeFlags(uint8_t slopeFlags)
        {
            _type &= ~0xC0;
            _type |= slopeFlags & 0xC0; // Note slope flags are already << 6
        }
        void setPrimaryColour(Colour colour)
        {
            _6 = (_6 & 0xE0) | (enumValue(colour) & 0x1F);
            _6 = ((enumValue(colour) & 0x07) << 5) | (_6 & 0x1F);
        }
        void setSecondaryColour(Colour colour)
        {
            _6 &= 0x1F;
            _6 |= (enumValue(colour) & 0x7) << 5;
            _flags &= ~0x60; // Reuse flags 5 and 6 for storing bits of the primary colour
            _flags |= (enumValue(colour) & 0x18) << 2;
        }
        void setTertiaryColour(Colour colour)
        {
            _5 = enumValue(colour);
        }
    };
#pragma pack(pop)
    static_assert(sizeof(WallElement) == kTileElementSize);
}
