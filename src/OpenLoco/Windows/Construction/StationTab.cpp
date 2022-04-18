#include "../../Audio/Audio.h"
#include "../../CompanyManager.h"
#include "../../GameCommands/GameCommands.h"
#include "../../Graphics/ImageIds.h"
#include "../../Industry.h"
#include "../../Input.h"
#include "../../Localisation/FormatArguments.hpp"
#include "../../Localisation/StringIds.h"
#include "../../Objects/AirportObject.h"
#include "../../Objects/CargoObject.h"
#include "../../Objects/DockObject.h"
#include "../../Objects/IndustryObject.h"
#include "../../Objects/ObjectManager.h"
#include "../../Objects/RoadObject.h"
#include "../../Objects/RoadStationObject.h"
#include "../../Objects/TrackObject.h"
#include "../../Objects/TrainStationObject.h"
#include "../../StationManager.h"
#include "../../Ui/Dropdown.h"
#include "../../Widget.h"
#include "Construction.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Map::TileManager;

namespace OpenLoco::Ui::Windows::Construction::Station
{
    Widget widgets[] = {
        commonWidgets(138, 190, StringIds::stringid_2),
        makeDropdownWidgets({ 3, 45 }, { 132, 12 }, WidgetType::combobox, WindowColour::secondary, 0xFFFFFFFF, StringIds::tooltip_select_station_type),
        makeWidget({ 35, 60 }, { 68, 68 }, WidgetType::wt_3, WindowColour::secondary),
        makeWidget({ 112, 104 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::rotate_object, StringIds::rotate_90),
        widgetEnd(),
    };

    WindowEventList events;

    // 0x0049E228
    static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Common::widx::close_button:
                WindowManager::close(self);
                break;

            case Common::widx::tab_construction:
            case Common::widx::tab_overhead:
            case Common::widx::tab_signal:
            case Common::widx::tab_station:
                Common::switchTab(self, widgetIndex);
                break;

            case widx::rotate:
                _constructionRotation++;
                _constructionRotation = _constructionRotation & 3;
                _stationCost = 0x80000000;
                self->invalidate();
                break;
        }
    }

    template<typename obj_type>
    void AddStationsToDropdown(const uint8_t stationCount)
    {
        for (auto stationIndex = 0; stationIndex < stationCount; stationIndex++)
        {
            auto station = _stationList[stationIndex];
            if (station == _lastSelectedStationType)
                Dropdown::setHighlightedItem(stationIndex);

            auto obj = ObjectManager::get<obj_type>(station);
            Dropdown::add(stationIndex, obj->name);
        }
    }

    // 0x0049E249
    static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::station_dropdown:
            {
                uint8_t stationCount = 0;
                while (_stationList[stationCount] != 0xFF)
                    stationCount++;

                auto widget = self->widgets[widx::station];
                auto xPos = widget.left + self->x;
                auto yPos = widget.top + self->y;
                auto width = widget.width() + 2;
                auto height = widget.height();
                Dropdown::show(xPos, yPos, width, height, self->getColour(WindowColour::secondary), stationCount, (1 << 7));

                if (_byte_1136063 & (1 << 7))
                {
                    AddStationsToDropdown<AirportObject>(stationCount);
                }
                else if (_byte_1136063 & (1 << 6))
                {
                    AddStationsToDropdown<DockObject>(stationCount);
                }
                else if (_trackType & (1 << 7))
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
                Input::toolCancel();
                Input::toolSet(self, widgetIndex, CursorId::placeStation);
                break;
            }
        }
    }

    // 0x0049E256
    static void onDropdown(Window* self, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (widgetIndex == widx::station_dropdown)
        {
            if (itemIndex == -1)
                return;

            auto selectedStation = _stationList[itemIndex];
            _lastSelectedStationType = selectedStation;

            if (_byte_1136063 & (1 << 7))
            {
                _lastAirport = selectedStation;
            }
            else if (_byte_1136063 & (1 << 6))
            {
                _lastShipPort = selectedStation;
            }
            else if (_trackType & (1 << 7))
            {
                auto trackType = _trackType & ~(1 << 7);
                _scenarioRoadStations[trackType] = selectedStation;
            }
            else
            {
                _scenarioTrainStations[_trackType] = selectedStation;
            }

            self->invalidate();
        }
    }

    // 0x0049E437
    static void onUpdate(Window* self)
    {
        Common::onUpdate(self, (1 << 3));
    }

    // 0x0049FF4B
    void removeStationGhost()
    {
        if (_byte_522096 & (1 << 3))
        {
            if (Input::hasMapSelectionFlag(Input::MapSelectionFlags::catchmentArea))
            {
                Windows::Station::sub_491BC6();
                Input::resetMapSelectionFlag(Input::MapSelectionFlags::catchmentArea);
            }
            if (_stationGhostType & (1 << 15))
            {
                GameCommands::AirportRemovalArgs args;
                args.pos = _stationGhostPos;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
            }
            else if (_stationGhostType & (1 << 14))
            {
                GameCommands::PortRemovalArgs args;
                args.pos = _stationGhostPos;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
            }
            else if (_stationGhostType & (1 << 7))
            {
                GameCommands::RoadStationRemovalArgs args;
                args.pos = _stationGhostPos;
                args.rotation = _stationGhostRotation;
                args.roadId = _stationGhostTrackId;
                args.index = _stationGhostTileIndex;
                args.type = _stationGhostType & ~(1 << 7);
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
            }
            else
            {
                GameCommands::TrackStationRemovalArgs args;
                args.pos = _stationGhostPos;
                args.rotation = _stationGhostRotation;
                args.trackId = _stationGhostTrackId;
                args.index = _stationGhostTileIndex;
                args.type = _stationGhostType;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
            }
            _byte_522096 = _byte_522096 & ~(1 << 3);
        }
    }

    // 0x0049E421
    static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = X86Pointer(&self);
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049E421, regs);
    }

    static loco_global<Map::Pos2, 0x001135F7C> _1135F7C;
    static loco_global<Map::Pos2, 0x001135F80> _1135F90;

    // 0x004A47D9
    static std::optional<GameCommands::AirportPlacementArgs> getAirportPlacementArgsFromCursor(const int16_t x, const int16_t y)
    {
        auto pos = ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y });
        if (!pos)
        {
            return std::nullopt;
        }

        GameCommands::AirportPlacementArgs placementArgs;
        placementArgs.type = _lastSelectedStationType;
        placementArgs.rotation = _constructionRotation;
        auto* airportObject = ObjectManager::get<AirportObject>(placementArgs.type);
        TilePos2 minPos(airportObject->min_x, airportObject->min_y);
        TilePos2 maxPos(airportObject->max_x, airportObject->max_y);

        minPos = Math::Vector::rotate(minPos, placementArgs.rotation);
        maxPos = Math::Vector::rotate(maxPos, placementArgs.rotation);

        minPos += *pos;
        maxPos += *pos;

        if (minPos.x > maxPos.x)
        {
            std::swap(minPos.x, maxPos.x);
        }

        if (minPos.y > maxPos.y)
        {
            std::swap(minPos.y, maxPos.y);
        }

        _1135F7C = minPos;
        _1135F90 = maxPos;
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

                const auto baseZ = surface->water() ? surface->water() * 4 : surface->baseZ();
                maxBaseZ = std::max(maxBaseZ, baseZ);
            }
        }
        placementArgs.pos = Map::Pos3(pos->x, pos->y, maxBaseZ * 4);
        return { placementArgs };
    }

    // 0x004A5550
    static void onToolDownAirport(const int16_t x, const int16_t y)
    {
        static loco_global<Ui::Point, 0x0113600C> _113600C;
        _113600C = Point(x, y);
        removeConstructionGhosts();

        const auto args = getAirportPlacementArgsFromCursor(x, y);
        if (!args)
        {
            return;
        }

        const auto* airportObject = ObjectManager::get<AirportObject>(_lastSelectedStationType);
        auto formatArgs = FormatArguments::common();
        formatArgs.skip(3 * sizeof(string_id));
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

        // count of water on each side of the placement
        // 0x0113608B
        std::array<uint8_t, 4> _nearbyWaterCount = { 0 };

        uint8_t directionOfIndustry = 0xFF;
        uint8_t waterHeight = 0;
        _1135F7C = *pos;
        _1135F90 = *pos + TilePos2(1, 1);

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
                const auto searchPos = offset + *pos;
                if (!validCoords(searchPos))
                {
                    continue;
                }
                const auto tile = TileManager::get(searchPos);
                bool surfaceFound = false;
                for (auto el : tile)
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
                        if (!(industryObj->flags & IndustryObjectFlags::built_on_water))
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
                        waterHeight = surface->water() * 4;
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
        placementArgs.type = _lastSelectedStationType;
        placementArgs.pos = Map::Pos3(pos->x, pos->y, waterHeight * 4);
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
    static void onToolDownDock(const int16_t x, const int16_t y)
    {
        static loco_global<Ui::Point, 0x0113600C> _113600C;
        _113600C = Point(x, y);
        removeConstructionGhosts();

        const auto args = getDockPlacementArgsFromCursor(x, y);
        if (!args)
        {
            return;
        }

        const auto* dockObject = ObjectManager::get<DockObject>(_lastSelectedStationType);
        auto formatArgs = FormatArguments::common();
        formatArgs.skip(3 * sizeof(string_id));
        formatArgs.push(dockObject->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);
        GameCommands::doCommand(*args, GameCommands::Flags::apply);
    }

    static std::optional<GameCommands::RoadStationPlacementArgs> getRoadStationPlacementArgsFromCursor(const int16_t x, const int16_t y)
    {
        const auto res = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~ViewportInteraction::InteractionItemFlags::roadAndTram);
        const auto& interaction = res.first;
        if (interaction.type != ViewportInteraction::InteractionItem::road)
        {
            return std::nullopt;
        }

        auto* elRoad = reinterpret_cast<const TileElement*>(interaction.object)->as<RoadElement>();
        if (elRoad == nullptr)
        {
            return std::nullopt;
        }

        GameCommands::RoadStationPlacementArgs placementArgs;
        placementArgs.pos = Map::Pos3(interaction.pos.x, interaction.pos.y, elRoad->baseZ() * 4);
        placementArgs.rotation = elRoad->unkDirection();
        placementArgs.roadId = elRoad->roadId();
        placementArgs.index = elRoad->sequenceIndex();
        placementArgs.roadObjectId = elRoad->roadObjectId();
        placementArgs.type = _lastSelectedStationType;
        return { placementArgs };
    }

    // 0x004A548F
    static void onToolDownRoadStation(const int16_t x, const int16_t y)
    {
        static loco_global<Ui::Point, 0x0113600C> _113600C;
        _113600C = Point(x, y);
        removeConstructionGhosts();

        const auto args = getRoadStationPlacementArgsFromCursor(x, y);
        if (!args)
        {
            return;
        }

        const auto* roadStationObject = ObjectManager::get<RoadStationObject>(_lastSelectedStationType);
        auto formatArgs = FormatArguments::common();
        formatArgs.skip(3 * sizeof(string_id));
        formatArgs.push(roadStationObject->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);
        if (GameCommands::doCommand(*args, GameCommands::Flags::apply) != GameCommands::FAILURE)
        {
            Audio::playSound(Audio::SoundId::construct, GameCommands::getPosition());
        }
    }

    static std::optional<GameCommands::TrackStationPlacementArgs> getTrackStationPlacementArgsFromCursor(const int16_t x, const int16_t y)
    {
        const auto res = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~ViewportInteraction::InteractionItemFlags::track);
        const auto& interaction = res.first;
        if (interaction.type != ViewportInteraction::InteractionItem::track)
        {
            return std::nullopt;
        }

        auto* elTrack = reinterpret_cast<const TileElement*>(interaction.object)->as<TrackElement>();
        if (elTrack == nullptr)
        {
            return std::nullopt;
        }

        GameCommands::TrackStationPlacementArgs placementArgs;
        placementArgs.pos = Map::Pos3(interaction.pos.x, interaction.pos.y, elTrack->baseZ() * 4);
        placementArgs.rotation = elTrack->unkDirection();
        placementArgs.trackId = elTrack->trackId();
        placementArgs.index = elTrack->sequenceIndex();
        placementArgs.trackObjectId = elTrack->trackObjectId();
        placementArgs.type = _lastSelectedStationType;
        return { placementArgs };
    }

    // 0x004A5390
    static void onToolDownTrackStation(const int16_t x, const int16_t y)
    {
        static loco_global<Ui::Point, 0x0113600C> _113600C;
        _113600C = Point(x, y);
        removeConstructionGhosts();

        const auto args = getTrackStationPlacementArgsFromCursor(x, y);
        if (!args)
        {
            return;
        }

        const auto* trainStationObject = ObjectManager::get<TrainStationObject>(_lastSelectedStationType);
        auto formatArgs = FormatArguments::common();
        formatArgs.skip(3 * sizeof(string_id));
        formatArgs.push(trainStationObject->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);

        if (args->trackObjectId != _trackType)
        {
            Error::open(StringIds::null, StringIds::wrong_type_of_track_road);
            return;
        }
        if (GameCommands::doCommand(*args, GameCommands::Flags::apply) != GameCommands::FAILURE)
        {
            Audio::playSound(Audio::SoundId::construct, GameCommands::getPosition());
        }
    }

    // 0x0049E42C
    static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::image)
        {
            return;
        }

        if (_byte_1136063 & (1 << 7))
        {
            onToolDownAirport(x, y);
        }
        else if (_byte_1136063 & (1 << 6))
        {
            onToolDownDock(x, y);
        }
        else if (_trackType & (1 << 7))
        {
            onToolDownRoadStation(x, y);
        }
        else
        {
            onToolDownTrackStation(x, y);
        }
    }

    // 0x0049DD39
    static void prepareDraw(Window* self)
    {
        Common::prepareDraw(self);

        self->widgets[widx::rotate].type = WidgetType::none;

        auto args = FormatArguments();

        if (_byte_1136063 & (1 << 7))
        {
            self->widgets[widx::rotate].type = WidgetType::buttonWithImage;

            auto airportObj = ObjectManager::get<AirportObject>(_lastSelectedStationType);

            self->widgets[widx::station].text = airportObj->name;

            args.push(StringIds::title_airport);
        }
        else if (_byte_1136063 & (1 << 6))
        {
            auto dockObj = ObjectManager::get<DockObject>(_lastSelectedStationType);

            self->widgets[widx::station].text = dockObj->name;

            args.push(StringIds::title_ship_port);
        }
        else if (_trackType & (1 << 7))
        {
            auto trackType = _trackType & ~(1 << 7);

            auto roadObj = ObjectManager::get<RoadObject>(trackType);

            args.push(roadObj->name);

            auto roadStationObject = ObjectManager::get<RoadStationObject>(_lastSelectedStationType);

            self->widgets[widx::station].text = roadStationObject->name;
        }
        else
        {
            auto trackObj = ObjectManager::get<TrackObject>(_trackType);

            args.push(trackObj->name);

            auto trainStationObject = ObjectManager::get<TrainStationObject>(_lastSelectedStationType);

            self->widgets[widx::station].text = trainStationObject->name;
        }

        Common::repositionTabs(self);
    }

    // 0x0049DE40
    static void draw(Window* self, Gfx::Context* context)
    {
        self->draw(context);
        Common::drawTabs(self, context);

        auto company = CompanyManager::get(_playerCompany);
        auto companyColour = company->mainColours.primary;
        int16_t xPos = self->widgets[widx::image].left + self->x;
        int16_t yPos = self->widgets[widx::image].top + self->y;

        if (_byte_1136063 & (1 << 7))
        {
            auto airportObj = ObjectManager::get<AirportObject>(_lastSelectedStationType);
            auto imageId = Gfx::recolour(airportObj->image, companyColour);
            Gfx::drawImage(context, xPos, yPos, imageId);
        }
        else if (_byte_1136063 & (1 << 6))
        {
            auto dockObj = ObjectManager::get<DockObject>(_lastSelectedStationType);
            auto imageId = Gfx::recolour(dockObj->image, companyColour);
            Gfx::drawImage(context, xPos, yPos, imageId);
        }
        else if (_trackType & (1 << 7))
        {
            auto roadStationObj = ObjectManager::get<RoadStationObject>(_lastSelectedStationType);

            auto imageId = Gfx::recolour(roadStationObj->image + RoadStation::ImageIds::preview_image, companyColour);
            Gfx::drawImage(context, xPos, yPos, imageId);

            auto colour = Colours::getTranslucent(companyColour);
            if (!(roadStationObj->flags & RoadStationFlags::recolourable))
            {
                colour = ExtColour::unk2E;
            }

            imageId = Gfx::recolourTranslucent(roadStationObj->image + RoadStation::ImageIds::preview_image_windows, colour);
            Gfx::drawImage(context, xPos, yPos, imageId);
        }
        else
        {
            auto trainStationObj = ObjectManager::get<TrainStationObject>(_lastSelectedStationType);

            auto imageId = Gfx::recolour(trainStationObj->image + TrainStation::ImageIds::preview_image, companyColour);
            Gfx::drawImage(context, xPos, yPos, imageId);

            auto colour = Colours::getTranslucent(companyColour);
            if (!(trainStationObj->flags & TrainStationFlags::recolourable))
            {
                colour = ExtColour::unk2E;
            }

            imageId = Gfx::recolourTranslucent(trainStationObj->image + TrainStation::ImageIds::preview_image_windows, colour);
            Gfx::drawImage(context, xPos, yPos, imageId);
        }

        if (_stationCost != 0x80000000 && _stationCost != 0)
        {
            xPos = self->x + 69;
            yPos = self->widgets[widx::image].bottom + self->y + 4;

            auto args = FormatArguments();
            args.push<uint32_t>(_stationCost);

            Gfx::drawStringCentred(*context, xPos, yPos, Colour::black, StringIds::build_cost, &args);
        }

        xPos = self->x + 3;
        yPos = self->widgets[widx::image].bottom + self->y + 16;
        auto width = self->width - 4;
        Gfx::drawRectInset(*context, xPos, yPos, width, 1, self->getColour(WindowColour::secondary).u8(), (1 << 5));

        if (!(_byte_522096 & (1 << 3)))
            return;

        auto args = FormatArguments();

        // Todo: change globals type to be StationId and make this StationId::null
        if (_constructingStationId == 0xFFFFFFFF)
        {
            args.push(StringIds::new_station);
        }
        else
        {
            auto station = StationManager::get(StationId(*_constructingStationId));
            args.push(station->name);
            args.push(station->town);
        }

        xPos = self->x + 69;
        yPos = self->widgets[widx::image].bottom + self->y + 18;
        width = self->width - 4;
        Gfx::drawStringCentredClipped(*context, xPos, yPos, width, Colour::black, StringIds::new_station_buffer, &args);

        xPos = self->x + 2;
        yPos = self->widgets[widx::image].bottom + self->y + 29;
        Ui::Point origin = { xPos, yPos };

        Gfx::drawString_494B3F(*context, &origin, Colour::black, StringIds::catchment_area_accepts);

        if (_constructingStationAcceptedCargoTypes == 0)
        {
            Gfx::drawString_494B3F(*context, origin.x, origin.y, Colour::black, StringIds::catchment_area_nothing);
        }
        else
        {
            yPos--;
            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::cargo); i++)
            {
                if (_constructingStationAcceptedCargoTypes & (1 << i))
                {
                    auto xPosMax = self->x + self->width - 12;
                    if (origin.x <= xPosMax)
                    {
                        auto cargoObj = ObjectManager::get<CargoObject>(i);

                        Gfx::drawImage(context, origin.x, origin.y, cargoObj->unit_inline_sprite);
                        origin.x += 10;
                    }
                }
            }
        }

        xPos = self->x + 2;
        yPos = self->widgets[widx::image].bottom + self->y + 49;
        origin = { xPos, yPos };

        Gfx::drawString_494B3F(*context, &origin, Colour::black, StringIds::catchment_area_produces);

        if (_constructingStationProducedCargoTypes == 0)
        {
            Gfx::drawString_494B3F(*context, origin.x, origin.y, Colour::black, StringIds::catchment_area_nothing);
        }
        else
        {
            yPos--;
            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::cargo); i++)
            {
                if (_constructingStationProducedCargoTypes & (1 << i))
                {
                    auto xPosMax = self->x + self->width - 12;
                    if (origin.x <= xPosMax)
                    {
                        auto cargoObj = ObjectManager::get<CargoObject>(i);

                        Gfx::drawImage(context, origin.x, origin.y, cargoObj->unit_inline_sprite);
                        origin.x += 10;
                    }
                }
            }
        }
    }

    void tabReset(Window* self)
    {
        self->callOnMouseDown(Station::widx::image);
    }

    void initEvents()
    {
        events.onClose = Common::onClose;
        events.onMouseUp = onMouseUp;
        events.onMouseDown = onMouseDown;
        events.onDropdown = onDropdown;
        events.onUpdate = onUpdate;
        events.onToolUpdate = onToolUpdate;
        events.onToolDown = onToolDown;
        events.prepareDraw = prepareDraw;
        events.draw = draw;
    }
}
