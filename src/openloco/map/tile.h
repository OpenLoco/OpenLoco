#pragma once

#include "../Types.hpp"
#include "../viewport.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>

namespace openloco
{
    struct building_object;
    struct industry;
}

namespace openloco::map
{
    constexpr coord_t tile_size = 32;
    constexpr coord_t map_rows = 384;
    constexpr coord_t map_columns = 384;
    constexpr coord_t map_height = map_rows * tile_size;
    constexpr coord_t map_width = map_columns * tile_size;
    constexpr int32_t map_size = map_columns * map_rows;

    constexpr coord_t tileFloor(coord_t coord)
    {
        return coord & (tile_size - 1);
    }

#pragma pack(push, 1)
    struct map_pos
    {
        coord_t x = 0;
        coord_t y = 0;

        map_pos() {}
        map_pos(coord_t x, coord_t y)
            : x(x)
            , y(y)
        {
        }
    };

    struct TilePos
    {
        tile_coord_t x = 0;
        tile_coord_t y = 0;
        TilePos() = default;
        TilePos(const map_pos& pos)
            : x(pos.x / tile_size)
            , y(pos.y / tile_size)
        {
        }
        TilePos(const tile_coord_t _x, const tile_coord_t _y)
            : x(_x)
            , y(_y)
        {
        }
    };
#pragma pack(pop)
    constexpr bool operator==(const map_pos& lhs, const map_pos& rhs)
    {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }
    constexpr bool operator!=(const map_pos& lhs, const map_pos& rhs)
    {
        return !(lhs == rhs);
    }

#pragma pack(push, 1)
    struct map_pos3
    {
        coord_t x = 0;
        coord_t y = 0;
        coord_t z = 0;

        operator map_pos() const
        {
            return map_pos(x, y);
        }
    };
#pragma pack(pop)

    struct tileHeight
    {
        coord_t landHeight;
        coord_t waterHeight;
    };

    tileHeight tileElementHeight(int16_t x, int16_t y);
    ui::viewport_pos coordinate3dTo2d(int16_t x, int16_t y, int16_t z, int rotation);
    map_pos rotate2dCoordinate(map_pos pos, uint8_t rotation);

    enum class element_type
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

    namespace element_flags
    {
        constexpr uint8_t flag_4 = 1 << 4;
        constexpr uint8_t flag_5 = 1 << 5;
        constexpr uint8_t flag_6 = 1 << 6;
        constexpr uint8_t last = 1 << 7;
    }

    struct surface_element;
    struct track_element;
    struct station_element;
    struct signal_element;
    struct building_element;
    struct tree_element;
    struct wall_element;
    struct road_element;
    struct industry_element;

#pragma pack(push, 1)
    struct tile_element_base
    {
    protected:
        uint8_t _type;
        uint8_t _flags;
        uint8_t _base_z;
        uint8_t _clear_z;

    public:
        // Temporary, use this to get fields easily before they are defined
        const uint8_t* data() const;
        element_type type() const;
        uint8_t flags() const { return _flags; }
        uint8_t baseZ() const { return _base_z; }
        uint8_t clearZ() const { return _clear_z; }

        bool hasHighTypeFlag() const { return _type & 0x80; }
        bool isFlag4() const { return _flags & element_flags::flag_4; }
        bool isFlag5() const { return _flags & element_flags::flag_5; }
        void setFlag6() { _flags |= element_flags::flag_6; }
        bool isLast() const;
    };

    struct tile_element : public tile_element_base
    {
    private:
        uint8_t pad[4];

        template<typename TType, element_type TClass>
        TType* as() const
        {
            return type() == TClass ? (TType*)this : nullptr;
        }

        uint8_t baseZ() const { return _base_z; }

    public:
        surface_element* asSurface() const { return as<surface_element, element_type::surface>(); }
        track_element* asTrack() const { return as<track_element, element_type::track>(); }
        station_element* asStation() const { return as<station_element, element_type::station>(); }
        signal_element* asSignal() const { return as<signal_element, element_type::signal>(); }
        building_element* asBuilding() const { return as<building_element, element_type::building>(); }
        tree_element* asTree() const { return as<tree_element, element_type::tree>(); }
        wall_element* asWall() const { return as<wall_element, element_type::wall>(); }
        road_element* asRoad() const { return as<road_element, element_type::road>(); }
        industry_element* asIndustry() const { return as<industry_element, element_type::industry>(); }
    };
    static_assert(sizeof(tile_element) == 8);

    namespace surface_slope
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

    struct surface_element : public tile_element_base
    {
    private:
        uint8_t _slope;
        uint8_t _water;
        uint8_t _terrain;
        uint8_t _industry;

    public:
        bool isSlopeDoubleHeight() const { return _slope & surface_slope::double_height; }
        uint8_t slopeCorners() const { return _slope & 0x0F; }
        uint8_t slope() const { return _slope & 0x1F; }
        uint8_t var_4_E0() const { return _slope & 0xE0; }
        uint8_t water() const { return _water & 0x1F; }
        uint8_t terrain() const { return _terrain & 0x1F; }
        uint8_t var_6_SLR5() const { return _terrain >> 5; }
        uint8_t industryId() const { return _industry; }
        void createWave(int16_t x, int16_t y, int animationIndex);
    };

    struct station_element : public tile_element_base
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint16_t _station_id;

    public:
        uint8_t objectId() const { return _5 & 0x1F; }
        uint8_t stationType() const { return _5 >> 5; }
        uint8_t rotation() const { return _type & 0x3; }
        station_id_t stationId() const { return _station_id & 0x3FF; }
    };

    struct building_element : public tile_element_base
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        bool has_40() const { return (_type & 0x40) != 0; }
        bool hasStationElement() const { return (_type & 0x80) != 0; }
        uint8_t objectId() const { return _4; }
        building_object* object() const;
        uint8_t multiTileIndex() const { return _5 & 3; }
    };

    struct tree_element : public tile_element_base
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;
    };

    struct wall_element : public tile_element_base
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;
    };

    struct track_element : public tile_element_base
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        bool hasStationElement() const { return (_type & 0x80) != 0; }
        uint8_t unkZ() const { return (_type & 0x03) | ((_4 & 0x3F) << 3); }
        bool hasSignal() const { return (_type & 0x40) != 0; }
        uint8_t unkDirection() const { return _type & 0x03; }
        uint8_t unk_4() const { return _4 & 0x3F; }
        bool has_4_80() const { return (_4 & 0x80) != 0; }
        uint8_t trackObjectId() const { return _5 >> 4; } // _5u
        uint8_t unk_5l() const { return _5 & 0xF; }
        uint8_t unk_6() const { return _6; }
        uint8_t owner() const { return _7 & 0xF; } // _7l
        uint8_t unk_7u() const { return _7 >> 4; }
    };

    struct signal_element : public tile_element_base
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;
    };

    struct road_element : public tile_element_base
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t unk_4_F() const { return _4 & 0xF; }
        uint8_t roadObjectId() const { return _5 >> 4; } // _5u
        bool hasStationElement() const { return (_type & 0x80) != 0; }
        uint8_t owner() const { return _7 & 0xF; } // _7l
    };

    struct industry_element : public tile_element_base
    {
    private:
        industry_id_t _industryId;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        openloco::industry* industry();
    };
#pragma pack(pop)

    struct tile
    {
    private:
        tile_element* const _data;

    public:
        static constexpr size_t npos = std::numeric_limits<size_t>().max();

        const tile_coord_t x;
        const tile_coord_t y;

        tile(tile_coord_t x, tile_coord_t y, tile_element* data);
        bool isNull() const;
        tile_element* begin();
        tile_element* begin() const;
        tile_element* end();
        tile_element* end() const;
        size_t size();
        tile_element* operator[](size_t i);

        size_t indexOf(const tile_element_base* element) const;
        surface_element* surface();
    };
}
