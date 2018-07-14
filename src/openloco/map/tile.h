#pragma once

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
    using coord_t = int16_t;
    using tile_coord_t = uint16_t;

    constexpr coord_t tile_size = 32;
    constexpr coord_t map_rows = 384;
    constexpr coord_t map_columns = 384;
    constexpr coord_t map_size = map_columns * tile_size;

    constexpr coord_t tile_floor(coord_t coord)
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

    uint32_t tile_element_height(int16_t x, int16_t y);
    map_pos coordinate_3d_to_2d(int16_t x, int16_t y, int16_t z, int rotation);

    enum class element_type
    {
        surface,     // 0x00
        track,       // 0x04
        station = 2, // 0x08
        signal = 3,
        building = 4, // 0x10
        tree = 5,     // 0x14
        fence = 6,
        road = 7,
        industry = 8, // 0x20
    };

    namespace element_flags
    {
        constexpr uint8_t flag_4 = 1 << 4;
        constexpr uint8_t last = 1 << 7;
    }

    struct surface_element;
    struct station_element;
    struct building_element;
    struct industry_element;
    struct track_element;
    struct tree_element;

#pragma pack(push, 1)
    struct tile_element_base
    {
    protected:
        // ______XX:
        // __XXXX__: type
        // XX______:
        uint8_t _type;

        uint8_t _flags;
        uint8_t _base_z;
        uint8_t _clear_z;

    public:
        // Temporary, use this to get fields easily before they are defined
        const uint8_t* data() const;
        element_type type() const;
        uint8_t flags() const { return _flags; }
        uint8_t base_z() const { return _base_z; }
        uint8_t clear_z() const { return _clear_z; }

        bool is_flag_4() const { return _flags & element_flags::flag_4; }
        bool is_last() const;
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

        uint8_t base_z() const { return _base_z; }

    public:
        surface_element* as_surface() const { return as<surface_element, element_type::surface>(); }
        station_element* as_station() const { return as<station_element, element_type::station>(); }
        building_element* as_building() const { return as<building_element, element_type::building>(); }
        industry_element* as_industry() const { return as<industry_element, element_type::industry>(); }
        track_element* as_track() const { return as<track_element, element_type::track>(); }
        tree_element* as_tree() const { return as<tree_element, element_type::tree>(); }
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
        bool is_slope_dbl_height() const { return _slope & surface_slope::double_height; }
        uint8_t slope_corners() const { return _slope & 0x0F; }
        uint8_t slope() const { return _slope & 0x1F; }
        uint8_t water() const { return _water & 0x1F; }
        uint8_t terrain() const { return _terrain & 0x1F; }
        uint8_t industry_id() const { return _industry; }
    };

    struct station_element : public tile_element_base
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t object_id() const { return _5 & 0x1F; }
        uint8_t unk_5b() const { return _5 >> 5; }
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
        bool has_80() const { return (_type & 0x80) != 0; }
        uint8_t object_id() const { return _4; }
        building_object* object() const;
        uint8_t var_5b() const { return _5 & 3; }
    };

    struct industry_element : public tile_element_base
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        uint8_t industry_id() const { return _4; }
        openloco::industry* industry() const;
    };

    struct track_element : public tile_element_base
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;

    public:
        bool has_80() const { return (_type & 0x80) != 0; }
        uint8_t unk_z() const { return (_type & 0x03) | ((_4 & 0x3F) << 3); }
    };

    struct tree_element : public tile_element_base
    {
    private:
        uint8_t _4;
        uint8_t _5;
        uint8_t _6;
        uint8_t _7;
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
        bool is_null() const;
        tile_element* begin();
        tile_element* begin() const;
        tile_element* end();
        tile_element* end() const;
        size_t size();
        tile_element* operator[](size_t i);

        size_t index_of(const tile_element_base* element) const;
        surface_element* surface();
    };
}
