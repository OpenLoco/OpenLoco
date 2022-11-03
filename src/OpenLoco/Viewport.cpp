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

        Gfx::RenderTarget target1{};
        target1.width = rect.width();
        target1.height = rect.height();
        target1.x = rect.origin.x;
        target1.y = rect.origin.y;

        target1.width &= bitmask;
        target1.height &= bitmask;
        target1.x &= bitmask;
        target1.y &= bitmask;

        auto unkX = ((target1.x - static_cast<int32_t>(viewX & bitmask)) >> zoom) + x;

        auto unkY = ((target1.y - static_cast<int32_t>(viewY & bitmask)) >> zoom) + y;

        target1.pitch = rt->width + rt->pitch - (target1.width >> zoom);
        target1.bits = rt->bits + (unkX - rt->x) + ((unkY - rt->y) * (rt->width + rt->pitch));
        target1.zoomLevel = zoom;

        Gfx::RenderTarget target2{};
        target2.y = target1.y;
        target2.height = target1.height;
        target2.zoomLevel = target1.zoomLevel;
        // make sure, the compare operation is done in int32_t to avoid the loop becoming an infinite loop.
        // this as well as the [x += 32] in the loop causes signed integer overflow -> undefined behaviour.
        auto rightBorder = target1.x + target1.width;
        // Floors to nearest 32
        auto alignedX = target1.x & ~0x1F;

        // Generate and sort columns.
        for (auto columnX = alignedX; columnX < rightBorder; columnX += 32)
        {
            target2.width = target1.width;
            target2.pitch = target1.pitch;
            target2.bits = target1.bits;
            target2.x = target1.x;
            if (columnX >= target1.x)
            {
                auto leftPitch = columnX - target1.x;
                target2.width -= leftPitch;
                target2.pitch += (leftPitch >> target1.zoomLevel);
                target2.bits += (leftPitch >> target1.zoomLevel);
                target2.x = columnX;
            }
            auto columnRightX = columnX + 32;
            auto paintRight = target2.x + target2.width;
            if (paintRight >= columnRightX)
            {
                auto rightPitch = paintRight - columnX - 32;
                paintRight -= rightPitch;
                target2.pitch += rightPitch >> target1.zoomLevel;
            }

            target2.width = paintRight - target2.x;

            Gfx::clearSingle(target2, fillColour);
            auto* sess = Paint::allocateSession(target2, options);
            sess->generate();
            sess->arrangeStructs();
            sess->drawStructs();
            // Climate code used to draw here.

            if (!isTitleMode())
            {
                if (!(options.viewFlags & ViewportFlags::station_names_displayed))
                {
                    if (target2.zoomLevel <= Config::get().old.stationNamesMinScale)
                    {
                        drawStationNames(target2);
                    }
                }
                if (!(options.viewFlags & ViewportFlags::town_names_displayed))
                {
                    drawTownNames(target2);
                }
            }

            sess->drawStringStructs();
            drawRoutingNumbers(target2);
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
