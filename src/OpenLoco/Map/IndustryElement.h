#pragma once

#include "TileElementBase.h"

namespace OpenLoco
{
    struct Industry;
}

namespace OpenLoco::Map
{
#pragma pack(push, 1)

    struct IndustryElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::industry;

    private:
        IndustryId _industryId;
        uint8_t _5;
        uint16_t _6;

    public:
        IndustryId industryId() const { return _industryId; }
        Industry* industry() const;
        uint8_t buildingType() const;
        uint8_t rotation() const { return _type & 0x3; }
        bool hasHighTypeFlag() const { return _type & 0x80; } // isConstructed?
    };
#pragma pack(pop)
    static_assert(sizeof(IndustryElement) == kTileElementSize);
}
