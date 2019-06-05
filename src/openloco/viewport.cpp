#include "viewport.hpp"
#include "interop/interop.hpp"
#include "map/tile.h"
#include "window.h"

using namespace openloco::interop;

namespace openloco::ui
{
    // 0x004CA444
    void viewport::centre_2d_coordinates(int16_t _x, int16_t _y, int16_t _z, int16_t* outX, int16_t* outY)
    {
        auto centre = map::coordinate_3d_to_2d(_x, _y, _z, getRotation());

        *outX = centre.x - view_width / 2;
        *outY = centre.y - view_height / 2;
    }

    viewport_pos viewport::map_from_3d(loc16 loc, int32_t rotation)
    {
        ui::viewport_pos result;
        switch (rotation & 3)
        {
            case 0:
                result.x = loc.y - loc.x;
                result.y = ((loc.y + loc.x) / 2) - loc.z;
                break;
            case 1:
                result.x = -loc.x - loc.y;
                result.y = ((loc.y - loc.x) / 2) - loc.z;
                break;
            case 2:
                result.x = loc.x - loc.y;
                result.y = ((-loc.y - loc.x) / 2) - loc.z;
                break;
            case 3:
                result.x = loc.y + loc.x;
                result.y = ((loc.x - loc.y) / 2) - loc.z;
                break;
        }
        return result;
    }
}
