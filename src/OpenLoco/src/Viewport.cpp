#include "Viewport.hpp"
#include "Config.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Map/MapSelection.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Paint/Paint.h"
#include "SceneManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/Window.h"
#include "Ui/WindowManager.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/Orders.h"
#include "Vehicles/VehicleManager.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

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
    void Viewport::render(Gfx::DrawingContext& drawingCtx)
    {
        const auto& rt = drawingCtx.currentRenderTarget();

        auto uiRect = rt.getUiRect();
        auto viewRect = getUiRect();

        if (!uiRect.intersects(viewRect))
        {
            return;
        }
        auto intersection = uiRect.intersection(viewRect);
        paint(drawingCtx, screenToViewport(intersection));
    }

    // 0x0048DE97
    static void drawStationNames(Gfx::DrawingContext& drawingCtx)
    {
        const auto& rt = drawingCtx.currentRenderTarget();

        Gfx::RenderTarget unZoomedRt = rt;
        unZoomedRt.zoomLevel = 0;
        unZoomedRt.x >>= rt.zoomLevel;
        unZoomedRt.y >>= rt.zoomLevel;
        unZoomedRt.width >>= rt.zoomLevel;
        unZoomedRt.height >>= rt.zoomLevel;

        drawingCtx.pushRenderTarget(unZoomedRt);

        for (const auto& station : StationManager::stations())
        {
            if ((station.flags & StationFlags::flag_5) != StationFlags::none)
            {
                continue;
            }

            bool isHovered = (World::hasMapSelectionFlag(World::MapSelectionFlags::hoveringOverStation))
                && (station.id() == Input::getHoveredStationId());

            drawStationName(drawingCtx, station, rt.zoomLevel, isHovered);
        }

        drawingCtx.popRenderTarget();
    }

    // 0x004977E5
    static void drawTownNames(Gfx::DrawingContext& drawingCtx)
    {
        const auto& rt = drawingCtx.currentRenderTarget();

        Gfx::RenderTarget unZoomedRt = rt;
        unZoomedRt.zoomLevel = 0;
        unZoomedRt.x >>= rt.zoomLevel;
        unZoomedRt.y >>= rt.zoomLevel;
        unZoomedRt.width >>= rt.zoomLevel;
        unZoomedRt.height >>= rt.zoomLevel;

        drawingCtx.pushRenderTarget(unZoomedRt);

        for (auto& town : TownManager::towns())
        {
            town.drawLabel(drawingCtx, rt);
        }

        drawingCtx.popRenderTarget();
    }

    // 0x00470A62
    static void drawRoutingNumbers(Gfx::DrawingContext& drawingCtx)
    {
        if (!World::hasMapSelectionFlag(World::MapSelectionFlags::unk_04))
        {
            return;
        }

        const auto& rt = drawingCtx.currentRenderTarget();

        Gfx::RenderTarget unZoomedRt = rt;
        unZoomedRt.zoomLevel = 0;
        unZoomedRt.x >>= rt.zoomLevel;
        unZoomedRt.y >>= rt.zoomLevel;
        unZoomedRt.width >>= rt.zoomLevel;
        unZoomedRt.height >>= rt.zoomLevel;

        drawingCtx.pushRenderTarget(unZoomedRt);

        auto tr = Gfx::TextRenderer(drawingCtx);

        auto orderNum = 0;
        for (auto& orderFrame : Vehicles::OrderManager::displayFrames())
        {
            auto orderRing = Vehicles::OrderRingView(orderFrame.orderOffset);
            auto* order = orderRing.atIndex(0);
            if (!order || !order->hasFlags(Vehicles::OrderFlags::HasNumber))
            {
                continue;
            }
            orderNum++;
            if (!orderFrame.frame.contains(rt.getDrawableRect(), rt.zoomLevel))
            {
                continue;
            }
            auto res = Vehicles::OrderManager::generateOrderUiStringAndLoc(orderFrame.orderOffset, orderNum);
            auto& orderString = res.second;
            if (orderString.empty())
            {
                continue;
            }

            tr.setCurrentFont(Gfx::Font::medium_normal);

            auto point = Point(orderFrame.frame.left[rt.zoomLevel] + 1, orderFrame.frame.top[rt.zoomLevel]);
            tr.drawString(point, AdvancedColour(Colour::white).outline(), const_cast<char*>(orderString.c_str()));
        }

        drawingCtx.popRenderTarget();
    }

    // 0x0045A1A4
    void Viewport::paint(Gfx::DrawingContext& drawingCtx, const Rect& rect)
    {
        const auto& rt = drawingCtx.currentRenderTarget();

        Paint::SessionOptions options{};
        if (hasFlags(ViewportFlags::seeThroughScenery | ViewportFlags::seeThroughTracks))
        {
            // TODO: unused
            options.foregroundCullHeight = viewHeight / 2 + viewY;
        }
        PaletteIndex_t fillColour = PaletteIndex::brown2;
        if (hasFlags(ViewportFlags::underground_view | ViewportFlags::flag_7 | ViewportFlags::flag_8))
        {
            fillColour = PaletteIndex::black0;
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

        zoomViewRt.pitch = rt.width + rt.pitch - (zoomViewRt.width >> zoom);
        zoomViewRt.bits = rt.bits + (unkX - rt.x) + ((unkY - rt.y) * (rt.width + rt.pitch));
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

            drawingCtx.pushRenderTarget(columnRt);

            {
                drawingCtx.clearSingle(fillColour);
                auto sess = Paint::PaintSession(columnRt, options);
                sess.generate();
                sess.arrangeStructs();
                sess.drawStructs(drawingCtx);
                // Climate code used to draw here.

                if (!isTitleMode())
                {
                    if (!options.hasFlags(ViewportFlags::station_names_displayed))
                    {
                        if (columnRt.zoomLevel <= Config::get().old.stationNamesMinScale)
                        {
                            drawStationNames(drawingCtx);
                        }
                    }
                    if (!options.hasFlags(ViewportFlags::town_names_displayed))
                    {
                        drawTownNames(drawingCtx);
                    }
                }

                sess.drawStringStructs(drawingCtx);
                drawRoutingNumbers(drawingCtx);
            }

            drawingCtx.popRenderTarget();
        }
    }

    // 0x004CA444
    viewport_pos Viewport::centre2dCoordinates(const Pos3& loc)
    {
        auto centre = World::gameToScreen(loc, getRotation());

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
            const auto z = World::TileManager::getHeight(result);
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
