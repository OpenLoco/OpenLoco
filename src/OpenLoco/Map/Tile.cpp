#include "Tile.h"
#include "../IndustryManager.h"
#include "../Interop/Interop.hpp"
#include "../Objects/ObjectManager.h"
#include "../Ui/WindowManager.h"
#include <cassert>

using namespace OpenLoco;
using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::WindowManager;

const uint8_t* tile_element_base::data() const
{
    return (uint8_t*)this;
}

element_type tile_element_base::type() const
{
    return (element_type)((_type & 0x3C) >> 2);
}

bool tile_element_base::isLast() const
{
    return (_flags & ElementFlags::last) != 0;
}

building_object* building_element::object() const
{
    return ObjectManager::get<building_object>(objectId());
}

tile::tile(tile_coord_t x, tile_coord_t y, tile_element* data)
    : _data(data)
    , x(x)
    , y(y)
{
}

bool tile::isNull() const
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
    } while (!(el - 1)->isLast());
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

size_t tile::indexOf(const tile_element_base* element) const
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
        result = tile.asSurface();
        if (result != nullptr)
        {
            break;
        }
    }
    return result;
}

OpenLoco::industry* industry_element::industry()
{
    return IndustryManager::get(_industryId);
}

namespace OpenLoco::Map
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
    tileHeight tileElementHeight(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.cx = y;
        call(0x00467297, regs);

        tileHeight tileZ;

        tileZ.landHeight = regs.dx;
        tileZ.waterHeight = regs.edx >> 16;

        return tileZ;
    }

    /**
     *
     * @param x @<ax>
     * @param y
     * @param z
     * @param rotation
     * @return
     */
    Ui::viewport_pos coordinate3dTo2d(int16_t x, int16_t y, int16_t z, int rotation)
    {
        Ui::viewport_pos coordinate_2d;

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

    map_pos rotate2dCoordinate(map_pos pos, uint8_t rotation)
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
}
