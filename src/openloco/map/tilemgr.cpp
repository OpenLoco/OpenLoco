#include "tilemgr.h"
#include "../interop/interop.hpp"
#include <tuple>

using namespace openloco::interop;

namespace openloco::map::tilemgr
{
    static loco_global<tile_element * [0x30004], 0x00E40134> _tiles;

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
        if (data == (tile_element*)0xFFFFFFFF)
        {
            data = nullptr;
        }
        return tile(tileX, tileY, data);
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
        auto surfaceEl = get(xTile,yTile).surface();

        if (surfaceEl == nullptr) {
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
        if (slope == surface_slope::N_CORNER_UP ||
            slope == surface_slope::E_CORNER_UP ||
            slope == surface_slope::S_CORNER_UP ||
            slope == surface_slope::W_CORNER_UP)
        {
            switch (slope) {
            case surface_slope::N_CORNER_UP:
                quad = xl + yl - TILE_SIZE;
                break;
            case surface_slope::E_CORNER_UP:
                quad = xl - yl;
                break;
            case surface_slope::S_CORNER_UP:
                quad = TILE_SIZE - yl - xl;
                break;
            case surface_slope::W_CORNER_UP:
                quad = yl - xl;
                break;
            }
            // If the element is in the quadrant with the slope, raise its height
            if (quad > 0) {
                height += quad / 2;
            }
        }

        // One side up
        switch (slope) {
        case surface_slope::NE_SIDE_UP:
            height += xl / 2 + 1;
            break;
        case surface_slope::SE_SIDE_UP:
            height += (TILE_SIZE - yl) / 2;
            break;
        case surface_slope::NW_SIDE_UP:
            height += yl / 2;
            height++;
            break;
        case surface_slope::SW_SIDE_UP:
            height += (TILE_SIZE - xl) / 2;
            break;
        }

        // One corner down
        if ((slope == surface_slope::W_CORNER_DN) ||
            (slope == surface_slope::S_CORNER_DN) ||
            (slope == surface_slope::E_CORNER_DN) ||
            (slope == surface_slope::N_CORNER_DN)) {
            switch (slope) {
            case surface_slope::W_CORNER_DN:
                quad_extra = xl + TILE_SIZE - yl;
                quad = xl - yl;
                break;
            case surface_slope::S_CORNER_DN:
                quad_extra = xl + yl;
                quad = xl + yl - TILE_SIZE - 1;
                break;
            case surface_slope::E_CORNER_DN:
                quad_extra = TILE_SIZE - xl + yl;
                quad = yl - xl;
                break;
            case surface_slope::N_CORNER_DN:
                quad_extra = (TILE_SIZE - xl) + (TILE_SIZE - yl);
                quad = TILE_SIZE - yl - xl - 1;
                break;
            }

            if (surfaceEl->is_slope_dbl_height()) {
                height += quad_extra / 2;
                height++;
                return std::make_tuple(height, waterHeight);
            }
            // This tile is essentially at the next height level
            height += 0x10;
            // so we move *down* the slope
            if (quad < 0) {
                height += quad / 2;
            }
        }

        // Valleys
        if ((slope == surface_slope::W_E_VALLEY) ||
            (slope == surface_slope::N_S_VALLEY)) {
            switch (slope) {
            case surface_slope::W_E_VALLEY:
                if (xl + yl <= TILE_SIZE + 1) {
                    return std::make_tuple(height, waterHeight);
                }
                quad = TILE_SIZE - xl - yl;
                break;
            case surface_slope::N_S_VALLEY:
                quad = xl - yl;
                break;
            }
            if (quad > 0) {
                height += quad / 2;
            }
        }

        return std::make_tuple(height, waterHeight);
    }
}
