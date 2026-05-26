#pragma once
#include "TileElementBase.h"

namespace OpenLoco
{
    enum class StationType : uint8_t;
}

namespace OpenLoco::World
{
    struct Animation;

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
        void setOwner(CompanyId owner)
        {
            _4 &= ~0xF;
            _4 |= enumValue(owner) & 0xF;
        }
        void setUnk4SLR4(uint8_t val)
        {
            _4 &= ~0xF0;
            _4 |= (val & 0xF) << 4;
        }
        uint8_t objectId() const { return _5 & 0x1F; }
        void setObjectId(uint8_t objectId)
        {
            _5 &= ~0x1F;
            _5 |= objectId & 0x1F;
        }
        StationType stationType() const;
        void setStationType(StationType type);
        uint8_t rotation() const { return _type & 0x3; }
        void setRotation(uint8_t rotation)
        {
            _type &= ~0x3;
            _type |= rotation & 0x3;
        }
        uint8_t sequenceIndex() const { return (_type >> 6) & 3; }
        void setSequenceIndex(uint8_t index)
        {
            _type &= ~0xC0;
            _type |= (index & 3) << 6;
        }
        StationId stationId() const { return StationId(_stationId & 0x3FF); }
        void setStationId(StationId id)
        {
            _stationId &= ~0x3FF;
            _stationId |= enumValue(id) & 0x3FF;
        }
        // ((_6 & 0xFC00) >> 10) Note: Only non-zero for airports
        uint8_t buildingType() const
        {
            return (_stationId & 0xFC00) >> 10;
        }
        // ((_6 & 0xFC00) >> 10) Note: Only non-zero for airports
        void setBuildingType(uint8_t type)
        {
            _stationId &= ~0xFC00;
            _stationId |= (type & 0x3F) << 10;
        }
    };
#pragma pack(pop)
    static_assert(sizeof(StationElement) == kTileElementSize);

    bool updateDockStationAnimation(const Animation& anim);
    bool updateAirportStationAnimation(const Animation& anim);
}
