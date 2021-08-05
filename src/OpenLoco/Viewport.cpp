#include "Viewport.hpp"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "Map/Tile.h"
#include "Map/TileManager.h"
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
        regs.esi = X86Pointer(this);
        regs.edi = X86Pointer(context);
        call(0x0045A1A4, regs);
    }

    // 0x004CA444
    viewport_pos Viewport::centre2dCoordinates(const Pos3& loc)
    {
        auto centre = Map::gameToScreen(loc, getRotation());

        return viewport_pos(centre.x - view_width / 2, centre.y - view_height / 2);
    }

    SavedViewSimple Viewport::toSavedView() const
    {
        SavedViewSimple result;
        const auto centre = getCentre();
        result.viewX = centre.x;
        result.viewY = centre.y;
        result.zoomLevel = static_cast<ZoomLevel>(zoom);
        result.rotation = getRotation();
        return result;
    }

    viewport_pos Viewport::getCentre() const
    {
        return viewport_pos(view_x + view_width / 2, view_y + view_height / 2);
    }

    xy32 Viewport::getUiCentre() const
    {
        return { x + width / 2,
                 y + height / 2 };
    }

    // 0x0045F997
    Pos2 Viewport::getCentreMapPosition() const
    {
        const viewport_pos initialVPPos = getCentre();

        const auto rotation = getRotation();
        // Vanilla unrolled on rotation at this point

        auto result = viewportCoordToMapCoord(initialVPPos.x, initialVPPos.y, 0, rotation);
        for (auto i = 0; i < 6; i++)
        {
            const auto z = Map::TileManager::getHeight(result);
            result = viewportCoordToMapCoord(initialVPPos.x, initialVPPos.y, z, rotation);
        }

        return result;
    }

    std::optional<Pos2> Viewport::getCentreScreenMapPosition() const
    {
        auto res = Ui::ViewportInteraction::getSurfaceLocFromUi(getUiCentre());
        if (!res)
        {
            return {};
        }
        return { res->first };
    }
}
