#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace openloco::map
{
    using coord_t = int16_t;
    using tile_coord_t = uint16_t;

    constexpr coord_t tile_size = 32;
    constexpr coord_t map_columns = 384;
    constexpr coord_t map_size = map_columns * tile_size;

    constexpr coord_t tile_floor(coord_t coord)
    {
        return coord & (tile_size - 1);
    }

    enum class element_type
    {
        surface,
        unk_20 = 20,
    };

    struct surface_element;

    struct tile_element
    {
    private:
        uint8_t _type;
        uint8_t _flags;
        uint8_t _base_z;
        uint8_t _clear_z;

        template<typename TType, element_type TClass>
        TType* as() const
        {
            return type() == TClass ? (TType*)this : nullptr;
        }

    public:
        // Temporary, use this to get fields easily before they are defined
        const uint8_t* data() const;
        element_type type() const;
        bool is_last() const;

    public:
        surface_element* as_surface() const { return as<surface_element, element_type::surface>(); }
    };

    struct surface_element : public tile_element
    {
    private:
        uint8_t _slope;
        uint8_t _water;
        uint8_t _terrain;
        uint8_t _industry;

    public:
        uint8_t slope() const { return _slope & 0x1F; }
        uint8_t water() const { return _water & 0x1F; }
        uint8_t terrain() const { return _terrain & 0x1F; }
        uint8_t industry_id() const { return _industry; }
    };

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

        size_t index_of(const tile_element* element) const;
        surface_element* surface();
    };
}
