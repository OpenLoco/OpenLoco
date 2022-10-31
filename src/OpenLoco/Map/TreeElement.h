#pragma once

#include "TileElementBase.h"

namespace OpenLoco::Map
{
#pragma pack(push, 1)
    struct TreeElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::tree;

    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t treeObjectId() const { return _4; } // _4
        void setTreeObjectId(uint8_t type)
        {
            _4 = type;
        }
        uint8_t rotation() const { return _type & 0x03; }
        void setRotation(uint8_t rotation)
        {
            _type &= ~0x3;
            _type |= rotation & 0x3;
        }
        uint8_t quadrant() const { return (_type >> 6) & 0x03; } // _0_C0
        void setQuadrant(uint8_t quad)
        {
            _type &= ~0xC0;
            _type |= (quad & 0x3) << 6;
        }
        uint8_t unk5l() const { return _5 & 0xF; }
        void setUnk5l(uint8_t unk)
        {
            _5 &= ~0xF;
            _5 |= unk & 0xF;
        }
        void setUnk5h(uint8_t unk)
        {
            _5 &= ~0xF0;
            _5 |= (unk & 0xF) << 4;
        }
        Colour colour() const { return static_cast<Colour>(_6 & 0x1F); } //_6l
        void setColour(Colour colour)
        {
            _6 &= ~0x1F;
            _6 |= enumValue(colour) & 0x1F;
        }
        bool hasSnow() const { return _6 & 0x40; } //_6_40
        void setSnow(bool hasSnow)
        {
            _6 &= ~0x40;
            _6 |= hasSnow ? 0x40 : 0;
        }
        void setUnk6_80(bool unk)
        {
            _6 &= ~0x80;
            _6 |= unk ? 0x80 : 0;
        }
        uint8_t unk7l() const { return _7 & 0x7; }
        void setUnk7l(uint8_t unk)
        {
            _7 &= ~0x7;
            _7 |= unk & 0x7;
        }
        uint8_t season() const { return (_7 >> 3) & 0x7; } // unsure of &0x7
        void setSeason(uint8_t season)
        {
            _7 &= ~0xF8;
            _7 |= (season & 0x1F) << 3; // unsure of & 0x1F
        }
    };
#pragma pack(pop)
    static_assert(sizeof(TreeElement) == kTileElementSize);
}
