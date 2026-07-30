#include "Viewport.hpp"
#include "Config.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
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

#include <execution>

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
        paint(drawingCtx, uiRect.intersection(viewRect));
    }

    // 0x0048DE97
    static void drawStationNames(Gfx::DrawingContext& drawingCtx, ZoomLevel zoom)
    {
        for (const auto& station : StationManager::stations())
        {
            if ((station.flags & StationFlags::flag_5) != StationFlags::none)
            {
                continue;
            }

            bool isHovered = (World::hasMapSelectionFlag(World::MapSelectionFlags::hoveringOverStation))
                && (station.id() == Input::getHoveredStationId());

            drawStationName(drawingCtx, station, zoom, isHovered);
        }
    }

    // 0x004977E5
    static void drawTownNames(Gfx::DrawingContext& drawingCtx, ZoomLevel zoom)
    {
        for (auto& town : TownManager::towns())
        {
            town.drawLabel(drawingCtx, zoom);
        }
    }

    // 0x00470A62
    static void drawRoutingNumbers(Gfx::DrawingContext& drawingCtx, ZoomLevel zoom)
    {
        if (!World::hasMapSelectionFlag(World::MapSelectionFlags::unk_04))
        {
            return;
        }

        const auto& rt = drawingCtx.currentRenderTarget();

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
            if (!orderFrame.frame.contains(rt.getUiRect(), zoom))
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

            auto point = Point(orderFrame.frame.left[zoom.index()] + 1, orderFrame.frame.top[zoom.index()]);
            tr.drawString(point, AdvancedColour(Colour::white).outline(), const_cast<char*>(orderString.c_str()));
        }
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

        Gfx::RenderTarget zoomViewRt{};
        zoomViewRt.width = rect.width();
        zoomViewRt.height = rect.height();
        zoomViewRt.x = zoom.applyInversedTo(viewX) + (rect.left() - x);
        zoomViewRt.y = zoom.applyInversedTo(viewY) + (rect.top() - y);

        zoomViewRt.pitch = rt.width + rt.pitch - zoomViewRt.width;
        zoomViewRt.bits = rt.bits + (rect.left() - rt.x) + ((rect.top() - rt.y) * (rt.width + rt.pitch));

        // make sure, the compare operation is done in int32_t to avoid the loop becoming an infinite loop.
        // this as well as the [x += 32] in the loop causes signed integer overflow -> undefined behaviour.
        const auto columnWidth = zoom.applyInversedTo(32);
        auto rightBorder = zoomViewRt.x + zoomViewRt.width;
        // Floors to nearest column
        auto alignedX = zoomViewRt.x & ~(columnWidth - 1);

        // Drawing is performed in columns of 32 pixels (1 tile wide)
        sfl::small_vector<Gfx::RenderTarget, 512> columns;

        // Generate and sort columns.
        for (auto columnX = alignedX; columnX < rightBorder; columnX += columnWidth)
        {
            Gfx::RenderTarget columnRt = zoomViewRt;
            if (columnX >= columnRt.x)
            {
                auto leftPitch = columnX - columnRt.x;
                columnRt.width -= leftPitch;
                columnRt.pitch += leftPitch;
                columnRt.bits += leftPitch;
                columnRt.x = columnX;
            }
            auto columnRightX = columnX + columnWidth;
            auto paintRight = columnRt.x + columnRt.width;
            if (paintRight >= columnRightX)
            {
                auto rightPitch = paintRight - columnRightX;
                paintRight -= rightPitch;
                columnRt.pitch += rightPitch;
            }

            columnRt.width = paintRight - columnRt.x;

            columns.push_back(columnRt);
        }

        std::for_each(std::execution::par, columns.begin(), columns.end(), [&](const auto& columnRt) {
            // TODO: This bypasses the interface currently, needs refactoring to create a new drawing context per thread.
            Gfx::SoftwareDrawingContext columnDrawingCtx;
            columnDrawingCtx.pushRenderTarget(columnRt);

            columnDrawingCtx.clearSingle(fillColour);
            auto sess = Paint::PaintSession(columnRt, zoom, options);
            sess.generate();
            sess.arrangeStructs();
            sess.drawStructs(columnDrawingCtx);
            // Climate code used to draw here.

            if (!SceneManager::isTitleMode())
            {
                if (!options.hasFlags(ViewportFlags::hideStationNames))
                {
                    if (zoom <= Config::get().stationNamesMinScale)
                    {
                        drawStationNames(columnDrawingCtx, zoom);
                    }
                }
                if (!options.hasFlags(ViewportFlags::hideTownNames))
                {
                    drawTownNames(columnDrawingCtx, zoom);
                }
            }

            sess.drawStringStructs(columnDrawingCtx);
            drawRoutingNumbers(columnDrawingCtx, zoom);
        });
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

    Point Viewport::getWindowCentre() const
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
        auto* owner = WindowManager::findWindowForViewport(this);
        if (owner == nullptr)
        {
            return {};
        }

        auto res = Ui::ViewportInteraction::getSurfaceLocFromUi(getWindowCentre() + owner->position());
        if (!res)
        {
            return {};
        }
        return { res->first };
    }
}
