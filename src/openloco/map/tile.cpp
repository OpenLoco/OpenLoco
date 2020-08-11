#include "tile.h"
#include "../industrymgr.h"
#include "../interop/interop.hpp"
#include "../objects/objectmgr.h"
#include "../ui/WindowManager.h"
#include <cassert>

using namespace openloco;
using namespace openloco::interop;
using namespace openloco::ui::WindowManager;

const uint8_t* tile_element_base::data() const
{
    return (uint8_t*)this;
}

element_type tile_element_base::type() const
{
    return (element_type)((_type & 0x3C) >> 2);
}

bool tile_element_base::is_last() const
{
    return (_flags & element_flags::last) != 0;
}

building_object* building_element::object() const
{
    return objectmgr::get<building_object>(object_id());
}

tile::tile(tile_coord_t x, tile_coord_t y, tile_element* data)
    : _data(data)
    , x(x)
    , y(y)
{
}

bool tile::is_null() const
{
    return _data == nullptr;
}

tile_element* tile::begin()
{
    return _data;
}

tile_element* tile::begin() const
{
    return const_cast<tile&>(*this).begin();
}

tile_element* tile::end()
{
    auto el = _data;
    do
    {
        el++;
    } while (!(el - 1)->is_last());
    return el;
}

tile_element* tile::end() const
{
    return const_cast<tile&>(*this).end();
}

size_t tile::size()
{
    return end() - begin();
}

tile_element* tile::operator[](size_t i)
{
#if DEBUG
    assert(i < size());
#endif
    return &_data[i];
}

size_t tile::index_of(const tile_element_base* element) const
{
    size_t i = 0;
    for (const auto& tile : *this)
    {
        if (&tile == element)
        {
            return i;
        }
        i++;
    }
    return npos;
}

surface_element* tile::surface()
{
    surface_element* result = nullptr;
    for (auto& tile : *this)
    {
        result = tile.as_surface();
        if (result != nullptr)
        {
            break;
        }
    }
    return result;
}

namespace openloco::map
{
    /**
     * Return the absolute height of an element, given its (x, y) coordinates
     * remember to & with 0xFFFF if you don't want water affecting results
     *
     * @param x @<ax>
     * @param y @<cx>
     * @return height @<edx>
     *
     * 0x00467297
     */
    uint32_t tile_element_height(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.cx = y;
        call(0x00467297, regs);

        return regs.edx;
    }

    /**
     *
     * @param x @<ax>
     * @param y
     * @param z
     * @param rotation
     * @return
     */
    ui::viewport_pos coordinate_3d_to_2d(int16_t x, int16_t y, int16_t z, int rotation)
    {
        ui::viewport_pos coordinate_2d;

        switch (rotation)
        {
            default:
            case 0:
                coordinate_2d.x = y - x;
                coordinate_2d.y = ((y + x) >> 1) - z;
                break;
            case 1:
                coordinate_2d.x = -y - x;
                coordinate_2d.y = ((y - x) >> 1) - z;
                break;
            case 2:
                coordinate_2d.x = -y + x;
                coordinate_2d.y = ((-y - x) >> 1) - z;
                break;
            case 3:
                coordinate_2d.x = y + x;
                coordinate_2d.y = ((-y + x) >> 1) - z;
                break;
        }

        return coordinate_2d;
    }

    map_pos rotate2DCoordinate(map_pos pos, uint8_t rotation)
    {
        map_pos coordinate2D;

        switch (rotation)
        {
            default:
            case 0:
                coordinate2D = pos;
                break;
            case 1:
                coordinate2D.x = pos.y;
                coordinate2D.y = -pos.x;
                break;
            case 2:
                coordinate2D.x = -pos.x;
                coordinate2D.y = -pos.y;
                break;
            case 3:
                coordinate2D.x = -pos.y;
                coordinate2D.y = pos.x;
                break;
        }

        return coordinate2D;
    }

    xy32 mapWindowPosToLocation(map_pos pos)
    {
        pos.x = ((pos.x + 8) - map_columns) / 2;
        pos.y = ((pos.y + 8)) / 2;
        xy32 location = { static_cast<coord_t>(pos.y - pos.x), static_cast<coord_t>(pos.x + pos.y) };
        location.x *= tile_size;
        location.y *= tile_size;

        switch (getCurrentRotation())
        {
            case 0:
                return location;
            case 1:
                return { map_width - 1 - location.y, location.x };
            case 2:
                return { map_width - 1 - location.x, map_height - 1 - location.y };
            case 3:
                return { location.y, map_height - 1 - location.x };
        }

        return { 0, 0 }; // unreachable
    }

    map_pos locationToMapWindowPos(xy32 pos)
    {
        coord_t x = pos.x;
        coord_t y = pos.y;

        switch (getCurrentRotation())
        {
            case 3:
                std::swap(x, y);
                x = map_width - 1 - x;
                break;
            case 2:
                x = map_width - 1 - x;
                y = map_height - 1 - y;
                break;
            case 1:
                std::swap(x, y);
                y = map_height - 1 - y;
                break;
            case 0:
                break;
        }

        x /= tile_size;
        y /= tile_size;

        return { static_cast<coord_t>(-x + y + map_columns - 8), static_cast<coord_t>(x + y - 8) };
    }
}
