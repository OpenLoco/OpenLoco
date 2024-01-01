#include "Viewport.hpp"
#include "Config.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Paint/Paint.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/Orders.h"
#include "Vehicles/VehicleManager.h"
#include "Window.h"
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

    struct StationBorder
    {
        uint32_t left;
        uint32_t right;
        uint16_t width;
    };
    static constexpr std::array<StationBorder, 4> kZoomToStationBorder = {
        StationBorder{
            ImageIds::curved_border_left_medium,
            ImageIds::curved_border_right_medium,
            3,
        },
        StationBorder{
            ImageIds::curved_border_left_medium,
            ImageIds::curved_border_right_medium,
            3,
        },
        StationBorder{
            ImageIds::curved_border_left_small,
            ImageIds::curved_border_right_small,
            1,
        },
        StationBorder{
            ImageIds::curved_border_left_small,
            ImageIds::curved_border_right_small,
            1,
        },
    };

    static constexpr std::array<int16_t, 4> kZoomToStationFonts = {
        Font::medium_bold,
        Font::medium_bold,
        Font::small,
        Font::small,
    };

    // 0x0048DF4D, 0x0048E13B
    static void drawStationName(Gfx::RenderTarget& unZoomedRt, const Station& station, uint8_t zoom, bool isHovered)
    {
        if (!station.labelFrame.contains(unZoomedRt.getDrawableRect(), zoom))
        {
            return;
        }

        auto& borderImages = kZoomToStationBorder[zoom];

        const auto companyColour = [&station]() {
            if (station.owner == CompanyId::null)
            {
                return Colour::grey;
            }
            else
            {
                return CompanyManager::getCompanyColour(station.owner);
            }
        }();
        const auto colour = Colours::getTranslucent(companyColour, isHovered ? 0 : 1);

        Ui::Point topLeft = { station.labelFrame.left[zoom],
                              station.labelFrame.top[zoom] };
        Ui::Point bottomRight = { station.labelFrame.right[zoom],
                                  station.labelFrame.bottom[zoom] };

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(unZoomedRt, topLeft, ImageId(borderImages.left).withTranslucency(ExtColour::unk34));
        drawingCtx.drawImage(unZoomedRt, topLeft, ImageId(borderImages.left).withTranslucency(colour));

        Ui::Point topRight = { static_cast<int16_t>(bottomRight.x - borderImages.width), topLeft.y };
        drawingCtx.drawImage(unZoomedRt, topRight, ImageId(borderImages.right).withTranslucency(ExtColour::unk34));
        drawingCtx.drawImage(unZoomedRt, topRight, ImageId(borderImages.right).withTranslucency(colour));

        drawingCtx.drawRect(unZoomedRt, topLeft.x + borderImages.width + 1, topLeft.y, bottomRight.x - topLeft.x - 2 * borderImages.width, bottomRight.y - topLeft.y + 1, enumValue(ExtColour::unk34), Drawing::RectFlags::transparent);
        drawingCtx.drawRect(unZoomedRt, topLeft.x + borderImages.width + 1, topLeft.y, bottomRight.x - topLeft.x - 2 * borderImages.width, bottomRight.y - topLeft.y + 1, enumValue(colour), Drawing::RectFlags::transparent);

        char buffer[512]{};

        FormatArguments args;
        args.push<uint16_t>(enumValue(station.town));
        auto* str = buffer;
        *str++ = ControlCodes::Colour::black;
        str = StringManager::formatString(str, station.name, &args);
        *str++ = ' ';
        StringManager::formatString(str, getTransportIconsFromStationFlags(station.flags));

        drawingCtx.setCurrentFontSpriteBase(kZoomToStationFonts[zoom]);
        drawingCtx.drawString(unZoomedRt, topLeft.x + borderImages.width, topLeft.y, Colour::black, buffer);
    }

    // 0x0048DE97
    static void drawStationNames(Gfx::RenderTarget& rt)
    {
        Gfx::RenderTarget unZoomedRt = rt;
        unZoomedRt.zoomLevel = 0;
        unZoomedRt.x >>= rt.zoomLevel;
        unZoomedRt.y >>= rt.zoomLevel;
        unZoomedRt.width >>= rt.zoomLevel;
        unZoomedRt.height >>= rt.zoomLevel;

        for (const auto& station : StationManager::stations())
        {
            if ((station.flags & StationFlags::flag_5) != StationFlags::none)
            {
                continue;
            }

            bool isHovered = (Input::hasMapSelectionFlag(Input::MapSelectionFlags::hoveringOverStation))
                && (station.id() == Input::getHoveredStationId());

            drawStationName(unZoomedRt, station, rt.zoomLevel, isHovered);
        }
    }

    static constexpr std::array<int16_t, 4> kZoomToTownFonts = {
        Font::medium_bold,
        Font::medium_bold,
        Font::medium_normal,
        Font::medium_normal,
    };

    // 0x004977E5
    static void drawTownNames(Gfx::RenderTarget& rt)
    {
        Gfx::RenderTarget unZoomedRt = rt;
        unZoomedRt.zoomLevel = 0;
        unZoomedRt.x >>= rt.zoomLevel;
        unZoomedRt.y >>= rt.zoomLevel;
        unZoomedRt.width >>= rt.zoomLevel;
        unZoomedRt.height >>= rt.zoomLevel;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        char buffer[512]{};
        for (const auto& town : TownManager::towns())
        {
            if (!town.labelFrame.contains(rt.getDrawableRect(), rt.zoomLevel))
            {
                continue;
            }

            StringManager::formatString(buffer, town.name);
            drawingCtx.setCurrentFontSpriteBase(kZoomToTownFonts[rt.zoomLevel]);
            drawingCtx.drawString(unZoomedRt, town.labelFrame.left[rt.zoomLevel] + 1, town.labelFrame.top[rt.zoomLevel] + 1, AdvancedColour(Colour::white).outline(), buffer);
        }
    }

    // 0x00470A62
    static void drawRoutingNumbers(Gfx::RenderTarget& rt)
    {
        if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::unk_04))
        {
            return;
        }

        Gfx::RenderTarget unZoomedRt = rt;
        unZoomedRt.zoomLevel = 0;
        unZoomedRt.x >>= rt.zoomLevel;
        unZoomedRt.y >>= rt.zoomLevel;
        unZoomedRt.width >>= rt.zoomLevel;
        unZoomedRt.height >>= rt.zoomLevel;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

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

            drawingCtx.setCurrentFontSpriteBase(Font::medium_normal);
            drawingCtx.drawString(unZoomedRt, orderFrame.frame.left[rt.zoomLevel] + 1, orderFrame.frame.top[rt.zoomLevel], AdvancedColour(Colour::white).outline(), const_cast<char*>(orderString.c_str()));
        }
    }

    // 0x0045A1A4
    void Viewport::paint(Gfx::RenderTarget* rt, const Rect& rect)
    {
        Paint::SessionOptions options{};
        if (hasFlags(ViewportFlags::seeThroughScenery | ViewportFlags::seeThroughTracks))
        {
            // TODO: unused
            options.foregroundCullHeight = viewHeight / 2 + viewY;
        }
        PaletteIndex_t fillColour = PaletteIndex::index_D8;
        if (hasFlags(ViewportFlags::underground_view | ViewportFlags::flag_7 | ViewportFlags::flag_8))
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

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

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

            drawingCtx.clearSingle(columnRt, fillColour);
            auto* sess = Paint::allocateSession(columnRt, options);
            sess->generate();
            sess->arrangeStructs();
            sess->drawStructs();
            // Climate code used to draw here.

            if (!isTitleMode())
            {
                if (!options.hasFlags(ViewportFlags::station_names_displayed))
                {
                    if (columnRt.zoomLevel <= Config::get().old.stationNamesMinScale)
                    {
                        drawStationNames(columnRt);
                    }
                }
                if (!options.hasFlags(ViewportFlags::town_names_displayed))
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
