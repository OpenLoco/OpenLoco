#include "tilemgr.h"
#include "../interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::map::tilemgr
{
    static loco_global<uint32_t[0x30004], 0x00E40134> _tiles;

    tile get(map_pos pos)
    {
        return get(pos.x, pos.y);
    }

    tile get(coord_t x, coord_t y)
    {
        tile_coord_t tileX = x / 32;
        tile_coord_t tileY = y / 32;

        size_t index = ((y << 9) | x) >> 5;
        auto data = _tiles[index];
        auto data2 = (tile_element*)_tiles[index];
        if (data == 0xFFFFFFFF)
        {
            data2 = nullptr;
        }
        return tile(tileX, tileY, data2);
    }

    /**
    * Return the absolute height of an element, given its (x,y) coordinates
    *
    * ax: x
    * cx: y
    * return dx: height
    * return edx >> 16: waterHeight
    * loco: 0x0067297 rct2: 0x00662783 (numbers different)
    */
    std::tuple<int16_t, int16_t> get_height(coord_t x, coord_t y)
    {
        // Off the map
        if ((unsigned)x >= 12287 || (unsigned)y >= 12287)
            return std::make_tuple(16, 0);

        // Truncate subtile coordinates
        auto xTile = x & 0xFFE0;
        auto yTile = y & 0xFFE0;

        // Get the surface element for the tile
        auto surfaceEl = get(xTile, yTile).surface();

        if (surfaceEl == nullptr)
        {
            return std::make_tuple(16, 0);
        }

        int16_t waterHeight = surfaceEl->water() * 16;
        int16_t height = surfaceEl->base_z() * 4;

        auto slope = surfaceEl->slope_corners();
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
        if (slope == surface_slope::n_corner_up || slope == surface_slope::e_corner_up || slope == surface_slope::s_corner_up || slope == surface_slope::w_corner_up)
        {
            switch (slope)
            {
                case surface_slope::n_corner_up:
                    quad = xl + yl - TILE_SIZE;
                    break;
                case surface_slope::e_corner_up:
                    quad = xl - yl;
                    break;
                case surface_slope::s_corner_up:
                    quad = TILE_SIZE - yl - xl;
                    break;
                case surface_slope::w_corner_up:
                    quad = yl - xl;
                    break;
            }
            // If the element is in the quadrant with the slope, raise its height
            if (quad > 0)
            {
                height += quad / 2;
            }
        }

        // One side up
        switch (slope)
        {
            case surface_slope::ne_side_up:
                height += xl / 2 + 1;
                break;
            case surface_slope::se_side_up:
                height += (TILE_SIZE - yl) / 2;
                break;
            case surface_slope::nw_side_up:
                height += yl / 2;
                height++;
                break;
            case surface_slope::sw_side_up:
                height += (TILE_SIZE - xl) / 2;
                break;
        }

        // One corner down
        if ((slope == surface_slope::w_corner_dn) || (slope == surface_slope::s_corner_dn) || (slope == surface_slope::e_corner_dn) || (slope == surface_slope::n_corner_dn))
        {
            switch (slope)
            {
                case surface_slope::w_corner_dn:
                    quad_extra = xl + TILE_SIZE - yl;
                    quad = xl - yl;
                    break;
                case surface_slope::s_corner_dn:
                    quad_extra = xl + yl;
                    quad = xl + yl - TILE_SIZE - 1;
                    break;
                case surface_slope::e_corner_dn:
                    quad_extra = TILE_SIZE - xl + yl;
                    quad = yl - xl;
                    break;
                case surface_slope::n_corner_dn:
                    quad_extra = (TILE_SIZE - xl) + (TILE_SIZE - yl);
                    quad = TILE_SIZE - yl - xl - 1;
                    break;
            }

            if (surfaceEl->is_slope_dbl_height())
            {
                height += quad_extra / 2;
                height++;
                return std::make_tuple(height, waterHeight);
            }
            // This tile is essentially at the next height level
            height += 0x10;
            // so we move *down* the slope
            if (quad < 0)
            {
                height += quad / 2;
            }
        }

        // Valleys
        if ((slope == surface_slope::w_e_valley) || (slope == surface_slope::n_s_valley))
        {
            switch (slope)
            {
                case surface_slope::w_e_valley:
                    if (xl + yl <= TILE_SIZE + 1)
                    {
                        return std::make_tuple(height, waterHeight);
                    }
                    quad = TILE_SIZE - xl - yl;
                    break;
                case surface_slope::n_s_valley:
                    quad = xl - yl;
                    break;
            }
            if (quad > 0)
            {
                height += quad / 2;
            }
        }

        return std::make_tuple(height, waterHeight);
    }
}
