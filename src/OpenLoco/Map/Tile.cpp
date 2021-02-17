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

surface_element* tile::surface() const
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

OpenLoco::industry* industry_element::industry() const
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
     * 0x00467297 rct2: 0x00662783 (numbers different)
     */
    TileHeight tile::getHeight() const
    {
        TileHeight height{ 16, 0 };
        // Off the map
        if ((unsigned)x >= 12287 || (unsigned)y >= 12287)
            return height;

        // Get the surface element for the tile
        auto surfaceEl = surface();

        if (surfaceEl == nullptr)
        {
            return height;
        }

        height.waterHeight = surfaceEl->water() * 16;
        height.landHeight = surfaceEl->baseZ() * 4;

        auto slope = surfaceEl->slopeCorners();
        int8_t quad = 0, quad_extra = 0; // which quadrant the element is in?
                                         // quad_extra is for extra height tiles

        uint8_t TILE_SIZE = 31;

        // Subtile coords
        auto xl = x & 0x1f;
        auto yl = y & 0x1f;

        // Slope logic:
        // Each of the four bits in slope represents that corner being raised
        // slope == 15 (all four bits) is not used and slope == 0 is flat
        // If the extra_height bit is set, then the slope goes up two z-levels

        // We arbitrarily take the SW corner to be closest to the viewer

        // One corner up
        if (slope == SurfaceSlope::n_corner_up || slope == SurfaceSlope::e_corner_up || slope == SurfaceSlope::s_corner_up || slope == SurfaceSlope::w_corner_up)
        {
            switch (slope)
            {
                case SurfaceSlope::n_corner_up:
                    quad = xl + yl - TILE_SIZE;
                    break;
                case SurfaceSlope::e_corner_up:
                    quad = xl - yl;
                    break;
                case SurfaceSlope::s_corner_up:
                    quad = TILE_SIZE - yl - xl;
                    break;
                case SurfaceSlope::w_corner_up:
                    quad = yl - xl;
                    break;
            }
            // If the element is in the quadrant with the slope, raise its height
            if (quad > 0)
            {
                height.landHeight += quad / 2;
            }
        }

        // One side up
        switch (slope)
        {
            case SurfaceSlope::ne_side_up:
                height.landHeight += xl / 2 + 1;
                break;
            case SurfaceSlope::se_side_up:
                height.landHeight += (TILE_SIZE - yl) / 2;
                break;
            case SurfaceSlope::nw_side_up:
                height.landHeight += yl / 2;
                height.landHeight++;
                break;
            case SurfaceSlope::sw_side_up:
                height.landHeight += (TILE_SIZE - xl) / 2;
                break;
        }

        // One corner down
        if ((slope == SurfaceSlope::w_corner_dn) || (slope == SurfaceSlope::s_corner_dn) || (slope == SurfaceSlope::e_corner_dn) || (slope == SurfaceSlope::n_corner_dn))
        {
            switch (slope)
            {
                case SurfaceSlope::w_corner_dn:
                    quad_extra = xl + TILE_SIZE - yl;
                    quad = xl - yl;
                    break;
                case SurfaceSlope::s_corner_dn:
                    quad_extra = xl + yl;
                    quad = xl + yl - TILE_SIZE - 1;
                    break;
                case SurfaceSlope::e_corner_dn:
                    quad_extra = TILE_SIZE - xl + yl;
                    quad = yl - xl;
                    break;
                case SurfaceSlope::n_corner_dn:
                    quad_extra = (TILE_SIZE - xl) + (TILE_SIZE - yl);
                    quad = TILE_SIZE - yl - xl - 1;
                    break;
            }

            if (surfaceEl->isSlopeDoubleHeight())
            {
                height.landHeight += quad_extra / 2;
                height.landHeight++;
                return height;
            }
            // This tile is essentially at the next height level
            height.landHeight += 0x10;
            // so we move *down* the slope
            if (quad < 0)
            {
                height.landHeight += quad / 2;
            }
        }

        // Valleys
        if ((slope == SurfaceSlope::w_e_valley) || (slope == SurfaceSlope::n_s_valley))
        {
            switch (slope)
            {
                case SurfaceSlope::w_e_valley:
                    if (xl + yl <= TILE_SIZE + 1)
                    {
                        return height;
                    }
                    quad = TILE_SIZE - xl - yl;
                    break;
                case SurfaceSlope::n_s_valley:
                    quad = xl - yl;
                    break;
            }
            if (quad > 0)
            {
                height.landHeight += quad / 2;
            }
        }

        return height;
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
