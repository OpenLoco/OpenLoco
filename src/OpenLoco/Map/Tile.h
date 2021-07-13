#pragma once

#include "../Types.hpp"
#include "../Viewport.hpp"
#include "Map.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>

namespace OpenLoco
{
    struct BuildingObject;
    struct Industry;
    enum class StationType : uint8_t;
}

namespace OpenLoco::Map
{
    struct TileHeight
    {
        coord_t landHeight;
        coord_t waterHeight;

        operator coord_t() const
        {
            return waterHeight == 0 ? landHeight : waterHeight;
        }
    };

    // 0x004F9296, 0x4F9298
    constexpr Pos2 offsets[4] = { { 0, 0 }, { 0, 32 }, { 32, 32 }, { 32, 0 } };

    Ui::viewport_pos coordinate3dTo2d(int16_t x, int16_t y, int16_t z, int rotation);

    enum class ElementType
    {
        surface,  // 0x00
        track,    // 0x04
        station,  // 0x08
        signal,   // 0x0C
        building, // 0x10
        tree,     // 0x14
        wall,     // 0x18
        road,     // 0x1C
        industry, // 0x20
    };

    namespace ElementFlags
    {
        constexpr uint8_t ghost = 1 << 4;
        constexpr uint8_t flag_5 = 1 << 5;
        constexpr uint8_t flag_6 = 1 << 6;
        constexpr uint8_t last = 1 << 7;
    }

    struct SurfaceElement;
    struct TrackElement;
    struct StationElement;
    struct SignalElement;
    struct BuildingElement;
    struct TreeElement;
    struct WallElement;
    struct RoadElement;
    struct IndustryElement;

#pragma pack(push, 1)
    struct TileElementBase
    {
    protected:
        uint8_t _type;
        uint8_t _flags;
        uint8_t _base_z;
        uint8_t _clear_z;

    public:
        // Temporary, use this to get fields easily before they are defined
        const uint8_t* data() const;
        ElementType type() const;
        uint8_t flags() const { return _flags; }
        uint8_t baseZ() const { return _base_z; }
        uint8_t clearZ() const { return _clear_z; }

        bool hasHighTypeFlag() const { return _type & 0x80; }
        void setHighTypeFlag(bool state)
        {
            _type &= ~0x80;
            _type |= state ? 0x80 : 0;
        }
        bool isGhost() const { return _flags & ElementFlags::ghost; }
        bool isFlag5() const { return _flags & ElementFlags::flag_5; }
        void setFlag6(bool state)
        {
            _flags &= ~ElementFlags::flag_6;
            _flags |= state == true ? ElementFlags::flag_6 : 0;
        }
        void setClearZ(uint8_t value) { _clear_z = value; }
        bool isLast() const;

        std::array<uint8_t, 8>& rawData()
        {
            auto array = reinterpret_cast<std::array<uint8_t, 8>*>(this);
            return *array;
        }
    };

    struct TileElement : public TileElementBase
    {
    private:
        uint8_t pad[4];

        template<typename TType, ElementType TClass>
        TType* as() const
        {
            return type() == TClass ? (TType*)this : nullptr;
        }

    public:
        SurfaceElement* asSurface() const { return as<SurfaceElement, ElementType::surface>(); }
        TrackElement* asTrack() const { return as<TrackElement, ElementType::track>(); }
        StationElement* asStation() const { return as<StationElement, ElementType::station>(); }
        SignalElement* asSignal() const { return as<SignalElement, ElementType::signal>(); }
        BuildingElement* asBuilding() const { return as<BuildingElement, ElementType::building>(); }
        TreeElement* asTree() const { return as<TreeElement, ElementType::tree>(); }
        WallElement* asWall() const { return as<WallElement, ElementType::wall>(); }
        RoadElement* asRoad() const { return as<RoadElement, ElementType::road>(); }
        IndustryElement* asIndustry() const { return as<IndustryElement, ElementType::industry>(); }
    };
    static_assert(sizeof(TileElement) == 8);

    namespace SurfaceSlope
    {
        constexpr uint8_t flat = 0x00;
        constexpr uint8_t all_corners_up = 0x0F;

        constexpr uint8_t n_corner_up = (1 << 0);
        constexpr uint8_t e_corner_up = (1 << 1);
        constexpr uint8_t s_corner_up = (1 << 2);
        constexpr uint8_t w_corner_up = (1 << 3);
        constexpr uint8_t double_height = (1 << 4);

        constexpr uint8_t w_corner_dn = all_corners_up & ~w_corner_up;
        constexpr uint8_t s_corner_dn = all_corners_up & ~s_corner_up;
        constexpr uint8_t e_corner_dn = all_corners_up & ~e_corner_up;
        constexpr uint8_t n_corner_dn = all_corners_up & ~n_corner_up;

        constexpr uint8_t ne_side_up = n_corner_up | e_corner_up;
        constexpr uint8_t se_side_up = e_corner_up | s_corner_up;
        constexpr uint8_t nw_side_up = n_corner_up | w_corner_up;
        constexpr uint8_t sw_side_up = s_corner_up | w_corner_up;

        constexpr uint8_t w_e_valley = e_corner_up | w_corner_up;
        constexpr uint8_t n_s_valley = n_corner_up | s_corner_up;
    }

    struct SurfaceElement : public TileElementBase
    {
    private:
        uint8_t _slope;    // 0x4
        uint8_t _water;    // 0x5
        uint8_t _terrain;  // 0x6
        uint8_t _industry; // 0x7

    public:
        bool isSlopeDoubleHeight() const { return _slope & SurfaceSlope::double_height; }
        uint8_t slopeCorners() const { return _slope & 0x0F; }
        uint8_t slope() const { return _slope & 0x1F; }
        uint8_t var_4_E0() const { return _slope & 0xE0; }
        uint8_t water() const { return _water & 0x1F; }
        void setWater(uint8_t level) { _water = (_water & 0xE0) | (level & 0x1F); };
        uint8_t terrain() const { return _terrain & 0x1F; }
        uint8_t var_6_SLR5() const { return _terrain >> 5; }
        void setVar6SLR5(uint8_t var6)
        {
            _terrain &= 0x1F;
            _terrain |= var6 << 5;
        }
        uint8_t industryId() const { return _industry; }
        void setIndustry(uint8_t industry) { _industry = industry; }
        void setType6Flag(bool state)
        {
            _type &= ~0x40;
            _type |= state ? 0x40 : 0;
        }
        void createWave(int16_t x, int16_t y, int animationIndex);
    };

    struct StationElement : public TileElementBase
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint16_t _station_id;

    public:
        uint8_t objectId() const { return _5 & 0x1F; }
        StationType stationType() const;
        uint8_t rotation() const { return _type & 0x3; }
        StationId_t stationId() const { return _station_id & 0x3FF; }
    };

    struct BuildingElement : public TileElementBase
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint16_t _6;

    public:
        bool has_40() const { return (_type & 0x40) != 0; }
        bool hasStationElement() const { return (_type & 0x80) != 0; }
        uint8_t colour() const { return _6 >> 11; }
        void setColour(Colour_t colour) { _6 = (_6 & 0x7FF) | (colour << 11); }
        uint8_t objectId() const { return _4; }
        BuildingObject* object() const;
        uint8_t multiTileIndex() const { return _5 & 3; }
    };

    struct TreeElement : public TileElementBase
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t treeObjectId() const { return _4; } // _4
    };

    struct WallElement : public TileElementBase
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t wallObjectId() const { return _4; } // _4
    };

    struct TrackElement : public TileElementBase
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        bool hasStationElement() const { return (_type & 0x80) != 0; }
        bool hasSignal() const { return (_type & 0x40) != 0; }
        uint8_t unkDirection() const { return _type & 0x03; }
        uint8_t trackId() const { return _4 & 0x3F; } // _4
        bool has_4_80() const { return (_4 & 0x80) != 0; }
        uint8_t trackObjectId() const { return _5 >> 4; }  // _5u
        uint8_t sequenceIndex() const { return _5 & 0xF; } // _5l
        uint8_t unk_6() const { return _6; }               // _6
        uint8_t owner() const { return _7 & 0xF; }         // _7l
        void setOwner(uint8_t newOwner) { _7 = (_7 & 0xF0) | (newOwner & 0xF); }
        uint8_t unk_7u() const { return _7 >> 4; } // _7u
    };

    struct SignalElement : public TileElementBase
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        bool hasLeftSignal() const { return _4 & 0x80; }
        uint8_t leftSignalObjectId() const { return _4 & 0xF; } // _4l
        bool hasRightSignal() const { return _6 & 0x80; }
        uint8_t rightSignalObjectId() const { return _6 & 0xF; } // _6l
    };

    struct RoadElement : public TileElementBase
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t unkDirection() const { return _type & 0x03; }
        uint8_t roadId() const { return _4 & 0xF; }        // _4l
        uint8_t roadObjectId() const { return _5 >> 4; }   // _5u
        uint8_t sequenceIndex() const { return _5 & 0x3; } // _5l
        bool hasStationElement() const { return (_type & 0x80) != 0; }
        uint8_t owner() const { return _7 & 0xF; } // _7l
        void setOwner(uint8_t newOwner) { _7 = (_7 & 0xF0) | (newOwner & 0xF); }
    };

    struct IndustryElement : public TileElementBase
    {
    private:
        IndustryId_t _industryId;
        uint8_t _5;
        uint16_t _6;

    public:
        OpenLoco::IndustryId_t industryId() const { return _industryId; }
        OpenLoco::Industry* industry() const;
        uint8_t var_6_1F() const;
    };
#pragma pack(pop)

    struct Tile
    {
    private:
        TileElement* const _data;

    public:
        static constexpr size_t npos = std::numeric_limits<size_t>().max();

        const TilePos2 pos;

        Tile(const TilePos2& tPos, TileElement* data);
        bool isNull() const;
        TileElement* begin();
        TileElement* begin() const;
        TileElement* end();
        TileElement* end() const;
        size_t size();
        TileElement* operator[](size_t i);

        size_t indexOf(const TileElementBase* element) const;
        SurfaceElement* surface() const;
        StationElement* trackStation(uint8_t trackId, uint8_t direction, uint8_t baseZ) const;
        StationElement* roadStation(uint8_t roadId, uint8_t direction, uint8_t baseZ) const;
    };
}
