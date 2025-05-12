#include "Audio/Audio.h"
#include "Construction.h"
#include "GameCommands/Airports/CreateAirport.h"
#include "GameCommands/Airports/RemoveAirport.h"
#include "GameCommands/Docks/CreatePort.h"
#include "GameCommands/Docks/RemovePort.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Road/CreateRoadStation.h"
#include "GameCommands/Road/RemoveRoadStation.h"
#include "GameCommands/Track/CreateTrainStation.h"
#include "GameCommands/Track/RemoveTrainStation.h"
#include "GameState.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/IndustryElement.h"
#include "Map/MapSelection.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TrackElement.h"
#include "Objects/AirportObject.h"
#include "Objects/CargoObject.h"
#include "Objects/DockObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/RoadStationObject.h"
#include "Objects/TrackObject.h"
#include "Objects/TrainStationObject.h"
#include "Ui/Dropdown.h"
#include "Ui/ToolManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/DropdownWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/Wt3Widget.h"
#include "World/CompanyManager.h"
#include "World/Industry.h"
#include "World/StationManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;
using namespace OpenLoco::World::TileManager;

namespace OpenLoco::Ui::Windows::Construction::Station
{
    static loco_global<uint8_t, 0x00508F09> _suppressErrorSound;
    static loco_global<World::Pos3, 0x00F24942> _constructionArrowPos;
    static loco_global<uint8_t, 0x00F24948> _constructionArrowDirection;
    static loco_global<uint32_t, 0x00112C734> _lastConstructedAdjoiningStationId;           // Can be 0xFFFF'FFFFU for no adjoining station
    static loco_global<World::Pos2, 0x00112C792> _lastConstructedAdjoiningStationCentrePos; // Can be x = -1 for no adjoining station

    // TODO: move to ConstructionState when no longer a loco_global
    static bool _isDragging = false;
    static World::TilePos2 _toolPosDrag;
    static World::TilePos2 _toolPosInitial;

    static constexpr auto widgets = makeWidgets(
        Common::makeCommonWidgets(138, 190, StringIds::stringid_2),
        Widgets::dropdownWidgets({ 3, 45 }, { 132, 12 }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_station_type),
        Widgets::Wt3Widget({ 35, 60 }, { 68, 68 }, WindowColour::secondary),
        Widgets::ImageButton({ 112, 104 }, { 24, 24 }, WindowColour::secondary, ImageIds::rotate_object, StringIds::rotate_90));

    std::span<const Widget> getWidgets()
    {
        return widgets;
    }

    WindowEventList events;

    // 0x0049E228
    static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case Common::widx::close_button:
                WindowManager::close(&self);
                break;

            case Common::widx::tab_construction:
            case Common::widx::tab_overhead:
            case Common::widx::tab_signal:
            case Common::widx::tab_station:
                Common::switchTab(self, widgetIndex);
                break;

            case widx::rotate:
                _cState->constructionRotation++;
                _cState->constructionRotation = _cState->constructionRotation & 3;
                _cState->stationCost = 0x80000000;
                self.invalidate();
                break;
        }
    }

    template<typename obj_type>
    void AddStationsToDropdown(const uint8_t stationCount)
    {
        for (auto stationIndex = 0; stationIndex < stationCount; stationIndex++)
        {
            auto station = _cState->stationList[stationIndex];
            if (station == _cState->lastSelectedStationType)
            {
                Dropdown::setHighlightedItem(stationIndex);
            }

            auto obj = ObjectManager::get<obj_type>(station);
            Dropdown::add(stationIndex, obj->name);
        }
    }

    // 0x0049E249
    static void onMouseDown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case widx::station_dropdown:
            {
                uint8_t stationCount = 0;
                while (_cState->stationList[stationCount] != 0xFF)
                {
                    stationCount++;
                }

                auto widget = self.widgets[widx::station];
                auto xPos = self.x + widget.left;
                auto yPos = self.y + widget.top;
                auto width = widget.width() + 2;
                auto height = widget.height();
                Dropdown::show(xPos, yPos, width, height, self.getColour(WindowColour::secondary), stationCount, (1 << 7));

                if (_cState->byte_1136063 & (1 << 7))
                {
                    AddStationsToDropdown<AirportObject>(stationCount);
                }
                else if (_cState->byte_1136063 & (1 << 6))
                {
                    AddStationsToDropdown<DockObject>(stationCount);
                }
                else if (_cState->trackType & (1 << 7))
                {
                    AddStationsToDropdown<RoadStationObject>(stationCount);
                }
                else
                {
                    AddStationsToDropdown<TrainStationObject>(stationCount);
                }
                break;
            }
            case widx::image:
            {
                ToolManager::toolCancel();
                ToolManager::toolSet(&self, widgetIndex, CursorId::placeStation);
                break;
            }
        }
    }

    // 0x0049E256
    static void onDropdown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t itemIndex)
    {
        if (widgetIndex == widx::station_dropdown)
        {
            if (itemIndex == -1)
            {
                return;
            }

            auto selectedStation = _cState->stationList[itemIndex];
            _cState->lastSelectedStationType = selectedStation;

            if (_cState->byte_1136063 & (1 << 7))
            {
                getGameState().lastAirport = selectedStation;
            }
            else if (_cState->byte_1136063 & (1 << 6))
            {
                getGameState().lastShipPort = selectedStation;
            }
            else if (_cState->trackType & (1 << 7))
            {
                auto trackType = _cState->trackType & ~(1 << 7);
                Scenario::getConstruction().roadStations[trackType] = selectedStation;
            }
            else
            {
                Scenario::getConstruction().trainStations[_cState->trackType] = selectedStation;
            }

            self.invalidate();
        }
    }

    // 0x0049E437
    static void onUpdate(Window& self)
    {
        Common::onUpdate(&self, GhostVisibilityFlags::station);
    }

    // 0x0049FF4B
    void removeStationGhost()
    {
        if ((_ghostVisibilityFlags & GhostVisibilityFlags::station) != GhostVisibilityFlags::none)
        {
            if (World::hasMapSelectionFlag(World::MapSelectionFlags::catchmentArea))
            {
                Windows::Station::sub_491BC6();
                World::resetMapSelectionFlag(World::MapSelectionFlags::catchmentArea);
            }
            if (_cState->stationGhostType & (1 << 15))
            {
                GameCommands::AirportRemovalArgs args;
                args.pos = _cState->stationGhostPos;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
            }
            else if (_cState->stationGhostType & (1 << 14))
            {
                GameCommands::PortRemovalArgs args;
                args.pos = _cState->stationGhostPos;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
            }
            else if (_cState->stationGhostType & (1 << 7))
            {
                GameCommands::RoadStationRemovalArgs args;
                args.pos = _cState->stationGhostPos;
                args.rotation = _cState->stationGhostRotation;
                args.roadId = _cState->stationGhostTrackId;
                args.index = _cState->stationGhostTileIndex;
                args.roadObjectId = _cState->stationGhostType & ~(1 << 7);
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
            }
            else
            {
                GameCommands::TrainStationRemovalArgs args;
                args.pos = _cState->stationGhostPos;
                args.rotation = _cState->stationGhostRotation;
                args.trackId = _cState->stationGhostTrackId;
                args.index = _cState->stationGhostTileIndex;
                args.type = _cState->stationGhostType;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
            }
            _ghostVisibilityFlags = _ghostVisibilityFlags & ~GhostVisibilityFlags::station;
        }
    }

    static void setMapSelectedTilesFromRange(const World::TilePosRangeView& range)
    {
        size_t i = 0;
        for (const auto& pos : range)
        {
            _mapSelectedTiles[i++] = World::toWorldSpace(pos);
        }

        _mapSelectedTiles[i].x = -1;
        mapInvalidateMapSelectionTiles();
    }

    static std::optional<GameCommands::AirportPlacementArgs> getAirportPlacementArgsFromCursor(const int16_t x, const int16_t y);
    static std::optional<GameCommands::PortPlacementArgs> getDockPlacementArgsFromCursor(const int16_t x, const int16_t y);
    static std::optional<GameCommands::RoadStationPlacementArgs> getRoadStationPlacementArgsFromCursor(const int16_t x, const int16_t y);
    static std::optional<GameCommands::TrainStationPlacementArgs> getTrainStationPlacementArgsFromCursor(const int16_t x, const int16_t y);

    static std::optional<GameCommands::AirportPlacementArgs> getAirportPlacementArgs(const World::Pos2 pos);
    static std::optional<GameCommands::PortPlacementArgs> getDockPlacementArgs(const World::Pos2 pos);
    static std::optional<GameCommands::RoadStationPlacementArgs> getRoadStationPlacementArgs(const World::Pos2 pos, const World::RoadElement* roadEl);
    static std::optional<GameCommands::TrainStationPlacementArgs> getTrainStationPlacementArgs(const World::Pos2 pos, const World::TrackElement* trackEl);

    static loco_global<World::Pos2, 0x001135F7C> _1135F7C;
    static loco_global<World::Pos2, 0x001135F80> _1135F90;

    // 0x004A4CF9
    static void onToolUpdateFail()
    {
        removeConstructionGhosts();
        if (_cState->stationCost != 0x80000000U)
        {
            _cState->stationCost = 0x80000000U;
            Ui::WindowManager::invalidate(Ui::WindowType::construction);
        }
    }

    // 0x004A4F3B
    static void onToolUpdateAirport(const Ui::Point& mousePos)
    {
        World::mapInvalidateMapSelectionTiles();
        World::resetMapSelectionFlag(World::MapSelectionFlags::enable | World::MapSelectionFlags::enableConstruct | World::MapSelectionFlags::enableConstructionArrow);
        const auto args = getAirportPlacementArgsFromCursor(mousePos.x, mousePos.y);
        if (!args.has_value())
        {
            onToolUpdateFail();
            return;
        }

        World::setMapSelectionFlags(World::MapSelectionFlags::enableConstruct | World::MapSelectionFlags::enableConstructionArrow);
        World::resetMapSelectionFlag(World::MapSelectionFlags::unk_03);
        _constructionArrowDirection = args->rotation;
        _constructionArrowPos = args->pos;

        setMapSelectedTilesFromRange(World::getClampedRange(*_1135F7C, *_1135F90));

        if ((_ghostVisibilityFlags & GhostVisibilityFlags::station) != GhostVisibilityFlags::none)
        {
            if (_cState->stationGhostPos == args->pos && _cState->stationGhostRotation == args->rotation && _cState->stationGhostTypeDockAirport == args->type)
            {
                return;
            }
            removeConstructionGhosts();
        }

        _cState->stationGhostPos = args->pos;
        _cState->stationGhostRotation = args->rotation;
        _cState->stationGhostTypeDockAirport = args->type;
        _cState->stationGhostType = (1U << 15);

        const auto cost = GameCommands::doCommand(*args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);

        _cState->stationCost = cost;

        Ui::WindowManager::invalidate(Ui::WindowType::construction);

        if (cost == GameCommands::FAILURE)
        {
            return;
        }

        _ghostVisibilityFlags = _ghostVisibilityFlags | GhostVisibilityFlags::station;
        World::setMapSelectionFlags(World::MapSelectionFlags::catchmentArea);
        _cState->constructingStationId = _lastConstructedAdjoiningStationId;

        auto* station = _lastConstructedAdjoiningStationId != 0xFFFFFFFFU ? StationManager::get(static_cast<StationId>(*_lastConstructedAdjoiningStationId)) : nullptr;
        setCatchmentDisplay(station, CatchmentFlags::flag_0);
        auto pos = *_lastConstructedAdjoiningStationCentrePos;
        if (pos.x == -1)
        {
            pos = args->pos;
        }

        sub_491C6F(_cState->stationGhostTypeDockAirport, pos, _cState->stationGhostRotation, CatchmentFlags::flag_0);
        Windows::Station::sub_491BC6();
        auto res = calcAcceptedCargoAirportGhost(station, _cState->stationGhostTypeDockAirport, pos, _cState->stationGhostRotation, 0xFFFFFFFFU);
        _cState->constructingStationAcceptedCargoTypes = res.accepted;
        _cState->constructingStationProducedCargoTypes = res.produced;
    }

    // 0x004A5158
    static void onToolUpdateDock(const Ui::Point& mousePos)
    {
        World::mapInvalidateMapSelectionTiles();
        World::resetMapSelectionFlag(World::MapSelectionFlags::enable | World::MapSelectionFlags::enableConstruct | World::MapSelectionFlags::enableConstructionArrow);
        const auto args = getDockPlacementArgsFromCursor(mousePos.x, mousePos.y);
        if (!args.has_value())
        {
            onToolUpdateFail();
            return;
        }

        World::setMapSelectionFlags(World::MapSelectionFlags::enableConstruct | World::MapSelectionFlags::enableConstructionArrow);
        World::resetMapSelectionFlag(World::MapSelectionFlags::unk_03);
        _constructionArrowDirection = args->rotation;
        _constructionArrowPos = args->pos;

        setMapSelectedTilesFromRange(World::getClampedRange(*_1135F7C, *_1135F90));

        if ((_ghostVisibilityFlags & GhostVisibilityFlags::station) != GhostVisibilityFlags::none)
        {
            if (_cState->stationGhostPos == args->pos && _cState->stationGhostRotation == args->rotation && _cState->stationGhostTypeDockAirport == args->type)
            {
                return;
            }
            removeConstructionGhosts();
        }

        _cState->stationGhostPos = args->pos;
        _cState->stationGhostRotation = args->rotation;
        _cState->stationGhostTypeDockAirport = args->type;
        _cState->stationGhostType = (1U << 14);

        const auto cost = GameCommands::doCommand(*args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);

        _cState->stationCost = cost;

        Ui::WindowManager::invalidate(Ui::WindowType::construction);

        if (cost == GameCommands::FAILURE)
        {
            return;
        }

        _ghostVisibilityFlags = _ghostVisibilityFlags | GhostVisibilityFlags::station;
        World::setMapSelectionFlags(World::MapSelectionFlags::catchmentArea);
        _cState->constructingStationId = _lastConstructedAdjoiningStationId;

        auto* station = _lastConstructedAdjoiningStationId != 0xFFFFFFFFU ? StationManager::get(static_cast<StationId>(*_lastConstructedAdjoiningStationId)) : nullptr;
        setCatchmentDisplay(station, CatchmentFlags::flag_0);
        auto pos = *_lastConstructedAdjoiningStationCentrePos;
        if (pos.x == -1)
        {
            pos = args->pos;
        }

        sub_491D20(pos, CatchmentFlags::flag_0);
        Windows::Station::sub_491BC6();
        auto res = calcAcceptedCargoDockGhost(station, pos, 0xFFFFFFFFU);
        _cState->constructingStationAcceptedCargoTypes = res.accepted;
        _cState->constructingStationProducedCargoTypes = res.produced;
    }

    // 0x004A4D21
    static void onToolUpdateRoadStation(const Ui::Point& mousePos)
    {
        const auto args = getRoadStationPlacementArgsFromCursor(mousePos.x, mousePos.y);
        if (!args.has_value())
        {
            onToolUpdateFail();
            return;
        }

        if ((_ghostVisibilityFlags & GhostVisibilityFlags::station) != GhostVisibilityFlags::none)
        {
            if (_cState->stationGhostPos == args->pos
                && _cState->stationGhostRotation == args->rotation
                && _cState->stationGhostTrackId == args->roadId
                && _cState->stationGhostTileIndex == args->index
                && _cState->stationGhostType == (args->roadObjectId | (1 << 7)))
            {
                return;
            }
            removeConstructionGhosts();
        }

        _cState->stationGhostPos = args->pos;
        _cState->stationGhostRotation = args->rotation;
        _cState->stationGhostTrackId = args->roadId;
        _cState->stationGhostTileIndex = args->index;
        _cState->stationGhostType = args->roadObjectId | (1 << 7);

        const auto cost = GameCommands::doCommand(*args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);

        _cState->stationCost = cost;

        Ui::WindowManager::invalidate(Ui::WindowType::construction);

        if (cost == GameCommands::FAILURE)
        {
            return;
        }

        _ghostVisibilityFlags = _ghostVisibilityFlags | GhostVisibilityFlags::station;
        World::setMapSelectionFlags(World::MapSelectionFlags::catchmentArea);
        _cState->constructingStationId = _lastConstructedAdjoiningStationId;

        auto* station = _lastConstructedAdjoiningStationId != 0xFFFFFFFFU ? StationManager::get(static_cast<StationId>(*_lastConstructedAdjoiningStationId)) : nullptr;
        setCatchmentDisplay(station, CatchmentFlags::flag_0);
        auto pos = *_lastConstructedAdjoiningStationCentrePos;
        if (pos.x == -1)
        {
            pos = args->pos;
        }

        sub_491BF5(pos, CatchmentFlags::flag_0);
        Windows::Station::sub_491BC6();

        const auto* roadStationObj = ObjectManager::get<RoadStationObject>(args->type);
        uint32_t filter = 0xFFFFFFFFU;
        if (roadStationObj->hasFlags(RoadStationFlags::passenger))
        {
            filter = 1U << roadStationObj->cargoType;
        }
        if (roadStationObj->hasFlags(RoadStationFlags::freight))
        {
            filter = 0xFFFFFFFFU;
            filter &= ~(1U << roadStationObj->cargoType);
        }

        auto res = calcAcceptedCargoTrainStationGhost(station, pos, filter);
        _cState->constructingStationAcceptedCargoTypes = res.accepted;
        _cState->constructingStationProducedCargoTypes = res.produced;
    }

    // 0x004A4B2E
    static void onToolUpdateTrainStation(const Ui::Point& mousePos)
    {
        const auto args = getTrainStationPlacementArgsFromCursor(mousePos.x, mousePos.y);
        if (!args.has_value())
        {
            onToolUpdateFail();
            return;
        }

        if ((_ghostVisibilityFlags & GhostVisibilityFlags::station) != GhostVisibilityFlags::none)
        {
            if (_cState->stationGhostPos == args->pos
                && _cState->stationGhostRotation == args->rotation
                && _cState->stationGhostTrackId == args->trackId
                && _cState->stationGhostTileIndex == args->index
                && _cState->stationGhostType == args->trackObjectId)
            {
                return;
            }
            removeConstructionGhosts();
        }

        _cState->stationGhostPos = args->pos;
        _cState->stationGhostRotation = args->rotation;
        _cState->stationGhostTrackId = args->trackId;
        _cState->stationGhostTileIndex = args->index;
        _cState->stationGhostType = args->trackObjectId;

        const auto cost = GameCommands::doCommand(*args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);

        _cState->stationCost = cost;

        Ui::WindowManager::invalidate(Ui::WindowType::construction);

        if (cost == GameCommands::FAILURE)
        {
            return;
        }

        _ghostVisibilityFlags = _ghostVisibilityFlags | GhostVisibilityFlags::station;
        World::setMapSelectionFlags(World::MapSelectionFlags::catchmentArea);
        _cState->constructingStationId = _lastConstructedAdjoiningStationId;

        auto* station = _lastConstructedAdjoiningStationId != 0xFFFFFFFFU ? StationManager::get(static_cast<StationId>(*_lastConstructedAdjoiningStationId)) : nullptr;
        setCatchmentDisplay(station, CatchmentFlags::flag_0);
        auto pos = *_lastConstructedAdjoiningStationCentrePos;
        if (pos.x == -1)
        {
            pos = args->pos;
        }

        sub_491BF5(pos, CatchmentFlags::flag_0);
        Windows::Station::sub_491BC6();

        auto res = calcAcceptedCargoTrainStationGhost(station, pos, 0xFFFFFFFFU);
        _cState->constructingStationAcceptedCargoTypes = res.accepted;
        _cState->constructingStationProducedCargoTypes = res.produced;
    }

    // 0x0049E421
    static void onToolUpdate(Window&, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::image)
        {
            return;
        }

        if (_isDragging)
        {
            mapInvalidateMapSelectionTiles();
            removeConstructionGhosts();
            return;
        }

        if (_cState->byte_1136063 & (1 << 7))
        {
            onToolUpdateAirport({ x, y });
        }
        else if (_cState->byte_1136063 & (1 << 6))
        {
            onToolUpdateDock({ x, y });
        }
        else if (_cState->trackType & (1 << 7))
        {
            onToolUpdateRoadStation({ x, y });
        }
        else
        {
            onToolUpdateTrainStation({ x, y });
        }
    }

    static void onToolDown([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
    {
        auto res = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::surface | ViewportInteraction::InteractionItemFlags::water));
        auto& interaction = res.first;
        if (interaction.type == ViewportInteraction::InteractionItem::noInteraction)
        {
            return;
        }

        _toolPosInitial = World::toTileSpace(interaction.pos);
        _isDragging = false;
    }

    static void onToolDrag([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y)
    {
        mapInvalidateSelectionRect();
        removeConstructionGhosts();

        auto res = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::surface | ViewportInteraction::InteractionItemFlags::water));
        auto& interaction = res.first;
        if (interaction.type == ViewportInteraction::InteractionItem::noInteraction)
        {
            return;
        }

        _toolPosDrag = World::toTileSpace(interaction.pos);
        _isDragging = _toolPosInitial != _toolPosDrag;
        if (!_isDragging)
        {
            return;
        }

        setMapSelectionFlags(MapSelectionFlags::enable);
        setMapSelectionCorner(MapSelectionType::full);

        setMapSelectionArea(toWorldSpace(_toolPosInitial), toWorldSpace(_toolPosDrag));
        mapInvalidateSelectionRect();
    }

    // 0x004A47D9
    static std::optional<GameCommands::AirportPlacementArgs> getAirportPlacementArgsFromCursor(const int16_t x, const int16_t y)
    {
        auto pos = ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y });
        if (!pos)
        {
            return std::nullopt;
        }

        return getAirportPlacementArgs(pos.value());
    }

    static std::optional<GameCommands::AirportPlacementArgs> getAirportPlacementArgs(const World::Pos2 pos)
    {
        GameCommands::AirportPlacementArgs placementArgs;
        placementArgs.type = _cState->lastSelectedStationType;
        placementArgs.rotation = _cState->constructionRotation;

        const auto airportObj = ObjectManager::get<AirportObject>(placementArgs.type);
        const auto [minPos, maxPos] = airportObj->getAirportExtents(World::toTileSpace(pos), placementArgs.rotation);

        _1135F7C = World::toWorldSpace(minPos);
        _1135F90 = World::toWorldSpace(maxPos);
        auto maxBaseZ = 0;
        for (auto checkPos = minPos; checkPos.y <= maxPos.y; ++checkPos.y)
        {
            for (checkPos.x = minPos.x; checkPos.x <= maxPos.x; ++checkPos.x)
            {
                if (!validCoords(checkPos))
                {
                    continue;
                }
                const auto tile = TileManager::get(checkPos);
                const auto* surface = tile.surface();
                if (surface == nullptr)
                {
                    return std::nullopt;
                }

                const auto baseZ = surface->water() ? surface->water() * World::kMicroToSmallZStep : surface->baseZ();
                maxBaseZ = std::max(maxBaseZ, baseZ);
            }
        }
        placementArgs.pos = World::Pos3(pos.x, pos.y, maxBaseZ * World::kSmallZStep);
        return { placementArgs };
    }

    // 0x004A5550
    static void onToolUpAirport(const int16_t x, const int16_t y)
    {
        removeConstructionGhosts();

        const auto args = getAirportPlacementArgsFromCursor(x, y);
        if (!args)
        {
            return;
        }

        const auto* airportObject = ObjectManager::get<AirportObject>(_cState->lastSelectedStationType);
        auto formatArgs = FormatArguments::common();
        formatArgs.skip(3 * sizeof(StringId));
        formatArgs.push(airportObject->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);
        GameCommands::doCommand(*args, GameCommands::Flags::apply);
    }

    // 0x004A4903
    static std::optional<GameCommands::PortPlacementArgs> getDockPlacementArgsFromCursor(const int16_t x, const int16_t y)
    {
        auto pos = ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y });
        if (!pos)
        {
            return std::nullopt;
        }

        return getDockPlacementArgs(pos.value());
    }

    static std::optional<GameCommands::PortPlacementArgs> getDockPlacementArgs(const World::Pos2 pos)
    {
        // count of water on each side of the placement
        // 0x0113608B
        std::array<uint8_t, 4> _nearbyWaterCount = { 0 };

        uint8_t directionOfIndustry = 0xFF;
        uint8_t waterHeight = 0;
        _1135F7C = pos;
        _1135F90 = World::toWorldSpace(World::toTileSpace(pos) + TilePos2(1, 1));

        constexpr std::array<std::array<TilePos2, 2>, 4> searchArea = {
            std::array<TilePos2, 2>{ TilePos2{ -1, 0 }, TilePos2{ -1, 1 } },
            std::array<TilePos2, 2>{ TilePos2{ 0, 2 }, TilePos2{ 1, 2 } },
            std::array<TilePos2, 2>{ TilePos2{ 2, 0 }, TilePos2{ 2, 1 } },
            std::array<TilePos2, 2>{ TilePos2{ 0, -1 }, TilePos2{ 1, -1 } },
        };
        for (auto side = 0; side < 4; ++side)
        {
            for (const auto& offset : searchArea[side])
            {
                const auto searchPos = offset + World::toTileSpace(pos);
                if (!validCoords(searchPos))
                {
                    continue;
                }
                const auto tile = TileManager::get(searchPos);
                bool surfaceFound = false;
                for (const auto& el : tile)
                {
                    if (surfaceFound)
                    {
                        const auto* elIndustry = el.as<IndustryElement>();
                        if (elIndustry == nullptr)
                        {
                            continue;
                        }
                        if (elIndustry->isGhost())
                        {
                            continue;
                        }

                        auto* industry = elIndustry->industry();
                        const auto* industryObj = industry->getObject();
                        if (!industryObj->hasFlags(IndustryObjectFlags::builtOnWater))
                        {
                            continue;
                        }

                        directionOfIndustry = side;
                    }
                    const auto* surface = el.as<SurfaceElement>();
                    if (surface == nullptr)
                    {
                        continue;
                    }
                    else
                    {
                        surfaceFound = true;

                        if (!surface->water())
                        {
                            continue;
                        }
                        waterHeight = surface->water() * World::kMicroToSmallZStep;
                        if (waterHeight - 4 == surface->baseZ() && surface->isSlopeDoubleHeight())
                        {
                            continue;
                        }

                        _nearbyWaterCount[(side + 2) & 0x3]++;
                    }
                }
            }
        }

        if (waterHeight == 0)
        {
            return std::nullopt;
        }

        GameCommands::PortPlacementArgs placementArgs;
        placementArgs.type = _cState->lastSelectedStationType;
        placementArgs.pos = World::Pos3(pos.x, pos.y, waterHeight * World::kSmallZStep);
        if (directionOfIndustry != 0xFF)
        {
            placementArgs.rotation = directionOfIndustry;
        }
        else
        {
            auto res = std::find_if(std::begin(_nearbyWaterCount), std::end(_nearbyWaterCount), [](uint8_t value) { return value >= 2; });
            if (res != std::end(_nearbyWaterCount))
            {
                placementArgs.rotation = std::distance(std::begin(_nearbyWaterCount), res);
            }
            else
            {
                res = std::find_if(std::begin(_nearbyWaterCount), std::end(_nearbyWaterCount), [](uint8_t value) { return value >= 1; });
                if (res != std::end(_nearbyWaterCount))
                {
                    placementArgs.rotation = std::distance(std::begin(_nearbyWaterCount), res);
                }
                else
                {
                    static loco_global<uint8_t, 0x00113608A> _113608A; // ai rotation??
                    placementArgs.rotation = _113608A;
                }
            }
        }
        return { placementArgs };
    }

    // 0x004A55AB
    static void onToolUpDock(const int16_t x, const int16_t y)
    {
        removeConstructionGhosts();

        const auto args = getDockPlacementArgsFromCursor(x, y);
        if (!args)
        {
            return;
        }

        const auto* dockObject = ObjectManager::get<DockObject>(_cState->lastSelectedStationType);
        auto formatArgs = FormatArguments::common();
        formatArgs.skip(3 * sizeof(StringId));
        formatArgs.push(dockObject->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);
        GameCommands::doCommand(*args, GameCommands::Flags::apply);
    }

    static std::optional<GameCommands::RoadStationPlacementArgs> getRoadStationPlacementArgsFromCursor(const int16_t x, const int16_t y)
    {
        const auto res = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::roadAndTram | ViewportInteraction::InteractionItemFlags::station));
        const auto& interaction = res.first;
        if (interaction.type != ViewportInteraction::InteractionItem::road && interaction.type != ViewportInteraction::InteractionItem::roadStation)
        {
            return std::nullopt;
        }

        auto* elRoad = reinterpret_cast<const TileElement*>(interaction.object)->as<RoadElement>();
        auto* elStation = reinterpret_cast<const TileElement*>(interaction.object)->as<StationElement>();
        if (elRoad == nullptr && elStation == nullptr)
        {
            return std::nullopt;
        }

        // Deliberately always passing road and not station element, even if nullptr
        return getRoadStationPlacementArgs(interaction.pos, elRoad);
    }

    static std::optional<GameCommands::RoadStationPlacementArgs> getRoadStationPlacementArgs(const World::Pos2 pos, const RoadElement* elRoad)
    {
        if (elRoad == nullptr)
        {
            auto tile = World::TileManager::get(pos);
            for (auto& el : tile)
            {
                auto* elRoadCandidate = el.as<World::RoadElement>();
                if (elRoadCandidate != nullptr)
                {
                    elRoad = elRoadCandidate;
                    break;
                }
            }
        }

        if (elRoad == nullptr)
        {
            return std::nullopt;
        }

        GameCommands::RoadStationPlacementArgs placementArgs;
        placementArgs.pos = World::Pos3(pos.x, pos.y, elRoad->baseHeight());
        placementArgs.rotation = elRoad->rotation();
        placementArgs.roadId = elRoad->roadId();
        placementArgs.index = elRoad->sequenceIndex();
        placementArgs.roadObjectId = elRoad->roadObjectId();
        placementArgs.type = _cState->lastSelectedStationType;
        return { placementArgs };
    }

    // 0x004A548F
    static void onToolUpRoadStation(const int16_t x, const int16_t y)
    {
        removeConstructionGhosts();

        const auto args = getRoadStationPlacementArgsFromCursor(x, y);
        if (!args)
        {
            return;
        }

        const auto* roadStationObject = ObjectManager::get<RoadStationObject>(_cState->lastSelectedStationType);
        auto formatArgs = FormatArguments::common();
        formatArgs.skip(3 * sizeof(StringId));
        formatArgs.push(roadStationObject->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);
        if (GameCommands::doCommand(*args, GameCommands::Flags::apply) != GameCommands::FAILURE)
        {
            Audio::playSound(Audio::SoundId::construct, GameCommands::getPosition());
        }
    }

    static std::optional<GameCommands::TrainStationPlacementArgs> getTrainStationPlacementArgsFromCursor(const int16_t x, const int16_t y)
    {
        const auto res = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::track | ViewportInteraction::InteractionItemFlags::station));
        const auto& interaction = res.first;
        if (interaction.type != ViewportInteraction::InteractionItem::track && interaction.type != ViewportInteraction::InteractionItem::trainStation)
        {
            return std::nullopt;
        }

        auto* elTrack = reinterpret_cast<const TileElement*>(interaction.object)->as<TrackElement>();
        auto* elStation = reinterpret_cast<const TileElement*>(interaction.object)->as<StationElement>();
        if (elTrack == nullptr && elStation == nullptr)
        {
            return std::nullopt;
        }

        // Deliberately always passing track and not station element, even if nullptr
        return getTrainStationPlacementArgs(interaction.pos, elTrack);
    }

    static std::optional<GameCommands::TrainStationPlacementArgs> getTrainStationPlacementArgs(const World::Pos2 pos, const TrackElement* elTrack)
    {
        if (elTrack == nullptr)
        {
            auto tile = World::TileManager::get(pos);
            for (auto& el : tile)
            {
                auto* elTrackCandidate = el.as<World::TrackElement>();
                if (elTrackCandidate != nullptr)
                {
                    elTrack = elTrackCandidate;
                    break;
                }
            }
        }

        if (elTrack == nullptr)
        {
            return std::nullopt;
        }

        GameCommands::TrainStationPlacementArgs placementArgs;
        placementArgs.pos = World::Pos3(pos.x, pos.y, elTrack->baseHeight());
        placementArgs.rotation = elTrack->rotation();
        placementArgs.trackId = elTrack->trackId();
        placementArgs.index = elTrack->sequenceIndex();
        placementArgs.trackObjectId = elTrack->trackObjectId();
        placementArgs.type = _cState->lastSelectedStationType;
        return { placementArgs };
    }

    // 0x004A5390
    static void onToolUpTrainStation(const int16_t x, const int16_t y)
    {
        removeConstructionGhosts();

        const auto args = getTrainStationPlacementArgsFromCursor(x, y);
        if (!args)
        {
            return;
        }

        const auto* trainStationObject = ObjectManager::get<TrainStationObject>(_cState->lastSelectedStationType);
        auto formatArgs = FormatArguments::common();
        formatArgs.skip(3 * sizeof(StringId));
        formatArgs.push(trainStationObject->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);

        if (args->trackObjectId != _cState->trackType)
        {
            Error::open(StringIds::null, StringIds::wrong_type_of_track_road);
            return;
        }
        if (GameCommands::doCommand(*args, GameCommands::Flags::apply) != GameCommands::FAILURE)
        {
            Audio::playSound(Audio::SoundId::construct, GameCommands::getPosition());
        }
    }

    static void onToolUpSingle(const int16_t x, const int16_t y)
    {
        if (_cState->byte_1136063 & (1 << 7))
        {
            onToolUpAirport(x, y);
        }
        else if (_cState->byte_1136063 & (1 << 6))
        {
            onToolUpDock(x, y);
        }
        else if (_cState->trackType & (1 << 7))
        {
            onToolUpRoadStation(x, y);
        }
        else
        {
            onToolUpTrainStation(x, y);
        }
    }

    static uint32_t constructPieceAtPosition(World::Pos2 pos)
    {
        if (_cState->byte_1136063 & (1 << 7))
        {
            if (auto args = getAirportPlacementArgs(pos))
            {
                return GameCommands::doCommand(*args, GameCommands::Flags::apply);
            }
        }
        else if (_cState->byte_1136063 & (1 << 6))
        {
            if (auto args = getDockPlacementArgs(pos))
            {
                return GameCommands::doCommand(*args, GameCommands::Flags::apply);
            }
        }
        else if (_cState->trackType & (1 << 7))
        {
            if (auto args = getRoadStationPlacementArgs(pos, nullptr))
            {
                return GameCommands::doCommand(*args, GameCommands::Flags::apply);
            }
        }
        else
        {
            if (auto args = getTrainStationPlacementArgs(pos, nullptr))
            {
                return GameCommands::doCommand(*args, GameCommands::Flags::apply);
            }
        }
        return GameCommands::FAILURE;
    }

    static void onToolUpMultiple()
    {
        mapInvalidateSelectionRect();
        removeConstructionGhosts();
        World::resetMapSelectionFlags();

        auto dirX = _toolPosDrag.x - _toolPosInitial.x > 0 ? 1 : -1;
        auto dirY = _toolPosDrag.y - _toolPosInitial.y > 0 ? 1 : -1;

        bool builtAnything = false;

        for (auto yPos = _toolPosInitial.y; yPos != _toolPosDrag.y + dirY; yPos += dirY)
        {
            for (auto xPos = _toolPosInitial.x; xPos != _toolPosDrag.x + dirX; xPos += dirX)
            {
                auto pos = World::toWorldSpace({ xPos, yPos });
                _cState->x = pos.x;
                _cState->y = pos.y;

                auto height = TileManager::getHeight(pos);
                _cState->constructionZ = height.landHeight;

                // Try placing the station at this location, ignoring errors if they occur
                _suppressErrorSound = true;
                builtAnything |= constructPieceAtPosition(pos) != GameCommands::FAILURE;
                _suppressErrorSound = false;
            }
        }

        if (builtAnything)
        {
            WindowManager::close(WindowType::error);
        }

        // Leave the tool active, but make ghost piece visible for the next round
        _isDragging = false;
    }

    // 0x0049E42C
    static void onToolUp([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::image)
        {
            return;
        }

        if (_isDragging)
        {
            onToolUpMultiple();
        }
        else
        {
            onToolUpSingle(x, y);
        }
    }

    static void onToolAbort([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        if (widgetIndex != widx::image)
        {
            return;
        }

        _isDragging = false;
    }

    // 0x0049DD39
    static void prepareDraw(Window& self)
    {
        Common::prepareDraw(&self);

        self.widgets[widx::rotate].hidden = true;

        auto captionArgs = FormatArguments(self.widgets[Common::widx::caption].textArgs);

        if (_cState->byte_1136063 & (1 << 7))
        {
            self.widgets[widx::rotate].hidden = false;

            auto airportObj = ObjectManager::get<AirportObject>(_cState->lastSelectedStationType);

            self.widgets[widx::station].text = airportObj->name;

            captionArgs.push(StringIds::title_airport);
        }
        else if (_cState->byte_1136063 & (1 << 6))
        {
            auto dockObj = ObjectManager::get<DockObject>(_cState->lastSelectedStationType);

            self.widgets[widx::station].text = dockObj->name;

            captionArgs.push(StringIds::title_ship_port);
        }
        else if (_cState->trackType & (1 << 7))
        {
            auto trackType = _cState->trackType & ~(1 << 7);

            auto roadObj = ObjectManager::get<RoadObject>(trackType);

            captionArgs.push(roadObj->name);

            auto roadStationObject = ObjectManager::get<RoadStationObject>(_cState->lastSelectedStationType);

            self.widgets[widx::station].text = roadStationObject->name;
        }
        else
        {
            auto trackObj = ObjectManager::get<TrackObject>(_cState->trackType);

            captionArgs.push(trackObj->name);

            auto trainStationObject = ObjectManager::get<TrainStationObject>(_cState->lastSelectedStationType);

            self.widgets[widx::station].text = trainStationObject->name;
        }

        Common::repositionTabs(&self);
    }

    // 0x0049DE40
    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        self.draw(drawingCtx);
        Common::drawTabs(self, drawingCtx);

        auto company = CompanyManager::getPlayerCompany();
        auto companyColour = company->mainColours.primary;
        int16_t xPos = self.widgets[widx::image].left;
        int16_t yPos = self.widgets[widx::image].top;

        if (_cState->byte_1136063 & (1 << 7))
        {
            auto airportObj = ObjectManager::get<AirportObject>(_cState->lastSelectedStationType);
            auto imageId = Gfx::recolour(airportObj->image, companyColour);
            drawingCtx.drawImage(xPos, yPos, imageId);
        }
        else if (_cState->byte_1136063 & (1 << 6))
        {
            auto dockObj = ObjectManager::get<DockObject>(_cState->lastSelectedStationType);
            auto imageId = Gfx::recolour(dockObj->image, companyColour);
            drawingCtx.drawImage(xPos, yPos, imageId);
        }
        else if (_cState->trackType & (1 << 7))
        {
            auto roadStationObj = ObjectManager::get<RoadStationObject>(_cState->lastSelectedStationType);

            auto imageId = Gfx::recolour(roadStationObj->image + RoadStation::ImageIds::preview_image, companyColour);
            drawingCtx.drawImage(xPos, yPos, imageId);

            auto colour = Colours::getTranslucent(companyColour);
            if (!roadStationObj->hasFlags(RoadStationFlags::recolourable))
            {
                colour = ExtColour::unk2E;
            }

            imageId = Gfx::recolourTranslucent(roadStationObj->image + RoadStation::ImageIds::preview_image_windows, colour);
            drawingCtx.drawImage(xPos, yPos, imageId);
        }
        else
        {
            auto trainStationObj = ObjectManager::get<TrainStationObject>(_cState->lastSelectedStationType);

            auto imageId = Gfx::recolour(trainStationObj->image + TrainStation::ImageIds::preview_image, companyColour);
            drawingCtx.drawImage(xPos, yPos, imageId);

            auto colour = Colours::getTranslucent(companyColour);
            if (!trainStationObj->hasFlags(TrainStationFlags::recolourable))
            {
                colour = ExtColour::unk2E;
            }

            imageId = Gfx::recolourTranslucent(trainStationObj->image + TrainStation::ImageIds::preview_image_windows, colour);
            drawingCtx.drawImage(xPos, yPos, imageId);
        }

        if (_cState->stationCost != 0x80000000 && _cState->stationCost != 0)
        {
            auto& widget = self.widgets[widx::image];
            auto point = Point(69, widget.bottom + 4);

            FormatArguments args{};
            args.push<uint32_t>(_cState->stationCost);

            tr.drawStringCentred(point, Colour::black, StringIds::build_cost, args);
        }

        xPos = 3;
        yPos = self.widgets[widx::image].bottom + 16;
        auto width = self.width - 4;
        drawingCtx.drawRectInset(xPos, yPos, width, 1, self.getColour(WindowColour::secondary), Gfx::RectInsetFlags::borderInset);

        if ((_ghostVisibilityFlags & GhostVisibilityFlags::station) == GhostVisibilityFlags::none)
        {
            return;
        }

        FormatArguments args{};

        // Todo: change globals type to be StationId and make this StationId::null
        if (_cState->constructingStationId == 0xFFFFFFFF)
        {
            args.push(StringIds::new_station);
        }
        else
        {
            auto station = StationManager::get(StationId(_cState->constructingStationId));
            args.push(station->name);
            args.push(station->town);
        }

        xPos = 69;
        yPos = self.widgets[widx::image].bottom + 18;

        auto origin = Point(xPos, yPos);
        width = self.width - 4;
        tr.drawStringCentredClipped(origin, width, Colour::black, StringIds::new_station_buffer, args);

        xPos = 2;
        yPos = self.widgets[widx::image].bottom + 29;

        origin = Point(xPos, yPos);
        origin = tr.drawStringLeft(origin, Colour::black, StringIds::catchment_area_accepts);

        if (_cState->constructingStationAcceptedCargoTypes == 0)
        {
            origin = tr.drawStringLeft(origin, Colour::black, StringIds::catchment_area_nothing);
        }
        else
        {
            yPos--;
            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::cargo); i++)
            {
                if (_cState->constructingStationAcceptedCargoTypes & (1 << i))
                {
                    auto xPosMax = self.width - 12;
                    if (origin.x <= xPosMax)
                    {
                        auto cargoObj = ObjectManager::get<CargoObject>(i);

                        drawingCtx.drawImage(origin.x, origin.y, cargoObj->unitInlineSprite);
                        origin.x += 10;
                    }
                }
            }
        }

        xPos = 2;
        yPos = self.widgets[widx::image].bottom + 49;
        origin = Point(xPos, yPos);

        origin = tr.drawStringLeft(origin, Colour::black, StringIds::catchment_area_produces);

        if (_cState->constructingStationProducedCargoTypes == 0)
        {
            origin = tr.drawStringLeft(origin, Colour::black, StringIds::catchment_area_nothing);
        }
        else
        {
            yPos--;
            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::cargo); i++)
            {
                if (_cState->constructingStationProducedCargoTypes & (1 << i))
                {
                    auto xPosMax = self.width - 12;
                    if (origin.x <= xPosMax)
                    {
                        auto cargoObj = ObjectManager::get<CargoObject>(i);

                        drawingCtx.drawImage(origin.x, origin.y, cargoObj->unitInlineSprite);
                        origin.x += 10;
                    }
                }
            }
        }
    }

    void tabReset(Window& self)
    {
        self.callOnMouseDown(Station::widx::image, self.widgets[Station::widx::image].id);
    }

    static constexpr WindowEventList kEvents = {
        .onClose = Common::onClose,
        .onMouseUp = onMouseUp,
        .onMouseDown = onMouseDown,
        .onDropdown = onDropdown,
        .onUpdate = onUpdate,
        .onToolUpdate = onToolUpdate,
        .onToolDown = onToolDown,
        .toolDrag = onToolDrag,
        .toolUp = onToolUp,
        .onToolAbort = onToolAbort,
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
