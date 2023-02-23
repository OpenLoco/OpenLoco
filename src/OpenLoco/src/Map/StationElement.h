#pragma once
#include "TileElementBase.h"

namespace OpenLoco
{
    enum class StationType : uint8_t;
}

namespace OpenLoco::World
{

#pragma pack(push, 1)
    struct StationElement : public TileElementBase
    {
        static constexpr ElementType kElementType = ElementType::station;

    private:
        uint8_t _4;
        uint8_t _5;
        uint16_t _stationId;

    public:
        CompanyId owner() const { return CompanyId(_4 & 0xF); } // _4l
        uint8_t objectId() const { return _5 & 0x1F; }
        StationType stationType() const;
        uint8_t rotation() const { return _type & 0x3; }
        uint8_t multiTileIndex() const { return (_type >> 6) & 3; }
        StationId stationId() const { return StationId(_stationId & 0x3FF); }
    };
#pragma pack(pop)
    static_assert(sizeof(StationElement) == kTileElementSize);
}
