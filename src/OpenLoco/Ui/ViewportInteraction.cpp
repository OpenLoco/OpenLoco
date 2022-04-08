#include "../CompanyManager.h"
#include "../Config.h"
#include "../Entities/EntityManager.h"
#include "../IndustryManager.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Localisation/StringManager.h"
#include "../Map/TileManager.h"
#include "../Objects/BuildingObject.h"
#include "../Objects/CargoObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/RoadObject.h"
#include "../Objects/TrackExtraObject.h"
#include "../Objects/TrackObject.h"
#include "../Objects/TreeObject.h"
#include "../Objects/WallObject.h"
#include "../Paint/Paint.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Ui.h"
#include "../Ui/ScrollView.h"
#include "../Vehicles/Vehicle.h"
#include "../ViewportManager.h"
#include "../Window.h"
#include "WindowManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;

namespace OpenLoco::Ui::ViewportInteraction
{
    InteractionArg::InteractionArg(const Paint::PaintStruct& ps)
        : object(ps.entity)
        , type(ps.type)
        , unkBh(ps.var_29)
    {
        pos = Pos2{ ps.map_x, ps.map_y };
    }

    static bool getStationArguments(InteractionArg& interaction);
    static bool getStationArguments(StationId id);

    // 0x004CD95A
    static bool _track(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* track = tileElement->as<TrackElement>();
        if (track == nullptr)
            return false;
        if (!track->hasStationElement())
            return false;

        tileElement++;
        auto* station = tileElement->as<StationElement>();
        if (station == nullptr)
            return false;
        if (station->isFlag5())
            return false;

        interaction.type = InteractionItem::trackStation;
        interaction.object = station;
        return getStationArguments(interaction);
    }

    // 0x004CD974
    static bool _road(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* road = tileElement->as<RoadElement>();
        if (road == nullptr)
            return false;
        if (!road->hasStationElement())
            return false;

        Map::StationElement* station = nullptr;
        Map::Tile tile{ interaction.pos, tileElement };
        for (auto& t : tile)
        {
            station = t.as<StationElement>();
            if (station != nullptr)
            {
                break;
            }
        }

        if (station == nullptr)
        {
            return false;
        }

        interaction.object = station;
        interaction.type = InteractionItem::dock;
        if (station->isFlag5())
            return false;

        interaction.type = InteractionItem::roadStation;
        return getStationArguments(interaction);
    }

    // 0x004CD99A
    static bool getStationArguments(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* station = tileElement->as<StationElement>();
        if (station == nullptr)
            return false;
        if (station->isGhost())
            return false;

        interaction.value = enumValue(station->stationId());
        interaction.type = InteractionItem::stationLabel;
        return getStationArguments(station->stationId());
    }

    static loco_global<StationId, 0x00F252A4> _hoveredStationId;

    // 0x004CD9B0
    static bool getStationArguments(const StationId id)
    {
        _hoveredStationId = id;

        auto station = StationManager::get(id);

        Input::setMapSelectionFlags(Input::MapSelectionFlags::unk_6);
        ViewportManager::invalidate(station);
        Windows::MapToolTip::setOwner(station->owner);
        auto args = FormatArguments::mapToolTip(StringIds::stringid_stringid_wcolour3_stringid);
        args.push(station->name);
        args.push(station->town);
        args.push(getTransportIconsFromStationFlags(station->flags));
        char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_338));
        buffer = station->getStatusString(buffer);

        buffer = StringManager::formatString(buffer, StringIds::station_accepts);
        bool seperator = false; // First cargo item does not need a seperator
        for (uint32_t cargoId = 0; cargoId < kMaxCargoStats; cargoId++)
        {
            auto& stats = station->cargoStats[cargoId];

            if (!stats.isAccepted())
            {
                continue;
            }

            if (seperator)
            {
                buffer = StringManager::formatString(buffer, StringIds::unit_separator);
            }
            buffer = StringManager::formatString(buffer, ObjectManager::get<CargoObject>(cargoId)->name);
            seperator = true;
        }

        args.push(StringIds::buffer_338);

        return true;
    }

    // 0x004CD7FB
    static bool getTownArguments(const TownId id)
    {
        auto town = TownManager::get(id);

        auto args = FormatArguments::mapToolTip(StringIds::wcolour3_stringid_2, town->name); // args + 4 empty
        args.skip(2);
        args.push(StringIds::town_size_and_population);
        args.push(town->getTownSizeString());
        args.push(town->population);

        return true;
    }

    // 0x004CD8D5
    static bool getIndustryArguments(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* industryTile = tileElement->as<IndustryElement>();
        if (industryTile == nullptr)
            return false;
        if (industryTile->isGhost())
            return false;

        interaction.value = enumValue(industryTile->industryId());
        auto industry = industryTile->industry();

        char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_338));
        *buffer = 0;
        industry->getStatusString(buffer);
        auto args = FormatArguments::mapToolTip();
        if (std::strlen(buffer) != 0)
        {
            args.push(StringIds::wcolour3_stringid_2);
            args.push(industry->name);
            args.push(industry->town);
            args.push(StringIds::buffer_338);
        }
        else
        {
            args.push(industry->name);
            args.push(industry->town);
        }
        return true;
    }

    // 0x004CDA7C
    static bool getVehicleArguments(const InteractionArg& interaction)
    {
        auto* entity = reinterpret_cast<EntityBase*>(interaction.object);
        auto vehicle = entity->asBase<Vehicles::VehicleBase>();
        if (vehicle == nullptr)
        {
            return false;
        }
        auto head = EntityManager::get<Vehicles::VehicleHead>(vehicle->getHead());
        if (head == nullptr)
        {
            return false;
        }
        auto company = CompanyManager::get(head->owner);
        Windows::MapToolTip::setOwner(head->owner);
        auto status = head->getStatus();
        auto args = FormatArguments::mapToolTip();
        if (status.status2 == StringIds::null)
        {
            args.push(StringIds::wcolour3_stringid);
        }
        else
        {
            args.push(StringIds::wcolour3_stringid_stringid);
        }
        args.push(CompanyManager::getControllingId() == head->owner ? StringIds::company_vehicle : StringIds::competitor_vehicle);
        args.push(company->name); // args + 6 is empty
        args.skip(2);
        args.push(head->name);
        args.push(head->ordinalNumber);
        args.push(status.status1);
        args.push(status.status1Args); // 32bit
        args.push(status.status2);
        args.push(status.status2Args); // 32bit
        return true;
    }

    // 0x004CD84A
    static bool getHeadquarterArguments(const InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* buildingTile = tileElement->as<BuildingElement>();
        if (buildingTile == nullptr)
        {
            return false;
        }
        if (buildingTile->isGhost())
        {
            return false;
        }

        const auto index = buildingTile->multiTileIndex();
        const auto firstTile = interaction.pos - Map::offsets[index];
        const Map::Pos3 pos = { firstTile.x, firstTile.y, buildingTile->baseZ() };

        for (auto& company : CompanyManager::companies())
        {
            if (company.headquartersX != pos.x || company.headquartersY != pos.y || company.headquartersZ != pos.z)
            {
                continue;
            }

            Windows::MapToolTip::setOwner(company.id());
            auto args = FormatArguments::mapToolTip(StringIds::wcolour3_stringid_2, company.name);
            args.skip(2);
            args.push(StringIds::headquarters);
            return true;
        }
        return false;
    }

    static std::optional<uint32_t> vehicleDistanceFromLocation(const Vehicles::VehicleBase& component, const viewport_pos& targetPosition)
    {
        ViewportRect rect = {
            component.sprite_left,
            component.sprite_top,
            component.sprite_bottom,
            component.sprite_right
        };
        if (rect.contains(targetPosition))
        {
            uint32_t xDiff = std::abs(targetPosition.x - (component.sprite_right + component.sprite_left) / 2);
            uint32_t yDiff = std::abs(targetPosition.y - (component.sprite_top + component.sprite_bottom) / 2);
            return xDiff + yDiff;
        }
        return {};
    }

    static void checkAndSetNearestVehicle(uint32_t& nearestDistance, Vehicles::VehicleBase*& nearestVehicle, Vehicles::VehicleBase& checkVehicle, const viewport_pos& targetPosition)
    {
        if (checkVehicle.sprite_left != Location::null)
        {
            auto distanceRes = vehicleDistanceFromLocation(checkVehicle, targetPosition);
            if (distanceRes)
            {
                if (*distanceRes < nearestDistance)
                {
                    nearestDistance = *distanceRes;
                    nearestVehicle = &checkVehicle;
                }
            }
        }
    }

    // 0x004CD658
    InteractionArg getItemLeft(int16_t tempX, int16_t tempY)
    {
        if (OpenLoco::isTitleMode())
            return InteractionArg{};

        auto interactionsToInclude = ~(InteractionItemFlags::entity | InteractionItemFlags::townLabel | InteractionItemFlags::stationLabel);
        auto res = getMapCoordinatesFromPos(tempX, tempY, interactionsToInclude);

        auto interaction = res.first;
        if (interaction.type != InteractionItem::entity)
        {
            // clang-format off
            interactionsToInclude = ~(InteractionItemFlags::entity | InteractionItemFlags::track | InteractionItemFlags::roadAndTram
                | InteractionItemFlags::headquarterBuilding | InteractionItemFlags::station | InteractionItemFlags::townLabel
                | InteractionItemFlags::stationLabel | InteractionItemFlags::industry);
            // clang-format on
            res = getMapCoordinatesFromPos(tempX, tempY, interactionsToInclude);
            interaction = res.first;
        }

        // TODO: Rework so that getting the interaction arguments and getting the map tooltip format arguments are seperated
        bool success = false;
        switch (interaction.type)
        {
            case InteractionItem::track:
                success = _track(interaction);
                break;

            case InteractionItem::road:
                success = _road(interaction);
                break;
            case InteractionItem::townLabel:
                success = getTownArguments(static_cast<TownId>(interaction.value));
                break;
            case InteractionItem::stationLabel:
                success = getStationArguments(static_cast<StationId>(interaction.value));
                break;
            case InteractionItem::trackStation:
            case InteractionItem::roadStation:
            case InteractionItem::airport:
            case InteractionItem::dock:
                success = getStationArguments(interaction);
                break;
            case InteractionItem::industry:
                success = getIndustryArguments(interaction);
                break;
            case InteractionItem::headquarterBuilding:
                success = getHeadquarterArguments(interaction);
                break;
            case InteractionItem::entity:
                success = getVehicleArguments(interaction);
                break;
            default:
                break;
        }
        if (success == true)
        {
            return interaction;
        }
        auto window = WindowManager::findAt(tempX, tempY);
        if (window == nullptr)
            return InteractionArg{};

        auto viewport = window->viewports[0];
        if (viewport == nullptr)
            return InteractionArg{};

        if (viewport->zoom > Config::get().vehiclesMinScale)
            return InteractionArg{};

        uint32_t nearestDistance = std::numeric_limits<uint32_t>().max();
        Vehicles::VehicleBase* nearestVehicle = nullptr;
        auto targetPosition = viewport->screenToViewport({ tempX, tempY });

        for (auto v : EntityManager::VehicleList())
        {
            auto train = Vehicles::Vehicle(*v);
            checkAndSetNearestVehicle(nearestDistance, nearestVehicle, *train.veh2, targetPosition);
            for (auto car : train.cars)
            {
                for (auto carComponent : car)
                {
                    checkAndSetNearestVehicle(nearestDistance, nearestVehicle, *carComponent.front, targetPosition);
                    checkAndSetNearestVehicle(nearestDistance, nearestVehicle, *carComponent.back, targetPosition);
                    checkAndSetNearestVehicle(nearestDistance, nearestVehicle, *carComponent.body, targetPosition);
                }
            }
        }

        if (nearestDistance <= 32 && nearestVehicle != nullptr)
        {
            interaction.type = InteractionItem::entity;
            interaction.object = reinterpret_cast<void*>(nearestVehicle);
            interaction.pos = nearestVehicle->position;

            getVehicleArguments(interaction);
            return interaction;
        }

        return InteractionArg{};
    }

    // 0x004CE1D4
    static bool rightOverTrack(InteractionArg& interaction)
    {
        interaction.type = InteractionItem::track;
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* track = tileElement->as<TrackElement>();
        if (track == nullptr)
            return false;

        if (track->isGhost())
        {
            return false;
        }

        if (Ui::Windows::MapToolTip::getTooltipTimeout() < 45)
        {
            return true;
        }

        auto* trackObj = ObjectManager::get<TrackObject>(track->trackObjectId());
        if (track->owner() == CompanyManager::getControllingId())
        {
            FormatArguments::mapToolTip(StringIds::stringid_right_click_to_modify, trackObj->name);
        }
        else
        {
            auto* company = CompanyManager::get(track->owner());
            FormatArguments::mapToolTip(StringIds::string_owned_by_string, trackObj->name, company->name);
            Windows::MapToolTip::setOwner(track->owner());
        }
        return true;
    }

    // 0x004CE18F
    static bool rightOverTrackExtra(InteractionArg& interaction)
    {
        if (!Windows::Construction::isOverheadTabOpen())
        {
            return rightOverTrack(interaction);
        }

        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* track = tileElement->as<TrackElement>();
        if (track == nullptr)
            return false;

        if (track->isGhost())
        {
            return false;
        }

        if (!track->hasMod(interaction.unkBh))
        {
            return rightOverTrack(interaction);
        }
        auto* trackObj = ObjectManager::get<TrackObject>(track->trackObjectId());
        auto* trackExtraObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[interaction.unkBh]);
        FormatArguments::mapToolTip(StringIds::stringid_right_click_to_remove, trackExtraObj->name);
        return true;
    }

    // 0x004CDBEA
    static bool rightOverSignal(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* signal = tileElement->as<SignalElement>();
        auto* track = tileElement->prev()->as<TrackElement>();
        if (signal == nullptr || track == nullptr)
        {
            return false;
        }

        if (signal->isGhost())
        {
            return false;
        }
        if (!Windows::Construction::isSignalTabOpen())
        {
            interaction.object = track;
            return rightOverTrack(interaction);
        }

        if (track->owner() != CompanyManager::getControllingId())
        {
            return false;
        }
        FormatArguments::mapToolTip(StringIds::stringid_right_click_to_remove, StringIds::capt_signal);
        return true;
    }

    // 0x004CDD8C
    static bool rightOverTrackStation(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* elStation = tileElement->as<StationElement>();
        auto* track = tileElement->prev()->as<TrackElement>();
        if (elStation == nullptr || track == nullptr)
        {
            return false;
        }

        if (elStation->isGhost())
        {
            return false;
        }
        if (!Windows::Construction::isStationTabOpen())
        {
            interaction.object = track;
            return rightOverTrack(interaction);
        }

        if (track->owner() != CompanyManager::getControllingId())
        {
            return false;
        }

        auto* station = StationManager::get(elStation->stationId());
        FormatArguments::mapToolTip(StringIds::stringid_right_click_to_remove, StringIds::string_station_platform, station->name, station->town);
        return true;
    }

    // 0x004CE2C1
    static bool rightOverRoad(InteractionArg& interaction)
    {
        interaction.type = InteractionItem::road;
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* road = tileElement->as<RoadElement>();
        if (road == nullptr)
            return false;

        if (road->isGhost())
        {
            return false;
        }

        if (Ui::Windows::MapToolTip::getTooltipTimeout() < 45)
        {
            return true;
        }

        auto* roadObj = ObjectManager::get<RoadObject>(road->roadObjectId());
        if (road->owner() == CompanyManager::getControllingId() || road->owner() == CompanyId::neutral)
        {
            FormatArguments::mapToolTip(StringIds::stringid_right_click_to_modify, roadObj->name);
        }
        else
        {
            auto* company = CompanyManager::get(road->owner());
            FormatArguments::mapToolTip(StringIds::string_owned_by_string, roadObj->name, company->name);
            Windows::MapToolTip::setOwner(road->owner());
        }
        return true;
    }

    // 0x004CE271
    static bool rightOverRoadExtra(InteractionArg& interaction)
    {
        if (!Windows::Construction::isOverheadTabOpen())
        {
            return rightOverRoad(interaction);
        }

        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* road = tileElement->as<RoadElement>();
        if (road == nullptr)
            return false;

        if (road->isGhost())
        {
            return false;
        }

        if (!road->hasMod(interaction.unkBh))
        {
            return rightOverRoad(interaction);
        }
        auto* roadObj = ObjectManager::get<RoadObject>(road->roadObjectId());
        auto* roadExtraObj = ObjectManager::get<TrackExtraObject>(roadObj->mods[interaction.unkBh]);
        FormatArguments::mapToolTip(StringIds::stringid_right_click_to_remove, roadExtraObj->name);
        return true;
    }

    // 0x004CDDF2
    static bool rightOverRoadStation(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* elStation = tileElement->as<StationElement>();
        auto* road = tileElement->prev()->as<RoadElement>();
        if (elStation == nullptr || road == nullptr)
        {
            return false;
        }

        if (elStation->isGhost())
        {
            return false;
        }
        if (!Windows::Construction::isStationTabOpen())
        {
            interaction.object = road;
            return rightOverRoad(interaction);
        }
        auto* station = StationManager::get(elStation->stationId());
        if (station == nullptr)
        {
            return false;
        }

        if (station->owner != CompanyManager::getControllingId())
        {
            return false;
        }

        FormatArguments::mapToolTip(StringIds::stringid_right_click_to_remove, StringIds::string_station_building_bus_stop, station->name, station->town);
        return true;
    }

    // 0x004CDC26
    static bool rightOverAirport(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* elStation = tileElement->as<StationElement>();
        if (elStation == nullptr)
        {
            return false;
        }

        if (elStation->isGhost())
        {
            return false;
        }

        if (elStation->owner() != CompanyManager::getControllingId())
        {
            return false;
        }

        auto* station = StationManager::get(elStation->stationId());
        auto args = FormatArguments::mapToolTip();
        if (Windows::Construction::isStationTabOpen())
        {
            args.push(StringIds::stringid_right_click_to_remove);
        }
        else
        {
            args.push(StringIds::stringid_right_click_to_modify);
        }
        args.push(StringIds::quote_string_quote);
        args.push(station->name);
        args.push(station->town);
        return true;
    }

    // 0x004CDCD9
    static bool rightOverDock(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* elStation = tileElement->as<StationElement>();
        if (elStation == nullptr)
        {
            return false;
        }

        if (elStation->isGhost())
        {
            return false;
        }

        if (elStation->owner() != CompanyManager::getControllingId())
        {
            return false;
        }

        auto* station = StationManager::get(elStation->stationId());
        auto args = FormatArguments::mapToolTip();
        if (Windows::Construction::isStationTabOpen())
        {
            args.push(StringIds::stringid_right_click_to_remove);
        }
        else
        {
            args.push(StringIds::stringid_right_click_to_modify);
        }
        args.push(StringIds::quote_string_quote2);
        args.push(station->name);
        args.push(station->town);
        return true;
    }

    // 0x004CDE58
    static bool rightOverTree(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* tree = tileElement->as<TreeElement>();
        if (tree == nullptr)
        {
            return false;
        }

        auto* treeObj = ObjectManager::get<TreeObject>(tree->treeObjectId());
        FormatArguments::mapToolTip(StringIds::stringid_right_click_to_remove, treeObj->name);
        return true;
    }

    // 0x004CE0D8
    static bool rightOverWall(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* wall = tileElement->as<WallElement>();
        if (wall == nullptr)
        {
            return false;
        }

        if (!isEditorMode())
        {
            return false;
        }
        auto* wallObj = ObjectManager::get<WallObject>(wall->wallObjectId());
        FormatArguments::mapToolTip(StringIds::stringid_right_click_to_remove, wallObj->name);
        return true;
    }

    // 0x004CDE78
    // Note: this is only when in a constructing mode
    static bool rightOverBuildingConstruct(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* building = tileElement->as<BuildingElement>();
        if (building == nullptr)
        {
            return false;
        }

        const auto* buildingObj = building->getObject();
        auto args = FormatArguments::mapToolTip();
        if (isEditorMode() || !(buildingObj->flags & BuildingObjectFlags::undestructible))
        {
            args.push(StringIds::stringid_right_click_to_remove);
        }
        args.push(buildingObj->name);
        return true;
    }

    // 0x004CE107
    static bool rightOverHeadquarters(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* building = tileElement->as<BuildingElement>();
        if (building == nullptr)
        {
            return false;
        }

        auto firstTile = interaction.pos - Map::offsets[building->multiTileIndex()];
        auto height = building->baseZ();
        for (auto& company : CompanyManager::companies())
        {
            if (company.headquartersX == firstTile.x && company.headquartersY == firstTile.y && company.headquartersZ == height)
            {
                FormatArguments::mapToolTip(StringIds::stringid_right_click_to_remove, StringIds::stringid_headquarters, company.name);
                return true;
            }
        }
        return false;
    }

    constexpr std::array<string_id, 7> quantityToString = {
        StringIds::quantity_eigth,
        StringIds::quantity_quarter,
        StringIds::quantity_three_eigths,
        StringIds::quantity_half,
        StringIds::quantity_five_eigths,
        StringIds::quantity_three_quarters,
        StringIds::quantity_seven_eigths,
    };

    // 0x004CDEB8
    // Note: this is only when in a none constructing mode
    static bool rightOverBuilding(InteractionArg& interaction)
    {
        interaction.type = InteractionItem::buildingInfo;
        if (Ui::Windows::MapToolTip::getTooltipTimeout() < 45)
        {
            return true;
        }

        auto* tileElement = reinterpret_cast<Map::TileElement*>(interaction.object);
        auto* building = tileElement->as<BuildingElement>();
        if (building == nullptr)
        {
            return false;
        }

        const auto* buildingObj = building->getObject();
        auto* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_338));
        buffer = StringManager::formatString(buffer, buildingObj->name);
        if (!building->isConstructed())
        {
            buffer = StringManager::formatString(buffer, StringIds::under_construction);
        }
        else
        {
            if (buildingObj->producedQuantity[0] != 0 || buildingObj->producedQuantity[1] != 0)
            {
                buffer = StringManager::formatString(buffer, StringIds::produces);
                bool requiresComma = false;
                for (auto i = 0; i < 2; ++i)
                {
                    if (buildingObj->producedQuantity[i] != 0)
                    {
                        if (requiresComma)
                        {
                            buffer = StringManager::formatString(buffer, StringIds::comma);
                        }
                        requiresComma = true;
                        auto* cargo = ObjectManager::get<CargoObject>(buildingObj->producedCargoType[i]);
                        buffer = StringManager::formatString(buffer, cargo->name);
                    }
                }
            }
            if (buildingObj->var_A6[0] != 0 || buildingObj->var_A6[1] != 0 || buildingObj->var_A8[0] != 0 || buildingObj->var_A8[1] != 0)
            {
                buffer = StringManager::formatString(buffer, StringIds::accepts);
                bool requiresComma = false;
                for (auto i = 0; i < 2; ++i)
                {
                    if (buildingObj->var_A6[i] != 0)
                    {
                        if (requiresComma)
                        {
                            buffer = StringManager::formatString(buffer, StringIds::comma);
                        }
                        if (buildingObj->var_A6[i] < 8)
                        {
                            buffer = StringManager::formatString(buffer, quantityToString[buildingObj->var_A6[i]]);
                        }
                        requiresComma = true;
                        auto* cargo = ObjectManager::get<CargoObject>(buildingObj->producedCargoType[i]);
                        buffer = StringManager::formatString(buffer, cargo->name);
                    }
                }
                for (auto i = 0; i < 2; ++i)
                {
                    if (buildingObj->var_A8[i] != 0)
                    {
                        if (requiresComma)
                        {
                            buffer = StringManager::formatString(buffer, StringIds::comma);
                        }
                        if (buildingObj->var_A8[i] < 8)
                        {
                            buffer = StringManager::formatString(buffer, quantityToString[buildingObj->var_A8[i]]);
                        }
                        requiresComma = true;
                        auto* cargo = ObjectManager::get<CargoObject>(buildingObj->var_A4[i]);
                        buffer = StringManager::formatString(buffer, cargo->name);
                    }
                }
            }
        }
        FormatArguments::mapToolTip(StringIds::buffer_338);
        return true;
    }

    // 0x004CDB2B
    InteractionArg rightOver(int16_t x, int16_t y)
    {
        if (OpenLoco::isTitleMode())
            return InteractionArg{};

        // Interaction types to exclude by default
        auto interactionsToExclude = 0 | InteractionItemFlags::surface | InteractionItemFlags::water;

        // TODO: Handle in the paint functions
        // Get the viewport and add extra flags for hidden scenery
        auto screenPos = Ui::Point(x, y);
        auto w = WindowManager::findAt(screenPos);
        if (w != nullptr)
        {
            for (auto vp : w->viewports)
            {
                if (vp != nullptr && vp->containsUi({ screenPos.x, screenPos.y }))
                {
                    if (vp->flags & ViewportFlags::hide_foreground_scenery_buildings)
                    {
                        interactionsToExclude |= InteractionItemFlags::building | InteractionItemFlags::headquarterBuilding | InteractionItemFlags::industry | InteractionItemFlags::tree | InteractionItemFlags::wall;
                    }
                }
            }
        }

        bool hasInteraction = false;
        auto res = getMapCoordinatesFromPos(x, y, interactionsToExclude);
        auto& interaction = res.first;
        switch (interaction.type)
        {
            case InteractionItem::track:
                hasInteraction = rightOverTrack(interaction);
                break;
            case InteractionItem::entity:
                // No interaction
                break;
            case InteractionItem::trackExtra:
                hasInteraction = rightOverTrackExtra(interaction);
                break;
            case InteractionItem::signal:
                hasInteraction = rightOverSignal(interaction);
                break;
            case InteractionItem::trackStation:
                hasInteraction = rightOverTrackStation(interaction);
                break;
            case InteractionItem::roadStation:
                hasInteraction = rightOverRoadStation(interaction);
                break;
            case InteractionItem::airport:
                hasInteraction = rightOverAirport(interaction);
                break;
            case InteractionItem::dock:
                hasInteraction = rightOverDock(interaction);
                break;
            case InteractionItem::road:
                hasInteraction = rightOverRoad(interaction);
                break;
            case InteractionItem::roadExtra:
                hasInteraction = rightOverRoadExtra(interaction);
                break;
            default:
                if (!(Input::hasFlag(Input::Flags::toolActive) && Input::hasFlag(Input::Flags::flag6)))
                {
                    if (WindowManager::find(WindowType::construction) == nullptr)
                    {
                        if (interaction.type == InteractionItem::building)
                        {
                            hasInteraction = rightOverBuilding(interaction);
                        }
                        break;
                    }
                }
                // 0x4CDBC5
                switch (interaction.type)
                {
                    case InteractionItem::tree:
                        hasInteraction = rightOverTree(interaction);
                        break;
                    case InteractionItem::wall:
                        hasInteraction = rightOverWall(interaction);
                        break;
                    case InteractionItem::building:
                        hasInteraction = rightOverBuildingConstruct(interaction);
                        break;
                    case InteractionItem::headquarterBuilding:
                        hasInteraction = rightOverHeadquarters(interaction);
                        break;
                    default:
                        hasInteraction = true;
                        break;
                }
                break;
        }
        return hasInteraction ? interaction : InteractionArg{};
    }

    // 0x00459E54
    std::pair<ViewportInteraction::InteractionArg, Viewport*> getMapCoordinatesFromPos(int32_t screenX, int32_t screenY, int32_t flags)
    {
        static loco_global<uint8_t, 0x0050BF68> _50BF68; // If in get map coords
        static loco_global<Gfx::Context, 0x00E0C3E4> _context1;
        static loco_global<Gfx::Context, 0x00E0C3F4> _context2;

        _50BF68 = 1;
        ViewportInteraction::InteractionArg interaction{};
        Ui::Point screenPos = { static_cast<int16_t>(screenX), static_cast<int16_t>(screenY) };
        auto w = WindowManager::findAt(screenPos);
        if (w == nullptr)
        {
            _50BF68 = 0;
            return std::make_pair(interaction, nullptr);
        }

        Viewport* chosenV = nullptr;
        for (auto vp : w->viewports)
        {
            if (vp == nullptr)
                continue;

            if (!vp->containsUi({ screenPos.x, screenPos.y }))
                continue;

            chosenV = vp;
            auto vpPos = vp->screenToViewport({ screenPos.x, screenPos.y });
            _context1->zoom_level = vp->zoom;
            _context1->x = (0xFFFF << vp->zoom) & vpPos.x;
            _context1->y = (0xFFFF << vp->zoom) & vpPos.y;
            _context2->x = _context1->x;
            _context2->y = _context1->y;
            _context2->width = 1;
            _context2->height = 1;
            _context2->zoom_level = _context1->zoom_level;
            auto* session = Paint::allocateSession(_context2, vp->flags);
            session->generate();
            session->arrangeStructs();
            interaction = session->getNormalInteractionInfo(flags);
            if (!(vp->flags & ViewportFlags::station_names_displayed))
            {
                if (_context2->zoom_level <= Config::get().stationNamesMinScale)
                {
                    auto stationInteraction = session->getStationNameInteractionInfo(flags);
                    if (stationInteraction.type != InteractionItem::noInteraction)
                    {
                        interaction = stationInteraction;
                    }
                }
            }
            if (!(vp->flags & ViewportFlags::town_names_displayed))
            {
                auto townInteraction = session->getTownNameInteractionInfo(flags);
                if (townInteraction.type != InteractionItem::noInteraction)
                {
                    interaction = townInteraction;
                }
            }
            break;
        }
        _50BF68 = 0;
        return std::make_pair(interaction, chosenV);
    }

    // 0x00460781
    // regs.ax = screenCoords.x;
    // regs.bx = screenCoords.y;
    // returns
    // regs.edx = InteractionInfo.value (unsure if ever used)
    // regs.ax = mapX, 0x8000 - in case of failure
    // regs.bx = mapY
    // regs.ecx = closestEdge (unsure if ever used)
    std::optional<Pos2> getSurfaceOrWaterLocFromUi(const Point& screenCoords)
    {
        // TODO: modify getSurfaceOrWaterLocFromUi to return the viewport then can use viewports rotation

        auto [info, viewport] = getMapCoordinatesFromPos(screenCoords.x, screenCoords.y, ~(InteractionItemFlags::surface | InteractionItemFlags::water));

        if (info.type == InteractionItem::noInteraction)
        {
            return {};
        }

        int16_t waterHeight = 0; // E40130
        if (info.type == InteractionItem::water)
        {
            auto* surface = static_cast<const SurfaceElement*>(info.object);
            waterHeight = surface->water() * 16;
        }

        const auto minPosition = info.pos;                  // E40128/A
        const auto maxPosition = info.pos + Pos2{ 31, 31 }; // E4012C/E
        auto mapPos = info.pos + Pos2{ 16, 16 };
        const auto initialVPPos = viewport->screenToViewport(screenCoords);

        for (int32_t i = 0; i < 5; i++)
        {
            int16_t z = waterHeight;
            if (info.type != InteractionItem::water)
            {
                z = TileManager::getHeight(mapPos).landHeight;
            }
            mapPos = viewportCoordToMapCoord(initialVPPos.x, initialVPPos.y, z, viewport->getRotation());
            mapPos.x = std::clamp(mapPos.x, minPosition.x, maxPosition.x);
            mapPos.y = std::clamp(mapPos.y, minPosition.y, maxPosition.y);
        }

        // Determine to which edge the cursor is closest
        [[maybe_unused]] uint32_t closestEdge = getSideFromPos(mapPos); // ecx

        return { Pos2(mapPos.x & 0xFFE0, mapPos.y & 0xFFE0) };
    }

    // 0x0045F1A7
    std::optional<std::pair<Pos2, Viewport*>> getSurfaceLocFromUi(const Point& screenCoords)
    {
        auto [info, viewport] = getMapCoordinatesFromPos(screenCoords.x, screenCoords.y, ~InteractionItemFlags::surface);

        if (info.type == InteractionItem::noInteraction)
        {
            return {};
        }

        const auto minPosition = info.pos;                  // E40128/A
        const auto maxPosition = info.pos + Pos2{ 31, 31 }; // E4012C/E
        auto mapPos = info.pos + Pos2{ 16, 16 };
        const auto initialVPPos = viewport->screenToViewport(screenCoords);

        for (int32_t i = 0; i < 5; i++)
        {
            const auto z = TileManager::getHeight(mapPos);
            mapPos = viewportCoordToMapCoord(initialVPPos.x, initialVPPos.y, z.landHeight, viewport->getRotation());
            mapPos.x = std::clamp(mapPos.x, minPosition.x, maxPosition.x);
            mapPos.y = std::clamp(mapPos.y, minPosition.y, maxPosition.y);
        }

        return { std::make_pair(mapPos, viewport) };
    }

    // 0x0045FE05
    // NOTE: Original call getSurfaceLocFromUi within this function
    // instead OpenLoco has split it in two. Also note that result of original
    // was a Pos2 start i.e. (& 0xFFE0) both components
    uint8_t getQuadrantFromPos(const Map::Pos2& loc)
    {
        const auto xNibble = loc.x & 0x1F;
        const auto yNibble = loc.y & 0x1F;
        if (xNibble > 16)
        {
            return (yNibble >= 16) ? 0 : 1;
        }
        else
        {
            return (yNibble >= 16) ? 3 : 2;
        }
    }

    // 0x0045FE4C
    // NOTE: Original call getSurfaceLocFromUi within this function
    // instead OpenLoco has split it in two. Also note that result of original
    // was a Pos2 start i.e. (& 0xFFE0) both components
    uint8_t getSideFromPos(const Map::Pos2& loc)
    {
        const auto xNibble = loc.x & 0x1F;
        const auto yNibble = loc.y & 0x1F;
        if (xNibble < yNibble)
        {
            return (xNibble + yNibble < 32) ? 0 : 1;
        }
        else
        {
            return (xNibble + yNibble < 32) ? 3 : 2;
        }
    }

    // 0x0045FD8E
    // NOTE: Original call getSurfaceLocFromUi within this function
    // instead OpenLoco has split it in two. Also note that result of original
    // was a Pos2 start i.e. (& 0xFFE0) both components
    uint8_t getQuadrantOrCentreFromPos(const Map::Pos2& loc)
    {
        // Determine to which quadrants the cursor is closest 4 == all quadrants
        const auto xNibbleCentre = std::abs((loc.x & 0xFFE0) + 16 - loc.x);
        const auto yNibbleCentre = std::abs((loc.y & 0xFFE0) + 16 - loc.y);
        if (std::max(xNibbleCentre, yNibbleCentre) <= 7)
        {
            // Is centre so all quadrants
            return 4;
        }

        return getQuadrantFromPos(loc);
    }
}
