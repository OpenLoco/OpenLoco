#include "Viewport.hpp"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "Map/Tile.h"
#include "Ptr.h"
#include "Window.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;

namespace OpenLoco::Ui
{
    // 0x0045A0E7
    void viewport::render(Gfx::drawpixelinfo_t* dpi)
    {
        auto contextRect = dpi->getUiRect();
        auto viewRect = getUiRect();

        if (!contextRect.intersects(viewRect))
        {
            return;
        }
        auto intersection = contextRect.intersection(viewRect);
        paint(dpi, uiToMap(intersection));
    }

    // 0x0045A1A4
    void viewport::paint(Gfx::drawpixelinfo_t* context, const Rect& rect)
    {
        registers regs{};
        regs.ax = rect.left();
        regs.bx = rect.top();
        regs.dx = rect.right();
        regs.bp = rect.bottom();
        regs.esi = ToInt(this);
        regs.edi = ToInt(context);
        call(0x0045A1A4, regs);
    }

    // 0x004CA444
    void viewport::centre2dCoordinates(int16_t _x, int16_t _y, int16_t _z, int16_t* outX, int16_t* outY)
    {
        auto centre = Map::coordinate3dTo2d(_x, _y, _z, getRotation());

        *outX = centre.x - view_width / 2;
        *outY = centre.y - view_height / 2;
    }

    SavedViewSimple viewport::toSavedView() const
    {
        SavedViewSimple result;
        result.mapX = view_x + (view_width >> 1);
        result.mapY = view_y + (view_height >> 1);
        result.zoomLevel = static_cast<ZoomLevel>(zoom);
        result.rotation = getRotation();
        return result;
    }

    viewport_pos viewport::mapFrom3d(loc16 loc, int32_t rotation)
    {
        Ui::viewport_pos result;
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

    map_pos viewport::getCentreMapPosition() const
    {
        registers regs;
        regs.ax = view_x + view_width / 2;
        regs.bx = view_y + view_height / 2;
        regs.edx = getRotation();
        call(0x0045F997, regs);
        return { regs.ax, regs.bx };
    }
}
