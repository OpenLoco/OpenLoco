#include "Viewport.hpp"
#include "Config.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Paint/Paint.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "Window.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;

namespace OpenLoco::Ui
{
    int Viewport::getRotation() const
    {
        return WindowManager::getCurrentRotation(); // Eventually this should become a variable of the viewport
    }

    void Viewport::setRotation(int32_t value)
    {
        WindowManager::setCurrentRotation(value); // Eventually this should become a variable of the viewport
    }

    // 0x0045A0E7
    void Viewport::render(Gfx::RenderTarget* rt)
    {
        auto uiRect = rt->getUiRect();
        auto viewRect = getUiRect();

        if (!uiRect.intersects(viewRect))
        {
            return;
        }
        auto intersection = uiRect.intersection(viewRect);
        paint(rt, screenToViewport(intersection));
    }

    // 0x0048DE97
    static void drawStationNames(Gfx::RenderTarget& rt)
    {
        registers regs{};
        regs.edi = X86Pointer(&rt);
        call(0x0048DE97, regs);
    }

    // 0x004977E5
    static void drawTownNames(Gfx::RenderTarget& rt)
    {
        registers regs{};
        regs.edi = X86Pointer(&rt);
        call(0x004977E5, regs);
    }

    // 0x00470A62
    static void drawRoutingNumbers(Gfx::RenderTarget& rt)
    {
        registers regs{};
        regs.edi = X86Pointer(&rt);
        call(0x00470A62, regs);
    }

    // 0x0045A1A4
    void Viewport::paint(Gfx::RenderTarget* rt, const Rect& rect)
    {
        Paint::SessionOptions options{};
        if (flags & (ViewportFlags::hide_foreground_scenery_buildings || ViewportFlags::hide_foreground_tracks_roads))
        {
            options.foregroundCullHeight = viewHeight / 2 + viewY;
        }
        PaletteIndex_t fillColour = PaletteIndex::index_D8;
        if (flags & (ViewportFlags::underground_view | ViewportFlags::flag_7 | ViewportFlags::flag_8))
        {
            fillColour = PaletteIndex::index_0A;
        }
        options.rotation = getRotation();
        options.viewFlags = flags;

        const uint32_t bitmask = 0xFFFFFFFF << zoom;

        // rt is in terms of the ui we need a target setup for the viewport zoom level
        Gfx::RenderTarget zoomViewRt{};
        zoomViewRt.width = rect.width();
        zoomViewRt.height = rect.height();
        zoomViewRt.x = rect.origin.x;
        zoomViewRt.y = rect.origin.y;

        zoomViewRt.width &= bitmask;
        zoomViewRt.height &= bitmask;
        zoomViewRt.x &= bitmask;
        zoomViewRt.y &= bitmask;

        auto unkX = ((zoomViewRt.x - static_cast<int32_t>(viewX & bitmask)) >> zoom) + x;

        auto unkY = ((zoomViewRt.y - static_cast<int32_t>(viewY & bitmask)) >> zoom) + y;

        zoomViewRt.pitch = rt->width + rt->pitch - (zoomViewRt.width >> zoom);
        zoomViewRt.bits = rt->bits + (unkX - rt->x) + ((unkY - rt->y) * (rt->width + rt->pitch));
        zoomViewRt.zoomLevel = zoom;

        // make sure, the compare operation is done in int32_t to avoid the loop becoming an infinite loop.
        // this as well as the [x += 32] in the loop causes signed integer overflow -> undefined behaviour.
        auto rightBorder = zoomViewRt.x + zoomViewRt.width;
        // Floors to nearest 32
        auto alignedX = zoomViewRt.x & ~0x1F;

        // Drawing is performed in columns of 32 pixels (1 tile wide)

        // Generate and sort columns.
        for (auto columnX = alignedX; columnX < rightBorder; columnX += 32)
        {
            Gfx::RenderTarget columnRt = zoomViewRt;
            if (columnX >= columnRt.x)
            {
                auto leftPitch = columnX - columnRt.x;
                columnRt.width -= leftPitch;
                columnRt.pitch += (leftPitch >> columnRt.zoomLevel);
                columnRt.bits += (leftPitch >> columnRt.zoomLevel);
                columnRt.x = columnX;
            }
            auto columnRightX = columnX + 32;
            auto paintRight = columnRt.x + columnRt.width;
            if (paintRight >= columnRightX)
            {
                auto rightPitch = paintRight - columnX - 32;
                paintRight -= rightPitch;
                columnRt.pitch += rightPitch >> columnRt.zoomLevel;
            }

            columnRt.width = paintRight - columnRt.x;

            Gfx::clearSingle(columnRt, fillColour);
            auto* sess = Paint::allocateSession(columnRt, options);
            sess->generate();
            sess->arrangeStructs();
            sess->drawStructs();
            // Climate code used to draw here.

            if (!isTitleMode())
            {
                if (!(options.viewFlags & ViewportFlags::station_names_displayed))
                {
                    if (columnRt.zoomLevel <= Config::get().old.stationNamesMinScale)
                    {
                        drawStationNames(columnRt);
                    }
                }
                if (!(options.viewFlags & ViewportFlags::town_names_displayed))
                {
                    drawTownNames(columnRt);
                }
            }

            sess->drawStringStructs();
            drawRoutingNumbers(columnRt);
        }
    }

    // 0x004CA444
    viewport_pos Viewport::centre2dCoordinates(const Pos3& loc)
    {
        auto centre = Map::gameToScreen(loc, getRotation());

        return viewport_pos(centre.x - viewWidth / 2, centre.y - viewHeight / 2);
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
        return viewport_pos(viewX + viewWidth / 2, viewY + viewHeight / 2);
    }

    Point Viewport::getUiCentre() const
    {
        return Point(x + width / 2, y + height / 2);
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
            result = viewportCoordToMapCoord(initialVPPos.x, initialVPPos.y, z.landHeight, rotation);
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
