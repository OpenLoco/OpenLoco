#pragma once

#include <cstdint>
#include <limits>

namespace openloco::map
{
    using coord_t = int16_t;
    using tile_coord_t = uint8_t;

    enum class element_type
    {
        surface,
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
