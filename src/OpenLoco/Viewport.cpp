#include "Viewport.hpp"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "Map/Tile.h"
#include "Window.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;

namespace OpenLoco::Ui
{
    static loco_global<uint32_t, 0x00E3F0B8> _rotation;

    int Viewport::getRotation() const
    {
        return _rotation;
    }

    void Viewport::setRotation(int32_t value)
    {
        _rotation = value;
    }

    // 0x0045A0E7
    void Viewport::render(Gfx::Context* context)
    {
        auto contextRect = context->getUiRect();
        auto viewRect = getUiRect();

        if (!contextRect.intersects(viewRect))
        {
            return;
        }
        auto intersection = contextRect.intersection(viewRect);
        paint(context, uiToMap(intersection));
    }

    // 0x0045A1A4
    void Viewport::paint(Gfx::Context* context, const Rect& rect)
    {
        registers regs{};
        regs.ax = rect.left();
        regs.bx = rect.top();
        regs.dx = rect.right();
        regs.bp = rect.bottom();
        regs.esi = reinterpret_cast<uint32_t>(this);
        regs.edi = reinterpret_cast<uint32_t>(context);
        call(0x0045A1A4, regs);
    }

    // 0x004CA444
    void Viewport::centre2dCoordinates(int16_t _x, int16_t _y, int16_t _z, int16_t* outX, int16_t* outY)
    {
        auto centre = Map::coordinate3dTo2d(_x, _y, _z, getRotation());

        *outX = centre.x - view_width / 2;
        *outY = centre.y - view_height / 2;
    }

    SavedViewSimple Viewport::toSavedView() const
    {
        SavedViewSimple result;
        result.mapX = view_x + (view_width >> 1);
        result.mapY = view_y + (view_height >> 1);
        result.zoomLevel = static_cast<ZoomLevel>(zoom);
        result.rotation = getRotation();
        return result;
    }

    viewport_pos Viewport::mapFrom3d(Map::Pos3 loc, int32_t rotation)
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

    Map::Pos2 Viewport::viewposToPos2(const viewport_pos& loc, const coord_t z, const int32_t rotation)
    {
        constexpr uint8_t inverseRotationMapping[4] = { 0, 3, 2, 1 };
        const auto result = Map::Pos2(loc.y - loc.x / 2 + z, loc.y + loc.x / 2 + z);
        return Math::Vector::rotate(result, inverseRotationMapping[rotation]);
    }

    Pos2 Viewport::getCentreMapPosition() const
    {
        registers regs;
        regs.ax = view_x + view_width / 2;
        regs.bx = view_y + view_height / 2;
        regs.edx = getRotation();
        call(0x0045F997, regs);
        return { regs.ax, regs.bx };
    }

    Pos2 Viewport::getCentreScreenMapPosition() const
    {
        registers regs;
        regs.ax = x + width / 2;
        regs.bx = y + height / 2;
        call(0x0045F1A7, regs);
        return { regs.ax, regs.bx };
    }
}
