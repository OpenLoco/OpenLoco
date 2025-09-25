#include "Engine/Limits.h"
#include "Entities/Entity.h"
#include "Entities/EntityManager.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameStateFlags.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/IndustryElement.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Objects/IndustryObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Objects/WaterObject.h"
#include "SceneManager.h"
#include "Types.hpp"
#include "Ui/LastMapWindowAttributes.h"
#include "Ui/ScrollView.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/ScrollViewWidget.h"
#include "Ui/Widgets/TabWidget.h"
#include "Ui/WindowManager.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/Orders.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include "World/CompanyManager.h"
#include "World/IndustryManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Core/Numerics.hpp>

using namespace OpenLoco::Ui::WindowManager;
using namespace OpenLoco::World;

namespace OpenLoco::Ui::Windows::MapWindow
{
    static constexpr uint16_t kMinimumWindowWidth = 229;  // Chosen so that the map cannot be smaller than its key
    static constexpr uint16_t kMinimumWindowHeight = 176; // Chosen so that the minimum size makes the map square

    static constexpr int16_t kRenderedMapWidth = TileManager::getMapColumns() * 2;
    static constexpr int16_t kRenderedMapHeight = kRenderedMapWidth;
    static constexpr int32_t kRenderedMapSize = kRenderedMapWidth * kRenderedMapHeight;

    // 0x004FDC4C
    static std::array<Point, 4> kViewFrameOffsetsByRotation = { {
        { TileManager::getMapColumns() - 8, 0 },
        { kRenderedMapWidth - 8, TileManager::getMapRows() },
        { TileManager::getMapColumns() - 8, kRenderedMapHeight },
        { -8, TileManager::getMapRows() },
    } };

    static constexpr std::array<PaletteIndex_t, 256> kFlashColours = []() {
        std::array<PaletteIndex_t, 256> colours;

        std::fill(colours.begin(), colours.end(), PaletteIndex::black0);
        std::fill(colours.begin() + 10, colours.begin() + 14, PaletteIndex::blackB);

        return colours;
    }(); // 0x004FDC5C

    static PaletteIndex_t* _mapPixels; // 0x00F253A8
    static PaletteIndex_t* _mapAltPixels;

    static std::array<uint16_t, 6> _vehicleTypeCounts = {};

    static uint32_t _flashingItems;              // 0x00F253A4
    static uint32_t _drawMapRowIndex;            // 0x00F253AC
    static uint8_t _assignedIndustryColours[16]; // 0x00F253CE
    static uint8_t _routeToObjectIdMap[19];      // 0x00F253DF
    static uint8_t _routeColours[19];            // 0x00F253F2
    static uint8_t _trackColours[8];             // 0x00F25404
    static uint8_t _roadColours[8];              // 0x00F2540C

    enum widx
    {
        frame = 0,
        caption,
        closeButton,
        panel,
        tabOverall,
        tabVehicles,
        tabIndustries,
        tabRoutes,
        tabOwnership,
        scrollview,
        statusBar,
    };

    static constexpr auto kWidgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, { 350, 272 }, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { 348, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, StringIds::title_map),
        Widgets::ImageButton({ 335, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 41 }, { 350, 230 }, WindowColour::secondary),
        Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tab_map_overall),
        Widgets::Tab({ 34, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tab_map_vehicles),
        Widgets::Tab({ 65, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tab_map_industries),
        Widgets::Tab({ 96, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tab_map_routes),
        Widgets::Tab({ 158, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tab_map_ownership),
        Widgets::ScrollView({ 3, 44 }, { 240, 215 }, WindowColour::secondary, Scrollbars::horizontal | Scrollbars::vertical),
        Widgets::Label({ 3, 250 }, { 322, 21 }, WindowColour::secondary, ContentAlign::center)

    );

    static Pos2 mapWindowPosToLocation(Point pos)
    {
        pos.x = ((pos.x + 8) - TileManager::getMapColumns()) / 2;
        pos.y = ((pos.y + 8)) / 2;
        Pos2 location = { static_cast<coord_t>(pos.y - pos.x), static_cast<coord_t>(pos.x + pos.y) };
        location.x *= kTileSize;
        location.y *= kTileSize;

        switch (getCurrentRotation())
        {
            case 0:
                return location;
            case 1:
                return { static_cast<coord_t>(TileManager::getMapWidth() - 1 - location.y), location.x };
            case 2:
                return { static_cast<coord_t>(TileManager::getMapWidth() - 1 - location.x), static_cast<coord_t>(TileManager::getMapHeight() - 1 - location.y) };
            case 3:
                return { location.y, static_cast<coord_t>(TileManager::getMapHeight() - 1 - location.x) };
        }

        return { 0, 0 }; // unreachable
    }

    static Point locationToMapWindowPos(Pos2 pos)
    {
        int32_t x = pos.x;
        int32_t y = pos.y;

        switch (getCurrentRotation())
        {
            case 3:
                std::swap(x, y);
                x = TileManager::getMapWidth() - 1 - x;
                break;
            case 2:
                x = TileManager::getMapWidth() - 1 - x;
                y = TileManager::getMapHeight() - 1 - y;
                break;
            case 1:
                std::swap(x, y);
                y = TileManager::getMapHeight() - 1 - y;
                break;
            case 0:
                break;
        }

        x /= kTileSize;
        y /= kTileSize;

        return Point(-x + y + TileManager::getMapColumns() - 8, x + y - 8);
    }

    // 0x0046B8E6
    static void onClose(Window& self)
    {
        Ui::getLastMapWindowAttributes().size = Ui::Size(self.width, self.height);
        Ui::getLastMapWindowAttributes().var88A = self.var_88A;
        Ui::getLastMapWindowAttributes().var88C = self.var_88C;
        Ui::getLastMapWindowAttributes().flags = self.flags | WindowFlags::flag_31;

        free(_mapPixels);
    }

    // 0x0046B8CF
    static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case widx::closeButton:
                WindowManager::close(&self);
                break;

            case widx::tabOverall:
            case widx::tabVehicles:
            case widx::tabIndustries:
            case widx::tabRoutes:
            case widx::tabOwnership:
            case widx::scrollview:
            {
                auto tabIndex = widgetIndex - widx::tabOverall;

                if (tabIndex == self.currentTab)
                {
                    return;
                }

                self.currentTab = tabIndex;
                self.frameNo = 0;
                self.var_854 = 0;
                break;
            }
        }
    }

    // 0x0046B9F7
    static void onResize(Window& self)
    {
        self.flags |= WindowFlags::resizable;
        self.minWidth = kMinimumWindowWidth;
        self.maxWidth = 800; // NB: frame background is only 800px :(
        self.maxHeight = 800;

        Ui::Size32 kMinWindowSize = { self.minWidth, self.minHeight };
        Ui::Size32 kMaxWindowSize = { self.maxWidth, self.maxHeight };
        self.setSize(kMinWindowSize, kMaxWindowSize);
    }

    // 0x0046C5E5
    static void setMapPixelsOverall(PaletteIndex_t* mapPtr, PaletteIndex_t* mapAltPtr, Pos2 pos, Pos2 delta)
    {
        for (auto rowCountLeft = TileManager::getMapColumns(); rowCountLeft > 0; rowCountLeft--)
        {
            // Coords shouldn't be at map edge
            if (!(pos.x > 0 && pos.y > 0 && pos.x < TileManager::getMapWidth() - kTileSize && pos.y < TileManager::getMapHeight() - kTileSize))
            {
                pos += delta;
                mapPtr += kRenderedMapWidth + 1;
                mapAltPtr += kRenderedMapWidth + 1;
                continue;
            }

            PaletteIndex_t colourFlash0{}, colourFlash1{}, colour0{}, colour1{};
            auto tile = TileManager::get(pos);
            for (auto& el : tile)
            {
                switch (el.type())
                {
                    case ElementType::surface:
                    {
                        auto* surfaceEl = el.as<SurfaceElement>();
                        if (surfaceEl == nullptr)
                        {
                            continue;
                        }

                        if (surfaceEl->water() == 0)
                        {
                            const auto* landObj = ObjectManager::get<LandObject>(surfaceEl->terrain());
                            const auto* landImage = Gfx::getG1Element(landObj->mapPixelImage);
                            auto offset = surfaceEl->baseZ() / kMicroToSmallZStep * 2;
                            colourFlash0 = landImage->offset[offset];
                            colourFlash1 = landImage->offset[offset + 1];
                        }
                        else
                        {
                            const auto* waterObj = ObjectManager::get<WaterObject>();
                            const auto* waterImage = Gfx::getG1Element(waterObj->mapPixelImage);
                            auto offset = (surfaceEl->water() * kMicroToSmallZStep - surfaceEl->baseZ()) / 2;
                            colourFlash0 = waterImage->offset[offset - 2];
                            colourFlash1 = waterImage->offset[offset - 1];
                        }

                        colour0 = colourFlash0;
                        colour1 = colourFlash1;
                        break;
                    }

                    case ElementType::track:
                        if (!el.isGhost() && !el.isAiAllocated())
                        {
                            auto* trackEl = el.as<TrackElement>();
                            if (trackEl == nullptr)
                            {
                                continue;
                            }

                            auto* trackObj = ObjectManager::get<TrackObject>(trackEl->trackObjectId());
                            if (trackObj->hasFlags(TrackObjectFlags::unk_02))
                            {
                                colour0 = colourFlash0 = PaletteIndex::black2;
                                if (_flashingItems & (1 << 2))
                                {
                                    colourFlash0 = kFlashColours[colourFlash0];
                                }
                            }
                            else
                            {
                                colour0 = colourFlash0 = PaletteIndex::black7;
                                if (_flashingItems & (1 << 3))
                                {
                                    colourFlash0 = kFlashColours[colourFlash0];
                                }
                            }

                            colourFlash1 = colourFlash0;
                            colour1 = colour0;
                        }
                        break;

                    case ElementType::station:
                        if (!el.isGhost() && !el.isAiAllocated())
                        {
                            colour0 = colourFlash0 = PaletteIndex::orange8;
                            if (_flashingItems & (1 << 4))
                            {
                                colourFlash0 = kFlashColours[colourFlash0];
                            }
                            colourFlash1 = colourFlash0;
                            colour1 = colour0;
                        }
                        break;

                    case ElementType::signal:
                        break;

                    case ElementType::building:
                        if (!el.isGhost())
                        {
                            colour0 = colourFlash0 = PaletteIndex::mutedDarkRed7;
                            if (_flashingItems & (1 << 0))
                            {
                                colourFlash0 = kFlashColours[colourFlash0];
                            }
                            colourFlash1 = colourFlash0;
                            colour1 = colour0;
                        }
                        break;

                    case ElementType::tree:
                        if (!el.isGhost())
                        {
                            colour1 = colourFlash1 = PaletteIndex::green6;
                            if (_flashingItems & (1 << 5))
                            {
                                colourFlash1 = PaletteIndex::black0;
                            }
                        }
                        break;

                    case ElementType::wall:
                        continue;

                    case ElementType::road:
                        if (!el.isGhost() && !el.isAiAllocated())
                        {
                            auto* roadEl = el.as<RoadElement>();
                            if (roadEl == nullptr)
                            {
                                continue;
                            }

                            auto* roadObj = ObjectManager::get<RoadObject>(roadEl->roadObjectId());
                            if (roadObj->hasFlags(RoadObjectFlags::unk_01))
                            {
                                colour0 = colourFlash0 = PaletteIndex::black7;
                                if (_flashingItems & (1 << 3))
                                {
                                    colourFlash0 = kFlashColours[colourFlash0];
                                }
                            }
                            else
                            {
                                colour0 = colourFlash0 = PaletteIndex::black2;
                                if (_flashingItems & (1 << 2))
                                {
                                    colourFlash0 = kFlashColours[colourFlash0];
                                }
                            }

                            colourFlash1 = colourFlash0;
                            colour1 = colour0;
                        }
                        break;

                    case ElementType::industry:
                        if (!el.isGhost())
                        {
                            colour0 = colourFlash0 = PaletteIndex::mutedPurple7;
                            if (_flashingItems & (1 << 1))
                            {
                                colourFlash0 = kFlashColours[colourFlash0];
                            }
                            colourFlash1 = colourFlash0;
                            colour1 = colour0;
                        }
                        break;
                };
            }

            mapPtr[0] = colour0;
            mapPtr[1] = colour1;
            mapAltPtr[0] = colourFlash0;
            mapAltPtr[1] = colourFlash1;

            pos += delta;
            mapPtr += kRenderedMapWidth + 1;
            mapAltPtr += kRenderedMapWidth + 1;
        }

        _drawMapRowIndex++;
        if (_drawMapRowIndex > TileManager::getMapColumns())
        {
            _drawMapRowIndex = 0;
        }
    }

    // 0x0046C873
    static void setMapPixelsVehicles(PaletteIndex_t* mapPtr, PaletteIndex_t* mapAltPtr, Pos2 pos, Pos2 delta)
    {
        for (auto rowCountLeft = TileManager::getMapColumns(); rowCountLeft > 0; rowCountLeft--)
        {
            // Coords shouldn't be at map edge
            if (!(pos.x > 0 && pos.y > 0 && pos.x < TileManager::getMapWidth() - kTileSize && pos.y < TileManager::getMapHeight() - kTileSize))
            {
                pos += delta;
                mapPtr += kRenderedMapWidth + 1;
                mapAltPtr += kRenderedMapWidth + 1;
                continue;
            }

            PaletteIndex_t colourFlash0{}, colourFlash1{}, colour0{}, colour1{};
            auto tile = TileManager::get(pos);
            for (auto& el : tile)
            {
                switch (el.type())
                {
                    case ElementType::surface:
                    {
                        auto* surfaceEl = el.as<SurfaceElement>();
                        if (surfaceEl == nullptr)
                        {
                            continue;
                        }

                        if (surfaceEl->water() == 0)
                        {
                            const auto* landObj = ObjectManager::get<LandObject>(surfaceEl->terrain());
                            const auto* landImage = Gfx::getG1Element(landObj->mapPixelImage);
                            colourFlash0 = colourFlash1 = landImage->offset[0];
                        }
                        else
                        {
                            const auto* waterObj = ObjectManager::get<WaterObject>();
                            const auto* waterImage = Gfx::getG1Element(waterObj->mapPixelImage);
                            colourFlash0 = colourFlash1 = waterImage->offset[0];
                        }

                        colour0 = colour1 = colourFlash0;
                        break;
                    }

                    case ElementType::track:
                    case ElementType::station:
                    case ElementType::road:
                        if (!el.isGhost() && !el.isAiAllocated())
                        {
                            colour0 = colourFlash0 = PaletteIndex::black2;
                            colourFlash1 = colourFlash0;
                            colour1 = colour0;
                        }
                        break;

                    case ElementType::building:
                    case ElementType::industry:
                        if (!el.isGhost())
                        {
                            colour0 = colourFlash0 = PaletteIndex::mutedDarkRed2;
                            colourFlash1 = colourFlash0;
                            colour1 = colour0;
                        }
                        break;

                    default:
                        break;
                };
            }

            mapPtr[0] = colour0;
            mapPtr[1] = colour1;
            mapAltPtr[0] = colourFlash0;
            mapAltPtr[1] = colourFlash1;

            pos += delta;
            mapPtr += kRenderedMapWidth + 1;
            mapAltPtr += kRenderedMapWidth + 1;
        }

        _drawMapRowIndex++;
        if (_drawMapRowIndex > TileManager::getMapColumns())
        {
            _drawMapRowIndex = 0;
        }
    }

    // 0x004FB464
    // clang-format off
    static constexpr std::array<PaletteIndex_t, 31> kIndustryColours = {
        PaletteIndex::black0,           PaletteIndex::black4,           PaletteIndex::blackB,           PaletteIndex::mutedOliveGreen9,
        PaletteIndex::mutedDarkYellow7, PaletteIndex::yellow7,          PaletteIndex::yellowA,          PaletteIndex::mutedDarkRed5,
        PaletteIndex::mutedDarkRed9,    PaletteIndex::mutedGrassGreen5, PaletteIndex::mutedGrassGreenA, PaletteIndex::mutedAvocadoGreen6,
        PaletteIndex::green8,           PaletteIndex::mutedOrange7,     PaletteIndex::mutedPurple7,     PaletteIndex::blue3,
        PaletteIndex::blue7,            PaletteIndex::purple3,          PaletteIndex::purple7,          PaletteIndex::purple9,
        PaletteIndex::red6,             PaletteIndex::orange6,          PaletteIndex::orange9,          PaletteIndex::mutedDarkTeal5,
        PaletteIndex::mutedDarkTeal8,   PaletteIndex::pink6,            PaletteIndex::pink9,            PaletteIndex::brown5,
        PaletteIndex::brown8,           PaletteIndex::mutedDarkYellow2, PaletteIndex::black8,
    };
    // clang-format on

    // 0x0046C9A8
    static void setMapPixelsIndustries(PaletteIndex_t* mapPtr, PaletteIndex_t* mapAltPtr, Pos2 pos, Pos2 delta)
    {
        for (auto rowCountLeft = TileManager::getMapColumns(); rowCountLeft > 0; rowCountLeft--)
        {
            // Coords shouldn't be at map edge
            if (!(pos.x > 0 && pos.y > 0 && pos.x < TileManager::getMapWidth() - kTileSize && pos.y < TileManager::getMapHeight() - kTileSize))
            {
                pos += delta;
                mapPtr += kRenderedMapWidth + 1;
                mapAltPtr += kRenderedMapWidth + 1;
                continue;
            }

            PaletteIndex_t colourFlash0{}, colourFlash1{}, colour0{}, colour1{};
            auto tile = TileManager::get(pos);
            for (auto& el : tile)
            {
                switch (el.type())
                {
                    case ElementType::surface:
                    {
                        auto* surfaceEl = el.as<SurfaceElement>();
                        if (surfaceEl == nullptr)
                        {
                            continue;
                        }

                        if (surfaceEl->water() > 0)
                        {
                            const auto* waterObj = ObjectManager::get<WaterObject>();
                            const auto* waterImage = Gfx::getG1Element(waterObj->mapPixelImage);
                            colour0 = colour1 = colourFlash0 = colourFlash1 = waterImage->offset[0];
                        }
                        else
                        {
                            const auto* landObj = ObjectManager::get<LandObject>(surfaceEl->terrain());
                            const auto* landImage = Gfx::getG1Element(landObj->mapPixelImage);
                            colour0 = colour1 = colourFlash0 = colourFlash1 = landImage->offset[0];
                        }

                        if (surfaceEl->isIndustrial())
                        {
                            const auto* industry = IndustryManager::get(surfaceEl->industryId());
                            const auto colourIndex = _assignedIndustryColours[industry->objectId];
                            colour0 = colourFlash0 = kIndustryColours[colourIndex];
                            if (_flashingItems & (1 << industry->objectId))
                            {
                                colourFlash0 = PaletteIndex::black0;
                            }
                        }
                        break;
                    }

                    case ElementType::building:
                        // Vanilla omitted the ghost check
                        if (!el.isGhost())
                        {
                            colour0 = colourFlash0 = PaletteIndex::mutedDarkRed2;
                            colourFlash1 = colourFlash0;
                            colour1 = colour0;
                        }
                        break;

                    case ElementType::industry:
                    {
                        if (el.isGhost())
                        {
                            continue;
                        }

                        auto* industryEl = el.as<IndustryElement>();
                        if (industryEl == nullptr)
                        {
                            continue;
                        }

                        const auto* industry = IndustryManager::get(industryEl->industryId());
                        const auto colourIndex = _assignedIndustryColours[industry->objectId];
                        colourFlash0 = colourFlash1 = colour0 = colour1 = kIndustryColours[colourIndex];
                        if (_flashingItems & (1 << industry->objectId))
                        {
                            colourFlash0 = colourFlash1 = PaletteIndex::black0;
                        }
                        break;
                    }

                    case ElementType::track:
                    case ElementType::station:
                    case ElementType::road:
                    {
                        if (el.isGhost() || el.isAiAllocated())
                        {
                            continue;
                        }

                        colour0 = colour1 = colourFlash1 = colourFlash0 = PaletteIndex::black2;
                        break;
                    }

                    default:
                        break;
                };
            }

            mapPtr[0] = colour0;
            mapPtr[1] = colour1;
            mapAltPtr[0] = colourFlash0;
            mapAltPtr[1] = colourFlash1;

            pos += delta;
            mapPtr += kRenderedMapWidth + 1;
            mapAltPtr += kRenderedMapWidth + 1;
        }

        _drawMapRowIndex++;
        if (_drawMapRowIndex > TileManager::getMapColumns())
        {
            _drawMapRowIndex = 0;
        }
    }

    // 0x0046CB68
    static void setMapPixelsRoutes(PaletteIndex_t* mapPtr, PaletteIndex_t* mapAltPtr, Pos2 pos, Pos2 delta)
    {
        for (auto rowCountLeft = TileManager::getMapColumns(); rowCountLeft > 0; rowCountLeft--)
        {
            // Coords shouldn't be at map edge
            if (!(pos.x > 0 && pos.y > 0 && pos.x < TileManager::getMapWidth() - kTileSize && pos.y < TileManager::getMapHeight() - kTileSize))
            {
                pos += delta;
                mapPtr += kRenderedMapWidth + 1;
                mapAltPtr += kRenderedMapWidth + 1;
                continue;
            }

            bool haveTrackOrRoad = false; // ch
            PaletteIndex_t colourFlash0{}, colourFlash1{}, colour0{}, colour1{};
            auto tile = TileManager::get(pos);
            for (auto& el : tile)
            {
                switch (el.type())
                {
                    case ElementType::surface:
                    {
                        auto* surfaceEl = el.as<SurfaceElement>();
                        if (surfaceEl == nullptr)
                        {
                            continue;
                        }

                        uint8_t terrainColour0{}, terrainColour1{};
                        if (surfaceEl->water() == 0)
                        {
                            const auto* landObj = ObjectManager::get<LandObject>(surfaceEl->terrain());
                            const auto* landImage = Gfx::getG1Element(landObj->mapPixelImage);
                            terrainColour0 = landImage->offset[0];
                            terrainColour1 = landImage->offset[1];
                        }
                        else
                        {
                            const auto* waterObj = ObjectManager::get<WaterObject>();
                            const auto* waterImage = Gfx::getG1Element(waterObj->mapPixelImage);
                            terrainColour0 = waterImage->offset[0];
                            terrainColour1 = waterImage->offset[1];
                        }

                        colour0 = colourFlash0 = terrainColour0;
                        if (!haveTrackOrRoad)
                        {
                            colour1 = colourFlash1 = terrainColour1;
                        }
                        break;
                    }

                    case ElementType::building:
                    case ElementType::industry:
                        if (!el.isGhost())
                        {
                            colour0 = colourFlash0 = PaletteIndex::mutedDarkRed2;
                            colourFlash1 = colourFlash0;
                            colour1 = colour0;
                        }
                        break;

                    case ElementType::track:
                    {
                        if (el.isGhost() || el.isAiAllocated())
                        {
                            continue;
                        }

                        auto* trackEl = el.as<TrackElement>();
                        if (trackEl == nullptr)
                        {
                            continue;
                        }

                        auto trackObjectId = trackEl->trackObjectId();
                        colourFlash0 = colour0 = _trackColours[trackObjectId];

                        auto firstFlashable = Numerics::bitScanForward(_flashingItems);
                        if (firstFlashable != -1)
                        {
                            if (_routeToObjectIdMap[firstFlashable] == trackObjectId)
                            {
                                colourFlash0 = kFlashColours[colourFlash0];
                            }
                        }

                        colourFlash1 = colourFlash0;
                        colour1 = colour0;

                        haveTrackOrRoad = true;
                        break;
                    }

                    case ElementType::station:
                    {
                        if (!el.isGhost() && !el.isAiAllocated())
                        {
                            colour1 = colourFlash1 = colour0 = colourFlash0 = PaletteIndex::orange8;
                        }
                        break;
                    }

                    case ElementType::road:
                    {
                        if (el.isGhost() || el.isAiAllocated())
                        {
                            continue;
                        }

                        auto* roadEl = el.as<RoadElement>();
                        if (roadEl == nullptr)
                        {
                            continue;
                        }

                        colourFlash0 = colour0 = _roadColours[roadEl->roadObjectId()];

                        auto firstFlashable = Numerics::bitScanForward(_flashingItems);
                        if (firstFlashable != -1)
                        {
                            if (_routeToObjectIdMap[firstFlashable] == (roadEl->roadObjectId() | (1 << 7)))
                            {
                                colourFlash0 = kFlashColours[colourFlash0];
                            }
                        }

                        colour1 = colour0;
                        colourFlash1 = colourFlash0;
                        haveTrackOrRoad = true;
                        break;
                    }

                    default:
                        break;
                };
            }

            mapPtr[0] = colour0;
            mapPtr[1] = colour1;
            mapAltPtr[0] = colourFlash0;
            mapAltPtr[1] = colourFlash1;

            pos += delta;
            mapPtr += kRenderedMapWidth + 1;
            mapAltPtr += kRenderedMapWidth + 1;
        }

        _drawMapRowIndex++;
        if (_drawMapRowIndex > TileManager::getMapColumns())
        {
            _drawMapRowIndex = 0;
        }
    }

    // 0x0046CD31
    static void setMapPixelsOwnership(PaletteIndex_t* mapPtr, PaletteIndex_t* mapAltPtr, Pos2 pos, Pos2 delta)
    {
        for (auto rowCountLeft = TileManager::getMapColumns(); rowCountLeft > 0; rowCountLeft--)
        {
            // Coords shouldn't be at map edge
            if (!(pos.x > 0 && pos.y > 0 && pos.x < TileManager::getMapWidth() - kTileSize && pos.y < TileManager::getMapHeight() - kTileSize))
            {
                pos += delta;
                mapPtr += kRenderedMapWidth + 1;
                mapAltPtr += kRenderedMapWidth + 1;
                continue;
            }

            bool haveTrackOrRoad = false; // ch
            PaletteIndex_t colourFlash0{}, colourFlash1{}, colour0{}, colour1{};
            auto tile = TileManager::get(pos);
            for (auto& el : tile)
            {
                switch (el.type())
                {
                    case ElementType::surface:
                    {
                        auto* surfaceEl = el.as<SurfaceElement>();
                        if (surfaceEl == nullptr)
                        {
                            continue;
                        }

                        uint8_t terrainColour0{}, terrainColour1{};
                        if (surfaceEl->water() == 0)
                        {
                            const auto* landObj = ObjectManager::get<LandObject>(surfaceEl->terrain());
                            const auto* landImage = Gfx::getG1Element(landObj->mapPixelImage);
                            terrainColour0 = landImage->offset[0];
                            terrainColour1 = landImage->offset[1];
                        }
                        else
                        {
                            const auto* waterObj = ObjectManager::get<WaterObject>();
                            const auto* waterImage = Gfx::getG1Element(waterObj->mapPixelImage);
                            terrainColour0 = waterImage->offset[0];
                            terrainColour1 = waterImage->offset[1];
                        }

                        colour0 = colourFlash0 = terrainColour0;
                        if (!haveTrackOrRoad)
                        {
                            colour1 = colourFlash1 = terrainColour1;
                        }
                        break;
                    }

                    case ElementType::track:
                    case ElementType::station:
                    case ElementType::road:
                    {
                        if (el.isGhost() || el.isAiAllocated())
                        {
                            continue;
                        }

                        auto owner = CompanyId::null;

                        if (auto* stationEl = el.as<StationElement>())
                        {
                            auto station = StationManager::get(stationEl->stationId());
                            owner = station->owner;
                        }
                        else if (auto* trackEl = el.as<TrackElement>())
                        {
                            owner = trackEl->owner();
                        }
                        else if (auto* roadEl = el.as<RoadElement>())
                        {
                            owner = roadEl->owner();
                        }

                        if (owner != CompanyId::neutral)
                        {
                            auto companyColour = CompanyManager::getCompanyColour(owner);
                            colourFlash1 = colourFlash0 = colour1 = colour0 = Colours::getShade(companyColour, 5);
                            if (_flashingItems & (1 << enumValue(owner)))
                            {
                                colourFlash1 = colourFlash0 = kFlashColours[colour0];
                            }
                            haveTrackOrRoad = true;
                            break;
                        }

                        [[fallthrough]];
                    }

                    case ElementType::building:
                    case ElementType::industry:
                        // Vanilla omitted the ghost check
                        if (!el.isGhost())
                        {
                            colour0 = colour1 = colourFlash1 = colourFlash0 = PaletteIndex::black1;
                        }
                        break;

                    default:
                        break;
                };
            }

            mapPtr[0] = colour0;
            mapPtr[1] = colour1;
            mapAltPtr[0] = colourFlash0;
            mapAltPtr[1] = colourFlash1;

            pos += delta;
            mapPtr += kRenderedMapWidth + 1;
            mapAltPtr += kRenderedMapWidth + 1;
        }

        _drawMapRowIndex++;
        if (_drawMapRowIndex > TileManager::getMapColumns())
        {
            _drawMapRowIndex = 0;
        }
    }

    // 0x0046C544
    static void setMapPixels(const Window& self)
    {
        _flashingItems = self.var_854;
        auto offset = _drawMapRowIndex * (kRenderedMapWidth - 1) + (TileManager::getMapRows() - 1);
        auto* mapPtr = &_mapPixels[offset];
        auto* mapAltPtr = &_mapAltPixels[offset];

        Pos2 pos{};
        Pos2 delta{};
        switch (WindowManager::getCurrentRotation())
        {
            case 0:
                pos = Pos2(_drawMapRowIndex * kTileSize, 0);
                delta = { 0, kTileSize };
                break;
            case 1:
                pos = Pos2(TileManager::getMapWidth() - kTileSize, _drawMapRowIndex * kTileSize);
                delta = { -kTileSize, 0 };
                break;
            case 2:
                pos = Pos2((TileManager::getMapColumns() - 1 - _drawMapRowIndex) * kTileSize, TileManager::getMapWidth() - kTileSize);
                delta = { 0, -kTileSize };
                break;
            case 3:
                pos = Pos2(0, (TileManager::getMapColumns() - 1 - _drawMapRowIndex) * kTileSize);
                delta = { kTileSize, 0 };
                break;
        }

        switch (self.currentTab)
        {
            case 0: setMapPixelsOverall(mapPtr, mapAltPtr, pos, delta); return;
            case 1: setMapPixelsVehicles(mapPtr, mapAltPtr, pos, delta); return;
            case 2: setMapPixelsIndustries(mapPtr, mapAltPtr, pos, delta); return;
            case 3: setMapPixelsRoutes(mapPtr, mapAltPtr, pos, delta); return;
            case 4: setMapPixelsOwnership(mapPtr, mapAltPtr, pos, delta); return;
        }
    }

    // 0x0046D34D based on
    static void setHoverItem(Window* self, int16_t y, int index)
    {
        uint32_t itemHover;

        if (y < 0)
        {
            itemHover = (1 << index);
        }
        else
        {
            itemHover = 0;
        }

        if (itemHover != self->var_854)
        {
            self->var_854 = itemHover;
            self->invalidate();
        }

        if (self->var_854 != 0)
        {
            self->invalidate();
        }
    }

    static uint8_t legendWidth = 100;
    static uint8_t legendItemHeight = 10;
    static constexpr uint8_t kOverallGraphKeySize = 6;

    static std::array<size_t, 5> legendLengths = {
        {
            kOverallGraphKeySize,
            std::size(_vehicleTypeCounts),
            ObjectManager::getMaxObjects(ObjectType::industry),
            0,
            Limits::kMaxCompanies,
        }
    };

    static void setHoverItemTab(Window* self, int16_t legendLeft, int16_t legendBottom)
    {
        if (Input::hasFlag(Input::Flags::rightMousePressed))
        {
            return;
        }

        auto cursorPos = Input::getMouseLocation2();
        auto window = WindowManager::findAt(cursorPos);

        if (window != self)
        {
            return;
        }

        cursorPos.x -= legendLeft;
        if (cursorPos.x < 0 || cursorPos.x > legendWidth)
        {
            setHoverItem(self, 0, 0);
            return;
        }

        cursorPos.y -= legendBottom;

        uint8_t i = 0;
        int16_t y = 0;

        if (self->currentTab == (widx::tabRoutes - widx::tabOverall))
        {
            y = cursorPos.y;

            for (; _routeToObjectIdMap[i] != 0xFF; i++)
            {
                y -= legendItemHeight;

                if (y < 0)
                {
                    break;
                }
            }
        }
        else if (cursorPos.y < static_cast<int16_t>(legendLengths[self->currentTab] * legendItemHeight))
        {
            y = cursorPos.y;

            for (; i < legendLengths[self->currentTab]; i++)
            {
                if (self->currentTab == (widx::tabIndustries - widx::tabOverall))
                {
                    auto industryObj = ObjectManager::get<IndustryObject>(i);

                    if (industryObj == nullptr)
                    {
                        continue;
                    }
                }
                else if (self->currentTab == (widx::tabOwnership - widx::tabOverall))
                {
                    auto company = CompanyManager::get(CompanyId(i));

                    if (company->empty())
                    {
                        continue;
                    }
                }

                y -= legendItemHeight;

                if (y < 0)
                {
                    break;
                }
            }
        }

        setHoverItem(self, y, i);
    }

    // 0x0046B69C
    static void clearMap()
    {
        std::fill(_mapPixels, _mapPixels + kRenderedMapSize * 2, PaletteIndex::black0);
    }

    // 0x00F2541D
    static uint16_t mapFrameNumber = 0;

    // 0x0046BA5B
    static void onUpdate(Window& self)
    {
        self.frameNo++;
        self.callPrepareDraw();

        WindowManager::invalidateWidget(WindowType::map, self.number, self.currentTab + widx::tabOverall);

        mapFrameNumber++;

        if (getCurrentRotation() != self.var_846)
        {
            self.var_846 = getCurrentRotation();
            clearMap();
        }

        auto i = 80;

        while (i > 0)
        {
            setMapPixels(self);
            i--;
        }

        self.invalidate();

        auto x = self.x + self.width - 104;
        auto y = self.y + 44;

        setHoverItemTab(&self, x, y);
    }

    // 0x0046B9E7
    static void getScrollSize([[maybe_unused]] Window& self, [[maybe_unused]] uint32_t scrollIndex, int32_t& scrollWidth, int32_t& scrollHeight)
    {
        scrollWidth = kRenderedMapWidth;
        scrollHeight = kRenderedMapHeight;
    }

    // 0x0046B9D4
    static void moveMainViewToMapView(Pos2 pos)
    {
        auto z = TileManager::getHeight(pos).landHeight;
        auto window = WindowManager::getMainWindow();

        if (window == nullptr)
        {
            return;
        }

        window->viewportCentreOnTile({ static_cast<coord_t>(pos.x), static_cast<coord_t>(pos.y), static_cast<coord_t>(z) });
    }

    // 0x0046B97C
    static void scrollMouseDown([[maybe_unused]] Window& self, int16_t x, int16_t y, [[maybe_unused]] uint8_t scrollIndex)
    {
        auto pos = mapWindowPosToLocation({ x, y });

        moveMainViewToMapView(pos);
    }

    // 0x0046B946
    static std::optional<FormatArguments> tooltip([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_map);
        return args;
    }

    // 0x0046B6BF
    static void prepareDraw(Window& self)
    {
        const StringId captionText[] = {
            StringIds::title_map,
            StringIds::title_map_vehicles,
            StringIds::title_map_industries,
            StringIds::title_map_routes,
            StringIds::title_map_companies,
        };

        self.widgets[widx::caption].text = captionText[self.currentTab];

        auto activatedWidgets = self.activatedWidgets;
        activatedWidgets &= ~((1ULL << widx::statusBar) | (1ULL << widx::scrollview) | (1ULL << widx::tabOwnership) | (1ULL << widx::tabRoutes) | (1ULL << widx::tabIndustries) | (1ULL << widx::tabVehicles) | (1ULL << widx::tabOverall));

        auto currentWidget = self.currentTab + widx::tabOverall;
        activatedWidgets |= (1ULL << currentWidget);
        self.activatedWidgets = activatedWidgets;

        self.widgets[widx::frame].right = self.width - 1;
        self.widgets[widx::frame].bottom = self.height - 1;
        self.widgets[widx::panel].right = self.width - 1;
        self.widgets[widx::panel].bottom = self.height - 1;

        self.widgets[widx::caption].right = self.width - 2;
        self.widgets[widx::closeButton].left = self.width - 15;
        self.widgets[widx::closeButton].right = self.width - 3;
        self.widgets[widx::scrollview].bottom = self.height - 14;
        self.widgets[widx::scrollview].right = self.width - 108;

        self.widgets[widx::statusBar].top = self.height - 12;
        self.widgets[widx::statusBar].bottom = self.height - 3;
        self.widgets[widx::statusBar].right = self.width - 14;

        auto disabledWidgets = 0;

        if (SceneManager::isEditorMode())
        {
            disabledWidgets |= (1 << widx::tabVehicles) | (1 << widx::tabRoutes) | (1 << widx::tabOwnership);
        }

        self.disabledWidgets = disabledWidgets;

        Widget::leftAlignTabs(self, widx::tabOverall, widx::tabOwnership);
    }

    // 0x0046D0E0
    static void drawTabs(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto skin = ObjectManager::get<InterfaceSkinObject>();

        // tabOverall
        {
            // TODO: use same list as top toolbar and time panel
            static constexpr uint32_t kMapSpritesByRotation[] = {
                InterfaceSkin::ImageIds::toolbar_menu_map_north,
                InterfaceSkin::ImageIds::toolbar_menu_map_west,
                InterfaceSkin::ImageIds::toolbar_menu_map_south,
                InterfaceSkin::ImageIds::toolbar_menu_map_east,
            };
            uint32_t mapSprite = skin->img + kMapSpritesByRotation[WindowManager::getCurrentRotation()];

            Widget::drawTab(self, drawingCtx, mapSprite, widx::tabOverall);
        }

        // tabVehicles,
        {
            if (!(self.disabledWidgets & (1 << widx::tabVehicles)))
            {
                static constexpr uint32_t vehicleImageIds[] = {
                    InterfaceSkin::ImageIds::vehicle_train_frame_0,
                    InterfaceSkin::ImageIds::vehicle_train_frame_1,
                    InterfaceSkin::ImageIds::vehicle_train_frame_2,
                    InterfaceSkin::ImageIds::vehicle_train_frame_3,
                    InterfaceSkin::ImageIds::vehicle_train_frame_4,
                    InterfaceSkin::ImageIds::vehicle_train_frame_5,
                    InterfaceSkin::ImageIds::vehicle_train_frame_6,
                    InterfaceSkin::ImageIds::vehicle_train_frame_7,
                };

                uint32_t imageId = skin->img;
                if (self.currentTab == widx::tabVehicles - widx::tabOverall)
                {
                    imageId += vehicleImageIds[(self.frameNo / 2) % std::size(vehicleImageIds)];
                }
                else
                {
                    imageId += vehicleImageIds[0];
                }

                auto colour = Colour::black;

                if (!SceneManager::isEditorMode() && !SceneManager::isSandboxMode())
                {
                    auto company = CompanyManager::getPlayerCompany();
                    colour = company->mainColours.primary;
                }

                imageId = Gfx::recolour(imageId, colour);

                Widget::drawTab(self, drawingCtx, imageId, widx::tabVehicles);
            }
        }

        // tabIndustries,
        {
            uint32_t imageId = skin->img;
            imageId += InterfaceSkin::ImageIds::toolbar_menu_industries;

            Widget::drawTab(self, drawingCtx, imageId, widx::tabIndustries);
        }

        // tabRoutes,
        {
            if (!(self.disabledWidgets & (1 << widx::tabRoutes)))
            {
                static constexpr uint32_t routeImageIds[] = {
                    InterfaceSkin::ImageIds::tab_routes_frame_0,
                    InterfaceSkin::ImageIds::tab_routes_frame_1,
                    InterfaceSkin::ImageIds::tab_routes_frame_2,
                    InterfaceSkin::ImageIds::tab_routes_frame_3,
                };

                uint32_t imageId = skin->img;
                if (self.currentTab == widx::tabRoutes - widx::tabOverall)
                {
                    imageId += routeImageIds[(self.frameNo / 16) % std::size(routeImageIds)];
                }
                else
                {
                    imageId += routeImageIds[0];
                }

                Widget::drawTab(self, drawingCtx, imageId, widx::tabRoutes);
            }
        }

        // tabOwnership,
        {
            if (!(self.disabledWidgets & (1 << widx::tabOwnership)))
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::tab_companies;

                Widget::drawTab(self, drawingCtx, imageId, widx::tabOwnership);
            }
        }
    }

    // 0x0046D273
    static void drawGraphKeyOverall(Window* self, Gfx::DrawingContext& drawingCtx, uint16_t x, uint16_t& y)
    {
        static constexpr PaletteIndex_t overallColours[] = {
            PaletteIndex::mutedDarkRed7,
            PaletteIndex::mutedPurple7,
            PaletteIndex::black2,
            PaletteIndex::black7,
            PaletteIndex::orange8,
            PaletteIndex::green6,
        };

        static constexpr StringId lineNames[] = {
            StringIds::map_key_towns,
            StringIds::map_key_industries,
            StringIds::map_key_roads,
            StringIds::map_key_railways,
            StringIds::map_key_stations,
            StringIds::map_key_vegetation,
        };

        auto tr = Gfx::TextRenderer(drawingCtx);

        for (auto i = 0; i < kOverallGraphKeySize; i++)
        {
            auto colour = overallColours[i];
            if (!(self->var_854 & (1 << i)) || !(mapFrameNumber & (1 << 2)))
            {
                drawingCtx.drawRect(x, y + 3, 5, 5, colour, Gfx::RectFlags::none);
            }

            FormatArguments args{};
            args.push(lineNames[i]);

            auto stringId = StringIds::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = StringIds::small_white_string;
            }

            auto point = Point(x + 6, y);
            tr.drawStringLeftClipped(point, 94, Colour::black, stringId, args);

            y += 10;
        }
    }

    // 0x004FDD62
    static constexpr PaletteIndex_t vehicleTypeColours[] = {
        PaletteIndex::red7,
        PaletteIndex::green9,
        PaletteIndex::purple8,
        PaletteIndex::orangeA,
        PaletteIndex::blackB,
        PaletteIndex::orange6, // changed from 136 to make ships more viewable on the map
    };

    // 0x0046D379
    static void drawGraphKeyVehicles(Window* self, Gfx::DrawingContext& drawingCtx, uint16_t x, uint16_t& y)
    {
        static constexpr StringId lineNames[] = {
            StringIds::forbid_trains,
            StringIds::forbid_buses,
            StringIds::forbid_trucks,
            StringIds::forbid_trams,
            StringIds::forbid_aircraft,
            StringIds::forbid_ships,
        };

        auto tr = Gfx::TextRenderer(drawingCtx);

        for (uint8_t i = 0; i < std::size(_vehicleTypeCounts); i++)
        {
            if (!(self->var_854 & (1 << i)) || !(mapFrameNumber & (1 << 2)))
            {
                auto colour = vehicleTypeColours[i];

                drawingCtx.drawRect(x, y + 3, 5, 5, colour, Gfx::RectFlags::none);
            }

            FormatArguments args{};
            args.push(lineNames[i]);

            auto stringId = StringIds::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = StringIds::small_white_string;
            }

            auto point = Point(x + 6, y);
            tr.drawStringLeftClipped(point, 94, Colour::black, stringId, args);

            y += 10;
        }
    }

    // 0x0046D47F
    static void drawGraphKeyIndustries(Window* self, Gfx::DrawingContext& drawingCtx, uint16_t x, uint16_t& y)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::industry); i++)
        {
            auto industry = ObjectManager::get<IndustryObject>(i);

            if (industry == nullptr)
            {
                continue;
            }

            if (!(self->var_854 & (1 << i)) || !(mapFrameNumber & (1 << 2)))
            {
                auto colour = kIndustryColours[_assignedIndustryColours[i]];

                drawingCtx.drawRect(x, y + 3, 5, 5, colour, Gfx::RectFlags::none);
            }

            FormatArguments args{};
            args.push(industry->name);

            auto stringId = StringIds::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = StringIds::small_white_string;
            }

            auto point = Point(x + 6, y);
            tr.drawStringLeftClipped(point, 94, Colour::black, stringId, args);

            y += 10;
        }
    }

    // 0x0046D5A4
    static void drawGraphKeyRoutes(Window* self, Gfx::DrawingContext& drawingCtx, uint16_t x, uint16_t& y)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        for (auto i = 0; _routeToObjectIdMap[i] != 0xFF; i++)
        {
            auto index = _routeToObjectIdMap[i];
            auto colour = _routeColours[i];

            if (!(self->var_854 & (1 << i)) || !(mapFrameNumber & (1 << 2)))
            {
                drawingCtx.drawRect(x, y + 3, 5, 5, colour, Gfx::RectFlags::none);
            }

            auto routeType = StringIds::map_routes_aircraft;

            if (index != 0xFE)
            {
                routeType = StringIds::map_routes_ships;

                if (index != 0xFD)
                {
                    if (index & (1 << 7))
                    {
                        auto roadObj = ObjectManager::get<RoadObject>(index & ~(1 << 7));
                        routeType = roadObj->name;
                    }
                    else
                    {
                        auto trackObj = ObjectManager::get<TrackObject>(index);
                        routeType = trackObj->name;
                    }
                }
            }

            FormatArguments args{};
            args.push(routeType);

            auto stringId = StringIds::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = StringIds::small_white_string;
            }

            auto point = Point(x + 6, y);
            tr.drawStringLeftClipped(point, 94, Colour::black, stringId, args);

            y += 10;
        }
    }

    // 0x0046D6E1
    static void drawGraphKeyCompanies(Window* self, Gfx::DrawingContext& drawingCtx, uint16_t x, uint16_t& y)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        for (const auto& company : CompanyManager::companies())
        {
            auto index = company.id();
            auto colour = Colours::getShade(company.mainColours.primary, 6);

            if (!(self->var_854 & (1 << enumValue(index))) || !(mapFrameNumber & (1 << 2)))
            {
                drawingCtx.drawRect(x, y + 3, 5, 5, colour, Gfx::RectFlags::none);
            }

            FormatArguments args{};
            args.push(company.name);

            auto stringId = StringIds::small_black_string;

            if (self->var_854 & (1 << enumValue(index)))
            {
                stringId = StringIds::small_white_string;
            }

            auto point = Point(x + 6, y);
            tr.drawStringLeftClipped(point, 94, Colour::black, stringId, args);

            y += 10;
        }
    }

    // 0x0046D81F
    static void formatVehicleString(Window* self, FormatArguments& args)
    {
        static constexpr StringId vehicleStringSingular[] = {
            StringIds::num_trains_singular,
            StringIds::num_buses_singular,
            StringIds::num_trucks_singular,
            StringIds::num_trams_singular,
            StringIds::num_aircrafts_singular,
            StringIds::num_ships_singular,
        };

        static constexpr StringId vehicleStringPlural[] = {
            StringIds::num_trains_plural,
            StringIds::num_buses_plural,
            StringIds::num_trucks_plural,
            StringIds::num_trams_plural,
            StringIds::num_aircrafts_plural,
            StringIds::num_ships_plural,
        };

        int16_t vehicleIndex = Numerics::bitScanForward(self->var_854);
        uint16_t totalVehicleCount = 0;
        auto stringId = StringIds::status_num_vehicles_plural;

        if (vehicleIndex == -1)
        {
            for (auto i = 0; i < 6; i++)
            {
                totalVehicleCount += _vehicleTypeCounts[i];
            }

            if (totalVehicleCount == 1)
            {
                stringId = StringIds::status_num_vehicles_singular;
            }
        }
        else
        {
            totalVehicleCount = _vehicleTypeCounts[vehicleIndex];
            stringId = vehicleStringPlural[vehicleIndex];

            if (totalVehicleCount == 1)
            {
                stringId = vehicleStringSingular[vehicleIndex];
            }
        }

        args.push(stringId);
        args.push(totalVehicleCount);
    }

    // 0x0046D87C
    static void formatIndustryString(Window* self, FormatArguments& args)
    {
        int16_t industryIndex = Numerics::bitScanForward(self->var_854);

        if (industryIndex == -1)
        {
            auto industries = IndustryManager::industries();
            auto industryCount = std::distance(std::begin(industries), std::end(industries));

            auto stringId = StringIds::status_num_industries_plural;

            if (industryCount == 1)
            {
                stringId = StringIds::status_num_industries_singular;
            }

            args.push(stringId);
            args.push(industryCount);
        }
        else
        {
            auto industryCount = 0;
            for (const auto& industry : IndustryManager::industries())
            {
                if (industry.objectId == industryIndex)
                {
                    industryCount++;
                }
            }

            auto industryObj = ObjectManager::get<IndustryObject>(industryIndex);
            auto stringId = industryObj->namePlural;

            if (industryCount == 1)
            {
                stringId = industryObj->nameSingular;
            }

            auto buffer = StringManager::getString(StringIds::buffer_1250);
            char* ptr = const_cast<char*>(buffer);

            auto argsBuf = FormatArgumentsBuffer{};
            auto argsTmp = FormatArguments{ argsBuf };
            argsTmp.push(industryCount);
            ptr = StringManager::formatString(ptr, stringId, argsTmp);

            *ptr++ = ' ';
            *ptr++ = '(';

            if (industryObj->requiresCargo())
            {
                ptr = StringManager::formatString(ptr, StringIds::industry_require);

                ptr = industryObj->getRequiredCargoString(ptr);

                if (industryObj->producesCargo())
                {
                    ptr = StringManager::formatString(ptr, StringIds::cargo_to_produce);

                    ptr = industryObj->getProducedCargoString(ptr);
                }
            }
            else if (industryObj->producesCargo())
            {
                ptr = StringManager::formatString(ptr, StringIds::industry_produce);

                ptr = industryObj->getProducedCargoString(ptr);
            }

            *ptr++ = ')';
            *ptr = '\0';

            args.push(StringIds::buffer_1250);
            args.push(industryCount);
        }
    }

    // 0x0046B779
    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        self.draw(drawingCtx);
        drawTabs(self, drawingCtx);

        {
            auto x = self.width - 104;
            uint16_t y = 44;

            switch (self.currentTab + widx::tabOverall)
            {
                case widx::tabOverall:
                    drawGraphKeyOverall(&self, drawingCtx, x, y);
                    break;

                case widx::tabVehicles:
                    drawGraphKeyVehicles(&self, drawingCtx, x, y);
                    break;

                case widx::tabIndustries:
                    drawGraphKeyIndustries(&self, drawingCtx, x, y);
                    break;

                case widx::tabRoutes:
                    drawGraphKeyRoutes(&self, drawingCtx, x, y);
                    break;

                case widx::tabOwnership:
                    drawGraphKeyCompanies(&self, drawingCtx, x, y);
                    break;
            }

            y += 14;
            y = std::max(y, kMinimumWindowHeight);

            self.minHeight = y;
        }

        FormatArguments args{};

        switch (self.currentTab + widx::tabOverall)
        {
            case widx::tabOverall:
            case widx::tabRoutes:
            case widx::tabOwnership:
                args.push(StringIds::empty);
                break;

            case widx::tabVehicles:
                formatVehicleString(&self, args);
                break;

            case widx::tabIndustries:
                formatIndustryString(&self, args);
                break;
        }

        auto& widget = self.widgets[widx::statusBar];
        auto point = Point(widget.left - 1, widget.top - 1);
        auto width = widget.width();

        tr.drawStringLeftClipped(point, width, Colour::black, StringIds::black_stringid, args);
    }

    // 0x0046BF0F based on
    static void drawVehicleOnMap(Gfx::DrawingContext& drawingCtx, Vehicles::VehicleBase* vehicle, uint8_t colour)
    {
        if (vehicle->position.x == Location::null)
        {
            return;
        }

        auto trainPos = locationToMapWindowPos(vehicle->position);

        drawingCtx.fillRect(trainPos.x, trainPos.y, trainPos.x, trainPos.y, colour, Gfx::RectFlags::none);
    }

    // 0x0046C294
    static std::pair<Point, Point> drawRouteLine(Gfx::DrawingContext& drawingCtx, Point startPos, Point endPos, Pos2 stationPos, uint8_t colour)
    {
        auto newStartPos = locationToMapWindowPos({ stationPos.x, stationPos.y });

        if (endPos.x != Location::null)
        {
            drawingCtx.drawLine(endPos, newStartPos, colour);
        }

        endPos = newStartPos;

        if (startPos.x == Location::null)
        {
            startPos = newStartPos;
        }

        return std::make_pair(startPos, endPos);
    }

    static std::optional<uint8_t> getRouteColour(Vehicles::Vehicle train)
    {
        uint8_t colour;
        if (train.head->vehicleType == VehicleType::aircraft)
        {
            colour = 211;
            auto index = Numerics::bitScanForward(_flashingItems);
            if (index != -1)
            {
                if (_routeToObjectIdMap[index] == 0xFE)
                {
                    if (mapFrameNumber & (1 << 2))
                    {
                        colour = kFlashColours[colour];
                    }
                }
            }
        }
        else if (train.head->vehicleType == VehicleType::ship)
        {
            colour = 139;
            auto index = Numerics::bitScanForward(_flashingItems);
            if (index != -1)
            {
                if (_routeToObjectIdMap[index] == 0xFD)
                {
                    if (mapFrameNumber & (1 << 2))
                    {
                        colour = kFlashColours[colour];
                    }
                }
            }
        }
        else
        {
            return std::nullopt;
        }

        return colour;
    }

    // 0x0046C18D
    static void drawRoutesOnMap(Gfx::DrawingContext& drawingCtx, Vehicles::Vehicle train)
    {
        auto colour = getRouteColour(train);

        if (!colour)
        {
            return;
        }

        Point startPos = { Location::null, 0 };
        Point endPos = { Location::null, 0 };
        for (auto& order : Vehicles::OrderRingView(train.head->orderTableOffset))
        {
            if (order.hasFlags(Vehicles::OrderFlags::HasStation))
            {
                auto* stationOrder = static_cast<Vehicles::OrderStation*>(&order);
                auto station = StationManager::get(stationOrder->getStation());
                Pos2 stationPos = { station->x, station->y };

                auto routePos = drawRouteLine(drawingCtx, startPos, endPos, stationPos, *colour);
                startPos = routePos.first;
                endPos = routePos.second;
            }
        }

        if (startPos.x == Location::null || endPos.x == Location::null)
        {
            return;
        }

        drawingCtx.drawLine(startPos, endPos, *colour);
    }

    // 0x0046C426
    static uint8_t getVehicleColour(WidgetIndex_t widgetIndex, Vehicles::Vehicle train, Vehicles::Car car)
    {
        auto colour = PaletteIndex::blackB;

        if (widgetIndex == widx::tabOwnership || widgetIndex == widx::tabVehicles)
        {
            auto companyId = car.front->owner;
            colour = Colours::getShade(CompanyManager::getCompanyColour(companyId), 7);

            if (widgetIndex == widx::tabVehicles)
            {
                auto index = enumValue(train.head->vehicleType);
                colour = vehicleTypeColours[index];
            }

            // clang-format off
            auto vehicleType = train.head->vehicleType;
            if ((widgetIndex == widx::tabOwnership && _flashingItems & (1 << enumValue(companyId))) ||
                (widgetIndex == widx::tabVehicles && _flashingItems & (1 << enumValue(vehicleType))))
            {
                if (mapFrameNumber & (1 << 2))
                {
                    colour = kFlashColours[colour];
                }
            }
            // clang-format on
        }

        return colour;
    }

    // 0x0046BFAD
    static void countVehiclesOnMap()
    {
        for (auto i = 0; i < 6; i++)
        {
            _vehicleTypeCounts[i] = 0;
        }

        for (auto* vehicle : VehicleManager::VehicleList())
        {
            Vehicles::Vehicle train(*vehicle);

            if (train.head->has38Flags(Vehicles::Flags38::isGhost))
            {
                continue;
            }

            if (train.head->position.x == Location::null)
            {
                continue;
            }

            auto vehicleType = train.head->vehicleType;
            _vehicleTypeCounts[static_cast<uint8_t>(vehicleType)] = _vehicleTypeCounts[static_cast<uint8_t>(vehicleType)] + 1;
        }
    }

    // 0x0046BE6E, 0x0046C35A
    static void drawVehiclesOnMap(Gfx::DrawingContext& drawingCtx, WidgetIndex_t widgetIndex)
    {
        for (auto* vehicle : VehicleManager::VehicleList())
        {
            Vehicles::Vehicle train(*vehicle);

            if (train.head->has38Flags(Vehicles::Flags38::isGhost))
            {
                continue;
            }

            if (train.head->position.x == Location::null)
            {
                continue;
            }

            for (auto& car : train.cars)
            {
                auto colour = getVehicleColour(widgetIndex, train, car);
                car.applyToComponents([&drawingCtx, colour](auto& component) {
                    drawVehicleOnMap(drawingCtx, &component, colour);
                });
            }

            if (widgetIndex == widx::tabRoutes)
            {
                drawRoutesOnMap(drawingCtx, train);
            }
        }
    }

    // 0x0046BE51, 0x0046BE34
    static void drawRectOnMap(Gfx::DrawingContext& drawingCtx, int16_t left, int16_t top, int16_t right, int16_t bottom, uint8_t colour, Gfx::RectFlags flags)
    {
        if (left > right)
        {
            std::swap(left, right);
        }

        if (top > bottom)
        {
            std::swap(top, bottom);
        }

        drawingCtx.fillRect(left, top, right, bottom, colour, flags);
    }

    // 0x0046BE51
    static void drawViewOnMap(Gfx::DrawingContext& drawingCtx, int16_t left, int16_t top, int16_t right, int16_t bottom)
    {
        left /= 32;
        top /= 16;
        right /= 32;
        bottom /= 16;
        left += kViewFrameOffsetsByRotation[getCurrentRotation()].x;
        top += kViewFrameOffsetsByRotation[getCurrentRotation()].y;
        right += kViewFrameOffsetsByRotation[getCurrentRotation()].x;
        bottom += kViewFrameOffsetsByRotation[getCurrentRotation()].y;

        const auto colour = PaletteIndex::black0;

        drawRectOnMap(drawingCtx, left, top, right, bottom, colour, Gfx::RectFlags::crossHatching);
    }

    // 0x0046BE34
    static void drawViewCornersOnMap(Gfx::DrawingContext& drawingCtx, int16_t left, int16_t top, int16_t leftOffset, int16_t topOffset, int16_t rightOffset, int16_t bottomOffset)
    {
        left /= 32;
        top /= 16;
        left += kViewFrameOffsetsByRotation[getCurrentRotation()].x;
        top += kViewFrameOffsetsByRotation[getCurrentRotation()].y;
        auto right = left;
        auto bottom = top;
        left += leftOffset;
        top += topOffset;
        right += rightOffset;
        bottom += bottomOffset;

        const auto colour = PaletteIndex::black0;

        drawRectOnMap(drawingCtx, left, top, right, bottom, colour, Gfx::RectFlags::none);
    }

    // 0x0046BAD5
    static void drawViewportPosition(Gfx::DrawingContext& drawingCtx)
    {
        auto window = WindowManager::getMainWindow();

        if (window == nullptr)
        {
            return;
        }

        auto viewport = window->viewports[0];

        if (viewport == nullptr)
        {
            return;
        }

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;
            auto right = viewport->viewX;
            auto bottom = viewport->viewY;
            right += viewport->viewWidth;

            drawViewOnMap(drawingCtx, left, top, right, bottom);
        }

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;
            top += viewport->viewHeight;
            auto right = viewport->viewX;
            auto bottom = viewport->viewY;
            right += viewport->viewWidth;
            bottom += viewport->viewHeight;

            drawViewOnMap(drawingCtx, left, top, right, bottom);
        }

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;
            auto right = viewport->viewX;
            auto bottom = viewport->viewY;
            bottom += viewport->viewHeight;

            drawViewOnMap(drawingCtx, left, top, right, bottom);
        }

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;
            left += viewport->viewWidth;
            auto right = viewport->viewX;
            auto bottom = viewport->viewY;
            right += viewport->viewWidth;
            bottom += viewport->viewHeight;

            drawViewOnMap(drawingCtx, left, top, right, bottom);
        }

        if (!(mapFrameNumber & (1 << 2)))
        {
            return;
        }

        if (_flashingItems != 0)
        {
            return;
        }

        uint8_t cornerSize = 5;

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;

            drawViewCornersOnMap(drawingCtx, left, top, 0, 0, cornerSize, 0);
        }

        {
            auto left = viewport->viewX;
            left += viewport->viewWidth;
            auto top = viewport->viewY;

            drawViewCornersOnMap(drawingCtx, left, top, -cornerSize, 0, 0, 0);
        }

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;

            drawViewCornersOnMap(drawingCtx, left, top, 0, 0, 0, cornerSize);
        }

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;
            top += viewport->viewHeight;

            drawViewCornersOnMap(drawingCtx, left, top, 0, -cornerSize, 0, 0);
        }

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;
            top += viewport->viewHeight;

            drawViewCornersOnMap(drawingCtx, left, top, 0, 0, cornerSize, 0);
        }

        {
            auto left = viewport->viewX;
            left += viewport->viewWidth;
            auto top = viewport->viewY;
            top += viewport->viewHeight;

            drawViewCornersOnMap(drawingCtx, left, top, -cornerSize, 0, 0, 0);
        }

        {
            auto left = viewport->viewX;
            left += viewport->viewWidth;
            auto top = viewport->viewY;

            drawViewCornersOnMap(drawingCtx, left, top, 0, 0, 0, cornerSize);
        }

        {
            auto left = viewport->viewX;
            left += viewport->viewWidth;
            auto top = viewport->viewY;
            top += viewport->viewHeight;

            drawViewCornersOnMap(drawingCtx, left, top, 0, -cornerSize, 0, 0);
        }
    }

    // 0x0046C481
    static void drawTownNames(Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        for (const auto& town : TownManager::towns())
        {
            auto townPos = locationToMapWindowPos({ town.x, town.y });

            char townNameBuffer[512]{};
            StringManager::formatString(townNameBuffer, town.name);
            tr.setCurrentFont(Gfx::Font::small);

            auto strWidth = tr.getStringWidth(townNameBuffer);

            strWidth /= 2;

            townPos.x -= strWidth;
            townPos.y -= 3;

            tr.setCurrentFont(Gfx::Font::small);
            tr.drawString(townPos, AdvancedColour(Colour::purple).outline(), townNameBuffer);
        }
    }

    // 0x0046B806
    static void drawScroll(Window& self, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const uint32_t scrollIndex)
    {
        if (!Game::hasFlags(GameStateFlags::tileManagerLoaded))
        {
            return;
        }

        drawingCtx.clearSingle(PaletteIndex::black0);

        auto* element = Gfx::getG1Element(0);
        auto backupElement = *element;

        auto* offset = _mapPixels;
        if (mapFrameNumber & (1 << 2))
        {
            offset = _mapAltPixels;
        }

        Gfx::getG1Element(0)->offset = offset;
        Gfx::getG1Element(0)->width = TileManager::getMapColumns() * 2;
        Gfx::getG1Element(0)->height = TileManager::getMapRows() * 2;
        Gfx::getG1Element(0)->xOffset = -8;
        Gfx::getG1Element(0)->yOffset = -8;
        Gfx::getG1Element(0)->flags = Gfx::G1ElementFlags::none;

        drawingCtx.drawImage(0, 0, 0);

        *element = backupElement;

        if (self.currentTab + widx::tabOverall == widx::tabVehicles)
        {
            countVehiclesOnMap();
        }

        drawVehiclesOnMap(drawingCtx, self.currentTab + widx::tabOverall);

        drawViewportPosition(drawingCtx);

        if (self.showTownNames)
        {
            drawTownNames(drawingCtx);
        }
    }

    static constexpr WindowEventList kEvents = {
        .onClose = onClose,
        .onMouseUp = onMouseUp,
        .onResize = onResize,
        .onUpdate = onUpdate,
        .getScrollSize = getScrollSize,
        .scrollMouseDown = scrollMouseDown,
        .scrollMouseDrag = scrollMouseDown,
        .tooltip = tooltip,
        .prepareDraw = prepareDraw,
        .draw = draw,
        .drawScroll = drawScroll,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }

    // 0x0046D0C3
    static uint32_t checkIndustryColours(PaletteIndex_t colour, uint32_t colourMask)
    {
        for (auto i = 0; i < 31; i++)
        {
            auto industryColour = kIndustryColours[i];
            auto diff = industryColour - colour;
            if (industryColour < colour)
            {
                diff = -diff;
            }

            if (diff <= 2)
            {
                colourMask &= ~(1U << i);
            }
        }

        return colourMask;
    }

    // 0x0046CFF0
    static void assignIndustryColours()
    {
        uint32_t availableColours = 0x7FFFFFFF;

        // First, assign water colour
        {
            auto waterObj = ObjectManager::get<WaterObject>();

            auto waterPixel = Gfx::getG1Element(waterObj->mapPixelImage)->offset[0];
            availableColours = checkIndustryColours(waterPixel, availableColours);

            waterPixel = Gfx::getG1Element(waterObj->mapPixelImage)->offset[1];
            availableColours = checkIndustryColours(waterPixel, availableColours);
        }

        // Then, assign surface texture colours
        for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::land); i++)
        {
            auto landObj = ObjectManager::get<LandObject>(i);
            if (landObj == nullptr)
            {
                continue;
            }

            auto landPixel = Gfx::getG1Element(landObj->mapPixelImage)->offset[0];
            availableColours = checkIndustryColours(landPixel, availableColours);

            landPixel = Gfx::getG1Element(landObj->mapPixelImage)->offset[1];
            availableColours = checkIndustryColours(landPixel, availableColours);
        }

        availableColours = checkIndustryColours(PaletteIndex::mutedDarkRed2, availableColours);
        availableColours = checkIndustryColours(PaletteIndex::black2, availableColours);
        availableColours = checkIndustryColours(PaletteIndex::black0, availableColours);

        // Reset assigned industry colours
        for (auto i = 0U; i < std::size(_assignedIndustryColours); i++)
        {
            _assignedIndustryColours[i] = 0xFF;
        }

        // Assign preferred industry colours, if possible
        for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::industry); i++)
        {
            auto industryObj = ObjectManager::get<IndustryObject>(i);
            if (industryObj == nullptr)
            {
                continue;
            }

            // Preferred colour still available?
            auto preferredColour = enumValue(industryObj->mapColour);
            if (availableColours & (1U << preferredColour))
            {
                _assignedIndustryColours[i] = preferredColour;
                availableColours &= ~(1U << preferredColour);
            }
        }

        // Assign alternative industry colours if needed
        for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::industry); i++)
        {
            auto industryObj = ObjectManager::get<IndustryObject>(i);
            if (industryObj == nullptr)
            {
                continue;
            }

            if (_assignedIndustryColours[i] != 0xFF)
            {
                continue;
            }

            auto freeColour = std::max(0, Numerics::bitScanForward(availableColours));
            availableColours &= ~(1U << freeColour);
            _assignedIndustryColours[i] = freeColour;
        }
    }

    // 0x0046CED0
    static void assignRouteColours()
    {
        uint32_t availableColours = 0x7FFFFFFF;

        // First, assign water colour
        {
            auto waterObj = ObjectManager::get<WaterObject>();

            auto waterPixel = Gfx::getG1Element(waterObj->mapPixelImage)->offset[0];
            availableColours = checkIndustryColours(waterPixel, availableColours);

            waterPixel = Gfx::getG1Element(waterObj->mapPixelImage)->offset[1];
            availableColours = checkIndustryColours(waterPixel, availableColours);
        }

        // Then, assign surface texture colours
        for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::land); i++)
        {
            auto landObj = ObjectManager::get<LandObject>(i);
            if (landObj == nullptr)
            {
                continue;
            }

            auto landPixel = Gfx::getG1Element(landObj->mapPixelImage)->offset[0];
            availableColours = checkIndustryColours(landPixel, availableColours);

            landPixel = Gfx::getG1Element(landObj->mapPixelImage)->offset[1];
            availableColours = checkIndustryColours(landPixel, availableColours);
        }

        availableColours = checkIndustryColours(PaletteIndex::mutedDarkRed2, availableColours);
        availableColours = checkIndustryColours(PaletteIndex::orange8, availableColours);
        availableColours = checkIndustryColours(PaletteIndex::pink9, availableColours);
        availableColours = checkIndustryColours(PaletteIndex::blue9, availableColours);
        availableColours = checkIndustryColours(PaletteIndex::black0, availableColours);
        availableColours = checkIndustryColours(PaletteIndex::blackB, availableColours);

        auto availableTracks = companyGetAvailableRailTracks(CompanyManager::getControllingId());
        auto availableRoads = companyGetAvailableRoads(CompanyManager::getControllingId());

        auto i = 0U;
        auto assignColour = [&i, &availableColours](uint8_t id) {
            _routeToObjectIdMap[i] = id;
            auto freeColour = std::max(0, Numerics::bitScanForward(availableColours));
            availableColours &= ~(1U << freeColour);

            auto colour = kIndustryColours[freeColour];
            _routeColours[i] = colour;

            if (id & (1U << 7))
            {
                _roadColours[id & ~(1U << 7)] = colour;
            }
            else
            {
                _trackColours[id] = colour;
            }
            i++;
        };

        for (auto& track : availableTracks)
        {
            assignColour(track);
        }
        for (auto& road : availableRoads)
        {
            assignColour(road);
        }

        // Airplanes
        _routeToObjectIdMap[i] = 0xFE;
        // Ships
        _routeToObjectIdMap[i + 1] = 0xFD;
        // End list
        _routeToObjectIdMap[i + 2] = 0xFF;

        // Airplanes
        _routeColours[i] = 0xD3;
        // Ships
        _routeColours[i + 1] = 0x8B;
    }

    static const WindowEventList& getEvents();

    // 0x0046B490
    void open()
    {
        auto window = WindowManager::bringToFront(WindowType::map, 0);
        if (window != nullptr)
        {
            return;
        }

        auto ptr = malloc(kRenderedMapSize * 2);
        if (ptr == nullptr)
        {
            return;
        }

        _mapPixels = static_cast<PaletteIndex_t*>(ptr);
        _mapAltPixels = &_mapPixels[kRenderedMapSize];

        Ui::Size32 size = { 350, 272 };

        if (Ui::getLastMapWindowAttributes().flags != WindowFlags::none)
        {
            size = { Ui::getLastMapWindowAttributes().size.width, Ui::getLastMapWindowAttributes().size.height };
            size.width = std::clamp<uint16_t>(size.width, 350, Ui::width());
            size.height = std::clamp<uint16_t>(size.height, 272, Ui::height() - 56);
        }

        window = WindowManager::createWindow(WindowType::map, size, WindowFlags::none, getEvents());
        window->setWidgets(kWidgets);
        window->initScrollWidgets();
        window->frameNo = 0;

        if (Ui::getLastMapWindowAttributes().flags != WindowFlags::none)
        {
            window->var_88A = Ui::getLastMapWindowAttributes().var88A;
            window->var_88C = Ui::getLastMapWindowAttributes().var88C;
            window->flags |= (Ui::getLastMapWindowAttributes().flags & WindowFlags::flag_16);
        }

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->windowTitlebarColour);
        window->setColour(WindowColour::secondary, skin->windowMapColour);

        window->var_846 = getCurrentRotation();

        clearMap();

        centerOnViewPoint();

        window->currentTab = 0;
        window->showTownNames = true;
        window->var_854 = 0;

        assignIndustryColours();
        assignRouteColours();

        mapFrameNumber = 0;
    }

    // 0x0046B5C0
    void centerOnViewPoint()
    {
        auto* mainWindow = WindowManager::getMainWindow();
        if (mainWindow == nullptr)
        {
            return;
        }

        auto* viewport = mainWindow->viewports[0];
        if (viewport == nullptr)
        {
            return;
        }

        auto* window = WindowManager::find(WindowType::map, 0);
        if (window == nullptr)
        {
            return;
        }

        // Ensure minimap/scroll widget has been resized
        window->callPrepareDraw();

        const int16_t vpCentreX = ((viewport->viewWidth / 2) + viewport->viewX) / 32;
        const int16_t vpCentreY = ((viewport->viewHeight / 2) + viewport->viewY) / 16;

        auto& widget = window->widgets[widx::scrollview];
        const int16_t miniMapWidth = widget.width() - ScrollView::kScrollbarSize;
        const int16_t miniMapHeight = widget.height() - ScrollView::kScrollbarSize;

        const int16_t visibleMapWidth = window->scrollAreas[0].contentWidth - miniMapWidth;
        const int16_t visibleMapHeight = window->scrollAreas[0].contentHeight - miniMapHeight;

        auto& offset = kViewFrameOffsetsByRotation[getCurrentRotation()];
        int16_t centreX = std::max(vpCentreX + offset.x - (miniMapWidth / 2), 0);
        int16_t centreY = std::max(vpCentreY + offset.y - (miniMapHeight / 2), 0);

        if (visibleMapWidth < centreX)
        {
            centreX = std::max(centreX + (visibleMapWidth - centreX), 0);
        }

        if (visibleMapHeight < centreY)
        {
            centreY = std::max(centreY + (visibleMapHeight - centreY), 0);
        }

        window->scrollAreas[0].contentOffsetX = centreX;
        window->scrollAreas[0].contentOffsetY = centreY;

        Ui::ScrollView::updateThumbs(*window, widx::scrollview);
    }
}
