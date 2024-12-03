#include "ViewportInteraction.h"
#include "Config.h"
#include "Entities/EntityManager.h"
#include "GameCommands/Airports/RemoveAirport.h"
#include "GameCommands/Buildings/RemoveBuilding.h"
#include "GameCommands/Company/RemoveCompanyHeadquarters.h"
#include "GameCommands/Docks/RemovePort.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Road/RemoveRoadMod.h"
#include "GameCommands/Road/RemoveRoadStation.h"
#include "GameCommands/Terraform/RemoveTree.h"
#include "GameCommands/Terraform/RemoveWall.h"
#include "GameCommands/Track/RemoveSignal.h"
#include "GameCommands/Track/RemoveTrackMod.h"
#include "GameCommands/Track/RemoveTrainStation.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Map/BuildingElement.h"
#include "Map/IndustryElement.h"
#include "Map/MapSelection.h"
#include "Map/RoadElement.h"
#include "Map/SignalElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Map/TreeElement.h"
#include "Map/WallElement.h"
#include "Objects/BuildingObject.h"
#include "Objects/CargoObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Objects/TreeObject.h"
#include "Objects/WallObject.h"
#include "Paint/Paint.h"
#include "SceneManager.h"
#include "Ui.h"
#include "Ui/ScrollView.h"
#include "Ui/ToolManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include "ViewportManager.h"
#include "Window.h"
#include "WindowManager.h"
#include "World/CompanyManager.h"
#include "World/IndustryManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::Ui::ViewportInteraction
{
    InteractionArg::InteractionArg(const Paint::PaintStruct& ps)
        : pos(ps.mapPos)
        , object(ps.entity)
        , type(ps.type)
        , modId(ps.modId)
    {
    }

    static bool getStationArguments(InteractionArg& interaction);
    static bool getStationArguments(StationId id);

    // 0x004CD95A
    static bool _track(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
        auto* track = tileElement->as<TrackElement>();
        if (track == nullptr)
        {
            return false;
        }
        if (!track->hasStationElement())
        {
            return false;
        }

        tileElement++;
        auto* station = tileElement->as<StationElement>();
        if (station == nullptr)
        {
            return false;
        }
        if (station->isAiAllocated())
        {
            return false;
        }

        interaction.type = InteractionItem::trainStation;
        interaction.object = station;
        return getStationArguments(interaction);
    }

    // 0x004CD974
    static bool _road(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
        auto* road = tileElement->as<RoadElement>();
        if (road == nullptr)
        {
            return false;
        }
        if (!road->hasStationElement())
        {
            return false;
        }

        World::StationElement* station = nullptr;
        World::Tile tile{ World::toTileSpace(interaction.pos), tileElement };
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
        if (station->isAiAllocated())
        {
            return false;
        }

        interaction.type = InteractionItem::roadStation;
        return getStationArguments(interaction);
    }

    // 0x004CD99A
    static bool getStationArguments(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
        auto* station = tileElement->as<StationElement>();
        if (station == nullptr)
        {
            return false;
        }
        if (station->isGhost())
        {
            return false;
        }

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

        World::setMapSelectionFlags(World::MapSelectionFlags::hoveringOverStation);
        ViewportManager::invalidate(station);
        Windows::MapToolTip::setOwner(station->owner);
        auto args = FormatArguments::mapToolTip(StringIds::stringid_stringid_wcolour3_stringid);
        args.push(station->name);
        args.push(station->town);
        args.push(getTransportIconsFromStationFlags(station->flags));
        char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_338));
        buffer = station->getStatusString(buffer);

        buffer = StringManager::formatString(buffer, StringIds::station_accepts);
        bool seperator = false; // First cargo item does not need a separator
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
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
        auto* industryTile = tileElement->as<IndustryElement>();
        if (industryTile == nullptr)
        {
            return false;
        }
        if (industryTile->isGhost())
        {
            return false;
        }

        interaction.value = enumValue(industryTile->industryId());
        auto industry = industryTile->industry();

        char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_338));
        *buffer = 0;
        industry->getStatusString(buffer);
        auto args = FormatArguments::mapToolTip();
        if (StringManager::locoStrlen(buffer) != 0)
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
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
        auto* buildingTile = tileElement->as<BuildingElement>();
        if (buildingTile == nullptr)
        {
            return false;
        }
        if (buildingTile->isGhost())
        {
            return false;
        }

        const auto index = buildingTile->sequenceIndex();
        const auto firstTile = interaction.pos - World::kOffsets[index];
        const World::Pos3 pos = { firstTile.x, firstTile.y, buildingTile->baseZ() };

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
            component.spriteLeft,
            component.spriteTop,
            component.spriteBottom,
            component.spriteRight
        };
        if (rect.contains(targetPosition))
        {
            uint32_t xDiff = std::abs(targetPosition.x - (component.spriteRight + component.spriteLeft) / 2);
            uint32_t yDiff = std::abs(targetPosition.y - (component.spriteTop + component.spriteBottom) / 2);
            return xDiff + yDiff;
        }
        return {};
    }

    static void checkAndSetNearestVehicle(uint32_t& nearestDistance, Vehicles::VehicleBase*& nearestVehicle, Vehicles::VehicleBase& checkVehicle, const viewport_pos& targetPosition)
    {
        if (checkVehicle.spriteLeft != Location::null)
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
        {
            return InteractionArg{};
        }

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

        // TODO: Rework so that getting the interaction arguments and getting the map tooltip format arguments are separated
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
            case InteractionItem::trainStation:
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
        {
            return InteractionArg{};
        }

        auto viewport = window->viewports[0];
        if (viewport == nullptr)
        {
            return InteractionArg{};
        }

        if (viewport->zoom > Config::get().old.vehiclesMinScale)
        {
            return InteractionArg{};
        }

        uint32_t nearestDistance = std::numeric_limits<uint32_t>().max();
        Vehicles::VehicleBase* nearestVehicle = nullptr;
        auto targetPosition = viewport->screenToViewport({ tempX, tempY });

        for (auto* v : VehicleManager::VehicleList())
        {
            auto train = Vehicles::Vehicle(*v);
            checkAndSetNearestVehicle(nearestDistance, nearestVehicle, *train.veh2, targetPosition);
            train.cars.applyToComponents([&nearestDistance, &nearestVehicle, &targetPosition](auto& component) {
                checkAndSetNearestVehicle(nearestDistance, nearestVehicle, component, targetPosition);
            });
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
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
        auto* track = tileElement->as<TrackElement>();
        if (track == nullptr)
        {
            return false;
        }

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

        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
        auto* track = tileElement->as<TrackElement>();
        if (track == nullptr)
        {
            return false;
        }

        if (track->isGhost())
        {
            return false;
        }

        if (!track->hasMod(interaction.modId))
        {
            return rightOverTrack(interaction);
        }
        auto* trackObj = ObjectManager::get<TrackObject>(track->trackObjectId());
        auto* trackExtraObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[interaction.modId]);
        FormatArguments::mapToolTip(StringIds::stringid_right_click_to_remove, trackExtraObj->name);
        return true;
    }

    // 0x004CDBEA
    static bool rightOverSignal(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
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
    static bool rightOverTrainStation(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
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
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
        auto* road = tileElement->as<RoadElement>();
        if (road == nullptr)
        {
            return false;
        }

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

        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
        auto* road = tileElement->as<RoadElement>();
        if (road == nullptr)
        {
            return false;
        }

        if (road->isGhost())
        {
            return false;
        }

        if (!road->hasMod(interaction.modId))
        {
            return rightOverRoad(interaction);
        }
        auto* roadObj = ObjectManager::get<RoadObject>(road->roadObjectId());
        auto* roadExtraObj = ObjectManager::get<TrackExtraObject>(roadObj->mods[interaction.modId]);
        FormatArguments::mapToolTip(StringIds::stringid_right_click_to_remove, roadExtraObj->name);
        return true;
    }

    // 0x004CDDF2
    static bool rightOverRoadStation(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
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
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
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
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
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
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
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
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
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
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
        auto* building = tileElement->as<BuildingElement>();
        if (building == nullptr)
        {
            return false;
        }

        const auto* buildingObj = building->getObject();
        auto args = FormatArguments::mapToolTip();
        if (isEditorMode() || isSandboxMode() || !buildingObj->hasFlags(BuildingObjectFlags::indestructible))
        {
            args.push(StringIds::stringid_right_click_to_remove);
        }
        args.push(buildingObj->name);
        return true;
    }

    // 0x004CE107
    static bool rightOverHeadquarters(InteractionArg& interaction)
    {
        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
        auto* building = tileElement->as<BuildingElement>();
        if (building == nullptr)
        {
            return false;
        }

        auto firstTile = interaction.pos - World::kOffsets[building->sequenceIndex()];
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

    constexpr std::array<StringId, 7> quantityToString = {
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

        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
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
                        auto* cargo = ObjectManager::get<CargoObject>(buildingObj->requiredCargoType[i]);
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
        {
            return InteractionArg{};
        }

        // Interaction types to exclude by default
        auto interactionsToExclude = InteractionItemFlags::none | InteractionItemFlags::surface | InteractionItemFlags::water;

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
                    if (vp->hasFlags(ViewportFlags::seeThroughBuildings))
                    {
                        interactionsToExclude |= InteractionItemFlags::building | InteractionItemFlags::headquarterBuilding | InteractionItemFlags::industry;
                    }
                    if (vp->hasFlags(ViewportFlags::seeThroughBridges))
                    {
                        // Bridges can't be interacted with ever so no need to exclude anything additional
                    }
                    if (vp->hasFlags(ViewportFlags::seeThroughTrees))
                    {
                        interactionsToExclude |= InteractionItemFlags::tree;
                    }
                    if (vp->hasFlags(ViewportFlags::seeThroughScenery))
                    {
                        interactionsToExclude |= InteractionItemFlags::wall;
                    }
                    if (vp->hasFlags(ViewportFlags::seeThroughTracks))
                    {
                        interactionsToExclude |= InteractionItemFlags::track | InteractionItemFlags::trackExtra | InteractionItemFlags::signal;
                    }
                    if (vp->hasFlags(ViewportFlags::seeThroughRoads))
                    {
                        interactionsToExclude |= InteractionItemFlags::roadAndTram | InteractionItemFlags::roadAndTramExtra;
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
            case InteractionItem::trainStation:
                hasInteraction = rightOverTrainStation(interaction);
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

    // 0x004A5AA1
    static void rightReleasedSignal(World::SignalElement* signal, const bool isLeftSignal, const World::Pos2 pos)
    {
        auto* track = signal->prev()->as<World::TrackElement>();
        if (track == nullptr)
        {
            return;
        }

        uint16_t unkFlags = 1 << 15; // right
        if (isLeftSignal)
        {
            unkFlags = 1 << 14; // left
        }

        // If in construction mode with both directions selection (actually does not single direction but this is what is implied)
        if (!ToolManager::isToolActive(WindowType::construction, 0, 11 /* Ui::Windows::Construction::Signal::widx::signal_direction */))
        {
            if (signal->getLeft().hasSignal() && signal->getRight().hasSignal())
            {
                unkFlags = (1 << 15) | (1 << 14); // both
            }
        }

        GameCommands::SignalRemovalArgs args;
        args.pos = Pos3(pos.x, pos.y, track->baseHeight());
        args.rotation = track->rotation();
        args.trackId = track->trackId();
        args.index = track->sequenceIndex();
        args.trackObjType = track->trackObjectId();
        args.flags = unkFlags;

        auto* window = WindowManager::find(WindowType::construction);
        if (window != nullptr)
        {
            Ui::Windows::Construction::removeConstructionGhosts();
        }

        GameCommands::setErrorTitle(StringIds::cant_remove_signal);
        if (GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE)
        {
            Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
        }
    }

    // 0x004A5B66
    static void rightReleasedTrainStation(World::StationElement* station, const World::Pos2 pos)
    {
        auto* track = station->prev()->as<World::TrackElement>();
        if (track == nullptr)
        {
            return;
        }

        GameCommands::setErrorTitle(StringIds::cant_remove_station);
        GameCommands::TrainStationRemovalArgs args;
        args.pos = Pos3(pos.x, pos.y, track->baseHeight());
        args.rotation = track->rotation();
        args.trackId = track->trackId();
        args.index = track->sequenceIndex();
        args.type = track->trackObjectId();
        if (GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE)
        {
            Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
        }
    }

    // 0x004A5BDF
    static void rightReleasedRoadStation(World::StationElement* station, const World::Pos2 pos)
    {
        auto* road = station->prev()->as<RoadElement>();
        if (road == nullptr)
        {
            return;
        }

        GameCommands::setErrorTitle(StringIds::cant_remove_station);
        GameCommands::RoadStationRemovalArgs args;
        args.pos = Pos3(pos.x, pos.y, road->baseHeight());
        args.rotation = road->rotation();
        args.roadId = road->roadId();
        args.index = road->sequenceIndex();
        args.roadObjectId = road->roadObjectId();
        if (GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE)
        {
            Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
        }
    }

    // 0x004A5C58
    static void rightReleasedAirport(World::StationElement* station, const World::Pos2 pos)
    {
        if (!Ui::Windows::Construction::isStationTabOpen())
        {
            Ui::Windows::Construction::openWithFlags(1ULL << 31);
            return;
        }
        GameCommands::setErrorTitle(StringIds::cant_remove_airport);
        GameCommands::AirportRemovalArgs args;
        args.pos = Pos3(pos.x, pos.y, station->baseHeight());
        if (GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE)
        {
            Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
        }
    }

    // 0x004A5CC5
    static void rightReleasedDock(World::StationElement* station, const World::Pos2 pos)
    {
        if (!Ui::Windows::Construction::isStationTabOpen())
        {
            Ui::Windows::Construction::openWithFlags(1ULL << 30);
            return;
        }
        GameCommands::setErrorTitle(StringIds::cant_remove_ship_port);
        GameCommands::PortRemovalArgs args;
        Pos2 firstTile = pos - World::kOffsets[station->sequenceIndex()];
        args.pos = Pos3(firstTile.x, firstTile.y, station->baseHeight());
        if (GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE)
        {
            Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
        }
    }

    // 0x004BB116
    static void rightReleasedTree(World::TreeElement* tree, const World::Pos2 pos)
    {
        GameCommands::setErrorTitle(StringIds::error_cant_remove_this);
        GameCommands::TreeRemovalArgs args;
        args.pos = Pos3(pos.x, pos.y, tree->baseHeight());
        args.elementType = tree->rawData()[0];
        args.type = tree->treeObjectId();
        GameCommands::doCommand(args, GameCommands::Flags::apply);
    }

    // 0x0042D9BF
    static void rightReleasedBuilding(World::BuildingElement* building, const World::Pos2 pos)
    {
        GameCommands::setErrorTitle(StringIds::error_cant_remove_this);
        GameCommands::BuildingRemovalArgs args;
        Pos2 firstTile = pos - World::kOffsets[building->sequenceIndex()];
        args.pos = Pos3(firstTile.x, firstTile.y, building->baseHeight());
        GameCommands::doCommand(args, GameCommands::Flags::apply);
    }

    // 0x004C4809
    static void rightReleasedWall(World::WallElement* wall, const World::Pos2 pos)
    {
        GameCommands::setErrorTitle(StringIds::error_cant_remove_this);
        GameCommands::WallRemovalArgs args;
        args.pos = Pos3(pos.x, pos.y, wall->baseHeight());
        args.rotation = wall->rotation();
        GameCommands::doCommand(args, GameCommands::Flags::apply);
    }

    // 0x0042F007
    static void rightReleasedHeadquarter(World::BuildingElement* building, const World::Pos2 pos)
    {
        GameCommands::setErrorTitle(StringIds::error_cant_remove_this);
        GameCommands::HeadquarterRemovalArgs args;
        Pos2 firstTile = pos - World::kOffsets[building->sequenceIndex()];
        args.pos = Pos3(firstTile.x, firstTile.y, building->baseHeight());
        GameCommands::doCommand(args, GameCommands::Flags::apply);
    }

    // 0x004A1303
    static void rightReleasedTrackExtra(TrackElement* track, const uint8_t bh, const Pos2 pos)
    {
        auto* window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            Windows::Construction::removeConstructionGhosts();
        }

        GameCommands::TrackModsRemovalArgs args{};
        args.pos = World::Pos3(pos, track->baseHeight());
        args.rotation = track->rotation();
        args.trackId = track->trackId();
        args.index = track->sequenceIndex();
        args.trackObjType = track->trackObjectId();
        args.type = 1U << bh;
        args.modSection = Windows::Construction::getLastSelectedTrackModSection();

        auto* trackObj = ObjectManager::get<TrackObject>(args.trackObjType);
        auto* trackExtraObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[bh]);
        auto fArgs = FormatArguments::common();
        fArgs.skip(6);
        fArgs.push(trackExtraObj->name);
        GameCommands::setErrorTitle(StringIds::cant_remove_pop3_string);

        if (GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE)
        {
            Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
        }
    }

    // 0x004A13C1
    static void rightReleaseRoadExtra(RoadElement* road, const uint8_t bh, const Pos2 pos)
    {
        auto* window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            Windows::Construction::removeConstructionGhosts();
        }

        GameCommands::RoadModsRemovalArgs args{};
        args.pos = World::Pos3(pos, road->baseHeight());
        args.rotation = road->rotation();
        args.roadId = road->roadId();
        args.index = road->sequenceIndex();
        args.roadObjType = road->roadObjectId();
        args.type = 1U << bh;
        args.modSection = Windows::Construction::getLastSelectedTrackModSection();

        auto* roadObj = ObjectManager::get<RoadObject>(args.roadObjType);
        auto* roadExtraObj = ObjectManager::get<RoadExtraObject>(roadObj->mods[bh]);
        auto fArgs = FormatArguments::common();
        fArgs.skip(6);
        fArgs.push(roadExtraObj->name);
        GameCommands::setErrorTitle(StringIds::cant_remove_pop3_string);

        if (GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE)
        {
            Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
        }
    }

    void handleRightReleased(Window* window, int16_t xPos, int16_t yPos)
    {
        auto interaction = ViewportInteraction::rightOver(xPos, yPos);

        auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);

        switch (interaction.type)
        {
            case InteractionItem::noInteraction:
            default:
            {
                auto item2 = ViewportInteraction::getItemLeft(xPos, yPos);
                switch (item2.type)
                {
                    case InteractionItem::entity:
                    {
                        auto* entity = reinterpret_cast<EntityBase*>(item2.object);
                        auto* veh = entity->asBase<Vehicles::VehicleBase>();
                        if (veh != nullptr)
                        {
                            auto* head = EntityManager::get<Vehicles::VehicleHead>(veh->getHead());
                            if (head != nullptr)
                            {
                                Ui::Windows::VehicleList::open(head->owner, head->vehicleType);
                            }
                        }
                        break;
                    }
                    case InteractionItem::townLabel:
                        Ui::Windows::TownList::open();
                        break;
                    case InteractionItem::stationLabel:
                    {
                        auto station = StationManager::get(StationId(item2.value));
                        Ui::Windows::StationList::open(station->owner);
                        break;
                    }
                    case InteractionItem::industry:
                        Ui::Windows::IndustryList::open();
                        break;
                    default:
                        break;
                }

                break;
            }

            case InteractionItem::track:
            {
                auto* track = tileElement->as<TrackElement>();
                if (track != nullptr)
                {
                    if (track->owner() == CompanyManager::getControllingId())
                    {
                        Ui::Windows::Construction::openAtTrack(*window, track, interaction.pos);
                    }
                    else
                    {
                        Ui::Windows::CompanyWindow::open(track->owner());
                    }
                }
                break;
            }
            case InteractionItem::road:
            {
                auto* road = tileElement->as<RoadElement>();
                if (road != nullptr)
                {
                    auto owner = road->owner();

                    auto roadObject = ObjectManager::get<RoadObject>(road->roadObjectId());
                    if (owner == CompanyManager::getControllingId() || owner == CompanyId::neutral || roadObject->hasFlags(RoadObjectFlags::unk_03))
                    {
                        Ui::Windows::Construction::openAtRoad(*window, road, interaction.pos);
                    }
                    else
                    {
                        Ui::Windows::CompanyWindow::open(owner);
                    }
                }
                break;
            }
            case InteractionItem::trackExtra:
            {
                auto* track = tileElement->as<TrackElement>();
                if (track != nullptr)
                {
                    rightReleasedTrackExtra(track, interaction.modId, interaction.pos);
                }
                break;
            }
            case InteractionItem::roadExtra:
            {
                auto* road = tileElement->as<RoadElement>();
                if (road != nullptr)
                {
                    rightReleaseRoadExtra(road, interaction.modId, interaction.pos);
                }
                break;
            }
            case InteractionItem::signal:
            {
                auto* signal = tileElement->as<SignalElement>();
                if (signal != nullptr)
                {
                    rightReleasedSignal(signal, interaction.modId != 0, interaction.pos);
                }
                break;
            }
            case InteractionItem::trainStation:
            {
                auto* station = tileElement->as<StationElement>();
                if (station != nullptr)
                {
                    rightReleasedTrainStation(station, interaction.pos);
                }
                break;
            }
            case InteractionItem::roadStation:
            {
                auto* station = tileElement->as<StationElement>();
                if (station != nullptr)
                {
                    rightReleasedRoadStation(station, interaction.pos);
                }
                break;
            }
            case InteractionItem::airport:
            {
                auto* station = tileElement->as<StationElement>();
                if (station != nullptr)
                {
                    rightReleasedAirport(station, interaction.pos);
                }
                break;
            }
            case InteractionItem::dock:
            {
                auto* station = tileElement->as<StationElement>();
                if (station != nullptr)
                {
                    rightReleasedDock(station, interaction.pos);
                }
                break;
            }
            case InteractionItem::tree:
            {
                auto* tree = tileElement->as<TreeElement>();
                if (tree != nullptr)
                {
                    rightReleasedTree(tree, interaction.pos);
                }
                break;
            }
            case InteractionItem::building:
            {
                auto* building = tileElement->as<BuildingElement>();
                if (building != nullptr)
                {
                    rightReleasedBuilding(building, interaction.pos);
                }
                break;
            }
            case InteractionItem::wall:
            {
                auto* wall = tileElement->as<WallElement>();
                if (wall != nullptr)
                {
                    rightReleasedWall(wall, interaction.pos);
                }
                break;
            }
            case InteractionItem::headquarterBuilding:
            {
                auto* building = tileElement->as<BuildingElement>();
                if (building != nullptr)
                {
                    rightReleasedHeadquarter(building, interaction.pos);
                }
                break;
            }
        }
    }

    // 0x00459E54
    std::pair<ViewportInteraction::InteractionArg, Viewport*> getMapCoordinatesFromPos(int32_t screenX, int32_t screenY, InteractionItemFlags flags)
    {
        static loco_global<uint8_t, 0x0050BF68> _50BF68; // If in get map coords
        static loco_global<Gfx::RenderTarget, 0x00E0C3E4> _rt1;
        static loco_global<Gfx::RenderTarget, 0x00E0C3F4> _rt2;

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
            {
                continue;
            }

            if (!vp->containsUi({ screenPos.x, screenPos.y }))
            {
                continue;
            }

            chosenV = vp;
            auto vpPos = vp->screenToViewport({ screenPos.x, screenPos.y });
            _rt1->zoomLevel = vp->zoom;
            _rt1->x = (0xFFFF << vp->zoom) & vpPos.x;
            _rt1->y = (0xFFFF << vp->zoom) & vpPos.y;
            _rt2->x = _rt1->x;
            _rt2->y = _rt1->y;
            _rt2->width = 1;
            _rt2->height = 1;
            _rt2->zoomLevel = _rt1->zoomLevel;

            Paint::SessionOptions options{};
            options.rotation = vp->getRotation();
            options.viewFlags = vp->flags;
            // Todo: should this pass the cullHeight...

            auto session = Paint::PaintSession(_rt2, options);
            session.generate();
            session.arrangeStructs();
            interaction = session.getNormalInteractionInfo(flags);
            if (!vp->hasFlags(ViewportFlags::station_names_displayed))
            {
                if (_rt2->zoomLevel <= Config::get().old.stationNamesMinScale)
                {
                    auto stationInteraction = session.getStationNameInteractionInfo(flags);
                    if (stationInteraction.type != InteractionItem::noInteraction)
                    {
                        interaction = stationInteraction;
                    }
                }
            }
            if (!vp->hasFlags(ViewportFlags::town_names_displayed))
            {
                auto townInteraction = session.getTownNameInteractionInfo(flags);
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
            waterHeight = surface->waterHeight();
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
        [[maybe_unused]] uint32_t closestEdge = World::getSideFromPos(mapPos); // ecx

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
}
