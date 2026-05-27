#pragma once

#include "TileElementBase.h"

namespace OpenLoco
{
    struct BuildingObject;
}

namespace OpenLoco::World
{
#pragma pack(push, 1)
    struct BuildingElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::building;

    private:
        uint8_t _4;
        uint8_t _5;
        uint16_t _6;

    public:
        uint8_t rotation() const { return _type & 0x03; }
        void setRotation(uint8_t rotation)
        {
            _type &= ~0x03;
            _type |= rotation & 0x3;
        }
        // Saves having to access the building object flags
        bool isMiscBuilding() const { return (_type & 0x40) != 0; }
        void setIsMiscBuilding(bool state)
        {
            _type &= ~0x40;
            _type |= state ? 0x40 : 0;
        }
        bool isConstructed() const { return (_type & 0x80) != 0; }
        void setConstructed(bool state)
        {
            _type &= ~0x80;
            _type |= state ? 0x80 : 0;
        }
        Colour colour() const { return static_cast<Colour>(_6 >> 11); }
        void setColour(Colour colour) { _6 = (_6 & 0x7FF) | (enumValue(colour) << 11); }
        uint8_t objectId() const { return _4; }
        void setObjectId(uint8_t id) { _4 = id; }
        const BuildingObject* getObject() const;
        uint8_t sequenceIndex() const { return _5 & 3; }
        void setSequenceIndex(uint8_t index)
        {
            _5 &= ~0x03;
            _5 |= index & 0x3;
        }
        uint8_t unk5u() const { return _5 >> 5; } // likely age related as well (higher precision)
        void setUnk5u(uint8_t value)
        {
            _5 &= ~0xE0;
            _5 |= value << 5;
        }
        uint8_t variation() const { return (_6 >> 6) & 0x1F; }
        void setVariation(uint8_t variation)
        {
            _6 &= ~0x07C0;
            _6 |= (variation & 0x1F) << 6;
        }
        uint8_t age() const { return _6 & 0x3F; } // 6l
        void setAge(uint8_t value)                // 6l
        {
            _6 &= ~0x3F;
            _6 |= value & 0x3F;
        }
        bool update(const World::Pos2& loc);
    };
#pragma pack(pop)
    static_assert(sizeof(BuildingElement) == kTileElementSize);

    struct Animation;
    bool updateBuildingAnimation1(const Animation& anim);
    bool updateBuildingAnimation2(const Animation& anim);
}
