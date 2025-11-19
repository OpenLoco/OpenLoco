#include "CompanyAi.h"
#include "CompanyAiPathfinding.h"
#include "CompanyAiPlaceVehicle.h"
#include "Date.h"
#include "Economy/Economy.h"
#include "GameCommands/Airports/CreateAirport.h"
#include "GameCommands/Airports/RemoveAirport.h"
#include "GameCommands/Company/BuildCompanyHeadquarters.h"
#include "GameCommands/Company/RemoveCompanyHeadquarters.h"
#include "GameCommands/CompanyAi/AiCreateRoadAndStation.h"
#include "GameCommands/CompanyAi/AiCreateTrackAndStation.h"
#include "GameCommands/CompanyAi/AiTrackReplacement.h"
#include "GameCommands/Docks/CreatePort.h"
#include "GameCommands/Docks/RemovePort.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Road/CreateRoad.h"
#include "GameCommands/Road/CreateRoadMod.h"
#include "GameCommands/Road/CreateRoadStation.h"
#include "GameCommands/Road/RemoveRoad.h"
#include "GameCommands/Road/RemoveRoadStation.h"
#include "GameCommands/Track/CreateSignal.h"
#include "GameCommands/Track/CreateTrackMod.h"
#include "GameCommands/Track/RemoveTrack.h"
#include "GameCommands/Track/RemoveTrainStation.h"
#include "GameCommands/Vehicles/CreateVehicle.h"
#include "GameCommands/Vehicles/VehicleChangeRunningMode.h"
#include "GameCommands/Vehicles/VehicleOrderInsert.h"
#include "GameCommands/Vehicles/VehicleOrderSkip.h"
#include "GameCommands/Vehicles/VehiclePickup.h"
#include "GameCommands/Vehicles/VehiclePickupAir.h"
#include "GameCommands/Vehicles/VehiclePickupWater.h"
#include "GameCommands/Vehicles/VehicleRefit.h"
#include "GameCommands/Vehicles/VehicleSell.h"
#include "GameState.h"
#include "Map/BuildingElement.h"
#include "Map/IndustryElement.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Map/TreeElement.h"
#include "MessageManager.h"
#include "Objects/BridgeObject.h"
#include "Objects/BuildingObject.h"
#include "Objects/CargoObject.h"
#include "Objects/CompetitorObject.h"
#include "Objects/DockObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/ObjectUtils.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Objects/RoadStationObject.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Objects/TrainSignalObject.h"
#include "Objects/TrainStationObject.h"
#include "Objects/TreeObject.h"
#include "Random.h"
#include "Vehicles/Orders.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include "World/Company.h"
#include "World/CompanyManager.h"
#include "World/Industry.h"
#include "World/IndustryManager.h"
#include "World/Station.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <bit>
#include <numeric>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;
using namespace OpenLoco::Literals;
using namespace OpenLoco::CompanyAi; // Eventually this will all be under this namespace

namespace OpenLoco
{
    static void removeEntityFromThought(AiThought& thought, size_t index);

    enum class ThoughtTypeFlags : uint32_t
    {
        none = 0U,

        singleDestination = 1U << 0, // I.e. could be all based in one town
        destinationAIsIndustry = 1U << 1,
        destinationBIsIndustry = 1U << 2,
        railBased = 1U << 3,
        tramBased = 1U << 4,
        roadBased = 1U << 5, // But not tram
        unk6 = 1U << 6,      // Circular track - 4 stations
        unk7 = 1U << 7,
        unk8 = 1U << 8,
        unk9 = 1U << 9, // Tunnel (unused)
        unk10 = 1U << 10,
        unk11 = 1U << 11,
        unk12 = 1U << 12,
        unk13 = 1U << 13,
        unk14 = 1U << 14,
        airBased = 1U << 15,
        waterBased = 1U << 16,
        unk17 = 1U << 17,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(ThoughtTypeFlags);

    // 0x004FE720
    static constexpr std::array<ThoughtTypeFlags, kAiThoughtTypeCount> kThoughtTypeFlags = {
        ThoughtTypeFlags::railBased | ThoughtTypeFlags::singleDestination | ThoughtTypeFlags::unk6 | ThoughtTypeFlags::unk11,
        ThoughtTypeFlags::tramBased | ThoughtTypeFlags::singleDestination | ThoughtTypeFlags::unk14,
        ThoughtTypeFlags::tramBased | ThoughtTypeFlags::singleDestination | ThoughtTypeFlags::unk6 | ThoughtTypeFlags::unk14,
        ThoughtTypeFlags::railBased | ThoughtTypeFlags::unk11,
        ThoughtTypeFlags::railBased | ThoughtTypeFlags::unk11 | ThoughtTypeFlags::unk17,
        ThoughtTypeFlags::roadBased | ThoughtTypeFlags::singleDestination | ThoughtTypeFlags::unk10 | ThoughtTypeFlags::unk12,
        ThoughtTypeFlags::roadBased | ThoughtTypeFlags::unk8 | ThoughtTypeFlags::unk12,
        ThoughtTypeFlags::railBased | ThoughtTypeFlags::destinationAIsIndustry | ThoughtTypeFlags::destinationBIsIndustry | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::unk8 | ThoughtTypeFlags::unk11,
        ThoughtTypeFlags::railBased | ThoughtTypeFlags::destinationAIsIndustry | ThoughtTypeFlags::destinationBIsIndustry | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::unk8 | ThoughtTypeFlags::unk11 | ThoughtTypeFlags::unk17,
        ThoughtTypeFlags::railBased | ThoughtTypeFlags::destinationAIsIndustry | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::unk8 | ThoughtTypeFlags::unk11,
        ThoughtTypeFlags::railBased | ThoughtTypeFlags::destinationAIsIndustry | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::unk8 | ThoughtTypeFlags::unk11 | ThoughtTypeFlags::unk17,
        ThoughtTypeFlags::roadBased | ThoughtTypeFlags::destinationAIsIndustry | ThoughtTypeFlags::destinationBIsIndustry | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::unk8 | ThoughtTypeFlags::unk13,
        ThoughtTypeFlags::roadBased | ThoughtTypeFlags::destinationAIsIndustry | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::unk8 | ThoughtTypeFlags::unk13,
        ThoughtTypeFlags::airBased,
        ThoughtTypeFlags::airBased | ThoughtTypeFlags::destinationAIsIndustry | ThoughtTypeFlags::unk7,
        ThoughtTypeFlags::waterBased,
        ThoughtTypeFlags::waterBased | ThoughtTypeFlags::destinationAIsIndustry | ThoughtTypeFlags::destinationBIsIndustry | ThoughtTypeFlags::unk7,
        ThoughtTypeFlags::waterBased | ThoughtTypeFlags::destinationAIsIndustry | ThoughtTypeFlags::unk7,
        ThoughtTypeFlags::railBased | ThoughtTypeFlags::destinationAIsIndustry | ThoughtTypeFlags::unk11,
        ThoughtTypeFlags::railBased | ThoughtTypeFlags::destinationAIsIndustry | ThoughtTypeFlags::unk11 | ThoughtTypeFlags::unk17,
    };

    // 0x004FE770
    static constexpr std::array<uint8_t, kAiThoughtTypeCount> kThoughtTypeNumStations = {
        4,
        2,
        4,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
    };

    struct ThoughtMinMaxVehicles
    {
        uint8_t min;
        uint8_t max;
    };
    // 0x004FE784 & 0x004FE785
    static constexpr auto kThoughtTypeMinMaxNumVehicles = std::to_array<ThoughtMinMaxVehicles>({
        { 1, 3 },
        { 1, 3 },
        { 2, 6 },
        { 1, 1 },
        { 2, 5 },
        { 1, 3 },
        { 2, 5 },
        { 1, 1 },
        { 2, 5 },
        { 1, 1 },
        { 2, 5 },
        { 2, 5 },
        { 2, 5 },
        { 1, 3 },
        { 1, 2 },
        { 1, 3 },
        { 1, 4 },
        { 1, 3 },
        { 1, 1 },
        { 2, 5 },
    });
    static_assert(std::size(kThoughtTypeMinMaxNumVehicles) == kAiThoughtTypeCount);

    static bool thoughtTypeHasFlags(AiThoughtType type, ThoughtTypeFlags flags)
    {
        return (kThoughtTypeFlags[enumValue(type)] & flags) != ThoughtTypeFlags::none;
    }

    struct DestinationPositions
    {
        Pos2 posA;
        std::optional<Pos2> posB;
    };
    static DestinationPositions getDestinationPositions(const AiThought& thought)
    {
        DestinationPositions destPos;
        destPos.posA = thought.getDestinationPositionA();
        if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::singleDestination))
        {
            destPos.posB = thought.getDestinationPositionB();
        }
        return destPos;
    }

    // 0x00483778
    static void clearThought(AiThought& thought)
    {
        thought.type = AiThoughtType::null;
    }

    // 0x00487144
    static void sub_487144(Company& company)
    {
        company.var_85C2 = 0;
        company.var_85C3 = 0;
        company.var_85F0 = 0;
    }

    enum class PurchaseVehicleResult
    {
        success = 0, // But maybe not all required purchases
        allVehiclesPurchased = 1,
        failure = 2
    };

    // 0x00486ECF
    static PurchaseVehicleResult purchaseVehicle(Company& company, AiThought& thought)
    {
        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            return PurchaseVehicleResult::failure;
        }

        if (thought.numVehicles >= thought.var_43)
        {
            return PurchaseVehicleResult::allVehiclesPurchased;
        }

        EntityId trainHeadId = EntityId::null;
        for (auto i = 0; i < thought.var_45; ++i)
        {
            GameCommands::VehicleCreateArgs createArgs{};
            createArgs.vehicleId = trainHeadId;
            createArgs.vehicleType = thought.var_46[i];
            auto res = GameCommands::doCommand(createArgs, GameCommands::Flags::apply);
            if (res == GameCommands::FAILURE)
            {
                // If we can't create a vehicle try create a free vehicle!
                res = GameCommands::doCommand(createArgs, GameCommands::Flags::apply | GameCommands::Flags::noPayment);
                if (res == GameCommands::FAILURE)
                {
                    return PurchaseVehicleResult::failure;
                }
            }
            if (trainHeadId == EntityId::null)
            {
                trainHeadId = GameCommands::getLegacyReturnState().lastCreatedVehicleId;
            }
            // There was some broken code that would try read the head as a body here

            // auto* veh = EntityManager::get<Vehicles::VehicleBogie>(GameCommands::getLegacyReturnState().lastCreatedVehicleId); lol no this wouldn't work
            // auto train = Vehicles::Vehicle(veh->head);
            // auto car = [&train, veh]() {
            //     for (auto& car : train.cars)
            //     {
            //         if (car.front == veh)
            //         {
            //             return car;
            //         }
            //     }
            //     return Vehicles::Car{};
            // }();
            // if (car.front->secondaryCargo.maxQty != 0)
            //{
            //     if (car.front->secondaryCargo.acceptedTypes & (1U << thought.cargoType))
            //     {
            //         car.front->secondaryCargo.type = thought.cargoType;
            //     }
            // }
            // if (car.body->primaryCargo.maxQty != 0)
            //{
            //     if (car.body->primaryCargo.acceptedTypes & (1U << thought.cargoType))
            //     {
            //         car.body->primaryCargo.type = thought.cargoType;
            //     }
            // }
        }

        thought.vehicles[thought.numVehicles] = trainHeadId;
        thought.numVehicles++;

        auto train = Vehicles::Vehicle(trainHeadId);
        train.head->aiThoughtId = company.activeThoughtId;

        auto* vehicleObj = ObjectManager::get<VehicleObject>(train.cars.firstCar.front->objectId);
        if (vehicleObj->hasFlags(VehicleObjectFlags::refittable))
        {
            auto shouldRefit = [&vehicleObj, &thought]() {
                for (auto j = 0; j < 2; ++j)
                {
                    if (vehicleObj->maxCargo[j] != 0 && (vehicleObj->compatibleCargoCategories[j] & (1U << thought.cargoType)))
                    {
                        return false;
                    }
                }
                return true;
            }();
            if (shouldRefit)
            {
                auto* cargoObj = ObjectManager::get<CargoObject>(thought.cargoType);
                if (cargoObj->hasFlags(CargoObjectFlags::refit))
                {
                    GameCommands::VehicleRefitArgs refitArgs{};
                    refitArgs.head = trainHeadId;
                    refitArgs.cargoType = thought.cargoType;
                    GameCommands::doCommand(refitArgs, GameCommands::Flags::apply);
                }
            }
        }

        StationId destStation = StationId::null;
        uint32_t orderOffset = 0;
        for (auto i = 0; i < thought.numStations; ++i)
        {
            auto& aiStation = thought.stations[i];
            if (destStation == aiStation.id)
            {
                continue;
            }

            GameCommands::VehicleOrderInsertArgs insertArgs{};
            insertArgs.head = trainHeadId;
            insertArgs.orderOffset = orderOffset;
            Vehicles::OrderStopAt stopAt{ aiStation.id };
            destStation = aiStation.id;
            insertArgs.rawOrder = stopAt.getRaw();
            auto insertRes = GameCommands::doCommand(insertArgs, GameCommands::Flags::apply);
            // Fix vanilla bug
            if (insertRes != GameCommands::FAILURE)
            {
                orderOffset += sizeof(stopAt);
            }
            if (i != 0)
            {
                continue;
            }
            // Potential vanilla issue below it checks for 1ULL << 11 here 1ULL << 7
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk7)
                && thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationAIsIndustry))
            {
                GameCommands::VehicleOrderInsertArgs insertArgs2{};
                insertArgs2.head = trainHeadId;
                insertArgs2.orderOffset = orderOffset;
                Vehicles::OrderWaitFor waitFor{ thought.cargoType };
                insertArgs2.rawOrder = waitFor.getRaw();
                auto insert2Res = GameCommands::doCommand(insertArgs2, GameCommands::Flags::apply);
                // Fix vanilla bug
                if (insert2Res != GameCommands::FAILURE)
                {
                    orderOffset += sizeof(waitFor);
                }
            }
        }
        if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationAIsIndustry))
        {
            return PurchaseVehicleResult::success;
        }
        if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk11))
        {
            return PurchaseVehicleResult::success;
        }
        if (thought.var_43 > 1)
        {
            // Why??
            GameCommands::VehicleOrderSkipArgs skipArgs{};
            skipArgs.head = trainHeadId;
            GameCommands::doCommand(skipArgs, GameCommands::Flags::apply);
            GameCommands::doCommand(skipArgs, GameCommands::Flags::apply);
        }
        return PurchaseVehicleResult::success;
    }

    // 0x004876CB
    static void sub_4876CB(AiThought& thought)
    {
        for (auto i = 0U; i < thought.numVehicles; ++i)
        {
            auto* head = EntityManager::get<Vehicles::VehicleHead>(thought.vehicles[i]);
            if (head == nullptr)
            {
                continue;
            }
            if (head->tileX != -1)
            {
                continue;
            }

            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased | ThoughtTypeFlags::waterBased))
            {
                head->aiPlacementPos = thought.stations[0].pos;
                head->aiPlacementBaseZ = thought.stations[0].baseZ;
                head->aiPlacementTaD = thought.stations[0].rotation; // For air and water this doesn't actually get used
            }
            else
            {
                head->aiPlacementPos = thought.stations[0].pos;
                head->aiPlacementBaseZ = thought.stations[0].baseZ;

                uint8_t tad = thought.stations[0].rotation | (0U << 3); // Always trackId/roadId 0 (straight)
                if (thought.trackObjId & (1U << 7))
                {
                    if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk6))
                    {
                        if (i & 0b1)
                        {
                            tad ^= (1U << 2);
                        }
                    }
                }
                head->aiPlacementTaD = tad;
            }
            head->breakdownFlags |= Vehicles::BreakdownFlags::breakdownPending;
            thought.var_88 = 0;
        }
    }

    // 0x00494805
    static void tryRemovePortsAndAirports(Company& company)
    {
        // If set the port/airport is not removed?
        std::array<uint8_t, Limits::kMaxStations> unkStationFlags{};
        for (auto& thought : company.aiThoughts)
        {
            if (thought.type == AiThoughtType::null)
            {
                continue;
            }

            for (auto i = 0; i < 4 && i < thought.numStations; ++i)
            {
                const auto& aiStation = thought.stations[i];
                if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased))
                {
                    unkStationFlags[enumValue(aiStation.id)] |= 1U << 0;
                }
                if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::waterBased))
                {
                    unkStationFlags[enumValue(aiStation.id)] |= 1U << 1;
                }
            }
        }

        for (auto& station : StationManager::stations())
        {
            if (station.owner != company.id())
            {
                continue;
            }

            if (((station.flags & StationFlags::transportModeAir) != StationFlags::none)
                && !(unkStationFlags[enumValue(station.id())] & (1U << 0)))
            {

                for (auto i = 0; i < station.stationTileSize; ++i)
                {
                    auto& tileLoc = station.stationTiles[i];

                    const auto tile = World::TileManager::get(tileLoc);
                    bool stationElFound = false;
                    for (auto& el : tile)
                    {
                        auto* elStation = el.as<World::StationElement>();
                        if (elStation == nullptr)
                        {
                            continue;
                        }

                        if (elStation->baseZ() != tileLoc.z / World::kSmallZStep)
                        {
                            continue;
                        }

                        if (elStation->stationType() != StationType::airport)
                        {
                            continue;
                        }
                        stationElFound = true;
                        break;
                    }

                    if (!stationElFound)
                    {
                        continue;
                    }

                    GameCommands::AirportRemovalArgs args{};
                    args.pos = tileLoc;
                    args.pos.z = World::heightFloor(args.pos.z);
                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    break;
                }
            }
            if (((station.flags & StationFlags::transportModeWater) != StationFlags::none)
                && !(unkStationFlags[enumValue(station.id())] & (1U << 1)))
            {

                for (auto i = 0; i < station.stationTileSize; ++i)
                {
                    auto& tileLoc = station.stationTiles[i];

                    const auto tile = World::TileManager::get(tileLoc);
                    bool stationElFound = false;
                    for (auto& el : tile)
                    {
                        auto* elStation = el.as<World::StationElement>();
                        if (elStation == nullptr)
                        {
                            continue;
                        }

                        if (elStation->baseZ() != tileLoc.z / World::kSmallZStep)
                        {
                            continue;
                        }

                        if (elStation->stationType() != StationType::docks)
                        {
                            continue;
                        }
                        stationElFound = true;
                        break;
                    }

                    if (!stationElFound)
                    {
                        continue;
                    }

                    GameCommands::PortRemovalArgs args{};
                    args.pos = tileLoc;
                    args.pos.z = World::heightFloor(args.pos.z);
                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    break;
                }
            }
        }
    }

    // 0x004308D4
    static void aiThinkState0(Company& company)
    {
        company.var_85F6++;
        if (company.var_85F6 < 672)
        {
            company.var_4A4 = AiThinkState::unk2;
            company.var_4A5 = 0;
            return;
        }

        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            bool hasAssets = false;
            for (auto& thought : company.aiThoughts)
            {
                if (thought.type != AiThoughtType::null)
                {
                    hasAssets = true;
                    break;
                }
            }
            if (!hasAssets)
            {
                for (auto& station : StationManager::stations())
                {
                    if (station.owner == company.id())
                    {
                        hasAssets = true;
                        break;
                    }
                }
            }
            if (!hasAssets)
            {
                company.var_4A4 = AiThinkState::endCompany;
                company.var_85C4 = World::Pos2{ 0, 0 };
                return;
            }
        }

        company.var_85F6 = 0;
        company.var_4A4 = AiThinkState::unk1;
        company.activeThoughtId = kAiThoughtIdNull;
        tryRemovePortsAndAirports(company);
    }

    // 0x00487F8D
    static bool sub_487F8D(const Company& company, const AiThought& thought)
    {
        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            return true;
        }
        if (thought.var_88 < 3)
        {
            return false;
        }
        // 27 / 8 ???
        const auto val = thought.var_7C * 3;
        const auto val2 = val + (val / 8);
        return thought.var_84 < val2;
    }

    struct VehiclePurchaseObjects
    {
        uint16_t cargoObjId;
        uint16_t frontObjId;
        uint16_t secondObjId;
    };

    // 0x00480CD5
    static std::optional<VehiclePurchaseObjects> getAirBasedIdealObjects(const Company& company, const AiThought& thought)
    {
        Speed16 bestSpeed = 0_mph;
        uint16_t bestDesignedYear = 0;
        uint16_t bestVehicleObjId = 0xFFFF;
        auto* cargoObj = ObjectManager::get<CargoObject>(thought.cargoType);
        for (auto i = 0U; i < Limits::kMaxVehicleObjects; ++i)
        {
            auto* vehicleObj = ObjectManager::get<VehicleObject>(i);
            if (vehicleObj == nullptr)
            {
                continue;
            }
            if (vehicleObj->mode != TransportMode::air)
            {
                continue;
            }
            if (vehicleObj->hasFlags(VehicleObjectFlags::aircraftIsHelicopter))
            {
                continue;
            }
            bool compatibleCargo = false;
            for (auto j = 0U; j < 2; ++j)
            {
                if (vehicleObj->maxCargo[j] != 0 && (vehicleObj->compatibleCargoCategories[j] & (1U << thought.cargoType)))
                {
                    compatibleCargo = true;
                    break;
                }
            }
            if (!compatibleCargo)
            {
                if (!vehicleObj->hasFlags(VehicleObjectFlags::refittable) || !cargoObj->hasFlags(CargoObjectFlags::refit))
                {
                    continue;
                }
            }
            if (!company.unlockedVehicles[i])
            {
                continue;
            }
            if (vehicleObj->speed >= bestSpeed)
            {
                if (vehicleObj->speed == bestSpeed)
                {
                    if (bestDesignedYear > vehicleObj->designed)
                    {
                        continue;
                    }
                }
                bestSpeed = vehicleObj->speed;
                bestDesignedYear = vehicleObj->designed;
                bestVehicleObjId = i;
            }
        }
        if (bestSpeed == 0_mph)
        {
            return std::nullopt;
        }
        return VehiclePurchaseObjects{ bestVehicleObjId, 0xFFFFU, 0xFFFFU };
    }

    // 0x00480DC6
    static std::optional<VehiclePurchaseObjects> getWaterBasedIdealObjects(const Company& company, const AiThought& thought)
    {
        Speed16 bestSpeed = 0_mph;
        uint16_t bestDesignedYear = 0;
        uint16_t bestVehicleObjId = 0xFFFF;
        auto* cargoObj = ObjectManager::get<CargoObject>(thought.cargoType);
        for (auto i = 0U; i < Limits::kMaxVehicleObjects; ++i)
        {
            auto* vehicleObj = ObjectManager::get<VehicleObject>(i);
            if (vehicleObj == nullptr)
            {
                continue;
            }
            if (vehicleObj->mode != TransportMode::water)
            {
                continue;
            }
            bool compatibleCargo = false;
            for (auto j = 0U; j < 2; ++j)
            {
                if (vehicleObj->maxCargo[j] != 0 && (vehicleObj->compatibleCargoCategories[j] & (1U << thought.cargoType)))
                {
                    compatibleCargo = true;
                    break;
                }
            }
            if (!compatibleCargo)
            {
                if (!vehicleObj->hasFlags(VehicleObjectFlags::refittable) || !cargoObj->hasFlags(CargoObjectFlags::refit))
                {
                    continue;
                }
            }
            if (!company.unlockedVehicles[i])
            {
                continue;
            }
            if (vehicleObj->speed >= bestSpeed)
            {
                if (vehicleObj->speed == bestSpeed)
                {
                    if (bestDesignedYear > vehicleObj->designed)
                    {
                        continue;
                    }
                }
                bestSpeed = vehicleObj->speed;
                bestDesignedYear = vehicleObj->designed;
                bestVehicleObjId = i;
            }
        }
        if (bestSpeed == 0_mph)
        {
            return std::nullopt;
        }
        return VehiclePurchaseObjects{ bestVehicleObjId, 0xFFFFU, 0xFFFFU };
    }

    // 0x004802F7
    static std::optional<VehiclePurchaseObjects> getTrackAndRoadIdealObjects(const Company& company, const AiThought& thought)
    {
        uint8_t unk112C5A6 = thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk7) ? 3 : 6;
        uint8_t trackType = thought.trackObjId;
        TransportMode mode = TransportMode::rail;
        if (trackType & (1U << 7))
        {
            trackType &= ~(1U << 7);
            mode = TransportMode::road;
            auto* roadObj = ObjectManager::get<RoadObject>(trackType);
            if (roadObj->hasFlags(RoadObjectFlags::unk_03))
            {
                trackType = 0xFFU;
            }
        }

        Speed16 bestSpeed = 0_mph;
        uint16_t bestDesignedYear = 0;
        uint16_t bestVehicleObjId = 0xFFFF;
        for (auto i = 0U; i < Limits::kMaxVehicleObjects; ++i)
        {
            auto* vehicleObj = ObjectManager::get<VehicleObject>(i);
            if (vehicleObj == nullptr)
            {
                continue;
            }

            if (vehicleObj->mode != mode)
            {
                continue;
            }

            if (vehicleObj->trackType != trackType)
            {
                continue;
            }

            bool compatibleCargo = false;
            for (auto j = 0U; j < 2; ++j)
            {
                if (vehicleObj->maxCargo[j] != 0 && (vehicleObj->compatibleCargoCategories[j] & (1U << thought.cargoType)))
                {
                    compatibleCargo = true;
                    break;
                }
            }

            if (!compatibleCargo)
            {
                continue;
            }

            auto speed = vehicleObj->speed;
            if (vehicleObj->power != 0)
            {
                speed -= 1_mph;
                if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk6))
                {
                    speed = (speed + 1_mph) * 4;
                    const auto speedRand = Speed16(gPrng1().randNext() & 0x3F);
                    speed += speedRand;
                }
            }

            if (!company.unlockedVehicles[i])
            {
                continue;
            }

            if (speed >= bestSpeed)
            {
                if (speed == bestSpeed)
                {
                    if (bestDesignedYear > vehicleObj->designed)
                    {
                        continue;
                    }
                }
                if (vehicleObj->power != 0 && !vehicleObj->hasFlags(VehicleObjectFlags::rackRail))
                {
                    if (thought.hasPurchaseFlags(AiPurchaseFlags::unk0))
                    {
                        continue;
                    }
                }
                bestSpeed = speed;
                bestDesignedYear = vehicleObj->designed;
                bestVehicleObjId = i;
            }
        }

        if (bestSpeed == 0_mph)
        {
            return std::nullopt;
        }
        VehiclePurchaseObjects chosenObjects{ 0xFFFFU, 0xFFFFU, 0xFFFFU };
        chosenObjects.cargoObjId = bestVehicleObjId;

        auto* cargoCarriageObj = ObjectManager::get<VehicleObject>(chosenObjects.cargoObjId);
        auto minSpeed = cargoCarriageObj->speed;
        if (cargoCarriageObj->power == 0)
        {
            bool longDistane = false;
            if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk6 | ThoughtTypeFlags::singleDestination))
            {
                const auto posA = thought.getDestinationPositionA();
                const auto posB = thought.getDestinationPositionB();
                const auto distance = Math::Vector::distance2D(posA, posB);
                longDistane = distance > 40 * 32;
            }

            if (longDistane)
            {
                // 0x004806A9
                int16_t bestScore = 0;
                uint16_t bestDesignedYearObj2 = 0;
                uint16_t bestVehicleObjIdObj2 = 0xFFFF;
                for (auto i = 0U; i < Limits::kMaxVehicleObjects; ++i)
                {
                    auto* vehicleObj = ObjectManager::get<VehicleObject>(i);
                    if (vehicleObj == nullptr)
                    {
                        continue;
                    }

                    if (vehicleObj->mode != mode)
                    {
                        continue;
                    }

                    if (vehicleObj->trackType != trackType)
                    {
                        continue;
                    }

                    if (vehicleObj->power == 0)
                    {
                        continue;
                    }

                    if (!Vehicles::canVehiclesCouple(chosenObjects.cargoObjId, i))
                    {
                        continue;
                    }

                    const auto adjustedPower = vehicleObj->power >> unk112C5A6;
                    const auto adjustedSpeed = std::min(minSpeed, vehicleObj->speed);
                    const auto speedRand = Speed16(gPrng1().randNext() & 0x3F);
                    auto score = (adjustedSpeed + speedRand).getRaw() + adjustedPower;
                    if (score < bestScore)
                    {
                        continue;
                    }
                    if (score == bestScore)
                    {
                        if (bestDesignedYearObj2 > vehicleObj->designed)
                        {
                            continue;
                        }
                    }

                    if (!company.unlockedVehicles[i])
                    {
                        continue;
                    }

                    if (vehicleObj->power != 0 && !vehicleObj->hasFlags(VehicleObjectFlags::rackRail))
                    {
                        if (thought.hasPurchaseFlags(AiPurchaseFlags::unk0))
                        {
                            continue;
                        }
                    }
                    bestScore = score;
                    bestDesignedYearObj2 = vehicleObj->designed;
                    bestVehicleObjIdObj2 = i;
                }

                if (bestScore == 0)
                {
                    return std::nullopt;
                }
                chosenObjects.frontObjId = bestVehicleObjIdObj2;
            }
            else
            {
                // 0x00480551
                int16_t bestScore = -32000;
                uint16_t bestDesignedYearObj2 = 0;
                uint16_t bestVehicleObjIdObj2 = 0xFFFF;
                for (auto i = 0U; i < Limits::kMaxVehicleObjects; ++i)
                {
                    auto* vehicleObj = ObjectManager::get<VehicleObject>(i);
                    if (vehicleObj == nullptr)
                    {
                        continue;
                    }

                    if (vehicleObj->mode != mode)
                    {
                        continue;
                    }

                    if (vehicleObj->trackType != trackType)
                    {
                        continue;
                    }

                    if (vehicleObj->power == 0)
                    {
                        continue;
                    }

                    if (!Vehicles::canVehiclesCouple(chosenObjects.cargoObjId, i))
                    {
                        continue;
                    }

                    auto adjustedPower = vehicleObj->power >> unk112C5A6;
                    auto adjustedSpeed = std::min(minSpeed, vehicleObj->speed);
                    auto speed = Speed16((adjustedPower + adjustedSpeed.getRaw()) / 2);
                    const auto speedRand = Speed16(gPrng1().randNext() & 0x3F);
                    speed += speedRand;
                    const auto score = static_cast<int16_t>(speed.getRaw()) - static_cast<int16_t>(vehicleObj->getLength());
                    if (score < bestScore)
                    {
                        continue;
                    }

                    if (score == bestScore)
                    {
                        if (bestDesignedYearObj2 > vehicleObj->designed)
                        {
                            continue;
                        }
                    }

                    if (!company.unlockedVehicles[i])
                    {
                        continue;
                    }

                    if (vehicleObj->power != 0 && !vehicleObj->hasFlags(VehicleObjectFlags::rackRail))
                    {
                        if (thought.hasPurchaseFlags(AiPurchaseFlags::unk0))
                        {
                            continue;
                        }
                    }
                    bestScore = score;
                    bestDesignedYearObj2 = vehicleObj->designed;
                    bestVehicleObjIdObj2 = i;
                }

                if (bestScore == -32000)
                {
                    return std::nullopt;
                }
                chosenObjects.frontObjId = bestVehicleObjIdObj2;
            }
        }
        // 0x004807E5
        auto requiresFurtherVehicle = [](uint16_t objId) {
            if (objId == 0xFFFFU)
            {
                return true;
            }
            auto* vehicleObj = ObjectManager::get<VehicleObject>(objId);
            if (vehicleObj == nullptr)
            {
                return true;
            }
            if (vehicleObj->hasFlags(VehicleObjectFlags::topAndTailPosition))
            {
                return false;
            }
            if (vehicleObj->power == 0)
            {
                return true;
            }
            return vehicleObj->hasFlags(VehicleObjectFlags::centerPosition);
        };
        if (requiresFurtherVehicle(chosenObjects.cargoObjId) && requiresFurtherVehicle(chosenObjects.frontObjId))
        {
            Speed16 bestScore = 0_mph;
            uint16_t bestDesignedYearObj3 = 0;
            uint16_t bestVehicleObjIdObj3 = 0xFFFF;
            for (auto i = 0U; i < Limits::kMaxVehicleObjects; ++i)
            {
                auto* vehicleObj = ObjectManager::get<VehicleObject>(i);
                if (vehicleObj == nullptr)
                {
                    continue;
                }

                if (vehicleObj->mode != mode)
                {
                    continue;
                }

                if (vehicleObj->trackType != trackType)
                {
                    continue;
                }

                if (!vehicleObj->hasFlags(VehicleObjectFlags::topAndTailPosition))
                {
                    continue;
                }

                if (!Vehicles::canVehiclesCouple(chosenObjects.cargoObjId, i))
                {
                    continue;
                }

                const auto score = vehicleObj->speed;
                if (score < bestScore)
                {
                    continue;
                }
                if (score == bestScore)
                {
                    if (bestDesignedYearObj3 > vehicleObj->designed)
                    {
                        continue;
                    }
                }

                if (!company.unlockedVehicles[i])
                {
                    continue;
                }

                if (vehicleObj->power != 0 && !vehicleObj->hasFlags(VehicleObjectFlags::rackRail))
                {
                    if (thought.hasPurchaseFlags(AiPurchaseFlags::unk0))
                    {
                        continue;
                    }
                }
                bestScore = score;
                bestDesignedYearObj3 = vehicleObj->designed;
                bestVehicleObjIdObj3 = i;
            }

            if (bestScore == 0_mph)
            {
                return std::nullopt;
            }
            chosenObjects.secondObjId = bestVehicleObjIdObj3;
        }

        if (chosenObjects.secondObjId != 0xFFFFU)
        {
            return chosenObjects;
        }

        auto isTopAndTailVehicle = [](uint16_t objId) {
            if (objId == 0xFFFFU)
            {
                return false;
            }
            auto* vehicleObj = ObjectManager::get<VehicleObject>(objId);
            if (vehicleObj == nullptr)
            {
                return false;
            }
            return !vehicleObj->hasFlags(VehicleObjectFlags::topAndTailPosition);
        };
        if (!isTopAndTailVehicle(chosenObjects.cargoObjId) && !isTopAndTailVehicle(chosenObjects.frontObjId))
        {
            Speed16 bestScore = 0_mph;
            uint16_t bestDesignedYearObj3 = 0;
            uint16_t bestVehicleObjIdObj3 = 0xFFFF;
            for (auto i = 0U; i < Limits::kMaxVehicleObjects; ++i)
            {
                auto* vehicleObj = ObjectManager::get<VehicleObject>(i);
                if (vehicleObj == nullptr)
                {
                    continue;
                }

                if (vehicleObj->mode != mode)
                {
                    continue;
                }

                if (vehicleObj->trackType != trackType)
                {
                    continue;
                }

                if (!vehicleObj->hasFlags(VehicleObjectFlags::topAndTailPosition))
                {
                    continue;
                }

                if (vehicleObj->power != 0)
                {
                    continue;
                }

                if (!Vehicles::canVehiclesCouple(chosenObjects.cargoObjId, i))
                {
                    continue;
                }

                bool compatibleCargo = false;
                for (auto j = 0U; j < 2; ++j)
                {
                    if (vehicleObj->maxCargo[j] != 0 && (vehicleObj->compatibleCargoCategories[j] & (1U << thought.cargoType)))
                    {
                        compatibleCargo = true;
                        break;
                    }
                }

                if (!compatibleCargo)
                {
                    continue;
                }

                const auto score = vehicleObj->speed;
                if (score < bestScore)
                {
                    continue;
                }
                if (score == bestScore)
                {
                    if (bestDesignedYearObj3 > vehicleObj->designed)
                    {
                        continue;
                    }
                }

                if (!company.unlockedVehicles[i])
                {
                    continue;
                }

                bestScore = score;
                bestDesignedYearObj3 = vehicleObj->designed;
                bestVehicleObjIdObj3 = i;
            }

            if (bestScore != 0_mph)
            {
                chosenObjects.secondObjId = bestVehicleObjIdObj3;
            }
        }

        return chosenObjects;
    }

    struct VehiclePurchaseRequest
    {
        uint8_t numVehicleObjects; // cl
        uint8_t dl;                // dl
        currency32_t trainRunCost; // ebx
        currency32_t trainCost;    // eax
    };

    // 0x004802D0
    static VehiclePurchaseRequest aiGenerateVehiclePurchaseRequest(const Company& company, AiThought& thought, uint16_t* requestBuffer)
    {
        sfl::static_vector<uint16_t, 16> requests;
        std::optional<VehiclePurchaseObjects> chosenObjects;
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased))
        {
            chosenObjects = getAirBasedIdealObjects(company, thought);
        }
        else if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::waterBased))
        {
            chosenObjects = getWaterBasedIdealObjects(company, thought);
        }
        else
        {
            chosenObjects = getTrackAndRoadIdealObjects(company, thought);
        }
        // 0x00480A74
        if (!chosenObjects.has_value())
        {
            return VehiclePurchaseRequest{};
        }

        // Build a train from the chosen objects

        int32_t targetLengthWorld = 0;
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::roadBased))
        {
            targetLengthWorld = 44;
        }
        else if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::tramBased))
        {
            targetLengthWorld = 64;
        }
        else if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::waterBased | ThoughtTypeFlags::airBased))
        {
            targetLengthWorld = 1;
        }
        else
        {
            targetLengthWorld = thought.stationLength * 32 - 2;
        }
        auto targetLength = targetLengthWorld * 4;
        currency32_t totalCost = 0;
        uint8_t numVehicleObjects = 0;
        if (chosenObjects->frontObjId != 0xFFFFU)
        {
            auto* vehObj = ObjectManager::get<VehicleObject>(chosenObjects->frontObjId);
            const auto length = vehObj->getLength();
            targetLength -= length;
            requests.push_back(chosenObjects->frontObjId);
            totalCost += Economy::getInflationAdjustedCost(vehObj->costFactor, vehObj->costIndex, 6);
            numVehicleObjects++;
            if (vehObj->hasFlags(VehicleObjectFlags::mustHavePair))
            {
                targetLength -= length;
                totalCost += Economy::getInflationAdjustedCost(vehObj->costFactor, vehObj->costIndex, 6);
                requests.push_back(chosenObjects->frontObjId);
                numVehicleObjects++;
            }
        }

        if (chosenObjects->secondObjId != 0xFFFFU)
        {
            auto* vehObj = ObjectManager::get<VehicleObject>(chosenObjects->secondObjId);
            const auto length = vehObj->getLength();
            targetLength -= length;
            requests.push_back(chosenObjects->secondObjId);
            totalCost += Economy::getInflationAdjustedCost(vehObj->costFactor, vehObj->costIndex, 6);
            numVehicleObjects++;
            if (vehObj->hasFlags(VehicleObjectFlags::mustHavePair))
            {
                targetLength -= length;
                totalCost += Economy::getInflationAdjustedCost(vehObj->costFactor, vehObj->costIndex, 6);
                requests.push_back(chosenObjects->secondObjId);
                numVehicleObjects++;
            }
        }

        const auto* vehObj = ObjectManager::get<VehicleObject>(chosenObjects->cargoObjId);
        const int32_t length = vehObj->getLength();
        targetLength -= length;
        requests.push_back(chosenObjects->cargoObjId);
        totalCost += Economy::getInflationAdjustedCost(vehObj->costFactor, vehObj->costIndex, 6);
        numVehicleObjects++;
        if (vehObj->hasFlags(VehicleObjectFlags::mustHavePair))
        {
            targetLength -= length;
            totalCost += Economy::getInflationAdjustedCost(vehObj->costFactor, vehObj->costIndex, 6);
            requests.push_back(chosenObjects->cargoObjId);
            numVehicleObjects++;
        }

        if (Vehicles::canVehiclesCouple(chosenObjects->cargoObjId, chosenObjects->cargoObjId))
        {
            while (targetLength - length >= 0)
            {
                targetLength -= length;
                if (numVehicleObjects >= 16)
                {
                    break;
                }
                if (vehObj->hasFlags(VehicleObjectFlags::mustHavePair))
                {
                    if (targetLength - length < 0)
                    {
                        break;
                    }
                    targetLength -= length;
                    if (numVehicleObjects >= 15)
                    {
                        break;
                    }
                }
                requests.push_back(chosenObjects->cargoObjId);
                totalCost += Economy::getInflationAdjustedCost(vehObj->costFactor, vehObj->costIndex, 6);
                numVehicleObjects++;
                if (vehObj->hasFlags(VehicleObjectFlags::mustHavePair))
                {
                    totalCost += Economy::getInflationAdjustedCost(vehObj->costFactor, vehObj->costIndex, 6);
                    requests.push_back(chosenObjects->cargoObjId);
                    numVehicleObjects++;
                }
            }
        }

        currency32_t totalRunCost = 0;
        for (auto requestObjId : requests)
        {
            *requestBuffer++ = requestObjId;
            auto* tempObj = ObjectManager::get<VehicleObject>(requestObjId);
            totalRunCost += Economy::getInflationAdjustedCost(tempObj->runCostFactor, tempObj->runCostIndex, 10);
        }

        return VehiclePurchaseRequest{ .numVehicleObjects = static_cast<uint8_t>(requests.size()), .dl = kThoughtTypeMinMaxNumVehicles[enumValue(thought.type)].min, .trainRunCost = totalRunCost, .trainCost = totalCost };
    }

    // 0x004883D4
    static int32_t getUntransportedQuantity(const AiThought& thought)
    {
        int32_t quantity = 0;
        for (auto i = 0U; i < thought.numStations; ++i)
        {
            auto& aiStation = thought.stations[i];
            auto* station = StationManager::get(aiStation.id);
            auto& cargoStats = station->cargoStats[thought.cargoType];
            quantity += cargoStats.quantity;
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk7))
            {
                break;
            }
        }
        if (thought.numVehicles == 0)
        {
            return quantity;
        }
        auto train = Vehicles::Vehicle(thought.vehicles[0]);
        for (const auto& car : train.cars)
        {
            auto* vehicleObj = ObjectManager::get<VehicleObject>(car.front->objectId);
            for (auto i = 0U; i < 2; ++i)
            {
                if (vehicleObj->compatibleCargoCategories[i] & (1U << thought.cargoType))
                {
                    quantity -= vehicleObj->maxCargo[i];
                }
            }
        }
        return quantity;
    }

    // 0x00480096
    static bool determineStationAndTrackModTypes(AiThought& thought)
    {
        uint16_t mods = 0;
        uint8_t rackRail = 0xFFU;
        for (auto i = 0U; i < thought.var_45; ++i)
        {
            auto* vehicleObj = ObjectManager::get<VehicleObject>(thought.var_46[i]);
            for (auto j = 0U; j < vehicleObj->numTrackExtras; ++j)
            {
                mods |= (1U << vehicleObj->requiredTrackExtras[j]);
            }

            if (vehicleObj->hasFlags(VehicleObjectFlags::rackRail))
            {
                if (thought.hasPurchaseFlags(AiPurchaseFlags::unk0))
                {
                    rackRail = vehicleObj->rackRailType;
                }
            }
        }
        thought.mods = mods;
        thought.rackRailType = rackRail;

        uint8_t chosenStationObject = 0xFFU;

        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased))
        {
            const auto airports = getAvailableAirports();
            int16_t bestDesignYear = -1;

            for (const auto airportObjId : airports)
            {
                auto* airportObj = ObjectManager::get<AirportObject>(airportObjId);
                if (airportObj->hasFlags(AirportObjectFlags::acceptsHeavyPlanes | AirportObjectFlags::acceptsLightPlanes))
                {
                    if (bestDesignYear < airportObj->designedYear)
                    {
                        bestDesignYear = airportObj->designedYear;
                        chosenStationObject = airportObjId;
                    }
                }
            }
            if (bestDesignYear == -1)
            {
                return true;
            }
            thought.stationObjId = chosenStationObject;
            thought.signalObjId = 0xFFU;
            return false;
        }
        else if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::waterBased))
        {
            const auto docks = getAvailableDocks();
            int16_t bestDesignYear = -1;

            for (const auto dockObjId : docks)
            {
                auto* dockObj = ObjectManager::get<DockObject>(dockObjId);

                if (bestDesignYear < dockObj->designedYear)
                {
                    bestDesignYear = dockObj->designedYear;
                    chosenStationObject = dockObjId;
                }
            }
            if (bestDesignYear == -1)
            {
                return true;
            }
            thought.stationObjId = chosenStationObject;
            thought.signalObjId = 0xFFU;
            return false;
        }
        else if (thought.trackObjId & (1U << 7))
        {
            auto roadStations = getAvailableCompatibleStations(thought.trackObjId & ~(1U << 7), TransportMode::road);
            int16_t bestDesignYear = -1;
            bool hadIdealSelection = false;
            for (const auto roadStationObjId : roadStations)
            {
                auto* roadStationObj = ObjectManager::get<RoadStationObject>(roadStationObjId);
                if (roadStationObj->hasFlags(RoadStationFlags::passenger)
                    && roadStationObj->cargoType != thought.cargoType)
                {
                    continue;
                }
                if (roadStationObj->hasFlags(RoadStationFlags::freight)
                    && roadStationObj->cargoType == thought.cargoType) // Why??
                {
                    continue;
                }

                bool hasRequiredRoadEnd = roadStationObj->hasFlags(RoadStationFlags::roadEnd) == thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk8);

                // If we have previously used a fallback we want to remove the fallback and use
                // the ideal selection
                bool alwaysSelect = hasRequiredRoadEnd && !hadIdealSelection;

                // We have now entered ideal selection mode so can remove non-ideal stations
                // from potential selection
                if (hadIdealSelection && !hasRequiredRoadEnd)
                {
                    continue;
                }
                hadIdealSelection |= hasRequiredRoadEnd;

                if (!alwaysSelect)
                {
                    if (bestDesignYear >= roadStationObj->designedYear)
                    {
                        continue;
                    }
                }
                bestDesignYear = roadStationObj->designedYear;
                chosenStationObject = roadStationObjId;
            }

            if (bestDesignYear == -1)
            {
                return true;
            }
            thought.stationObjId = chosenStationObject;
            thought.signalObjId = 0xFFU;
            return false;
        }
        else
        {
            const auto trainStations = getAvailableCompatibleStations(thought.trackObjId, TransportMode::rail);
            int16_t bestDesignYear = -1;
            for (const auto trainStationObjId : trainStations)
            {
                auto* trainStationObj = ObjectManager::get<TrainStationObject>(trainStationObjId);

                if (bestDesignYear < trainStationObj->designedYear)
                {
                    bestDesignYear = trainStationObj->designedYear;
                    chosenStationObject = trainStationObjId;
                }
            }

            if (bestDesignYear == -1)
            {
                return true;
            }
            thought.stationObjId = chosenStationObject;

            const auto signals = getAvailableCompatibleSignals(thought.trackObjId);
            bestDesignYear = -1;
            uint8_t chosenSignal = 0xFFU;

            for (const auto signalObjId : signals)
            {
                auto* signalObj = ObjectManager::get<TrainSignalObject>(signalObjId);

                if (bestDesignYear < signalObj->designedYear)
                {
                    bestDesignYear = signalObj->designedYear;
                    chosenSignal = signalObjId;
                }
            }

            thought.signalObjId = chosenSignal;
            return false;
        }
    }

    // 0x00488050
    static bool sub_488050(const Company& company, AiThought& thought)
    {
        thought.purchaseFlags &= ~(AiPurchaseFlags::unk2 | AiPurchaseFlags::requiresMods);
        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            return false;
        }

        if (thought.numVehicles != 0)
        {
            auto train = Vehicles::Vehicle(thought.vehicles[0]);
            for (auto& car : train.cars)
            {
                auto* vehicleObj = ObjectManager::get<VehicleObject>(car.front->objectId);
                const auto reliability = car.front->reliability;
                if (vehicleObj->power != 0 && (getCurrentYear() >= vehicleObj->obsolete || (reliability != 0 && reliability < 0x1900)))
                {
                    const auto purchaseRequest = aiGenerateVehiclePurchaseRequest(company, thought, thought.var_46);
                    if (purchaseRequest.numVehicleObjects == 0)
                    {
                        return false;
                    }
                    thought.var_43 = purchaseRequest.dl;
                    thought.var_45 = purchaseRequest.numVehicleObjects;
                    thought.purchaseFlags |= AiPurchaseFlags::unk2;
                    if (determineStationAndTrackModTypes(thought))
                    {
                        return false;
                    }
                    if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased | ThoughtTypeFlags::waterBased))
                    {
                        return true;
                    }
                    uint16_t existingMods = 0xFFFFU;
                    auto tile = World::TileManager::get(thought.stations[0].pos);
                    for (const auto& el : tile)
                    {
                        if (el.baseZ() != thought.stations[0].baseZ)
                        {
                            continue;
                        }
                        auto* elTrack = el.as<World::TrackElement>();
                        if (elTrack != nullptr)
                        {
                            existingMods = 0U;
                            auto* trackObj = ObjectManager::get<TrackObject>(elTrack->trackObjectId());
                            for (auto i = 0U; i < 4; ++i)
                            {
                                if (elTrack->hasMod(i))
                                {
                                    existingMods |= 1U << trackObj->mods[i];
                                }
                            }
                            break;
                        }
                        auto* elRoad = el.as<World::RoadElement>();
                        if (elRoad != nullptr)
                        {
                            const bool targetIsNotTram = getGameState().roadObjectIdIsNotTram & (1U << (thought.trackObjId & ~(1U << 7)));
                            const bool elIsNotTram = getGameState().roadObjectIdIsNotTram & (1U << elRoad->roadObjectId());
                            if (targetIsNotTram)
                            {
                                if (!elIsNotTram)
                                {
                                    continue;
                                }
                            }
                            else
                            {
                                if (elIsNotTram)
                                {
                                    continue;
                                }
                                if (thought.trackObjId != elRoad->roadObjectId())
                                {
                                    continue;
                                }
                            }
                            existingMods = 0U;
                            auto* roadObj = ObjectManager::get<RoadObject>(elRoad->roadObjectId());
                            if (!roadObj->hasFlags(RoadObjectFlags::unk_03))
                            {
                                for (auto i = 0U; i < 2; ++i)
                                {
                                    if (elRoad->hasMod(i))
                                    {
                                        existingMods |= 1U << roadObj->mods[i];
                                    }
                                }
                            }
                            break;
                        }
                    }
                    if (existingMods == 0xFFFFU)
                    {
                        return false;
                    }
                    if (thought.mods != existingMods)
                    {
                        thought.mods |= existingMods;
                        thought.purchaseFlags |= AiPurchaseFlags::requiresMods;
                    }
                    return true;
                }
            }
        }
        // 0x00488149
        if (thought.var_88 < 2)
        {
            return false;
        }
        if (thought.hasPurchaseFlags(AiPurchaseFlags::unk4))
        {
            return false;
        }
        if (thought.numVehicles >= kThoughtTypeMinMaxNumVehicles[enumValue(thought.type)].max)
        {
            return false;
        }
        if (getUntransportedQuantity(thought) <= 50)
        {
            return false;
        }

        const auto purchaseRequest = aiGenerateVehiclePurchaseRequest(company, thought, thought.var_46);
        if (purchaseRequest.numVehicleObjects == 0)
        {
            return false;
        }
        thought.var_43 = thought.numVehicles + 1;
        thought.var_45 = purchaseRequest.numVehicleObjects;
        thought.purchaseFlags &= ~AiPurchaseFlags::unk2;
        if (determineStationAndTrackModTypes(thought))
        {
            return false;
        }
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased | ThoughtTypeFlags::waterBased))
        {
            return true;
        }
        uint16_t existingMods = 0xFFFFU;
        auto tile = World::TileManager::get(thought.stations[0].pos);
        for (const auto& el : tile)
        {
            if (el.baseZ() != thought.stations[0].baseZ)
            {
                continue;
            }
            auto* elTrack = el.as<World::TrackElement>();
            if (elTrack != nullptr)
            {
                existingMods = 0U;
                auto* trackObj = ObjectManager::get<TrackObject>(elTrack->trackObjectId());
                for (auto i = 0U; i < 4; ++i)
                {
                    if (elTrack->hasMod(i))
                    {
                        existingMods |= 1U << trackObj->mods[i];
                    }
                }
                break;
            }
            auto* elRoad = el.as<World::RoadElement>();
            if (elRoad != nullptr)
            {
                existingMods = 0U;
                auto* roadObj = ObjectManager::get<RoadObject>(elRoad->roadObjectId());
                for (auto i = 0U; i < 2; ++i)
                {
                    if (elRoad->hasMod(i))
                    {
                        existingMods |= 1U << roadObj->mods[i];
                    }
                }
                break;
            }
        }
        if (existingMods == 0xFFFFU)
        {
            return false;
        }
        if (thought.mods != existingMods)
        {
            thought.mods |= existingMods;
            thought.purchaseFlags |= AiPurchaseFlags::requiresMods;
        }
        return true;
    }

    // 0x00430971
    static void aiThinkState1(Company& company)
    {
        company.activeThoughtId++;
        if (company.activeThoughtId < kMaxAiThoughts)
        {
            auto& thought = company.aiThoughts[company.activeThoughtId];
            if (thought.type == AiThoughtType::null)
            {
                aiThinkState1(company);
                return;
            }

            if (sub_487F8D(company, thought))
            {
                company.var_4A4 = AiThinkState::unk7;
                company.var_4A5 = 0;
                companyEmotionEvent(company.id(), Emotion::disgusted);
                return;
            }

            if (sub_488050(company, thought))
            {
                company.var_4A4 = AiThinkState::unk8;
                company.var_4A5 = 0;
            }
            return;
        }
        if (((company.challengeFlags & CompanyFlags::unk0) != CompanyFlags::none)
            || (getCurrentDay() - company.startedDate <= 42))
        {
            company.var_4A4 = AiThinkState::unk2;
            company.var_4A5 = 0;
            return;
        }
        CompanyManager::aiDestroy(company.id());
    }

    // 0x00430B31
    static void state2ClearActiveThought(Company& company)
    {
        if (company.activeThoughtId != 0xFFU)
        {
            auto& thought = company.aiThoughts[company.activeThoughtId];
            clearThought(thought);
        }
        company.var_4A5 = 13;
    }

    // 0x0047EABE & 0x0047EB17
    static TownId generateTargetTownByPopulation(uint32_t randVal, uint32_t minPopulationCapcity)
    {
        sfl::static_vector<TownId, Limits::kMaxTowns> possibleTowns;
        for (auto& town : TownManager::towns())
        {
            if (town.populationCapacity >= minPopulationCapcity)
            {
                possibleTowns.push_back(town.id());
            }
        }
        if (possibleTowns.empty())
        {
            return TownId::null;
        }
        const auto randTown = possibleTowns[(randVal & 0x7F) * possibleTowns.size() / 128];
        randVal = std::rotr(randVal, 7);
        return randTown;
    }

    struct TownDestinations
    {
        TownId townA;
        TownId townB;
    };

    // 0x0047EB70, 0x0047EC43
    static TownDestinations generateTargetTowns(uint32_t randVal, uint32_t minPopulationCapcity, int32_t minDistance, int32_t maxDistance)
    {
        sfl::static_vector<TownId, Limits::kMaxTowns> possibleTowns;
        for (auto& town : TownManager::towns())
        {
            if (town.populationCapacity >= minPopulationCapcity)
            {
                possibleTowns.push_back(town.id());
            }
        }

        if (possibleTowns.empty())
        {
            return { TownId::null, TownId::null };
        }

        const auto randTownA = possibleTowns[(randVal & 0x7F) * possibleTowns.size() / 128];
        randVal = std::rotr(randVal, 7);

        const auto* townA = TownManager::get(randTownA);
        const auto townAPos = World::Pos2{ townA->x, townA->y };

        possibleTowns.clear();
        for (auto& town : TownManager::towns())
        {
            if (town.populationCapacity >= minPopulationCapcity && town.id() != randTownA)
            {
                const auto townBPos = World::Pos2{ town.x, town.y };
                const auto dist = Math::Vector::manhattanDistance2D(townAPos, townBPos);
                if (dist <= maxDistance && dist >= minDistance)
                {
                    possibleTowns.push_back(town.id());
                }
            }
        }

        if (possibleTowns.empty())
        {
            return { TownId::null, TownId::null };
        }

        const auto randTownB = possibleTowns[(randVal & 0x7F) * possibleTowns.size() / 128];
        randVal = std::rotr(randVal, 7);
        return { randTownA, randTownB };
    }

    // 0x0047EDF0
    static TownDestinations generateTargetTownsWater(uint32_t randVal, uint32_t minPopulationCapcity, int32_t minDistance, int32_t maxDistance)
    {
        sfl::static_vector<TownId, Limits::kMaxTowns> possibleTowns;
        for (auto& town : TownManager::towns())
        {
            if (town.populationCapacity < minPopulationCapcity)
            {
                continue;
            }
            const auto numWaterTiles = World::TileManager::countNearbyWaterTiles(World::Pos2{ town.x, town.y });
            if (numWaterTiles < 20)
            {
                continue;
            }
            possibleTowns.push_back(town.id());
        }

        if (possibleTowns.empty())
        {
            return { TownId::null, TownId::null };
        }

        const auto randTownA = possibleTowns[(randVal & 0x7F) * possibleTowns.size() / 128];
        randVal = std::rotr(randVal, 7);

        const auto* townA = TownManager::get(randTownA);
        const auto townAPos = World::Pos2{ townA->x, townA->y };

        possibleTowns.clear();
        for (auto& town : TownManager::towns())
        {
            if (town.id() == randTownA)
            {
                continue;
            }
            if (town.populationCapacity < minPopulationCapcity)
            {
                continue;
            }
            const auto townBPos = World::Pos2{ town.x, town.y };
            const auto numWaterTiles = World::TileManager::countNearbyWaterTiles(townBPos);
            if (numWaterTiles < 20)
            {
                continue;
            }

            const auto dist = Math::Vector::manhattanDistance2D(townAPos, townBPos);
            if (dist > maxDistance || dist < minDistance)
            {
                continue;
            }

            const auto middlePos = (townAPos + townBPos) / 2;
            const auto* elSurface = World::TileManager::get(middlePos).surface();
            if (elSurface->water() != 0)
            {
                possibleTowns.push_back(town.id());
            }
        }

        if (possibleTowns.empty())
        {
            return { TownId::null, TownId::null };
        }

        const auto randTownB = possibleTowns[(randVal & 0x7F) * possibleTowns.size() / 128];
        randVal = std::rotr(randVal, 7);
        return { randTownA, randTownB };
    }

    struct IndustryDestinations
    {
        IndustryId industryA;
        IndustryId industryB;
        uint8_t cargoType;
    };

    // 0x0047EF49 & 0x0047F0C7
    static IndustryDestinations generateTargetIndustries(uint32_t randVal, int32_t minDistance, int32_t maxDistance)
    {
        sfl::static_vector<IndustryId, Limits::kMaxIndustries> possibleIndustries;
        for (auto& industry : IndustryManager::industries())
        {
            if (industry.hasFlags(IndustryFlags::closingDown))
            {
                continue;
            }
            bool shouldAdd = false;
            for (auto i = 0; i < 2; ++i)
            {
                if (industry.producedCargoQuantityPreviousMonth[i] >= 150)
                {
                    shouldAdd = true;
                    break;
                }
                if (industry.dailyProduction[i] >= 8)
                {
                    shouldAdd = true;
                    break;
                }
            }
            if (shouldAdd)
            {
                possibleIndustries.push_back(industry.id());
            }
        }
        if (possibleIndustries.empty())
        {
            return { IndustryId::null, IndustryId::null, 0 };
        }
        const auto randIndustryA = possibleIndustries[(randVal & 0xFF) * possibleIndustries.size() / 256];
        randVal = std::rotr(randVal, 8);
        auto* industryA = IndustryManager::get(randIndustryA);
        const auto industryAPos = World::Pos2{ industryA->x, industryA->y };

        auto* industryObj = industryA->getObject();
        uint8_t cargoType = industryObj->producedCargoType[0];
        if (industryA->producedCargoQuantityPreviousMonth[1] >= 150 || industryA->dailyProduction[1] >= 8)
        {
            cargoType = industryObj->producedCargoType[1];
            if (industryA->producedCargoQuantityPreviousMonth[0] >= 150 || industryA->dailyProduction[0] >= 8)
            {
                if (randVal & (1U << 31))
                {
                    cargoType = industryObj->producedCargoType[0];
                }
                randVal = std::rotr(randVal, 1);
            }
        }

        possibleIndustries.clear();
        for (auto& industry : IndustryManager::industries())
        {
            auto* industryObjB = industry.getObject();
            bool takesRequiredCargo = false;
            for (auto i = 0U; i < 3; ++i)
            {
                if (industryObjB->requiredCargoType[i] == cargoType)
                {
                    takesRequiredCargo = true;
                    break;
                }
            }
            if (!takesRequiredCargo)
            {
                continue;
            }

            const auto industryBPos = World::Pos2{ industry.x, industry.y };
            const auto dist = Math::Vector::manhattanDistance2D(industryAPos, industryBPos);
            if (dist <= maxDistance && dist >= minDistance)
            {
                possibleIndustries.push_back(industry.id());
            }
        }
        if (possibleIndustries.empty())
        {
            return { IndustryId::null, IndustryId::null, 0 };
        }
        const auto randIndustryB = possibleIndustries[(randVal & 0xFF) * possibleIndustries.size() / 256];
        randVal = std::rotr(randVal, 8);
        return { randIndustryA, randIndustryB, cargoType };
    }

    // 0x0047F245
    static IndustryDestinations generateTargetIndustriesWater(uint32_t randVal, int32_t minDistance, int32_t maxDistance)
    {
        sfl::static_vector<IndustryId, Limits::kMaxIndustries> possibleIndustries;
        for (auto& industry : IndustryManager::industries())
        {
            if (industry.hasFlags(IndustryFlags::closingDown))
            {
                continue;
            }
            bool shouldAdd = false;
            for (auto i = 0; i < 2; ++i)
            {
                if (industry.producedCargoQuantityPreviousMonth[i] >= 150)
                {
                    shouldAdd = true;
                    break;
                }
                if (industry.dailyProduction[i] >= 8)
                {
                    shouldAdd = true;
                    break;
                }
            }
            if (!shouldAdd)
            {
                continue;
            }

            const auto numWaterTiles = World::TileManager::countNearbyWaterTiles(World::Pos2{ industry.x, industry.y });
            if (numWaterTiles < 20)
            {
                continue;
            }
            possibleIndustries.push_back(industry.id());
        }
        if (possibleIndustries.empty())
        {
            return { IndustryId::null, IndustryId::null, 0 };
        }
        const auto randIndustryA = possibleIndustries[(randVal & 0xFF) * possibleIndustries.size() / 256];
        randVal = std::rotr(randVal, 8);
        auto* industryA = IndustryManager::get(randIndustryA);
        const auto industryAPos = World::Pos2{ industryA->x, industryA->y };

        auto* industryObj = industryA->getObject();
        uint8_t cargoType = industryObj->producedCargoType[0];
        if (industryA->producedCargoQuantityPreviousMonth[1] >= 150 || industryA->dailyProduction[1] >= 8)
        {
            cargoType = industryObj->producedCargoType[1];
            if (industryA->producedCargoQuantityPreviousMonth[0] >= 150 || industryA->dailyProduction[0] >= 8)
            {
                if (randVal & (1U << 31))
                {
                    cargoType = industryObj->producedCargoType[0];
                }
                randVal = std::rotr(randVal, 1);
            }
        }

        possibleIndustries.clear();
        for (auto& industry : IndustryManager::industries())
        {
            auto* industryObjB = industry.getObject();
            bool takesRequiredCargo = false;
            for (auto i = 0U; i < 3; ++i)
            {
                if (industryObjB->requiredCargoType[i] == cargoType)
                {
                    takesRequiredCargo = true;
                    break;
                }
            }
            if (!takesRequiredCargo)
            {
                continue;
            }

            const auto numWaterTiles = World::TileManager::countNearbyWaterTiles(World::Pos2{ industry.x, industry.y });
            if (numWaterTiles < 20)
            {
                continue;
            }
            const auto industryBPos = World::Pos2{ industry.x, industry.y };
            const auto dist = Math::Vector::manhattanDistance2D(industryAPos, industryBPos);
            if (dist > maxDistance || dist < minDistance)
            {
                continue;
            }

            const auto middlePos = (industryAPos + industryBPos) / 2;
            const auto* elSurface = World::TileManager::get(middlePos).surface();
            if (elSurface->water() != 0)
            {
                possibleIndustries.push_back(industry.id());
            }
        }
        if (possibleIndustries.empty())
        {
            return { IndustryId::null, IndustryId::null, 0 };
        }
        const auto randIndustryB = possibleIndustries[(randVal & 0xFF) * possibleIndustries.size() / 256];
        randVal = std::rotr(randVal, 8);
        return { randIndustryA, randIndustryB, cargoType };
    }

    struct MixedDestinations
    {
        IndustryId industryA;
        TownId townB;
        uint8_t cargoType;
    };

    // 0x00492B33
    static uint32_t getTownRequiredCargoTypes(const Town& town)
    {
        std::array<uint32_t, Limits::kMaxCargoObjects> scores{};
        World::Pos2 townPos{ town.x, town.y };
        auto tilePosA = toTileSpace(townPos) - World::TilePos2{ 5, 5 };
        auto tilePosB = toTileSpace(townPos) + World::TilePos2{ 5, 5 };
        for (auto& tilePos : getClampedRange(tilePosA, tilePosB))
        {
            auto tile = TileManager::get(tilePos);
            for (auto& el : tile)
            {
                auto* elBuilding = el.as<BuildingElement>();
                if (elBuilding == nullptr)
                {
                    continue;
                }
                if (elBuilding->isGhost())
                {
                    continue;
                }
                if (elBuilding->isMiscBuilding())
                {
                    continue;
                }
                if (!elBuilding->isConstructed())
                {
                    continue;
                }
                auto* buildingObj = ObjectManager::get<BuildingObject>(elBuilding->objectId());
                for (auto i = 0U; i < 2; ++i)
                {
                    if (buildingObj->requiredCargoType[i] == 0xFFU)
                    {
                        continue;
                    }
                    uint8_t quantity = buildingObj->var_A8[i];
                    if (buildingObj->hasFlags(BuildingObjectFlags::largeTile))
                    {
                        quantity *= 4;
                    }
                    scores[buildingObj->requiredCargoType[i]] += quantity;
                }
            }
        }

        uint32_t cargoBitSet = 0U;
        for (auto i = 0U; i < scores.size(); ++i)
        {
            const auto score = scores[i];
            if (score >= 32)
            {
                cargoBitSet |= 1U << i;
            }
        }
        return cargoBitSet;
    }

    // 0x0047F43E & 0x0047F5D6 & 0x0047F76E
    static MixedDestinations generateTargetMixedIndustries(uint32_t randVal, int32_t minDistance, int32_t maxDistance)
    {
        const uint8_t cargoType = IndustryManager::getMostCommonBuildingCargoType();

        sfl::static_vector<std::pair<TownId, uint32_t>, Limits::kMaxTowns> possibleTowns;
        for (auto& town : TownManager::towns())
        {
            if (town.populationCapacity < 2200)
            {
                continue;
            }

            auto cargoTypes = getTownRequiredCargoTypes(town);
            cargoTypes &= ~(1U << cargoType);
            if (cargoTypes != 0)
            {
                possibleTowns.emplace_back(town.id(), cargoTypes);
            }
        }

        if (possibleTowns.empty())
        {
            return { IndustryId::null, TownId::null, 0 };
        }
        const auto [randTownB, townBCargoTypes] = possibleTowns[(randVal & 0x7F) * possibleTowns.size() / 128];
        randVal = std::rotr(randVal, 7);

        const auto* townB = TownManager::get(randTownB);
        const auto townBPos = World::Pos2{ townB->x, townB->y };

        sfl::static_vector<uint8_t, Limits::kMaxCargoObjects> possibleCargoTypes;
        for (auto i = 0U; i < 32; ++i)
        {
            if (townBCargoTypes & (1U << i))
            {
                possibleCargoTypes.push_back(i);
            }
        }
        if (possibleCargoTypes.empty())
        {
            return { IndustryId::null, TownId::null, 0 };
        }
        const auto chosenCargo = possibleCargoTypes[(randVal & 0x1F) * possibleCargoTypes.size() / 32];
        randVal = std::rotr(randVal, 5);

        sfl::static_vector<IndustryId, Limits::kMaxIndustries> possibleIndustries;
        for (auto& industry : IndustryManager::industries())
        {
            // NOTE: missing from vanilla!
            // if (industry.hasFlags(IndustryFlags::closingDown))
            //{
            //    continue;
            //}
            auto* industryObj = industry.getObject();
            bool shouldAdd = false;
            for (auto i = 0; i < 2; ++i)
            {
                if (industryObj->producedCargoType[i] != chosenCargo)
                {
                    continue;
                }
                if (industry.producedCargoQuantityPreviousMonth[i] >= 150)
                {
                    shouldAdd = true;
                    break;
                }
            }
            if (shouldAdd)
            {
                const auto industryAPos = World::Pos2{ industry.x, industry.y };
                const auto dist = Math::Vector::manhattanDistance2D(industryAPos, townBPos);
                if (dist <= maxDistance && dist >= minDistance)
                {
                    possibleIndustries.push_back(industry.id());
                }
            }
        }
        if (possibleIndustries.empty())
        {
            return { IndustryId::null, TownId::null, 0 };
        }
        const auto randIndustryA = possibleIndustries[(randVal & 0xFF) * possibleIndustries.size() / 256];
        randVal = std::rotr(randVal, 8);
        return { randIndustryA, randTownB, chosenCargo };
    }

    // 0x0047F8FF
    static MixedDestinations generateTargetMixedIndustriesWater(uint32_t randVal, int32_t minDistance, int32_t maxDistance)
    {
        const uint8_t cargoType = IndustryManager::getMostCommonBuildingCargoType();

        sfl::static_vector<std::pair<TownId, uint32_t>, Limits::kMaxTowns> possibleTowns;
        for (auto& town : TownManager::towns())
        {
            if (town.populationCapacity < 2200)
            {
                continue;
            }

            auto cargoTypes = getTownRequiredCargoTypes(town);
            cargoTypes &= ~(1U << cargoType);
            if (cargoTypes == 0)
            {
                continue;
            }
            const auto numWaterTiles = World::TileManager::countNearbyWaterTiles(World::Pos2{ town.x, town.y });
            if (numWaterTiles < 20)
            {
                continue;
            }
            possibleTowns.emplace_back(town.id(), cargoTypes);
        }

        if (possibleTowns.empty())
        {
            return { IndustryId::null, TownId::null, 0 };
        }
        const auto [randTownB, townBCargoTypes] = possibleTowns[(randVal & 0x7F) * possibleTowns.size() / 128];
        randVal = std::rotr(randVal, 7);

        const auto* townB = TownManager::get(randTownB);
        const auto townBPos = World::Pos2{ townB->x, townB->y };

        sfl::static_vector<uint8_t, Limits::kMaxCargoObjects> possibleCargoTypes;
        for (auto i = 0U; i < 32; ++i)
        {
            if (townBCargoTypes & (1U << i))
            {
                possibleCargoTypes.push_back(i);
            }
        }
        if (possibleCargoTypes.empty())
        {
            return { IndustryId::null, TownId::null, 0 };
        }
        const auto chosenCargo = possibleCargoTypes[(randVal & 0x1F) * possibleCargoTypes.size() / 32];
        randVal = std::rotr(randVal, 5);

        sfl::static_vector<IndustryId, Limits::kMaxIndustries> possibleIndustries;
        for (auto& industry : IndustryManager::industries())
        {
            // NOTE: missing from vanilla!
            // if (industry.hasFlags(IndustryFlags::closingDown))
            //{
            //    continue;
            //}
            auto* industryObj = industry.getObject();
            bool shouldAdd = false;
            for (auto i = 0; i < 2; ++i)
            {
                if (industryObj->producedCargoType[i] != chosenCargo)
                {
                    continue;
                }
                if (industry.producedCargoQuantityPreviousMonth[i] >= 150)
                {
                    shouldAdd = true;
                    break;
                }
            }
            if (!shouldAdd)
            {
                continue;
            }
            const auto industryAPos = World::Pos2{ industry.x, industry.y };
            const auto numWaterTiles = World::TileManager::countNearbyWaterTiles(industryAPos);
            if (numWaterTiles < 20)
            {
                continue;
            }
            const auto dist = Math::Vector::manhattanDistance2D(industryAPos, townBPos);
            if (dist > maxDistance || dist < minDistance)
            {
                continue;
            }

            const auto middlePos = (industryAPos + townBPos) / 2;
            const auto* elSurface = World::TileManager::get(middlePos).surface();
            if (elSurface->water() != 0)
            {
                possibleIndustries.push_back(industry.id());
            }
        }
        if (possibleIndustries.empty())
        {
            return { IndustryId::null, TownId::null, 0 };
        }
        const auto randIndustryA = possibleIndustries[(randVal & 0xFF) * possibleIndustries.size() / 256];
        randVal = std::rotr(randVal, 8);
        return { randIndustryA, randTownB, chosenCargo };
    }

    // 0x0047FB1D & 0x0047FC56
    static MixedDestinations generateTargetMixedIndustriesAir(uint32_t randVal, uint32_t minPopulationCapacity, int32_t minDistance, int32_t maxDistance)
    {
        sfl::static_vector<TownId, Limits::kMaxTowns> possibleTowns;
        for (auto& town : TownManager::towns())
        {
            if (town.populationCapacity < minPopulationCapacity)
            {
                continue;
            }

            possibleTowns.emplace_back(town.id());
        }

        if (possibleTowns.empty())
        {
            return { IndustryId::null, TownId::null, 0 };
        }
        const auto randTownB = possibleTowns[(randVal & 0x7F) * possibleTowns.size() / 128];
        randVal = std::rotr(randVal, 7);

        const uint8_t cargoType = IndustryManager::getMostCommonBuildingCargoType();

        const auto* townB = TownManager::get(randTownB);
        const auto townBPos = World::Pos2{ townB->x, townB->y };

        sfl::static_vector<IndustryId, Limits::kMaxIndustries> possibleIndustries;
        for (auto& industry : IndustryManager::industries())
        {
            // NOTE: missing from vanilla!
            // if (industry.hasFlags(IndustryFlags::closingDown))
            //{
            //    continue;
            //}
            auto* industryObj = industry.getObject();
            bool shouldAdd = false;
            for (auto i = 0; i < 2; ++i)
            {
                if (industryObj->producedCargoType[i] == cargoType)
                {
                    shouldAdd = true;
                    break;
                }
            }
            if (shouldAdd)
            {
                const auto industryAPos = World::Pos2{ industry.x, industry.y };
                const auto dist = Math::Vector::manhattanDistance2D(industryAPos, townBPos);
                if (dist <= maxDistance && dist >= minDistance)
                {
                    possibleIndustries.push_back(industry.id());
                }
            }
        }
        if (possibleIndustries.empty())
        {
            return { IndustryId::null, TownId::null, 0 };
        }
        const auto randIndustryA = possibleIndustries[(randVal & 0xFF) * possibleIndustries.size() / 256];
        randVal = std::rotr(randVal, 8);
        return { randIndustryA, randTownB, cargoType };
    }

    // 0x0047E7DC
    static void generateNewThought(Company& company, AiThought& thought)
    {
        thought.var_84 = 0;
        thought.var_80 = 0;
        thought.var_7C = 0;
        thought.var_76 = 0;
        thought.var_88 = 0;
        thought.trackObjId = 0xFFU;
        thought.signalObjId = 0xFFU;
        thought.stationObjId = 0xFFU;
        thought.mods = 0;
        thought.rackRailType = 0xFFU;
        thought.var_45 = 0xFFU;
        thought.numVehicles = 0;
        thought.var_43 = 0;
        thought.purchaseFlags = AiPurchaseFlags::none;

        auto randVal = gPrng1().randNext();
        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            thought.type = AiThoughtType::null;
            return;
        }

        bool use25BE = false;
        const auto var25BE = company.var_25BE;
        if (var25BE != AiThoughtType::null)
        {
            if (randVal & 0x1F)
            {
                use25BE = true;
            }
            randVal = std::rotr(randVal, 5);
        }
        thought.type = var25BE;
        if (!use25BE)
        {
            thought.type = static_cast<AiThoughtType>(((randVal & 0x1F) * 20) / 32);
            randVal = std::rotr(randVal, 5);
        }
        company.var_25BE = thought.type;

        switch (thought.type)
        {
            case AiThoughtType::unk0:
            case AiThoughtType::unk2:
            {
                // 0x0047E8C8
                const auto destination = generateTargetTownByPopulation(randVal, 1200);
                if (destination == TownId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(destination);
                thought.cargoType = IndustryManager::getMostCommonBuildingCargoType();
                break;
            }
            case AiThoughtType::unk1:
            case AiThoughtType::unk5:
            {
                // 0x0047E8AA
                const auto destination = generateTargetTownByPopulation(randVal, 600);
                if (destination == TownId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(destination);
                thought.cargoType = IndustryManager::getMostCommonBuildingCargoType();
                break;
            }
            case AiThoughtType::unk3:
            case AiThoughtType::unk6:
            {
                // 0x0047E8E6
                const auto [destinationA, destinationB] = generateTargetTowns(randVal, 800, 0, 85 * 32);
                if (destinationA == TownId::null || destinationB == TownId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(destinationA);
                thought.destinationB = enumValue(destinationB);

                thought.cargoType = IndustryManager::getMostCommonBuildingCargoType();
                break;
            }
            case AiThoughtType::unk4:
            {
                // 0x0047E907
                const auto [destinationA, destinationB] = generateTargetTowns(randVal, 800, 40 * 32, 220 * 32);
                if (destinationA == TownId::null || destinationB == TownId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(destinationA);
                thought.destinationB = enumValue(destinationB);

                thought.cargoType = IndustryManager::getMostCommonBuildingCargoType();
                break;
            }
            case AiThoughtType::unk7:
            case AiThoughtType::unk11:
            {
                // 0x0047E928
                const auto dest = generateTargetIndustries(randVal, 20 * 32, 80 * 32);
                if (dest.industryA == IndustryId::null || dest.industryB == IndustryId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(dest.industryA);
                thought.destinationB = enumValue(dest.industryB);

                thought.cargoType = dest.cargoType;
                break;
            }
            case AiThoughtType::unk8:
            {
                // 0x0047E944
                const auto dest = generateTargetIndustries(randVal, 60 * 32, 250 * 32);
                if (dest.industryA == IndustryId::null || dest.industryB == IndustryId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(dest.industryA);
                thought.destinationB = enumValue(dest.industryB);

                thought.cargoType = dest.cargoType;
                break;
            }
            case AiThoughtType::unk9:
            case AiThoughtType::unk12:
            {
                // 0x0047E960
                const auto dest = generateTargetMixedIndustries(randVal, 20 * 32, 80 * 32);
                if (dest.industryA == IndustryId::null || dest.townB == TownId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(dest.industryA);
                thought.destinationB = enumValue(dest.townB);

                thought.cargoType = dest.cargoType;
                break;
            }
            case AiThoughtType::unk10:
            {
                // 0x0047E97C
                const auto dest = generateTargetMixedIndustries(randVal, 60 * 32, 240 * 32);
                if (dest.industryA == IndustryId::null || dest.townB == TownId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(dest.industryA);
                thought.destinationB = enumValue(dest.townB);

                thought.cargoType = dest.cargoType;
                break;
            }
            case AiThoughtType::unk13:
            {
                // 0x0047E998
                const auto [destinationA, destinationB] = generateTargetTowns(randVal, 1200, 120 * 32, std::numeric_limits<int32_t>::max());
                if (destinationA == TownId::null || destinationB == TownId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(destinationA);
                thought.destinationB = enumValue(destinationB);

                thought.cargoType = IndustryManager::getMostCommonBuildingCargoType();
                break;
            }
            case AiThoughtType::unk14:
            {
                // 0x0047E9B9
                const auto dest = generateTargetMixedIndustries(randVal, 100 * 32, std::numeric_limits<int32_t>::max());
                if (dest.industryA == IndustryId::null || dest.townB == TownId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(dest.industryA);
                thought.destinationB = enumValue(dest.townB);

                thought.cargoType = dest.cargoType;
                break;
            }
            case AiThoughtType::unk15:
            {
                // 0x0047E9CE
                const auto [destinationA, destinationB] = generateTargetTownsWater(randVal, 800, 23 * 32, 105 * 32);
                if (destinationA == TownId::null || destinationB == TownId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(destinationA);
                thought.destinationB = enumValue(destinationB);

                thought.cargoType = IndustryManager::getMostCommonBuildingCargoType();
                break;
            }
            case AiThoughtType::unk16:
            {
                // 0x0047E9E8
                const auto dest = generateTargetIndustriesWater(randVal, 20 * 32, 95 * 32);
                if (dest.industryA == IndustryId::null || dest.industryB == IndustryId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(dest.industryA);
                thought.destinationB = enumValue(dest.industryB);

                thought.cargoType = dest.cargoType;
                break;
            }
            case AiThoughtType::unk17:
            {
                // 0x0047E9FD
                const auto dest = generateTargetMixedIndustriesWater(randVal, 20 * 32, 80 * 32);
                if (dest.industryA == IndustryId::null || dest.townB == TownId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(dest.industryA);
                thought.destinationB = enumValue(dest.townB);

                thought.cargoType = dest.cargoType;
                break;
            }
            case AiThoughtType::unk18:
            {
                // 0x0047EA12
                const auto dest = generateTargetMixedIndustriesAir(randVal, 450, 15 * 32, 80 * 32);
                if (dest.industryA == IndustryId::null || dest.townB == TownId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(dest.industryA);
                thought.destinationB = enumValue(dest.townB);

                thought.cargoType = dest.cargoType;
                break;
            }
            case AiThoughtType::unk19:
            {
                // 0x0047EA27
                const auto dest = generateTargetMixedIndustriesAir(randVal, 750, 60 * 32, 240 * 32);
                if (dest.industryA == IndustryId::null || dest.townB == TownId::null)
                {
                    thought.type = AiThoughtType::null;
                    return;
                }
                thought.destinationA = enumValue(dest.industryA);
                thought.destinationB = enumValue(dest.townB);

                thought.cargoType = dest.cargoType;
                break;
            }
            default:
                assert(false);
                break;
        }
    }

    // 0x00430A12
    static void sub_430A12(Company& company)
    {
        company.activeThoughtId = 0xFFU;
        for (auto thoughtId = 0U; thoughtId < std::size(company.aiThoughts); ++thoughtId)
        {
            auto& thought = company.aiThoughts[thoughtId];
            if (thought.type != AiThoughtType::null)
            {
                continue;
            }
            company.activeThoughtId = thoughtId;
            company.challengeFlags &= ~(CompanyFlags::unk1 | CompanyFlags::unk2);

            for (auto i = 0U; i < 20; ++i)
            {
                generateNewThought(company, thought);
                if (thought.type != AiThoughtType::null)
                {
                    company.var_4A5 = 1;
                    return;
                }
            }
            break;
        }
        state2ClearActiveThought(company);
    }

    struct SimilarThoughts
    {
        uint8_t total;
        uint8_t totalUnprofitable;
        bool inSameCompany;
    };

    // 0x00480EA8
    static SimilarThoughts getSimilarThoughtsInAllCompanies(Company& company, AiThought& thought)
    {
        uint8_t numSimilarThoughts = 0;
        uint8_t numUnprofitableThoughts = 0;
        for (auto& otherCompany : CompanyManager::companies())
        {
            for (auto& otherThought : otherCompany.aiThoughts)
            {
                if (otherThought.type == AiThoughtType::null)
                {
                    continue;
                }
                if (otherThought.cargoType != thought.cargoType)
                {
                    continue;
                }
                auto compatibleThought = [&otherThought, &thought]() {
                    if (thoughtTypeHasFlags(otherThought.type, ThoughtTypeFlags::singleDestination) != thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::singleDestination))
                    {
                        return false;
                    }
                    if (otherThought.destinationA == thought.destinationA)
                    {
                        if (thoughtTypeHasFlags(otherThought.type, ThoughtTypeFlags::singleDestination) || otherThought.destinationB == thought.destinationB)
                        {
                            if (thoughtTypeHasFlags(otherThought.type, ThoughtTypeFlags::destinationAIsIndustry) == thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationAIsIndustry)
                                && thoughtTypeHasFlags(otherThought.type, ThoughtTypeFlags::destinationBIsIndustry) == thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationBIsIndustry))
                            {
                                return true;
                            }
                        }
                    }
                    if (thoughtTypeHasFlags(otherThought.type, ThoughtTypeFlags::singleDestination))
                    {
                        return false;
                    }
                    if (otherThought.destinationB != thought.destinationA)
                    {
                        return false;
                    }
                    if (otherThought.destinationA != thought.destinationB)
                    {
                        return false;
                    }
                    // Note: unk1 unk2 are swapped on our thought
                    if (thoughtTypeHasFlags(otherThought.type, ThoughtTypeFlags::destinationAIsIndustry) != thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationBIsIndustry)
                        || thoughtTypeHasFlags(otherThought.type, ThoughtTypeFlags::destinationBIsIndustry) != thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationAIsIndustry))
                    {
                        return false;
                    }
                    return true;
                }();

                if (!compatibleThought)
                {
                    continue;
                }
                // 0x00480F6C
                if (&company != &otherCompany)
                {
                    if (numSimilarThoughts != 0xFFU)
                    {
                        ++numSimilarThoughts;
                    }
                    if (otherThought.var_84 >= 3 * otherThought.var_7C)
                    {
                        ++numUnprofitableThoughts;
                    }
                }
                else
                {
                    if (&otherThought == &thought)
                    {
                        continue;
                    }
                    return { 0, 0, true };
                }
            }
        }
        return SimilarThoughts{ numSimilarThoughts, numUnprofitableThoughts, false };
    }

    // 0x00430B5D
    static void sub_430B5D(Company& company)
    {
        auto& thought = company.aiThoughts[company.activeThoughtId];
        auto similarThoughts = getSimilarThoughtsInAllCompanies(company, thought);
        if (similarThoughts.inSameCompany || similarThoughts.total > 1)
        {
            clearThought(thought);
            company.var_4A5 = 13;
            return;
        }
        if (similarThoughts.total == 1)
        {
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk12 | ThoughtTypeFlags::unk13))
            {
                if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk6))
                {
                    clearThought(thought);
                    company.var_4A5 = 13;
                    return;
                }
            }
            if (similarThoughts.total != similarThoughts.totalUnprofitable)
            {
                clearThought(thought);
                company.var_4A5 = 13;
                return;
            }
        }
        company.var_4A5 = 2;
    }

    // 0x00480FC3
    static bool applyPlaystyleRestrictions(Company& company, AiThought& thought)
    {
        if ((company.aiPlaystyleFlags & AiPlaystyleFlags::unk0) != AiPlaystyleFlags::none
            && thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk11))
        {
            return true;
        }

        if ((company.aiPlaystyleFlags & AiPlaystyleFlags::unk1) != AiPlaystyleFlags::none
            && thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk12))
        {
            return true;
        }

        if ((company.aiPlaystyleFlags & AiPlaystyleFlags::unk2) != AiPlaystyleFlags::none
            && thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk13))
        {
            return true;
        }

        if ((company.aiPlaystyleFlags & AiPlaystyleFlags::unk3) != AiPlaystyleFlags::none
            && thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk14))
        {
            return true;
        }

        if ((company.aiPlaystyleFlags & AiPlaystyleFlags::noAir) != AiPlaystyleFlags::none
            && thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased))
        {
            return true;
        }

        if ((company.aiPlaystyleFlags & AiPlaystyleFlags::noWater) != AiPlaystyleFlags::none
            && thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::waterBased))
        {
            return true;
        }

        if ((company.aiPlaystyleFlags & AiPlaystyleFlags::unk7) != AiPlaystyleFlags::none
            && thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk7))
        {
            return true;
        }

        if ((company.aiPlaystyleFlags & AiPlaystyleFlags::unk6) != AiPlaystyleFlags::none
            && !thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk7))
        {
            return true;
        }

        if ((company.aiPlaystyleFlags & AiPlaystyleFlags::townIdSet) != AiPlaystyleFlags::none)
        {
            // The first thought of this play style must be located near to the home town chosen for the company
            // this ensures that the company is based around that town
            bool noOtherThoughts = true;
            for (auto& otherThought : company.aiThoughts)
            {
                if (&otherThought == &thought)
                {
                    continue;
                }
                if (otherThought.type != AiThoughtType::null)
                {
                    noOtherThoughts = false;
                    break;
                }
            }
            if (noOtherThoughts)
            {
                auto destinationReferenceHomeTown = [&company](bool isIndustry, uint8_t destination) {
                    if (isIndustry)
                    {
                        const auto* industry = IndustryManager::get(static_cast<IndustryId>(destination));
                        const auto* homeTown = TownManager::get(static_cast<TownId>(company.aiPlaystyleTownId));
                        const auto distance = Math::Vector::manhattanDistance2D(Pos2{ industry->x, industry->y }, Pos2{ homeTown->x, homeTown->y });
                        return distance < 33 * kTileSize;
                    }
                    else
                    {
                        return destination == company.aiPlaystyleTownId;
                    }
                };
                const bool destAReferencesHomeTown = destinationReferenceHomeTown(thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationAIsIndustry), thought.destinationA);

                if (!destAReferencesHomeTown)
                {
                    if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::singleDestination))
                    {
                        return true;
                    }
                    const bool destBReferencesHomeTown = destinationReferenceHomeTown(thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationBIsIndustry), thought.destinationB);
                    if (!destBReferencesHomeTown)
                    {
                        return true;
                    }
                }
            }
        }

        // Thoughts from a company must all be located nearby to at least one other thought from the same company
        // nearby is defined as within 60 tiles

        const auto destPositions = getDestinationPositions(thought);
        const auto destAPos = destPositions.posA;
        const auto destBPos = destPositions.posB.value_or(destAPos);

        bool otherThoughts = false;
        bool otherThoughtNearby = false;
        for (auto& otherThought : company.aiThoughts)
        {
            if (&otherThought == &thought)
            {
                continue;
            }
            if (otherThought.type == AiThoughtType::null)
            {
                continue;
            }
            otherThoughts = true;
            const auto otherDestPositions = getDestinationPositions(otherThought);
            const auto otherDestAPos = otherDestPositions.posA;
            const auto distAA = Math::Vector::distance2D(destAPos, otherDestAPos);
            if (distAA <= 60 * kTileSize)
            {
                otherThoughtNearby = true;
                break;
            }
            const auto distBA = Math::Vector::distance2D(destBPos, otherDestAPos);
            if (distBA <= 60 * kTileSize)
            {
                otherThoughtNearby = true;
                break;
            }
            if (!otherDestPositions.posB.has_value())
            {
                continue;
            }
            const auto otherDestBPos = otherDestPositions.posB.value();
            const auto distAB = Math::Vector::distance2D(destAPos, otherDestBPos);
            if (distAB <= 60 * kTileSize)
            {
                otherThoughtNearby = true;
                break;
            }
            const auto distBB = Math::Vector::distance2D(destBPos, otherDestBPos);
            if (distBB <= 60 * kTileSize)
            {
                otherThoughtNearby = true;
                break;
            }
        }
        if (otherThoughts && !otherThoughtNearby)
        {
            return true;
        }

        auto* competitorObj = ObjectManager::get<CompetitorObject>(company.competitorId);
        if (competitorObj->competitiveness < 5)
        {
            return getSimilarThoughtsInAllCompanies(company, thought).total != 0;
        }
        return false;
    }

    // 0x00430BAB
    static void sub_430BAB(Company& company)
    {
        auto& thought = company.aiThoughts[company.activeThoughtId];
        if (applyPlaystyleRestrictions(company, thought))
        {
            company.var_25BE = AiThoughtType::null;
            state2ClearActiveThought(company);
            return;
        }
        company.var_4A5 = 3;
    }

    // 0x00481433
    static int32_t distanceBetweenDestinations(AiThought& thought)
    {
        const auto posA = thought.getDestinationPositionA();
        const auto posB = thought.getDestinationPositionB();
        return Math::Vector::distance2D(posA, posB);
    }

    // 0x0048137F
    static void setupStationCountAndLength(AiThought& thought)
    {
        thought.numStations = kThoughtTypeNumStations[enumValue(thought.type)];
        for (auto i = 0U; i < thought.numStations; ++i)
        {
            auto& aiStation = thought.stations[i];
            aiStation.var_02 = AiThoughtStationFlags::none;
        }

        if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk11))
        {
            thought.stationLength = 1;
        }
        else if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk6))
        {
            if (getCurrentYear() < 1945)
            {
                thought.stationLength = 5;
            }
            else if (getCurrentYear() < 1991)
            {
                thought.stationLength = 6;
            }
            else
            {
                thought.stationLength = 7;
            }
        }
        else
        {
            const auto distance = distanceBetweenDestinations(thought);
            auto baseStationLength = (std::max(0, distance - 32 * 28) / 1024) + 5;
            auto yearAdditionalLength = 0;
            if (getCurrentYear() >= 1925 && getCurrentYear() < 1955)
            {
                yearAdditionalLength = 1;
            }
            else if (getCurrentYear() < 1985)
            {
                yearAdditionalLength = 2;
            }
            else
            {
                yearAdditionalLength = 3;
            }
            const auto minLength = thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk17) ? 7 : 0;
            thought.stationLength = std::clamp(baseStationLength + yearAdditionalLength, minLength, 11);
        }
    }

    // 0x00481D6F
    // If there are less than 4 buildings in the area (5x5) then the station can be placed there
    static bool isSuitableForStation(const World::Pos2& pos)
    {
        auto tilePosA = toTileSpace(pos) - TilePos2{ 2, 2 };
        auto tilePosB = toTileSpace(pos) + TilePos2{ 2, 2 };
        auto numBuildings = 0U;
        for (const auto& tilePos : TilePosRangeView(tilePosA, tilePosB))
        {
            auto tile = World::TileManager::get(tilePos);
            for (auto& el : tile)
            {
                auto* elBuilding = el.as<World::BuildingElement>();
                if (elBuilding != nullptr)
                {
                    numBuildings++;
                }
            }
        }
        return numBuildings < 4;
    }

    // 0x00503CBC 3bit yaw to rotation offset
    static constexpr std::array<World::Pos2, 8> kYaw0RotationOffsets = {
        World::Pos2{ -32, 0 },
        World::Pos2{ -32, 32 },
        World::Pos2{ 0, 32 },
        World::Pos2{ 32, 32 },
        World::Pos2{ 32, 0 },
        World::Pos2{ 32, -32 },
        World::Pos2{ 0, -32 },
        World::Pos2{ -32, -32 },
    };

    // 0x00481A2D
    static bool setupIntraCityUnk6Stations(AiThought& thought)
    {
        // 0x00112C5A4
        // 2bit rotation
        auto randDirection = gPrng1().randNext() & 0b10;

        const auto* town = TownManager::get(static_cast<TownId>(thought.destinationA));
        const auto townPos = World::Pos2{ town->x, town->y };

        auto& aiStation0 = thought.stations[0];
        if (!aiStation0.hasFlags(AiThoughtStationFlags::operational))
        {
            auto pos0 = kRotationOffset[randDirection] * 3 + townPos;
            for (auto i = 0U; i < 18; ++i)
            {
                if (isSuitableForStation(pos0))
                {
                    break;
                }
                pos0 += kRotationOffset[randDirection];
                if (i == 17)
                {
                    return true;
                }
            }
            pos0 -= kRotationOffset[randDirection];
            aiStation0.pos = pos0;
            aiStation0.rotation = 1;
            aiStation0.var_9 = 3;
            aiStation0.var_A = 1;
            aiStation0.var_B = 0;
            aiStation0.var_C = 0;
        }
        auto& aiStation1 = thought.stations[1];
        if (!aiStation1.hasFlags(AiThoughtStationFlags::operational))
        {
            auto pos1 = kRotationOffset[1] * 3 + townPos;
            for (auto i = 0U; i < 18; ++i)
            {
                if (isSuitableForStation(pos1))
                {
                    break;
                }
                pos1 += kRotationOffset[1];
                if (i == 17)
                {
                    return true;
                }
            }
            pos1 -= kRotationOffset[1];
            aiStation1.pos = pos1;
            aiStation1.rotation = 0b10 ^ randDirection;
            aiStation1.var_9 = 0;
            aiStation1.var_A = 2;
            aiStation1.var_B = 0;
            aiStation1.var_C = 0;
        }
        auto& aiStation2 = thought.stations[2];
        if (!aiStation2.hasFlags(AiThoughtStationFlags::operational))
        {
            const auto stationRot = randDirection ^ 0b10;
            auto pos1 = kRotationOffset[stationRot] * 3 + townPos;
            for (auto i = 0U; i < 18; ++i)
            {
                if (isSuitableForStation(pos1))
                {
                    break;
                }
                pos1 += kRotationOffset[stationRot];
                if (i == 17)
                {
                    return true;
                }
            }
            pos1 -= kRotationOffset[stationRot];
            aiStation2.pos = pos1;
            aiStation2.rotation = 3;
            aiStation2.var_9 = 1;
            aiStation2.var_A = 3;
            aiStation2.var_B = 0;
            aiStation2.var_C = 0;
        }
        auto& aiStation3 = thought.stations[3];
        if (!aiStation3.hasFlags(AiThoughtStationFlags::operational))
        {
            auto pos1 = kRotationOffset[3] * 3 + townPos;
            for (auto i = 0U; i < 18; ++i)
            {
                if (isSuitableForStation(pos1))
                {
                    break;
                }
                pos1 += kRotationOffset[3];
                if (i == 17)
                {
                    return true;
                }
            }
            pos1 -= kRotationOffset[3];
            aiStation3.pos = pos1;
            aiStation3.rotation = randDirection;
            aiStation3.var_9 = 2;
            aiStation3.var_A = 0;
            aiStation3.var_B = 0;
            aiStation3.var_C = 0;
        }

        auto minBaseZ = std::numeric_limits<SmallZ>::max();
        auto maxBaseZ = std::numeric_limits<SmallZ>::min();
        for (auto& aiStation : thought.stations)
        {
            if (!World::validCoords(aiStation.pos))
            {
                continue;
            }
            auto* elSurface = World::TileManager::get(aiStation.pos).surface();
            minBaseZ = std::min(elSurface->baseZ(), minBaseZ);
            maxBaseZ = std::max(elSurface->baseZ(), maxBaseZ);
        }
        return (maxBaseZ - minBaseZ > 20);
    }

    // 0x004816D9
    static bool setupIntraCityBasicStations(AiThought& thought)
    {
        // 3bit yaw rotation
        auto randDirection = gPrng1().randNext() & 0b111;

        const auto* town = TownManager::get(static_cast<TownId>(thought.destinationA));
        const auto townPos = World::Pos2{ town->x, town->y };

        auto& aiStation0 = thought.stations[0];
        if (!aiStation0.hasFlags(AiThoughtStationFlags::operational))
        {
            auto pos0 = kYaw0RotationOffsets[randDirection] * 3 + townPos;
            for (auto i = 0U; i < 15; ++i)
            {
                if (isSuitableForStation(pos0))
                {
                    break;
                }
                pos0 += kYaw0RotationOffsets[randDirection];
                if (i == 14)
                {
                    return true;
                }
            }
            pos0 -= kYaw0RotationOffsets[randDirection] * 2;
            aiStation0.pos = pos0;
            aiStation0.rotation = randDirection;
            aiStation0.var_9 = 0xFFU;
            aiStation0.var_A = 1;
            if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::roadBased))
            {
                aiStation0.var_9 = 0;
            }
            aiStation0.var_B = 0;
            aiStation0.var_C = 0;
        }
        auto& aiStation1 = thought.stations[1];
        if (!aiStation1.hasFlags(AiThoughtStationFlags::operational))
        {
            const auto direction = randDirection ^ 0b100;
            auto pos1 = kYaw0RotationOffsets[direction] * 3 + townPos;
            for (auto i = 0U; i < 15; ++i)
            {
                if (isSuitableForStation(pos1))
                {
                    break;
                }
                pos1 += kYaw0RotationOffsets[direction];
                if (i == 14)
                {
                    return true;
                }
            }
            pos1 -= kYaw0RotationOffsets[direction] * 2;
            aiStation1.pos = pos1;
            aiStation1.var_9 = 0xFFU;
            aiStation1.var_A = 0;
            if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::roadBased))
            {
                aiStation1.var_9 = 1;
            }
            aiStation1.var_B = 0;
            aiStation1.var_C = 0;
        }
        if (thought.numStations > 2)
        {
            auto& aiStation2 = thought.stations[2];
            if (!aiStation2.hasFlags(AiThoughtStationFlags::operational))
            {
                const auto direction = (randDirection + 0b10) & 0x7;
                auto pos2 = kYaw0RotationOffsets[direction] * 3 + townPos;
                for (auto i = 0U; i < 9; ++i)
                {
                    if (isSuitableForStation(pos2))
                    {
                        break;
                    }
                    pos2 += kYaw0RotationOffsets[direction];
                }
                pos2 -= kYaw0RotationOffsets[direction] * 3;
                aiStation2.pos = pos2;
                aiStation2.var_9 = 0;
                aiStation2.var_A = 3;
                aiStation2.var_B = 0;
                aiStation2.var_C = 0;

                aiStation0.var_A = 2;
                aiStation1.var_A = 3;
            }
            auto& aiStation3 = thought.stations[3];
            if (!aiStation3.hasFlags(AiThoughtStationFlags::operational))
            {
                const auto direction = (randDirection - 0b10) & 0x7;
                auto pos3 = kYaw0RotationOffsets[direction] * 3 + townPos;
                for (auto i = 0U; i < 9; ++i)
                {
                    if (isSuitableForStation(pos3))
                    {
                        break;
                    }
                    pos3 += kYaw0RotationOffsets[direction];
                }
                pos3 -= kYaw0RotationOffsets[direction] * 3;
                aiStation3.pos = pos3;
                aiStation3.var_9 = 2;
                aiStation3.var_A = 1;
                aiStation3.var_B = 0;
                aiStation3.var_C = 0;
            }
        }
        return false;
    }

    // 0x004816D9
    static bool setupPointToPointStations(AiThought& thought)
    {
        auto& aiStationA = thought.stations[0];
        if (!aiStationA.hasFlags(AiThoughtStationFlags::operational))
        {
            auto posA = thought.getDestinationPositionA();
            aiStationA.pos = posA;
            aiStationA.var_9 = 1;
            aiStationA.var_A = 0xFFU;
            aiStationA.var_B = 0;
            aiStationA.var_C = 0;
            aiStationA.var_B |= thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk17) ? (1U << 0) : 0;
        }
        auto& aiStationB = thought.stations[1];
        if (!aiStationB.hasFlags(AiThoughtStationFlags::operational))
        {
            auto posB = thought.getDestinationPositionB();
            aiStationB.pos = posB;
            aiStationB.var_9 = 0;
            aiStationB.var_A = 0xFFU;
            aiStationB.var_B = 0;
            aiStationB.var_C = 0;
            aiStationB.var_B |= thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk17) ? (1U << 0) : 0;
        }

        if (!aiStationA.hasFlags(AiThoughtStationFlags::operational))
        {
            const auto posDiff1 = toTileSpace(aiStationB.pos - aiStationA.pos);
            const auto rotation1 = Vehicles::calculateYaw1FromVector(posDiff1.x, posDiff1.y) / 8;

            aiStationA.pos += kYaw0RotationOffsets[rotation1] * 4;

            const auto posDiff2 = aiStationB.pos - aiStationA.pos;
            const auto rotation2 = (Vehicles::calculateYaw0FromVector(posDiff2.x, posDiff2.y) / 16) ^ (1U << 1);
            aiStationA.rotation = rotation2;
        }
        if (!aiStationB.hasFlags(AiThoughtStationFlags::operational))
        {
            const auto posDiff1 = toTileSpace(aiStationA.pos - aiStationB.pos);
            const auto rotation1 = Vehicles::calculateYaw1FromVector(posDiff1.x, posDiff1.y) / 8;

            aiStationB.pos += kYaw0RotationOffsets[rotation1] * 4;

            const auto posDiff2 = aiStationA.pos - aiStationB.pos;
            const auto rotation2 = (Vehicles::calculateYaw0FromVector(posDiff2.x, posDiff2.y) / 16) ^ (1U << 1);
            aiStationB.rotation = rotation2;
        }
        return false;
    }

    // 0x004814D6
    static bool setupAiStations(AiThought& thought)
    {
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::singleDestination))
        {
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk6))
            {
                return setupIntraCityUnk6Stations(thought);
            }
            else
            {
                return setupIntraCityBasicStations(thought);
            }
        }
        else
        {
            return setupPointToPointStations(thought);
        }
    }

    // 0x00430BDA
    static void sub_430BDA(Company& company)
    {
        auto& thought = company.aiThoughts[company.activeThoughtId];
        setupStationCountAndLength(thought);
        if (setupAiStations(thought))
        {
            state2ClearActiveThought(company);
            return;
        }
        company.var_4A5 = 4;
    }

    // 0x0047FE3A
    static bool chooseTrackObject(CompanyId companyId, AiThought& thought)
    {
        const auto destinations = getDestinationPositions(thought);
        using enum World::Track::TrackTraitFlags;
        auto requiredTraits = smallCurve | slope | junction;
        if (destinations.posB.has_value())
        {
            auto* surfaceA = TileManager::get(destinations.posA).surface();
            auto* surfaceB = TileManager::get(destinations.posB.value()).surface();
            auto heightDiff = std::abs(surfaceA->baseZ() - surfaceB->baseZ());
            const auto dist = Math::Vector::distance2D(destinations.posA, destinations.posB.value());
            if (heightDiff > 32 && dist <= 45 * 32)
            {
                requiredTraits |= steepSlope;
            }
        }

        const auto tracks = companyGetAvailableRailTracks(companyId);
        Speed16 maxSpeed = 0_mph;
        uint8_t bestTrack = 0xFFU;
        for (const auto trackObjId : tracks)
        {
            if (trackObjId & (1U << 7))
            {
                continue;
            }
            auto* trackObj = ObjectManager::get<TrackObject>(trackObjId);
            if ((trackObj->trackPieces & requiredTraits) != requiredTraits)
            {
                continue;
            }
            if (maxSpeed < trackObj->curveSpeed)
            {
                maxSpeed = trackObj->curveSpeed;
                bestTrack = trackObjId;
            }
        }
        if (bestTrack == 0xFFU)
        {
            return true;
        }
        thought.trackObjId = bestTrack;
        if ((requiredTraits & steepSlope) != World::Track::TrackTraitFlags::none)
        {
            thought.purchaseFlags |= AiPurchaseFlags::unk0;
        }
        auto* trackObj = ObjectManager::get<TrackObject>(bestTrack);
        if (trackObj->hasFlags(TrackObjectFlags::unk_04))
        {
            thought.purchaseFlags |= AiPurchaseFlags::unk1;
        }
        return false;
    }

    // 0x0047FFE5
    static bool chooseBasicRoadObject(CompanyId companyId, AiThought& thought)
    {
        const auto roads = companyGetAvailableRoads(companyId);
        const auto requiredTraits = World::Track::RoadTraitFlags::verySmallCurve | World::Track::RoadTraitFlags::slope | World::Track::RoadTraitFlags::steepSlope | World::Track::RoadTraitFlags::unk4 | World::Track::RoadTraitFlags::junction;
        Speed16 maxSpeed = 0_mph;
        uint8_t bestRoad = 0xFFU;
        for (const auto roadObjId : roads)
        {
            if (!(roadObjId & (1U << 7)))
            {
                continue;
            }

            auto* roadObj = ObjectManager::get<RoadObject>(roadObjId & ~(1U << 7));
            using enum RoadObjectFlags;
            if ((roadObj->flags & (unk_07 | isRoad | unk_03 | unk_02)) != (unk_07 | isRoad | unk_03 | unk_02))
            {
                continue;
            }
            if (roadObj->hasFlags(isOneWay))
            {
                continue;
            }
            if ((roadObj->roadPieces & requiredTraits) != requiredTraits)
            {
                continue;
            }
            if (maxSpeed < roadObj->maxSpeed)
            {
                maxSpeed = roadObj->maxSpeed;
                bestRoad = roadObjId;
            }
        }
        if (bestRoad == 0xFFU)
        {
            return true;
        }
        thought.trackObjId = bestRoad | (1U << 7);
        return false;
    }

    // 0x0047FF77
    static bool chooseTramRoadObject(CompanyId companyId, AiThought& thought)
    {
        const auto roads = companyGetAvailableRailTracks(companyId);
        const auto requiredTraits = World::Track::RoadTraitFlags::verySmallCurve | World::Track::RoadTraitFlags::slope | World::Track::RoadTraitFlags::steepSlope | World::Track::RoadTraitFlags::unk4 | World::Track::RoadTraitFlags::junction | World::Track::RoadTraitFlags::turnaround;
        Speed16 maxSpeed = 0_mph;
        uint8_t bestRoad = 0xFFU;
        for (const auto roadObjId : roads)
        {
            if (!(roadObjId & (1U << 7)))
            {
                continue;
            }

            auto* roadObj = ObjectManager::get<RoadObject>(roadObjId & ~(1U << 7));
            using enum RoadObjectFlags;
            if (roadObj->hasFlags(unk_07 | isRoad | unk_03 | isOneWay))
            {
                continue;
            }
            if ((roadObj->roadPieces & requiredTraits) != requiredTraits)
            {
                continue;
            }
            if (maxSpeed < roadObj->maxSpeed)
            {
                maxSpeed = roadObj->maxSpeed;
                bestRoad = roadObjId;
            }
        }
        if (bestRoad == 0xFFU)
        {
            return true;
        }
        thought.trackObjId = bestRoad | (1U << 7);
        return false;
    }

    // 0x00480059
    static bool chooseTrackRoadObject(CompanyId companyId, AiThought& thought)
    {
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::waterBased | ThoughtTypeFlags::airBased))
        {
            return false;
        }
        else if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::railBased))
        {
            return chooseTrackObject(companyId, thought);
        }
        else if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::roadBased))
        {
            return chooseBasicRoadObject(companyId, thought);
        }
        else
        {
            return chooseTramRoadObject(companyId, thought);
        }
    }

    // 0x00430C06
    static void sub_430C06(Company& company)
    {
        auto& thought = company.aiThoughts[company.activeThoughtId];
        if (chooseTrackRoadObject(company.id(), thought))
        {
            state2ClearActiveThought(company);
        }
        else
        {
            company.var_4A5 = 5;
        }
    }

    // 0x00430C2D
    static void sub_430C2D(Company& company)
    {
        auto& thought = company.aiThoughts[company.activeThoughtId];
        const auto request = aiGenerateVehiclePurchaseRequest(company, thought, thought.var_46);
        if (request.numVehicleObjects == 0)
        {
            state2ClearActiveThought(company);
            return;
        }
        thought.var_45 = request.numVehicleObjects;
        thought.var_43 = request.dl;
        thought.var_7C = request.dl * request.trainRunCost;
        thought.var_76 += request.trainCost;
        company.var_85F2 = request.trainCost;
        company.var_4A5 = 6;
    }

    // 0x00430C73
    static void sub_430C73(Company& company)
    {
        auto& thought = company.aiThoughts[company.activeThoughtId];
        if (determineStationAndTrackModTypes(thought))
        {
            state2ClearActiveThought(company);
        }
        else
        {
            company.var_4A5 = 7;
        }
    }

    // 0x00481DE3
    static currency32_t estimateStationCost(const AiThought& thought)
    {
        currency32_t baseCost = 0;
        uint8_t costMultiplier = kThoughtTypeNumStations[enumValue(thought.type)];

        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased))
        {
            auto* airportObj = ObjectManager::get<AirportObject>(thought.stationObjId);
            baseCost = Economy::getInflationAdjustedCost(airportObj->buildCostFactor, airportObj->costIndex, 6);
        }
        else if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::waterBased))
        {
            auto* dockObj = ObjectManager::get<DockObject>(thought.stationObjId);
            baseCost = Economy::getInflationAdjustedCost(dockObj->buildCostFactor, dockObj->costIndex, 7);
        }
        else
        {
            if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::tramBased | ThoughtTypeFlags::roadBased))
            {
                costMultiplier *= thought.stationLength;
            }

            if (thought.trackObjId & (1U << 7))
            {
                {
                    const auto roadObjId = thought.trackObjId & ~(1U << 7);
                    auto* roadObj = ObjectManager::get<RoadObject>(roadObjId);
                    const auto trackBaseCost = Economy::getInflationAdjustedCost(roadObj->buildCostFactor, roadObj->costIndex, 10);
                    const auto cost = (trackBaseCost * World::TrackData::getRoadMiscData(0).costFactor) / 256;
                    baseCost += cost;
                }
                {
                    for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::roadExtra); ++i)
                    {
                        if (thought.mods & (1U << i))
                        {
                            auto* roadExtraObj = ObjectManager::get<RoadExtraObject>(i);
                            const auto trackExtraBaseCost = Economy::getInflationAdjustedCost(roadExtraObj->buildCostFactor, roadExtraObj->costIndex, 10);
                            const auto cost = (trackExtraBaseCost * World::TrackData::getRoadMiscData(0).costFactor) / 256;
                            baseCost += cost;
                        }
                    }
                }
                {
                    auto* stationObj = ObjectManager::get<RoadStationObject>(thought.stationObjId);
                    const auto stationBaseCost = Economy::getInflationAdjustedCost(stationObj->buildCostFactor, stationObj->costIndex, 8);
                    const auto cost = (stationBaseCost * World::TrackData::getRoadMiscData(0).costFactor) / 256;
                    baseCost += cost;
                }
            }
            else
            {

                auto* trackObj = ObjectManager::get<TrackObject>(thought.trackObjId);
                {
                    const auto trackBaseCost = Economy::getInflationAdjustedCost(trackObj->buildCostFactor, trackObj->costIndex, 10);
                    const auto cost = (trackBaseCost * World::TrackData::getTrackMiscData(0).costFactor) / 256;
                    baseCost += cost;
                }
                {
                    for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::trackExtra); ++i)
                    {
                        if (thought.mods & (1U << i))
                        {
                            auto* trackExtraObj = ObjectManager::get<TrackExtraObject>(i);
                            const auto trackExtraBaseCost = Economy::getInflationAdjustedCost(trackExtraObj->buildCostFactor, trackExtraObj->costIndex, 10);
                            const auto cost = (trackExtraBaseCost * World::TrackData::getTrackMiscData(0).costFactor) / 256;
                            baseCost += cost;
                        }
                    }
                }
                {
                    auto* stationObj = ObjectManager::get<TrainStationObject>(thought.stationObjId);
                    const auto stationBaseCost = Economy::getInflationAdjustedCost(stationObj->buildCostFactor, stationObj->costIndex, 8);
                    const auto cost = (stationBaseCost * World::TrackData::getTrackMiscData(0).costFactor) / 256;
                    baseCost += cost;

                    if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk9))
                    {
                        const auto tunnelBaseCost = Economy::getInflationAdjustedCost(trackObj->tunnelCostFactor, trackObj->costIndex, 10);
                        const auto tunnelCost = (tunnelBaseCost * World::TrackData::getTrackMiscData(0).costFactor) / 256;
                        baseCost += tunnelCost;
                    }
                }
            }
        }
        return baseCost * costMultiplier;
    }

    // 0x00430C9A
    static void sub_430C9A(Company& company)
    {
        auto& thought = company.aiThoughts[company.activeThoughtId];
        thought.var_76 += estimateStationCost(thought);

        company.var_4A5 = 8;
    }

    // 0x004821C5
    static uint32_t aiStationsManhattenTileDistance(const AiThought& thought, uint8_t aiStationIdx0, uint8_t aiStationIdx1)
    {
        auto& aiStation0 = thought.stations[aiStationIdx0];
        auto& aiStation1 = thought.stations[aiStationIdx1];

        const auto tilePos0 = toTileSpace(aiStation0.pos);
        const auto tilePos1 = toTileSpace(aiStation1.pos);

        return Math::Vector::manhattanDistance2D(tilePos0, tilePos1);
    }

    // 0x00481FF0
    static currency32_t estimateTrackPlacementCosts(const AiThought& thought)
    {
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased | ThoughtTypeFlags::waterBased))
        {
            return 0;
        }

        currency32_t tileCost = 0;
        if (thought.trackObjId & (1U << 7))
        {
            {
                const auto roadObjId = thought.trackObjId & ~(1U << 7);
                auto* roadObj = ObjectManager::get<RoadObject>(roadObjId);
                const auto trackBaseCost = Economy::getInflationAdjustedCost(roadObj->buildCostFactor, roadObj->costIndex, 10);
                const auto cost = (trackBaseCost * World::TrackData::getRoadMiscData(0).costFactor) / 256;
                tileCost += cost;
            }
            {
                for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::roadExtra); ++i)
                {
                    if (thought.mods & (1U << i))
                    {
                        auto* roadExtraObj = ObjectManager::get<RoadExtraObject>(i);
                        const auto trackExtraBaseCost = Economy::getInflationAdjustedCost(roadExtraObj->buildCostFactor, roadExtraObj->costIndex, 10);
                        const auto cost = (trackExtraBaseCost * World::TrackData::getRoadMiscData(0).costFactor) / 256;
                        tileCost += cost;
                    }
                }
            }
        }
        else
        {
            auto* trackObj = ObjectManager::get<TrackObject>(thought.trackObjId);
            {
                const auto trackBaseCost = Economy::getInflationAdjustedCost(trackObj->buildCostFactor, trackObj->costIndex, 10);
                const auto cost = (trackBaseCost * World::TrackData::getTrackMiscData(0).costFactor) / 256;
                tileCost += cost;
            }
            {
                for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::trackExtra); ++i)
                {
                    if (thought.mods & (1U << i))
                    {
                        auto* trackExtraObj = ObjectManager::get<TrackExtraObject>(i);
                        const auto trackExtraBaseCost = Economy::getInflationAdjustedCost(trackExtraObj->buildCostFactor, trackExtraObj->costIndex, 10);
                        const auto cost = (trackExtraBaseCost * World::TrackData::getTrackMiscData(0).costFactor) / 256;
                        tileCost += cost;
                    }
                }
            }

            if (thought.rackRailType != 0xFFU)
            {
                auto* trackExtraObj = ObjectManager::get<TrackExtraObject>(thought.rackRailType);
                const auto trackExtraBaseCost = Economy::getInflationAdjustedCost(trackExtraObj->buildCostFactor, trackExtraObj->costIndex, 10);
                const auto cost = (trackExtraBaseCost * World::TrackData::getTrackMiscData(0).costFactor) / 256;
                tileCost += cost;
            }

            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk9))
            {
                const auto trackBaseCost = Economy::getInflationAdjustedCost(trackObj->tunnelCostFactor, trackObj->costIndex, 10);
                const auto cost = (trackBaseCost * World::TrackData::getTrackMiscData(0).costFactor) / 256;
                tileCost += cost;
            }
        }

        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk10))
        {
            return 0;
        }

        uint32_t totalTileDistance = 0;
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk6))
        {
            totalTileDistance += aiStationsManhattenTileDistance(thought, 0, 1);
            totalTileDistance += aiStationsManhattenTileDistance(thought, 1, 2);
            totalTileDistance += aiStationsManhattenTileDistance(thought, 2, 3);
            totalTileDistance += aiStationsManhattenTileDistance(thought, 3, 0);
        }
        else
        {
            totalTileDistance += aiStationsManhattenTileDistance(thought, 0, 1);
        }
        return totalTileDistance * tileCost;
    }

    // 0x00430CBE
    static void sub_430CBE(Company& company)
    {
        auto& thought = company.aiThoughts[company.activeThoughtId];
        thought.var_76 += estimateTrackPlacementCosts(thought);

        company.var_4A5 = 9;
        // TODO: activeThoughtRevenueEstimate has same address as the thoughtState2AiStationIdx variable
        // in the future we should use a new offset.
        company.thoughtState2AiStationIdx = 0;
    }

    // 0x004821EF
    static currency32_t estimateStationClearageCosts(const AiThought& thought, uint8_t aiStationIdx)
    {
        if (aiStationIdx >= thought.numStations)
        {
            return 0;
        }
        auto& aiStation = thought.stations[aiStationIdx];
        if (aiStation.hasFlags(AiThoughtStationFlags::operational))
        {
            return 0;
        }

        currency32_t totalCost = 0;
        const auto minPos = aiStation.pos - World::Pos2{ 64, 64 };
        const auto maxPos = aiStation.pos + World::Pos2{ 64, 64 };
        for (const auto& pos : getClampedRange(minPos, maxPos))
        {
            auto tile = TileManager::get(pos);
            for (const auto& el : tile)
            {
                auto* elTree = el.as<TreeElement>();
                auto* elBuilding = el.as<BuildingElement>();
                if (elBuilding != nullptr)
                {
                    if (elBuilding->sequenceIndex() == 0)
                    {
                        auto* buildingObj = ObjectManager::get<BuildingObject>(elBuilding->objectId());
                        if (!buildingObj->hasFlags(BuildingObjectFlags::isHeadquarters | BuildingObjectFlags::indestructible))
                        {
                            totalCost += Economy::getInflationAdjustedCost(buildingObj->clearCostFactor, buildingObj->clearCostIndex, 8);
                        }
                    }
                }
                else if (elTree != nullptr)
                {
                    auto* treeObj = ObjectManager::get<TreeObject>(elTree->treeObjectId());
                    totalCost += Economy::getInflationAdjustedCost(treeObj->clearCostFactor, treeObj->costIndex, 12);
                }
            }
        }
        return totalCost;
    }

    // 0x00430CEC
    static void sub_430CEC(Company& company)
    {
        auto& thought = company.aiThoughts[company.activeThoughtId];
        // TODO: activeThoughtRevenueEstimate has same address as the thoughtState2AiStationIdx variable
        // in the future we should use a new offset.
        thought.var_76 += estimateStationClearageCosts(thought, company.thoughtState2AiStationIdx);
        company.thoughtState2AiStationIdx++;
        if (company.thoughtState2AiStationIdx >= 4)
        {
            company.var_4A5 = 10;
        }
    }

    // 0x004824F8
    static uint32_t aiStationsTileDistance(const AiThought& thought, uint8_t aiStationIdx0, uint8_t aiStationIdx1)
    {
        auto& aiStation0 = thought.stations[aiStationIdx0];
        auto& aiStation1 = thought.stations[aiStationIdx1];

        const auto tilePos0 = toTileSpace(aiStation0.pos);
        const auto tilePos1 = toTileSpace(aiStation1.pos);

        return Math::Vector::distance2D(tilePos0, tilePos1);
    }

    // 0x004822E8
    static currency32_t estimateThoughtRevenue(const AiThought& thought)
    {
        uint32_t averageTrackTileDistance = 0;
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk6))
        {
            uint32_t totalTrackTileDistance = aiStationsTileDistance(thought, 0, 1);
            totalTrackTileDistance += aiStationsTileDistance(thought, 1, 2);
            totalTrackTileDistance += aiStationsTileDistance(thought, 2, 3);
            totalTrackTileDistance += aiStationsTileDistance(thought, 3, 0);

            averageTrackTileDistance = totalTrackTileDistance / 4;
        }
        else
        {
            averageTrackTileDistance += aiStationsTileDistance(thought, 0, 1);
        }

        Speed16 minSpeed = kSpeed16Max;
        for (auto i = 0U; i < thought.var_45; ++i)
        {
            auto* vehicleObj = ObjectManager::get<VehicleObject>(thought.var_46[i]);
            minSpeed = std::min(vehicleObj->speed, minSpeed);
        }

        const auto speedFactor = ((minSpeed.getRaw() / 2) * 180) / 256;

        // 0x0112C3AA
        const auto distanceFactor = std::clamp(averageTrackTileDistance * 2 / (speedFactor == 0 ? 1 : speedFactor), 1U, 256U);

        // 0x0112C3AC
        auto distanceFactor2 = distanceFactor * 4;
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk7))
        {
            distanceFactor2 *= 2;
        }

        distanceFactor2 /= thought.var_43;

        uint32_t estimatedNumUnits = 0;
        const auto cargoObj = ObjectManager::get<CargoObject>(thought.cargoType);
        for (auto i = 0U; i < thought.var_45; ++i)
        {
            bool cargoFound = false;
            auto* vehicleObj = ObjectManager::get<VehicleObject>(thought.var_46[i]);
            for (auto j = 0U; j < 2; ++j)
            {
                if (vehicleObj->compatibleCargoCategories[j] & (1U << thought.cargoType))
                {
                    estimatedNumUnits += vehicleObj->maxCargo[j];
                    cargoFound = true;
                    break;
                }
            }
            if (cargoFound)
            {
                continue;
            }

            if (vehicleObj->hasFlags(VehicleObjectFlags::refittable))
            {
                if (cargoObj->hasFlags(CargoObjectFlags::refit))
                {
                    const auto sourceCargoType = Numerics::bitScanForward(vehicleObj->compatibleCargoCategories[0]);
                    estimatedNumUnits += Vehicles::getNumUnitsForCargo(vehicleObj->maxCargo[0], sourceCargoType, thought.cargoType);
                }
            }
        }

        const auto transferTimeFactor = (std::min<int32_t>((cargoObj->cargoTransferTime * estimatedNumUnits) / 256, 65535) / 192) * 4;

        const auto estimatedNumDays = distanceFactor + transferTimeFactor / 8;
        const auto estimatedPayment = CompanyManager::calculateDeliveredCargoPayment(thought.cargoType, estimatedNumUnits, averageTrackTileDistance, estimatedNumDays);

        const auto unkTimeFactor = 2920 / std::max<int32_t>(transferTimeFactor + distanceFactor2, 1);
        return estimatedPayment * unkTimeFactor;
    }

    // 0x00430D26
    static void sub_430D26(Company& company)
    {
        company.var_25BE = AiThoughtType::null;
        auto& thought = company.aiThoughts[company.activeThoughtId];
        company.activeThoughtRevenueEstimate = estimateThoughtRevenue(thought);
        company.var_4A5 = 11;
    }

    // 0x00430D54
    static void sub_430D54(Company& company)
    {
        company.var_2582 = 0;
        company.var_4A5 = 12;
    }

    static constexpr std::array<uint8_t, 12> kIntelligenceToMoneyFactor = {
        1,
        9,
        8,
        7,
        6,
        5,
        4,
        3,
        2,
        1,
        0,
        0,
    };

    static bool sub_482533(Company& company, AiThought& thought)
    {
        auto unk = company.activeThoughtRevenueEstimate - thought.var_7C * 24;
        if (unk <= 0)
        {
            return true;
        }

        auto* competitorObj = ObjectManager::get<CompetitorObject>(company.competitorId);
        unk = unk * kIntelligenceToMoneyFactor[competitorObj->intelligence] / 2;

        if (unk < thought.var_76)
        {
            return true;
        }

        return !CompanyManager::ensureCompanyFunding(company.id(), thought.var_76);
    }

    // 0x00430D7B
    static void sub_430D7B(Company& company)
    {
        auto& thought = company.aiThoughts[company.activeThoughtId];
        if (sub_482533(company, thought))
        {
            state2ClearActiveThought(company);
        }
        else
        {
            company.var_4A4 = AiThinkState::unk3;
            company.var_4A5 = 0;
            // 0x00482578
            company.var_259A = 0xFE;
            company.var_259B = 0xFE;
            company.var_259C = 0xFE;
            company.var_2596 = 0;
            thought.var_76 = 0;
        }
    }

    // 0x00430DAE
    static void sub_430DAE(Company& company)
    {
        company.var_4A4 = AiThinkState::unk0;
    }

    using AiThinkState2Function = void (*)(Company&);

    static constexpr std::array<AiThinkState2Function, 14> _funcs_4F94B0 = {
        sub_430A12,
        sub_430B5D,
        sub_430BAB,
        sub_430BDA,
        sub_430C06,
        sub_430C2D,
        sub_430C73,
        sub_430C9A,
        sub_430CBE,
        sub_430CEC,
        sub_430D26,
        sub_430D54,
        sub_430D7B,
        sub_430DAE,
    };

    // 0x004309FD
    static void aiThinkState2(Company& company)
    {
        company.var_85F6++;
        _funcs_4F94B0[company.var_4A5](company);
    }

    // 0x004834C0, 0x0048352E, 0x00493594
    template<typename Filter>
    static uint8_t sub_4834C0(const AiThought& thought, Filter&& bridgeFilter)
    {
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased | ThoughtTypeFlags::waterBased))
        {
            return 0;
        }

        sfl::static_vector<uint8_t, 8> bridges{};
        if (thought.trackObjId & (1U << 7))
        {
            const uint8_t roadObjId = thought.trackObjId & ~(1U << 7);
            bridges = getAvailableCompatibleBridges(roadObjId, TransportMode::road);
        }
        else
        {
            bridges = getAvailableCompatibleBridges(thought.trackObjId, TransportMode::rail);
        }

        const auto chosenBridge = [&bridges, &bridgeFilter]() {
            Speed16 maxSpeed = kSpeedZero;
            uint8_t bestBridge = 0xFFU;
            for (auto bridgeObjId : bridges)
            {
                auto* bridgeObj = ObjectManager::get<BridgeObject>(bridgeObjId);
                if (!bridgeFilter(*bridgeObj))
                {
                    continue;
                }
                auto speed = bridgeObj->maxSpeed;
                if (bridgeObj->maxSpeed == kSpeed16Null)
                {
                    speed = Speed16(0x7FFF);
                }
                if (maxSpeed < speed)
                {
                    maxSpeed = speed;
                    bestBridge = bridgeObjId;
                }
            }
            return bestBridge;
        }();
        return chosenBridge;
    }

    // 0x00482D07
    static bool sub_482D07_air(Company& company, AiThought& thought, uint8_t aiStationIdx)
    {
        // 0x00112C3C0 = bridgeHeight

        StationId foundStation = StationId::null;
        for (auto& station : StationManager::stations())
        {
            if (station.owner != company.id())
            {
                continue;
            }
            if ((station.flags & StationFlags::transportModeAir) == StationFlags::none)
            {
                continue;
            }
            auto& aiStation = thought.stations[aiStationIdx];
            const auto distance = Math::Vector::manhattanDistance2D(aiStation.pos, World::Pos2{ station.x, station.y });
            if (distance >= 512)
            {
                continue;
            }
            const auto cargoType = thought.cargoType;
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk7))
            {
                if (aiStationIdx == 0)
                {
                    if (station.cargoStats[cargoType].quantity != 0)
                    {
                        // 0x00482DA2
                        foundStation = station.id();
                        break;
                    }
                }
                else
                {
                    if (station.cargoStats[cargoType].isAccepted())
                    {
                        // 0x00482DA2
                        foundStation = station.id();
                        break;
                    }
                }
            }
            else
            {
                if (station.cargoStats[cargoType].isAccepted() && station.cargoStats[cargoType].quantity != 0)
                {
                    // 0x00482DA2
                    foundStation = station.id();
                    break;
                }
            }
        }
        if (foundStation != StationId::null)
        {
            // 0x00482DA2
            for (auto& otherThought : company.aiThoughts)
            {
                if (otherThought.type == AiThoughtType::null)
                {
                    continue;
                }
                if (!thoughtTypeHasFlags(otherThought.type, ThoughtTypeFlags::airBased))
                {
                    continue;
                }
                for (auto i = 0U; i < otherThought.numStations; ++i)
                {
                    auto& otherAiStation = otherThought.stations[i];
                    if (otherAiStation.id != foundStation)
                    {
                        continue;
                    }
                    if (!otherAiStation.hasFlags(AiThoughtStationFlags::operational))
                    {
                        continue;
                    }
                    // 0x00482E00
                    auto& aiStation = thought.stations[aiStationIdx];
                    aiStation.id = foundStation;
                    aiStation.pos = otherAiStation.pos;
                    aiStation.baseZ = otherAiStation.baseZ;
                    aiStation.rotation = otherAiStation.rotation;
                    aiStation.var_02 &= ~AiThoughtStationFlags::aiAllocated;
                    aiStation.var_02 |= AiThoughtStationFlags::operational;

                    return false;
                }
            }
        }
        // 0x00482E3C

        const auto randVal = gPrng1().randNext();
        const auto randTileX = (randVal & 0x1F) - 15;
        const auto randTileY = ((randVal >> 5) & 0x1F) - 15;
        const auto randTileOffset = World::TilePos2(randTileX, randTileY);

        auto& aiStation = thought.stations[aiStationIdx];
        const auto newAirportTilePos = World::toTileSpace(aiStation.pos) + randTileOffset;
        auto* airportObj = ObjectManager::get<AirportObject>(thought.stationObjId);
        const auto [minPos, maxPos] = airportObj->getAirportExtents(newAirportTilePos, aiStation.rotation);
        // 0x00482F21

        auto maxHeight = -1;
        for (auto& tilePos : getClampedRange(minPos, maxPos))
        {
            auto tile = TileManager::get(tilePos);
            auto* elSurface = tile.surface();
            auto height = elSurface->baseHeight();
            if (elSurface->water())
            {
                height = elSurface->waterHeight();
            }
            maxHeight = std::max<int>(maxHeight, height);
        }
        // 0x00482FA4
        if (!World::validCoords(maxPos))
        {
            return true;
        }

        const bool shouldCreateAirport = [&thought, aiStationIdx, minLoc = minPos, maxLoc = maxPos]() {
            const auto [acceptedCargo, producedCargo] = calcAcceptedCargoAi(minLoc, maxLoc);
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk7))
            {
                if (aiStationIdx == 0)
                {
                    if (!(producedCargo & (1U << thought.cargoType)))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!(acceptedCargo & (1U << thought.cargoType)))
                    {
                        return false;
                    }
                }
            }
            else
            {
                if (!(producedCargo & (1U << thought.cargoType)))
                {
                    return false;
                }
                if (!(acceptedCargo & (1U << thought.cargoType)))
                {
                    return false;
                }
            }
            return true;
        }();
        if (!shouldCreateAirport)
        {
            return true;
        }

        GameCommands::AirportPlacementArgs args{};
        args.pos = Pos3(World::toWorldSpace(newAirportTilePos), maxHeight);
        args.rotation = aiStation.rotation;
        args.type = thought.stationObjId;
        const auto res = GameCommands::doCommand(args, GameCommands::Flags::aiAllocated | GameCommands::Flags::apply | GameCommands::Flags::noPayment);
        if (res == GameCommands::FAILURE)
        {
            return true;
        }
        aiStation.pos = args.pos;
        aiStation.baseZ = args.pos.z / World::kSmallZStep;
        aiStation.var_02 |= AiThoughtStationFlags::aiAllocated;
        return false;
    }

    // TODO: Don't do this. Original logic isn't great
    // Where:
    // - Q is the port origin
    // - P is the port
    // - X represents the border offsets
    // - Y represents the border offsets that shouldn't be checked but are (see TODO)
    //
    //  Y X X Y
    //  X Q P X
    //  X P P X
    //  Y X X Y
    // Note: Order important!
    constexpr std::array<World::TilePos2, 12> kPortBorderOffsetsAi = {
        World::TilePos2{ -1, -1 },
        World::TilePos2{ 0, -1 },
        World::TilePos2{ 1, -1 },
        World::TilePos2{ 2, -1 },
        World::TilePos2{ -1, 0 },
        World::TilePos2{ 2, 0 },
        World::TilePos2{ -1, 1 },
        World::TilePos2{ 2, 1 },
        World::TilePos2{ -1, 2 },
        World::TilePos2{ 0, 2 },
        World::TilePos2{ 1, 2 },
        World::TilePos2{ 2, 2 },
    };

    // 0x00483088
    static bool sub_483088_water(Company& company, AiThought& thought, uint8_t aiStationIdx)
    {
        // 0x00112C3C0 = bridgeHeight

        // Mostly the same as air
        StationId foundStation = StationId::null;
        for (auto& station : StationManager::stations())
        {
            if (station.owner != company.id())
            {
                continue;
            }
            // Different flag to air
            if ((station.flags & StationFlags::transportModeWater) == StationFlags::none)
            {
                continue;
            }
            auto& aiStation = thought.stations[aiStationIdx];
            const auto distance = Math::Vector::manhattanDistance2D(aiStation.pos, World::Pos2{ station.x, station.y });
            // Different constant to air
            if (distance >= 448)
            {
                continue;
            }
            // Same as air
            const auto cargoType = thought.cargoType;
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk7))
            {
                if (aiStationIdx == 0)
                {
                    if (station.cargoStats[cargoType].quantity != 0)
                    {
                        // 0x00483123
                        foundStation = station.id();
                        break;
                    }
                }
                else
                {
                    if (station.cargoStats[cargoType].isAccepted())
                    {
                        // 0x00483123
                        foundStation = station.id();
                        break;
                    }
                }
            }
            else
            {
                if (station.cargoStats[cargoType].isAccepted() && station.cargoStats[cargoType].quantity != 0)
                {
                    // 0x00483123
                    foundStation = station.id();
                    break;
                }
            }
        }

        // Mostly the same as air
        if (foundStation != StationId::null)
        {
            // 0x00483123
            for (auto& otherThought : company.aiThoughts)
            {
                if (otherThought.type == AiThoughtType::null)
                {
                    continue;
                }
                // Different flag to air
                if (!thoughtTypeHasFlags(otherThought.type, ThoughtTypeFlags::waterBased))
                {
                    continue;
                }
                for (auto i = 0U; i < otherThought.numStations; ++i)
                {
                    auto& otherAiStation = otherThought.stations[i];
                    if (otherAiStation.id != foundStation)
                    {
                        continue;
                    }
                    if (!otherAiStation.hasFlags(AiThoughtStationFlags::operational))
                    {
                        continue;
                    }
                    // 0x00483181
                    auto& aiStation = thought.stations[aiStationIdx];
                    aiStation.id = foundStation;
                    aiStation.pos = otherAiStation.pos;
                    aiStation.baseZ = otherAiStation.baseZ;
                    aiStation.rotation = otherAiStation.rotation;
                    aiStation.var_02 &= ~AiThoughtStationFlags::aiAllocated;
                    aiStation.var_02 |= AiThoughtStationFlags::operational;

                    return false;
                }
            }
        }
        // 0x004831BD

        const auto randVal = gPrng1().randNext();
        // Different contants to air
        const auto randTileX = (randVal & 0xF) - 7;
        const auto randTileY = ((randVal >> 4) & 0xF) - 7;
        const auto randTileOffset = World::TilePos2(randTileX, randTileY);

        auto& aiStation = thought.stations[aiStationIdx];
        const auto newPortTilePos = World::toTileSpace(aiStation.pos) + randTileOffset;
        // Different to air for a while
        const auto minPos = newPortTilePos;
        const auto maxPos = newPortTilePos + TilePos2(1, 1);

        auto directionLand = 0xFFU;
        auto directionWaterIndustry = 0xFFU;
        auto height = -1;
        for (auto& offset : kPortBorderOffsetsAi)
        {
            const auto borderPos = offset + newPortTilePos;
            if (!World::validCoords(borderPos))
            {
                continue;
            }

            auto tile = TileManager::get(borderPos);
            {
                auto* elSurface = tile.surface();
                if (elSurface->water())
                {
                    height = elSurface->waterHeight();
                    if (height - (4 * World::kSmallZStep) != elSurface->baseHeight()
                        || !elSurface->isSlopeDoubleHeight())
                    {
                        // TODO: Use kRotationToBuildingFront instead of this broken logic
                        // (then we don't even need calculateYaw0FromVector)
                        const auto diffWorld = toWorldSpace(offset) - World::Pos2(16, 16);
                        // This gets the direction of this water from a point not at the origin which is
                        // not a good idea. The -16, -16 should really be removed. Only here to match vanilla
                        directionLand = (Vehicles::calculateYaw0FromVector(diffWorld.x, diffWorld.y) >> 4) ^ (1U << 1);
                    }
                }
            }
            bool passedSurface = false;
            for (auto& el : tile)
            {
                if (el.as<SurfaceElement>())
                {
                    passedSurface = true;
                    continue;
                }
                if (!passedSurface)
                {
                    continue;
                }
                auto* elIndustry = el.as<World::IndustryElement>();
                if (elIndustry == nullptr)
                {
                    continue;
                }
                if (elIndustry->isGhost())
                {
                    continue;
                }
                auto* industry = elIndustry->industry();
                auto* industryObj = ObjectManager::get<IndustryObject>(industry->objectId);
                if (industryObj->hasFlags(IndustryObjectFlags::builtOnWater))
                {
                    const auto diff = borderPos - minPos - World::TilePos2(1, 1);
                    const auto diffWorld = toWorldSpace(diff);
                    directionWaterIndustry = Vehicles::calculateYaw0FromVector(diffWorld.x, diffWorld.y) >> 4;
                    break;
                }
            }
        }

        if (!World::validCoords(maxPos) || height == -1)
        {
            return true;
        }

        const auto direction = directionWaterIndustry == 0xFFU ? directionLand : directionWaterIndustry;

        if (direction == 0xFFU)
        {
            return true;
        }

        // Same as air
        const bool shouldCreatePort = [&thought, aiStationIdx, minPos, maxPos]() {
            const auto [acceptedCargo, producedCargo] = calcAcceptedCargoAi(minPos, maxPos);
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk7))
            {
                if (aiStationIdx == 0)
                {
                    if (!(producedCargo & (1U << thought.cargoType)))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!(acceptedCargo & (1U << thought.cargoType)))
                    {
                        return false;
                    }
                }
            }
            else
            {
                if (!(producedCargo & (1U << thought.cargoType)))
                {
                    return false;
                }
                if (!(acceptedCargo & (1U << thought.cargoType)))
                {
                    return false;
                }
            }
            return true;
        }();
        if (!shouldCreatePort)
        {
            return true;
        }

        GameCommands::PortPlacementArgs args{};
        args.pos = Pos3(World::toWorldSpace(newPortTilePos), height);
        args.rotation = direction;
        args.type = thought.stationObjId;
        const auto res = GameCommands::doCommand(args, GameCommands::Flags::aiAllocated | GameCommands::Flags::apply | GameCommands::Flags::noPayment);
        if (res == GameCommands::FAILURE)
        {
            return true;
        }
        aiStation.pos = args.pos;
        aiStation.baseZ = args.pos.z / World::kSmallZStep;
        aiStation.var_02 |= AiThoughtStationFlags::aiAllocated;
        aiStation.rotation = direction;
        return false;
    }

    // 0x00482A00
    static bool sub_482A00_road(Company& company, AiThought& thought, uint8_t aiStationIdx, const World::Pos3 newStationPos)
    {
        auto& aiStation = thought.stations[aiStationIdx];

        GameCommands::AiRoadAndStationPlacementArgs args{};
        args.pos = newStationPos;
        args.rotation = aiStation.rotation;
        args.roadObjectId = thought.trackObjId & ~(1U << 7);
        args.stationObjectId = thought.stationObjId;
        args.stationLength = thought.stationLength;
        if (aiStation.var_9 != 0xFFU)
        {
            args.unk1 |= (1U << 1);
        }
        if (aiStation.var_A != 0xFFU)
        {
            args.unk1 |= (1U << 0);
        }
        args.bridge = company.var_259A;

        auto* roadObj = ObjectManager::get<RoadObject>(args.roadObjectId);
        for (auto i = 0U; i < 2; ++i)
        {
            if (roadObj->mods[i] != 0xFFU
                && (thought.mods & (1U << roadObj->mods[i])))
            {
                args.mods |= (1U << i);
            }
        }

        auto* stationObj = ObjectManager::get<RoadStationObject>(args.stationObjectId);
        bool doBasicPlacement = false;
        if (stationObj->hasFlags(RoadStationFlags::roadEnd))
        {
            doBasicPlacement = true;
        }

        bool isTram = false;
        if (!roadObj->hasFlags(RoadObjectFlags::unk_03))
        {
            isTram = true;
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk6))
            {
                doBasicPlacement = true;
            }
        }
        if (doBasicPlacement)
        {
            const auto res = GameCommands::doCommand(args, GameCommands::Flags::aiAllocated | GameCommands::Flags::apply | GameCommands::Flags::noPayment);
            if (res == GameCommands::FAILURE)
            {
                return true;
            }

            aiStation.pos = args.pos;
            aiStation.baseZ = args.pos.z / World::kSmallZStep;
            aiStation.var_02 |= AiThoughtStationFlags::aiAllocated;
            return false;
        }

        bool hasPlaced = false;
        auto tile = TileManager::get(newStationPos);
        for (auto& el : tile)
        {
            auto* elRoad = el.as<RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }
            if (elRoad->isGhost() || elRoad->isAiAllocated())
            {
                continue;
            }
            if (elRoad->roadId() != 0)
            {
                continue;
            }
            if (isTram)
            {
                args.rotation = elRoad->rotation();
                const auto res = GameCommands::doCommand(args, GameCommands::Flags::aiAllocated | GameCommands::Flags::apply | GameCommands::Flags::noPayment);
                if (res == GameCommands::FAILURE)
                {
                    continue;
                }
                hasPlaced = true;
                break;
            }
            else
            {
                // We are doing this as its used outside of this loop
                args.pos.z = elRoad->baseHeight();
                args.rotation = elRoad->rotation();

                GameCommands::RoadStationPlacementArgs args2{};
                args2.index = 0;
                args2.pos = args.pos;
                args2.roadId = 0;
                args2.roadObjectId = elRoad->roadObjectId();
                args2.rotation = args.rotation;
                args2.type = args.stationObjectId;
                const auto res = GameCommands::doCommand(args2, GameCommands::Flags::aiAllocated | GameCommands::Flags::apply | GameCommands::Flags::noPayment);
                if (res == GameCommands::FAILURE)
                {
                    continue;
                }
                hasPlaced = true;
                break;
            }
        }
        if (!hasPlaced)
        {
            return true;
        }

        // 0x00482BA3

        aiStation.pos = args.pos;
        aiStation.baseZ = args.pos.z / World::kSmallZStep;
        aiStation.rotation = args.rotation;
        aiStation.var_02 |= AiThoughtStationFlags::aiAllocated;

        if (aiStationIdx > 1)
        {
            return false;
        }
        auto otherAiStationIdx = aiStationIdx == 0 ? 1 : 0;

        auto& otherAiStation = thought.stations[otherAiStationIdx];
        const auto dx = Math::Vector::manhattanDistance2D(aiStation.pos + kRotationOffset[aiStation.rotation], otherAiStation.pos);
        const auto ax = Math::Vector::manhattanDistance2D(aiStation.pos - kRotationOffset[aiStation.rotation], otherAiStation.pos);
        if (ax < dx)
        {
            if (aiStation.var_9 == 0xFFU || aiStation.var_9 == aiStationIdx)
            {
                std::swap(aiStation.var_9, aiStation.var_A);
                std::swap(aiStation.var_B, aiStation.var_C);
            }
        }
        else
        {
            if (aiStation.var_A == 0xFFU || aiStation.var_A == aiStationIdx)
            {
                std::swap(aiStation.var_9, aiStation.var_A);
                std::swap(aiStation.var_B, aiStation.var_C);
            }
        }
        return false;
    }

    // 0x00482914
    static bool sub_482914_rail(Company& company, AiThought& thought, uint8_t aiStationIdx, const World::Pos3 newStationPos)
    {
        auto& aiStation = thought.stations[aiStationIdx];

        GameCommands::AiTrackAndStationPlacementArgs args{};
        args.pos = newStationPos;
        args.rotation = aiStation.rotation;
        args.trackObjectId = thought.trackObjId;
        args.stationObjectId = thought.stationObjId;
        args.stationLength = thought.stationLength;
        if (aiStation.var_9 != 0xFFU)
        {
            args.unk1 |= (1U << 1);
        }
        if (aiStation.var_A != 0xFFU)
        {
            args.unk1 |= (1U << 0);
        }
        args.bridge = company.var_259A;

        auto* trackObj = ObjectManager::get<TrackObject>(thought.trackObjId);
        for (auto i = 0U; i < 4; ++i)
        {
            if (trackObj->mods[i] != 0xFFU
                && (thought.mods & (1U << trackObj->mods[i])))
            {
                args.mods |= (1U << i);
            }
        }

        const auto res = GameCommands::doCommand(args, GameCommands::Flags::aiAllocated | GameCommands::Flags::apply | GameCommands::Flags::noPayment);

        if (res == GameCommands::FAILURE)
        {
            return true;
        }
        aiStation.pos = args.pos;
        aiStation.baseZ = args.pos.z / World::kSmallZStep;
        aiStation.var_02 |= AiThoughtStationFlags::aiAllocated;
        return false;
    }

    // 0x00482691
    static bool sub_482691_trackAndRoad(Company& company, AiThought& thought, uint8_t aiStationIdx, uint16_t bridgeHeight)
    {
        const auto randVal = gPrng1().randNext();
        // Different constants to air
        const auto randTileX = (randVal & 0x7) - 3;
        const auto randTileY = ((randVal >> 3) & 0x7) - 3;
        const auto randTileOffset = World::TilePos2(randTileX, randTileY);

        auto& aiStation = thought.stations[aiStationIdx];
        const auto randStationTilePos = World::toTileSpace(aiStation.pos) + randTileOffset;

        const auto length = thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::railBased) ? thought.stationLength : 1;
        const auto newStationTilePos = randStationTilePos - toTileSpace(kRotationOffset[aiStation.rotation]) * (length / 2);

        auto checkLength = length;
        auto minPos = newStationTilePos;
        if (!(thought.trackObjId & (1U << 7)))
        {
            if (aiStation.var_9 != 0xFFU)
            {
                minPos -= toTileSpace(kRotationOffset[aiStation.rotation]) * 2;
                checkLength += 2;
            }
            if (aiStation.var_A != 0xFFU)
            {
                minPos += toTileSpace(kRotationOffset[aiStation.rotation]) * 2;
                checkLength += 2;
            }
        }
        auto maxPos = minPos + toTileSpace(kRotationOffset[aiStation.rotation]) * (checkLength - 1);

        if (minPos.x > maxPos.x)
        {
            std::swap(minPos.x, maxPos.x);
        }
        if (minPos.y > maxPos.y)
        {
            std::swap(minPos.y, maxPos.y);
        }

        auto maxBaseZ = -1;
        auto tunnelBaseZ = std::numeric_limits<int32_t>::max();
        for (auto& tilePos : getClampedRange(minPos, maxPos))
        {
            auto tile = TileManager::get(tilePos);
            auto* elSurface = tile.surface();

            tunnelBaseZ = std::min<int32_t>(tunnelBaseZ, elSurface->baseZ());

            auto baseZ = World::TileManager::getSurfaceCornerHeight(*elSurface);
            auto waterBaseZ = (elSurface->water() + 1) * kMicroToSmallZStep;
            maxBaseZ = std::max<int32_t>(maxBaseZ, baseZ);
            maxBaseZ = std::max(maxBaseZ, waterBaseZ);
        }

        auto maxHeight = maxBaseZ * kSmallZStep + bridgeHeight;
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk9))
        {
            maxHeight = tunnelBaseZ * kSmallZStep - 32 - bridgeHeight;
        }
        // 0x0048282E

        auto stationMin = newStationTilePos;
        auto stationMax = stationMin;
        if (length != 1)
        {
            stationMax += toTileSpace(kRotationOffset[aiStation.rotation]) * (length - 1);
        }

        if (stationMin.x > stationMax.x)
        {
            std::swap(stationMin.x, stationMax.x);
        }
        if (stationMin.y > stationMax.y)
        {
            std::swap(stationMin.y, stationMax.y);
        }

        if (!World::validCoords(stationMax))
        {
            return true;
        }

        const bool shouldCreateStation = [&thought, aiStationIdx, stationMin, stationMax]() {
            const auto [acceptedCargo, producedCargo] = calcAcceptedCargoAi(stationMin, stationMax);
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk6) || aiStationIdx >= 2)
            {
                if (!(producedCargo & (1U << thought.cargoType)))
                {
                    return false;
                }
                return true;
            }

            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk7))
            {
                if (aiStationIdx == 0)
                {
                    if (!(producedCargo & (1U << thought.cargoType)))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!(acceptedCargo & (1U << thought.cargoType)))
                    {
                        return false;
                    }
                }
            }
            else
            {
                if (!(producedCargo & (1U << thought.cargoType)))
                {
                    return false;
                }
                if (!(acceptedCargo & (1U << thought.cargoType)))
                {
                    return false;
                }
            }
            return true;
        }();
        if (!shouldCreateStation)
        {
            return true;
        }

        if (thought.trackObjId & (1U << 7))
        {
            return sub_482A00_road(company, thought, aiStationIdx, Pos3(World::toWorldSpace(newStationTilePos), maxHeight));
        }
        else
        {
            return sub_482914_rail(company, thought, aiStationIdx, Pos3(World::toWorldSpace(newStationTilePos), maxHeight));
        }
    }

    // 0x00482662
    static bool sub_482662(Company& company, AiThought& thought, uint8_t station, uint16_t bridgeHeight)
    {
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased))
        {
            return sub_482D07_air(company, thought, station);
        }
        else if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::waterBased))
        {
            return sub_483088_water(company, thought, station);
        }

        return sub_482691_trackAndRoad(company, thought, station, bridgeHeight);
    }

    // 0x0048259F
    static uint8_t sub_48259F(Company& company, AiThought& thought)
    {
        if (company.var_259A == 254)
        {
            company.var_259A = sub_4834C0(thought, [](const BridgeObject& bridgeObj) {
                if (bridgeObj.maxHeight < 4)
                {
                    return false;
                }
                if ((bridgeObj.disabledTrackCfg
                     & (Track::CommonTraitFlags::slope
                        | Track::CommonTraitFlags::steepSlope
                        | Track::CommonTraitFlags::verySmallCurve
                        | Track::CommonTraitFlags::smallCurve
                        | Track::CommonTraitFlags::curve
                        | Track::CommonTraitFlags::largeCurve
                        | Track::CommonTraitFlags::sBendCurve
                        | Track::CommonTraitFlags::unk12))
                    != Track::CommonTraitFlags::none)
                {
                    return false;
                }
                return true;
            });
            return 0;
        }
        if (company.var_259B == 254)
        {
            company.var_259B = sub_4834C0(thought, [](const BridgeObject& bridgeObj) {
                if (bridgeObj.maxHeight < 8)
                {
                    return false;
                }
                return true;
            });
            return 0;
        }
        if (company.var_259C == 254)
        {
            company.var_259C = sub_4834C0(thought, [](const BridgeObject& bridgeObj) {
                if (bridgeObj.maxHeight < 8)
                {
                    return false;
                }
                if ((bridgeObj.disabledTrackCfg
                     & (Track::CommonTraitFlags::verySmallCurve
                        | Track::CommonTraitFlags::smallCurve
                        | Track::CommonTraitFlags::curve
                        | Track::CommonTraitFlags::largeCurve
                        | Track::CommonTraitFlags::sBendCurve))
                    != Track::CommonTraitFlags::none)
                {
                    return false;
                }
                return true;
            });
            return 0;
        }

        const auto chosenStation = [&thought]() {
            for (auto i = 0U; i < thought.numStations; ++i)
            {
                auto& aiStation = thought.stations[i];
                if (!aiStation.hasFlags(AiThoughtStationFlags::aiAllocated | AiThoughtStationFlags::operational))
                {
                    return i;
                }
            }
            return 0xFFU;
        }();

        if (chosenStation == 0xFFU)
        {
            return 2;
        }

        for (auto i = 0U; i < 3; ++i)
        {
            auto bridgeHeight = 0U;
            if (company.var_2596 >= 200)
            {
                bridgeHeight = 32;
            }
            else if (company.var_2596 >= 100)
            {
                bridgeHeight = 16;
            }
            if (!sub_482662(company, thought, chosenStation, bridgeHeight))
            {
                company.var_2596 = 0;
                return 0;
            }
            company.var_2596++;
            if (company.var_2596 >= 400)
            {
                return 1;
            }
        }
        return 0;
    }

    // 0x0048377C
    static void sub_48377C(Company& company, AiThought& thought)
    {
        company.var_85C2 = 0xFFU;
        company.var_85C3 = 0;
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk17))
        {
            company.var_85C3 |= (1U << 3);
        }
        if (thought.hasPurchaseFlags(AiPurchaseFlags::unk0))
        {
            company.var_85C3 |= (1U << 2);
        }
        if (thought.hasPurchaseFlags(AiPurchaseFlags::unk1))
        {
            company.var_85C3 |= (1U << 4);
        }
    }

    // 0x00430DDF
    static void sub_430DDF(Company& company, AiThought& thought)
    {
        if ((company.challengeFlags & CompanyFlags::unk1) != CompanyFlags::none)
        {
            company.var_4A4 = AiThinkState::unk6;
            company.var_4A5 = 3;
        }
        else
        {
            switch (sub_48259F(company, thought))
            {
                case 1:
                    company.var_4A4 = AiThinkState::unk6;
                    company.var_4A5 = 3;
                    break;
                case 2:
                    company.var_4A5 = 1;
                    sub_48377C(company, thought);
                    break;
                default:
                    break;
            }
        }
    }

    // 0x004837C2
    static bool sub_4837C2(Company& company, AiThought& thought)
    {
        company.var_85C3 &= ~((1U << 0) | (1U << 1));

        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased | ThoughtTypeFlags::waterBased))
        {
            return true;
        }

        auto findRequiredAiStation = [&thought, &company]() -> int8_t {
            for (auto i = 0U; i < thought.numStations; ++i)
            {
                const auto& aiStation = thought.stations[i];
                if (aiStation.var_9 != 0xFFU)
                {
                    if (!(aiStation.var_B & ((1U << 2) | (1U << 1))))
                    {
                        return i;
                    }
                    if (aiStation.var_B & (1U << 0))
                    {
                        if (!(aiStation.var_B & ((1U << 4) | (1U << 3))))
                        {
                            return i;
                        }
                    }
                }
                if (aiStation.var_A != 0xFFU)
                {
                    if (!(aiStation.var_C & ((1U << 2) | (1U << 1))))
                    {
                        company.var_85C3 |= 1U << 0;
                        return i;
                    }
                    if (aiStation.var_C & (1U << 0))
                    {
                        if (!(aiStation.var_C & ((1U << 4) | (1U << 3))))
                        {
                            company.var_85C3 |= 1U << 0;
                            return i;
                        }
                    }
                }
            }
            return -1;
        }();

        if (findRequiredAiStation == -1)
        {
            return true;
        }

        {
            company.var_85C2 = findRequiredAiStation;
            const auto& aiStation = thought.stations[findRequiredAiStation];
            auto pos = aiStation.pos;
            auto rotation = aiStation.rotation;
            if (company.var_85C3 & (1U << 0))
            {
                if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::railBased))
                {
                    const auto stationEndDiff = kRotationOffset[rotation] * (thought.stationLength - 1);
                    pos += stationEndDiff;
                }
                rotation ^= (1U << 1);
            }
            rotation ^= (1U << 1);

            company.var_85C4 = pos;
            company.var_85C8 = aiStation.baseZ;
            company.var_85CE = rotation;
            company.var_85D0 = pos;
            company.var_85D4 = aiStation.baseZ;
            company.var_85D5 = rotation;
        }
        {
            const auto aiStationIndex = (company.var_85C3 & (1U << 0)) ? thought.stations[findRequiredAiStation].var_A : thought.stations[findRequiredAiStation].var_9;
            const auto& aiStation = thought.stations[aiStationIndex];
            if (aiStation.var_9 != company.var_85C2)
            {
                company.var_85C3 |= (1U << 1);
            }
            auto pos = aiStation.pos;
            auto rotation = aiStation.rotation;
            if (company.var_85C3 & (1U << 1))
            {
                if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::railBased))
                {
                    const auto stationEndDiff = kRotationOffset[rotation] * (thought.stationLength - 1);
                    pos += stationEndDiff;
                }
                rotation ^= (1U << 1);
            }
            rotation ^= (1U << 1);

            company.var_85C9 = pos;
            company.var_85CD = aiStation.baseZ;
            company.var_85CF = rotation;
            company.var_85D7 = pos;
            company.var_85DB = aiStation.baseZ;
            company.var_85DC = rotation;
        }
        company.var_85F0 = 0;
        company.var_85EE = 0;
        company.var_85EF = 0;
        company.var_85DE = 0;
        company.var_85E2 = 0;
        company.var_85E8 = 0;

        const auto distance = std::max<uint16_t>(256, Math::Vector::distance3D(World::Pos3(company.var_85C4, company.var_85C8 * World::kSmallZStep), World::Pos3(company.var_85C9, company.var_85CD * World::kSmallZStep)));
        company.var_85EA = distance / 2 + distance * 2;
        // TODO: When diverging just set this all to a fixed value rather than only first entry
        for (auto& htEntry : company.var_25C0)
        {
            htEntry.var_00 = 0xFFFFU;
        }
        company.var_25C0_length = 0;
        return false;
    }

    // 0x00486324
    static bool sub_486324(Company& company, AiThought& thought)
    {
        // Likely this is asking if the track/road should try place a signal

        if (thought.trackObjId & (1U << 7))
        {
            return false;
        }

        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased | ThoughtTypeFlags::waterBased))
        {
            return false;
        }

        if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk17 | ThoughtTypeFlags::unk6))
        {
            return false;
        }

        if (thought.signalObjId == 0xFFU)
        {
            return true;
        }

        company.var_85C2 = 0;
        return false;
    }

    // 0x00430E21
    static void sub_430E21(Company& company, AiThought& thought)
    {
        if ((company.challengeFlags & CompanyFlags::unk1) != CompanyFlags::none)
        {
            company.var_4A4 = AiThinkState::unk6;
            company.var_4A5 = 2;
            company.var_85C4 = World::Pos2(0, 0);
            return;
        }

        if (company.var_85C2 != 0xFFU)
        {
            if (aiPathfind(company, thought))
            {
                company.var_4A4 = AiThinkState::unk6;
                company.var_4A5 = 2;
                company.var_85C4 = World::Pos2(0, 0);
            }
        }
        else
        {
            if (sub_4837C2(company, thought))
            {
                company.var_4A5 = 2;
                if (sub_486324(company, thought))
                {
                    company.var_4A4 = AiThinkState::unk6;
                    company.var_4A5 = 2;
                    company.var_85C4 = World::Pos2(0, 0);
                }
            }
        }
    }

    // 0x00486498
    // Will keep placing signals (one sided) until either:
    // * Reaches a station
    // * More than 1 track connection
    static void placeAiAllocatedSignalsEvenlySpaced(const World::Pos3 startPos, const uint16_t startTad, const uint8_t trackObjId, const uint16_t minSignalDistance, const uint8_t signalType, const uint8_t startSignalSide)
    {
        auto getNextTrack = [trackObjId](const World::Pos3 pos, const uint16_t tad) {
            const auto trackEnd = Track::getTrackConnectionEnd(pos, tad & Track::AdditionalTaDFlags::basicTaDMask);
            const auto tc = Track::getTrackConnectionsAi(trackEnd.nextPos, trackEnd.nextRotation, GameCommands::getUpdatingCompanyId(), trackObjId, 0, 0);
            return std::make_pair(trackEnd.nextPos, tc);
        };

        auto shouldContinue = [](const Track::TrackConnections& tc) {
            return tc.connections.size() == 1 && tc.stationId == StationId::null;
        };

        // We start with a large number to force a signal placement as soon as possible from the
        // start position
        auto distanceFromSignal = 3200;

        for (auto nextTrack = getNextTrack(startPos, startTad); shouldContinue(nextTrack.second); nextTrack = getNextTrack(nextTrack.first, nextTrack.second.connections[0]))
        {
            // Not const as when reversed need to get to the reverse start
            auto pos = nextTrack.first;
            // Not const as we might need to toggle the reverse bit
            auto tad = nextTrack.second.connections[0] & Track::AdditionalTaDFlags::basicTaDMask;
            const auto trackId = tad >> 3;
            const auto rotation = tad & 0x3;

            const auto lengthCopy = distanceFromSignal;
            distanceFromSignal += TrackData::getTrackMiscData(trackId).unkWeighting;
            if (lengthCopy < minSignalDistance)
            {
                continue;
            }

            uint8_t signalSide = startSignalSide;
            if (tad & (1U << 2))
            {
                auto& trackSize = TrackData::getUnkTrack(tad);
                pos += trackSize.pos;
                if (trackSize.rotationEnd < 12)
                {
                    pos -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
                }
                tad ^= (1U << 2); // Reverse
                signalSide = signalSide & (1U << 0) ? (1U << 1) : (1U << 0);
            }

            GameCommands::SignalPlacementArgs args{};
            args.pos = pos;
            args.pos.z += TrackData::getTrackPiece(trackId)[0].z;
            args.index = 0;
            args.rotation = rotation;
            args.trackId = trackId;
            args.trackObjType = trackObjId;
            args.sides = signalSide << 14;
            args.type = signalType;

            const auto res = GameCommands::doCommand(args, GameCommands::Flags::aiAllocated | GameCommands::Flags::apply | GameCommands::Flags::noPayment);
            if (res != GameCommands::FAILURE)
            {
                distanceFromSignal = 0;
            }
        }
    }

    // 0x0048635F
    // returns:
    //   true, all signals placed
    //   false, further spans between stations to look at
    static bool tryPlaceAiAllocatedSignalsBetweenStations(Company& company, AiThought& thought)
    {
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased | ThoughtTypeFlags::waterBased))
        {
            return true;
        }

        if (thought.trackObjId & (1U << 7))
        {
            return true;
        }
        // 0x0112C519
        const uint8_t trackObjId = thought.trackObjId;
        const uint8_t signalType = thought.signalObjId;
        // At least the length of the station (which is also the max length of the vehicles)
        const uint16_t minSignalSpacing = thought.stationLength * 32;

        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk6))
        {
            // 0x0048639B
            if (company.var_85C2 >= thought.numStations)
            {
                return true;
            }

            const auto& aiStation = thought.stations[company.var_85C2];

            const auto stationEnd = World::Pos3(
                aiStation.pos + World::Pos3{ kRotationOffset[aiStation.rotation], 0 } * (thought.stationLength - 1),
                aiStation.baseZ * World::kSmallZStep);

            const uint8_t signalSide = (1U << 0);
            const auto tad = 0 | aiStation.rotation;
            company.var_85C2++;

            placeAiAllocatedSignalsEvenlySpaced(stationEnd, tad, trackObjId, minSignalSpacing, signalType, signalSide);
            return false;
        }
        else if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk17))
        {
            if (company.var_85C2 >= 2)
            {
                return true;
            }

            // 0x0048640F
            const uint8_t signalSide = company.var_85C2 & 1 ? (1U << 0) : (1U << 1);

            const auto& aiStation = thought.stations[0];

            const auto trackEnd = Track::getTrackConnectionEnd(World::Pos3(aiStation.pos, aiStation.baseZ * World::kSmallZStep), aiStation.rotation ^ (1U << 1));
            const auto tc = Track::getTrackConnectionsAi(trackEnd.nextPos, trackEnd.nextRotation, GameCommands::getUpdatingCompanyId(), trackObjId, 0, 0);

            if (tc.connections.size() != 2)
            {
                return false;
            }
            const auto tad = tc.connections[company.var_85C2] & Track::AdditionalTaDFlags::basicTaDMask;
            company.var_85C2++;

            placeAiAllocatedSignalsEvenlySpaced(trackEnd.nextPos, tad, trackObjId, minSignalSpacing, signalType, signalSide);
            return false;
        }
        return true;
    }

    // 0x00430EB5
    static void sub_430EB5(Company& company, AiThought& thought)
    {
        // place signal aiAllocations

        if ((company.challengeFlags & CompanyFlags::unk1) != CompanyFlags::none)
        {
            company.var_4A4 = AiThinkState::unk6;
            company.var_4A5 = 2;
            company.var_85C4 = World::Pos2{ 0, 0 };
            return;
        }

        if (tryPlaceAiAllocatedSignalsBetweenStations(company, thought))
        {
            company.var_4A5 = 3;
        }
    }

    struct NearbyTrackResults
    {
        uint32_t numElements;                 // ebx ignores unowned aiallocated
        uint32_t numOwnedAiAllocatedElements; // edx
    };

    // 0x004A874D
    static NearbyTrackResults nearbyTrackElementsStats(World::TilePos2 middlePos)
    {
        const auto p1 = middlePos - World::TilePos2(7, 7);
        const auto p2 = middlePos + World::TilePos2(7, 7);

        NearbyTrackResults res{};

        for (const auto& tilePos : getClampedRange(p1, p2))
        {
            auto tile = TileManager::get(tilePos);
            bool hasPassedSurface = false;
            for (auto& el : tile)
            {
                auto* elSurface = el.as<SurfaceElement>();
                if (elSurface != nullptr)
                {
                    hasPassedSurface = true;
                    continue;
                }
                if (!hasPassedSurface)
                {
                    continue;
                }
                auto* elTrack = el.as<TrackElement>();
                if (elTrack == nullptr)
                {
                    continue;
                }
                if (elTrack->isGhost())
                {
                    continue;
                }
                if (!elTrack->hasBridge())
                {
                    continue;
                }
                if (elTrack->isAiAllocated())
                {
                    if (elTrack->owner() != GameCommands::getUpdatingCompanyId())
                    {
                        continue;
                    }
                    res.numOwnedAiAllocatedElements++;
                }
                res.numElements++;
            }
        }
        return res;
    }

    // 0x004865B4
    static uint8_t sub_4865B4(AiThought& thought)
    {
        if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased | ThoughtTypeFlags::waterBased | ThoughtTypeFlags::unk6))
        {
            const auto stationDist = Math::Vector::distance2D(thought.stations[0].pos, thought.stations[1].pos);
            if (stationDist < 224)
            {
                return 1;
            }
        }

        if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::railBased))
        {
            return 2;
        }
        for (auto i = 0U; i < 2; ++i)
        {
            auto [numElements, numOwnedAiAllocated] = nearbyTrackElementsStats(toTileSpace(thought.stations[i].pos));
            if (numOwnedAiAllocated > 4 && numElements > 120)
            {
                return 1;
            }
        }

        const auto mid = (thought.stations[0].pos + thought.stations[1].pos) / 2;
        auto [numElements, numOwnedAiAllocated] = nearbyTrackElementsStats(toTileSpace(mid));
        if (numOwnedAiAllocated > 4 && numElements > 120)
        {
            return 1;
        }
        return 2;
    }

    // 0x00430EEF
    static void sub_430EEF(Company& company, AiThought& thought)
    {
        // Decide if station placement attempt
        if ((company.challengeFlags & CompanyFlags::unk1) != CompanyFlags::none)
        {
            company.var_4A4 = AiThinkState::unk6;
            company.var_4A5 = 2;
            company.var_85C4 = World::Pos2{ 0, 0 };
            return;
        }
        auto res = sub_4865B4(thought);
        if (res == 1)
        {
            company.var_4A4 = AiThinkState::unk6;
            company.var_4A5 = 2;
            company.var_85C4 = World::Pos2{ 0, 0 };
        }
        else if (res == 2)
        {
            company.var_4A5 = 4;
        }
    }

    // 0x00486668
    static void sub_486668(Company& company, AiThought& thought)
    {
        thought.var_76 += estimateStationCost(thought);
        thought.var_76 += company.var_85F2;
    }

    // 0x00430F50
    static void sub_430F50(Company& company, AiThought& thought)
    {
        // Calculate station cost?
        if ((company.challengeFlags & CompanyFlags::unk1) != CompanyFlags::none)
        {
            company.var_4A4 = AiThinkState::unk6;
            company.var_4A5 = 2;
            company.var_85C4 = World::Pos2{ 0, 0 };
            return;
        }
        sub_486668(company, thought);
        company.var_4A5 = 5;
    }

    // 0x0048667A
    // return
    //   1 == No
    //   2 == Yes
    static uint8_t shouldConvertAiAllocated(Company& company, AiThought& thought)
    {
        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            return 1;
        }

        if (company.activeThoughtRevenueEstimate * 2 < thought.var_76)
        {
            return 1;
        }

        if (!CompanyManager::ensureCompanyFunding(company.id(), thought.var_76))
        {
            return 1;
        }

        const auto freeStations = StationManager::stations().capacity() - StationManager::stations().size();
        if (freeStations < 8)
        {
            return 1;
        }
        return 2;
    }

    // 0x00430F87
    static void sub_430F87(Company& company, AiThought& thought)
    {
        if ((company.challengeFlags & CompanyFlags::unk1) != CompanyFlags::none)
        {
            company.var_4A4 = AiThinkState::unk6;
            company.var_4A5 = 2;
            company.var_85C4 = World::Pos2{ 0, 0 };
            return;
        }

        const auto res = shouldConvertAiAllocated(company, thought);
        if (res == 1)
        {
            company.var_4A4 = AiThinkState::unk6;
            company.var_4A5 = 2;
            company.var_85C4 = World::Pos2{ 0, 0 };
        }
        else if (res == 2)
        {
            // Go to converting the aiAllocated items into real items
            company.var_4A4 = AiThinkState::unk4;
            company.var_4A5 = 0;
            company.challengeFlags |= CompanyFlags::unk2;
            if ((company.challengeFlags & CompanyFlags::unk0) != CompanyFlags::none)
            {
                return;
            }
            company.challengeFlags |= CompanyFlags::unk0;

            auto townId = static_cast<TownId>(thought.destinationA);
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationAIsIndustry))
            {
                townId = IndustryManager::get(static_cast<IndustryId>(thought.destinationA))->town;
            }
            const auto id = GameCommands::getUpdatingCompanyId();
            MessageManager::post(MessageType::newCompany, id, enumValue(id), enumValue(townId));
        }
    }

    using AiThinkState3Function = void (*)(Company&, AiThought&);

    static constexpr std::array<AiThinkState3Function, 6> _funcs_4F94E8 = {
        sub_430DDF,
        sub_430E21,
        sub_430EB5,
        sub_430EEF,
        sub_430F50,
        sub_430F87,
    };

    // 0x00430DB6
    static void aiThinkState3(Company& company)
    {
        company.var_85F6++;

        _funcs_4F94E8[company.var_4A5](company, company.aiThoughts[company.activeThoughtId]);
    }

    static StationElement* getAiAllocatedStationElement(const Pos3& pos)
    {
        auto tile = TileManager::get(pos.x, pos.y);
        auto baseZ = pos.z / 4;

        for (auto& element : tile)
        {
            auto* stationElement = element.as<StationElement>();

            if (stationElement == nullptr)
            {
                continue;
            }

            if (stationElement->baseZ() != baseZ)
            {
                continue;
            }

            if (stationElement->isAiAllocated())
            {
                return stationElement;
            }
            else
            {
                return nullptr;
            }
        }
        return nullptr;
    }

    struct RoadAndAiStationElements
    {
        const RoadElement* roadElement;
        const StationElement* stationElement;
    };

    static RoadAndAiStationElements getRoadAndAiStationElements(const World::Pos3 pos, const uint8_t rotation, const uint8_t roadObjectId, const uint8_t roadId, const uint8_t sequenceIndex, const bool noStations, const CompanyId companyId)
    {
        auto* newRoadObj = ObjectManager::get<RoadObject>(roadObjectId);
        auto tile = TileManager::get(pos);
        for (const auto& el : tile)
        {
            auto* elRoad = el.as<RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }
            if (elRoad->baseHeight() != pos.z)
            {
                continue;
            }
            if (elRoad->rotation() != rotation)
            {
                continue;
            }
            if (elRoad->sequenceIndex() != sequenceIndex)
            {
                continue;
            }
            if (noStations)
            {
                // Odd? why only in the no station branch!
                if (elRoad->owner() != companyId)
                {
                    continue;
                }
            }
            else
            {
                if (elRoad->roadObjectId() != roadObjectId)
                {
                    auto* existingRoadObj = ObjectManager::get<RoadObject>(elRoad->roadObjectId());
                    if (!existingRoadObj->hasFlags(RoadObjectFlags::unk_03))
                    {
                        continue;
                    }
                    if (!newRoadObj->hasFlags(RoadObjectFlags::unk_03))
                    {
                        continue;
                    }
                }
            }
            if (elRoad->roadId() != roadId)
            {
                continue;
            }

            if (!elRoad->hasStationElement())
            {
                return { elRoad, nullptr };
            }
            return { elRoad, getAiAllocatedStationElement(pos) };
        }
        return { nullptr, nullptr };
    }

    // 0x0047BA2C
    // Converts AiAllocated road to real road
    static uint32_t replaceAiAllocatedRoad(World::Pos3 pos, uint8_t rotation, uint8_t roadObjectId, uint8_t roadId, uint8_t sequenceIndex, bool noStations, uint8_t flags)
    {
        // Structured very like a game command
        GameCommands::setExpenditureType(ExpenditureType::Construction);
        const auto companyId = GameCommands::getUpdatingCompanyId();
        if (flags & GameCommands::Flags::apply)
        {
            const auto center = World::Pos2(pos) + World::Pos2{ 16, 16 };
            companySetObservation(companyId, ObservationStatus::buildingTrackRoad, center, EntityId::null, roadObjectId | (1U << 7));
        }

        auto [elRoad, elStation] = getRoadAndAiStationElements(pos, rotation, roadObjectId, roadId, sequenceIndex, noStations, companyId);
        if (elRoad == nullptr || (!elRoad->isAiAllocated() && elStation == nullptr))
        {
            return GameCommands::FAILURE;
        }
        uint8_t _112C2F4 = 0;
        uint8_t roadStationObjId = 0xFFU;
        if (!elRoad->isAiAllocated() && elStation != nullptr)
        {
            _112C2F4 |= 1U << 0; // hasNonAiAllocatedRoad
        }
        if (elStation != nullptr)
        {
            _112C2F4 |= 1U << 1; // hasStation
            roadStationObjId = elStation->objectId();
            if (noStations)
            {
                return GameCommands::FAILURE;
            }
        }

        currency32_t totalCost = 0;
        const auto existingRoadObjId = elRoad->roadObjectId();
        auto* existingRoadObj = ObjectManager::get<RoadObject>(existingRoadObjId);
        uint8_t mods = 0;
        if (!existingRoadObj->hasFlags(RoadObjectFlags::unk_03))
        {
            mods = elRoad->mods();
        }
        uint8_t bridge = 0xFFU;
        if (elRoad->hasBridge())
        {
            bridge = elRoad->bridge();
        }

        if (elStation != nullptr)
        {
            GameCommands::RoadStationRemovalArgs args{};
            args.pos = pos;
            args.roadId = roadId;
            args.roadObjectId = existingRoadObjId;
            args.rotation = rotation;
            args.index = sequenceIndex;
            GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment);
        }

        if (!(_112C2F4 & (1U << 0)))
        {
            assert(existingRoadObjId == roadObjectId);
            GameCommands::RoadRemovalArgs args{};
            args.pos = pos;
            args.objectId = existingRoadObjId;
            args.roadId = roadId;
            args.rotation = rotation;
            args.sequenceIndex = sequenceIndex;
            GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment);
            // Vanilla copied the bridge here via a global but
            // we have already previously done that
        }

        if (!(_112C2F4 & (1U << 0)))
        {
            GameCommands::RoadPlacementArgs args{};
            args.bridge = bridge;
            args.mods = mods;
            args.pos = pos;
            // We shift this down as we need to be at the start of the road piece
            // which for road that is sloping will not be the same as the
            // produced road element
            args.pos.z -= TrackData::getRoadPiece(roadId)[0].z;
            args.roadId = roadId;
            args.roadObjectId = roadObjectId;
            args.rotation = rotation;
            auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_7);
            if (res == GameCommands::FAILURE)
            {
                res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noPayment | GameCommands::Flags::flag_7);
            }
            if (res != GameCommands::FAILURE)
            {
                totalCost += res;
            }
        }

        if (elStation != nullptr)
        {
            GameCommands::RoadStationPlacementArgs args{};
            args.pos = pos;
            args.roadId = roadId;
            args.roadObjectId = existingRoadObjId;
            args.rotation = rotation;
            args.index = sequenceIndex;
            args.type = roadStationObjId;
            auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_7);
            if (res == GameCommands::FAILURE)
            {
                res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noPayment | GameCommands::Flags::flag_7);
            }
            if (res != GameCommands::FAILURE)
            {
                totalCost += res;
            }
            else
            {
                return GameCommands::FAILURE;
            }
        }

        // This total cost check seems counter productive as the AI is allowed
        // to build for free if it can't afford it. TODO: probably remove
        if ((flags & GameCommands::Flags::apply) && totalCost != 0)
        {
            GameCommands::playConstructionPlacementSound(pos);
        }
        return totalCost;
    }

    // 0x004866C8
    // Replaces AiAllocated station assets with real assets
    static uint8_t replaceAiAllocatedStation(const Company& company, AiThought& thought)
    {
        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            return 2;
        }

        bool found = false;
        auto i = 0U;
        for (; i < thought.numStations; ++i)
        {
            if (!thought.stations[i].hasFlags(AiThoughtStationFlags::aiAllocated))
            {
                continue;
            }
            if (thought.stations[i].hasFlags(AiThoughtStationFlags::operational))
            {
                continue;
            }
            found = true;
            break;
        }

        if (!found)
        {
            return 1;
        }

        auto& legacyGCReturn = GameCommands::getLegacyReturnState();
        auto& aiStation = thought.stations[i];
        const auto pos = World::Pos3(aiStation.pos, aiStation.baseZ * World::kSmallZStep);
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased))
        {
            {
                GameCommands::AirportRemovalArgs removeArgs{};
                removeArgs.pos = pos;
                GameCommands::doCommand(removeArgs, GameCommands::Flags::apply | GameCommands::Flags::noPayment | GameCommands::Flags::aiAllocated);
            }

            GameCommands::AirportPlacementArgs placeArgs{};
            placeArgs.pos = pos;
            placeArgs.rotation = aiStation.rotation;
            placeArgs.type = thought.stationObjId;

            if (GameCommands::doCommand(placeArgs, GameCommands::Flags::apply) == GameCommands::FAILURE)
            {
                if (GameCommands::doCommand(placeArgs, GameCommands::Flags::apply | GameCommands::Flags::noPayment) == GameCommands::FAILURE)
                {
                    return 2;
                }
            }

            if (legacyGCReturn.lastPlacedAirport != StationId::null)
            {
                aiStation.id = legacyGCReturn.lastPlacedAirport;
            }
        }
        else if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::waterBased))
        {
            {
                GameCommands::PortRemovalArgs removeArgs{};
                removeArgs.pos = pos;
                GameCommands::doCommand(removeArgs, GameCommands::Flags::apply | GameCommands::Flags::noPayment | GameCommands::Flags::aiAllocated);
            }

            GameCommands::PortPlacementArgs placeArgs{};
            placeArgs.pos = pos;
            placeArgs.rotation = aiStation.rotation;
            placeArgs.type = thought.stationObjId;

            if (GameCommands::doCommand(placeArgs, GameCommands::Flags::apply) == GameCommands::FAILURE)
            {
                if (GameCommands::doCommand(placeArgs, GameCommands::Flags::apply | GameCommands::Flags::noPayment) == GameCommands::FAILURE)
                {
                    return 2;
                }
            }

            if (legacyGCReturn.lastPlacedDock != StationId::null)
            {
                aiStation.id = legacyGCReturn.lastPlacedDock;
            }
        }
        else
        {
            if (thought.trackObjId & (1U << 7))
            {
                // Road

                // TODO: Vanilla bug passes rotation for flags???
                if (replaceAiAllocatedRoad(pos, aiStation.rotation, thought.trackObjId & ~(1U << 7), 0, 0, false, aiStation.rotation) == GameCommands::FAILURE)
                {
                    return 2;
                }

                if (legacyGCReturn.lastPlacedTrackRoadStationId != StationId::null)
                {
                    aiStation.id = legacyGCReturn.lastPlacedTrackRoadStationId;
                }
            }
            else
            {
                // Track
                GameCommands::AiTrackReplacementArgs placeArgs{};
                placeArgs.rotation = aiStation.rotation;
                placeArgs.trackObjectId = thought.trackObjId;
                placeArgs.trackId = 0;
                placeArgs.sequenceIndex = 0;
                placeArgs.pos = pos;

                for (auto j = 0U; j < thought.stationLength; ++j)
                {
                    if (GameCommands::doCommand(placeArgs, GameCommands::Flags::apply) == GameCommands::FAILURE)
                    {
                        if (GameCommands::doCommand(placeArgs, GameCommands::Flags::apply | GameCommands::Flags::noPayment) == GameCommands::FAILURE)
                        {
                            return 2;
                        }
                    }

                    if (legacyGCReturn.lastPlacedTrackRoadStationId != StationId::null)
                    {
                        aiStation.id = legacyGCReturn.lastPlacedTrackRoadStationId;
                    }
                    placeArgs.pos += World::Pos3{ kRotationOffset[placeArgs.rotation], 0 };
                }
            }
        }
        aiStation.var_02 &= ~AiThoughtStationFlags::aiAllocated;
        aiStation.var_02 |= AiThoughtStationFlags::operational;
        return 0;
    }

    // 0x004FE7AC
    constexpr std::array<uint8_t, 9> kAgrressivenessTable1 = { 25, 20, 15, 11, 8, 5, 3, 2, 1 };

    // 0x004869C2
    static void sub_4869C2(Company& company)
    {
        company.var_85C2 = 0xFF;
        company.var_85C3 = 0;
        company.var_85DE = 0;
        const auto* competitorObj = ObjectManager::get<CompetitorObject>(company.competitorId);
        company.var_85EA = kAgrressivenessTable1[competitorObj->aggressiveness - 1];
    }

    // 0x0043106B
    static void sub_43106B(Company& company, AiThought& thought)
    {
        const auto res = replaceAiAllocatedStation(company, thought);
        if (res == 2)
        {
            company.var_4A4 = AiThinkState::unk6;
            company.var_4A5 = 1;
            sub_487144(company);
        }
        else if (res == 1)
        {
            company.var_4A5 = 1;
            sub_4869C2(company);
        }
    }

    // 0x00486E63 & 0x00486C2C
    static void advanceAiStationAiAllocationReplacementState(Company& company, AiThought& thought)
    {
        const auto aiStationIndex = company.var_85C2;
        auto& aiStation = thought.stations[aiStationIndex];

        auto advanceStationSide = [](uint8_t& stationSide) {
            if (stationSide & (1U << 1))
            {
                stationSide &= ~(1U << 1);
                stationSide |= (1U << 2);
            }
            else
            {
                stationSide &= ~(1U << 3);
                stationSide |= (1U << 4);
            }
        };

        if (company.var_85C3 & (1U << 0))
        {
            advanceStationSide(aiStation.var_C);
        }
        else
        {
            advanceStationSide(aiStation.var_B);
        }
        company.var_85C2 = 0xFFU;
    }

    // 0x00486C98 & 0x00486A37
    // Returns 0 on aiStation found requiring replacement and 1 if no stations found
    static uint8_t setupNextAiStationAiAllocationReplacement(Company& company, AiThought& thought, bool isRoad)
    {
        for (auto i = 0U; i < thought.numStations; ++i)
        {
            if (thought.stations[i].var_B & 0b1010)
            {
                company.var_85C2 = i;
                company.var_85C3 &= ~(1U << 0);
                company.var_85D0 = thought.stations[i].pos;
                company.var_85D4 = thought.stations[i].baseZ;
                company.var_85D5 = thought.stations[i].rotation ^ (1U << 1);
                return 0;
            }
            else if (thought.stations[i].var_C & 0b1010)
            {
                company.var_85C2 = i;
                company.var_85C3 |= 1U << 0;
                const auto stationOffset = isRoad ? World::Pos2{ 0, 0 } : kRotationOffset[thought.stations[i].rotation] * (thought.stationLength - 1);
                company.var_85D0 = thought.stations[i].pos + stationOffset;
                company.var_85D4 = thought.stations[i].baseZ;
                company.var_85D5 = thought.stations[i].rotation;
                return 0;
            }
        }
        return 1;
    }

    // 0x004869F7
    // Replaces AiAllocated track / road assets with real assets
    static uint8_t replaceAiAllocatedTrackRoad(Company& company, AiThought& thought)
    {
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased | ThoughtTypeFlags::waterBased))
        {
            return 1;
        }

        company.var_85DE++;

        if (company.var_85DE < company.var_85EA)
        {
            return 0;
        }

        company.var_85DE = 0;
        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            return 2;
        }
        if (thought.trackObjId & (1U << 7))
        {
            // Road
            // 0x00486C98
            if (company.var_85C2 == 0xFFU)
            {
                return setupNextAiStationAiAllocationReplacement(company, thought, false);
            }
            else
            {
                // 0x00486D3E
                const auto pos = World::Pos3(company.var_85D0, company.var_85D4 * World::kSmallZStep);
                const auto tad = company.var_85D5;
                const auto queryMods = 0U;
                const auto requiredMods = 0U;
                const auto roadObjId = thought.trackObjId & ~(1U << 7);
                auto* roadObj = ObjectManager::get<RoadObject>(roadObjId);
                auto compatRoadObjId = roadObj->hasFlags(RoadObjectFlags::unk_03) ? 0xFFU : roadObjId;

                auto [nextPos, nextRotation] = Track::getRoadConnectionEnd(pos, tad);
                auto rc = Track::getRoadConnectionsAiAllocated(nextPos, nextRotation, company.id(), compatRoadObjId, requiredMods, queryMods);
                if (rc.connections.empty())
                {
                    advanceAiStationAiAllocationReplacementState(company, thought);
                    return 0;
                }
                else
                {
                    auto replaceTad = rc.connections[0] & Track::AdditionalTaDFlags::basicTaDMask;
                    auto replacePos = nextPos;

                    company.var_85D0 = replacePos;
                    company.var_85D4 = replacePos.z / World::kSmallZStep;
                    company.var_85D5 = replaceTad;

                    // This is completely wrong but it matches vanilla
                    // TODO: Remove and replace with 'apply' when we want to diverge
                    uint8_t flags = enumValue(company.id()) & 0b1;

                    if (replaceTad & (1U << 2))
                    {
                        auto& roadSize = TrackData::getUnkRoad(replaceTad);
                        replacePos += roadSize.pos;

                        // Again completely wrong but it matches vanilla
                        flags = roadSize.rotationEnd & 0b1;

                        replacePos.x -= kRotationOffset[roadSize.rotationEnd].x;
                        replacePos.y -= kRotationOffset[roadSize.rotationEnd].y;
                        replaceTad ^= (1U << 2);
                    }

                    const auto roadId = (replaceTad >> 3) & 0xF;
                    const auto rotation = replaceTad & 0x3;
                    replacePos.z += TrackData::getRoadPiece(roadId)[0].z;
                    const auto noStations = true;

                    const auto success = replaceAiAllocatedRoad(replacePos, rotation, roadObjId, roadId, 0, noStations, flags) != GameCommands::FAILURE;
                    return success ? 0 : 2;
                }
            }
        }
        else
        {
            // Rail
            // 0x00486A37
            if (company.var_85C2 == 0xFFU)
            {
                return setupNextAiStationAiAllocationReplacement(company, thought, true);
            }
            else
            {
                // 0x00486AFC
                const auto pos = World::Pos3(company.var_85D0, company.var_85D4 * World::kSmallZStep);
                const auto tad = company.var_85D5;
                const auto queryMods = 0U;
                const auto requiredMods = 0U;
                const auto trackObjId = thought.trackObjId;

                auto [nextPos, nextRotation] = Track::getTrackConnectionEnd(pos, tad);
                auto tc = Track::getTrackConnectionsAi(nextPos, nextRotation, company.id(), trackObjId, requiredMods, queryMods);
                if (tc.connections.empty())
                {
                    advanceAiStationAiAllocationReplacementState(company, thought);
                    return 0;
                }
                else
                {
                    auto replaceTad = tc.connections[0] & Track::AdditionalTaDFlags::basicTaDMask;
                    auto replacePos = nextPos;

                    company.var_85D0 = replacePos;
                    company.var_85D4 = replacePos.z / World::kSmallZStep;
                    company.var_85D5 = replaceTad;

                    if (replaceTad & (1U << 2))
                    {
                        auto& trackSize = TrackData::getUnkTrack(replaceTad);
                        replacePos += trackSize.pos;
                        if (trackSize.rotationEnd <= 12)
                        {
                            replacePos.x -= kRotationOffset[trackSize.rotationEnd].x;
                            replacePos.y -= kRotationOffset[trackSize.rotationEnd].y;
                        }
                        replaceTad ^= (1U << 2);
                    }

                    const auto trackId = (replaceTad >> 3) & 0x3F;
                    const auto rotation = replaceTad & 0x3;
                    replacePos.z += TrackData::getTrackPiece(trackId)[0].z;

                    GameCommands::AiTrackReplacementArgs args{};
                    args.pos = replacePos;
                    args.rotation = rotation;
                    args.trackObjectId = trackObjId;
                    args.trackId = trackId;
                    args.sequenceIndex = 0;

                    const auto success = GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE;
                    if (!success)
                    {
                        if (GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noPayment) == GameCommands::FAILURE)
                        {
                            return 2;
                        }
                    }
                    return 0;
                }
            }
        }
    }

    // 0x0043109A
    static void sub_43109A(Company& company, AiThought& thought)
    {
        const auto res = replaceAiAllocatedTrackRoad(company, thought);
        if (res == 2)
        {
            company.var_4A4 = AiThinkState::unk6;
            company.var_4A5 = 1;
            sub_487144(company);
        }
        else if (res == 1)
        {
            company.var_4A5 = 2;
        }
    }

    // 0x004310C4
    static void sub_4310C4(Company& company, AiThought& thought)
    {
        const auto res = purchaseVehicle(company, thought);
        if (res == PurchaseVehicleResult::failure)
        {
            company.var_4A4 = AiThinkState::unk6;
            company.var_4A5 = 0;
        }
        else if (res == PurchaseVehicleResult::allVehiclesPurchased)
        {
            // Move onto the next stage of this AiThinkState
            company.var_4A5 = 3;
        }
        // else will keep purchasing vehicles until allVehiclesPurchased
    }

    // 0x004310E9
    static void sub_4310E9(Company& company, AiThought& thought)
    {
        sub_4876CB(thought);
        companyEmotionEvent(company.id(), Emotion::happy);
        company.var_4A4 = AiThinkState::unk0;
    }

    using AiThinkState4Function = void (*)(Company&, AiThought&);

    static constexpr std::array<AiThinkState4Function, 4> _funcs_4F9500 = {
        sub_43106B,
        sub_43109A,
        sub_4310C4,
        sub_4310E9,
    };

    // 0x00431035
    static void aiThinkState4(Company& company)
    {
        companyEmotionEvent(company.id(), Emotion::thinking);
        company.var_85F6++;

        _funcs_4F9500[company.var_4A5](company, company.aiThoughts[company.activeThoughtId]);
    }

    static void nullsub_3([[maybe_unused]] Company& company)
    {
    }

    // 0x00487BA3
    // Sells one at a time
    // returns true when there are no vehicles left to sell
    static bool sellAiThoughtVehicle(AiThought& thought)
    {
        bool hasTriedToSell = false;
        for (auto i = 0U; i < thought.numVehicles; ++i)
        {
            hasTriedToSell = true;
            auto* head = EntityManager::get<Vehicles::VehicleHead>(thought.vehicles[i]);
            if (head == nullptr)
            {
                // Hmm should we cleanup this entry?
                continue;
            }
            head->breakdownFlags &= ~Vehicles::BreakdownFlags::breakdownPending;
            if (!head->hasVehicleFlags(VehicleFlags::commandStop))
            {
                GameCommands::VehicleChangeRunningModeArgs args{};
                args.head = head->head;
                args.mode = GameCommands::VehicleChangeRunningModeArgs::Mode::stopVehicle;
                GameCommands::doCommand(args, GameCommands::Flags::apply);
            }

            if (head->tileX != -1)
            {
                switch (head->mode)
                {
                    case TransportMode::rail:
                    case TransportMode::road:
                    {
                        GameCommands::VehiclePickupArgs args{};
                        args.head = head->id;
                        GameCommands::doCommand(args, GameCommands::Flags::apply);
                        break;
                    }
                    case TransportMode::air:
                    {
                        GameCommands::VehiclePickupAirArgs gcArgs{};
                        gcArgs.head = head->id;
                        GameCommands::doCommand(gcArgs, GameCommands::Flags::apply);
                        break;
                    }
                    case TransportMode::water:
                    {
                        GameCommands::VehiclePickupWaterArgs gcArgs{};
                        gcArgs.head = head->id;
                        GameCommands::doCommand(gcArgs, GameCommands::Flags::apply);
                        break;
                    }
                }
            }

            if (head->tileX != -1)
            {
                continue;
            }

            GameCommands::VehicleSellArgs args{};
            args.car = head->id;
            auto res = GameCommands::doCommand(args, GameCommands::Flags::apply);
            if (res == GameCommands::FAILURE)
            {
                continue;
            }
            removeEntityFromThought(thought, i);
            break;
        }
        return !hasTriedToSell;
    }

    // 0x004311B5
    static void sub_4311B5(Company& company, AiThought& thought)
    {
        if (sellAiThoughtVehicle(thought))
        {
            company.var_4A5 = 1;
            sub_487144(company);
        }
    }

    enum class TrackRoadRemoveQueryFlags : uint8_t
    {
        none = 0,
        roadTypeShouldNotBeRemoved = 1U << 0,
        // if there is an element missing from road/track
        // perhaps a user with cheats removed a different companies road
        malformed = 1U << 1,
        hasStation = 1U << 2,
        isRoadJunction = 1U << 3,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(TrackRoadRemoveQueryFlags);

    // 0x0046C1E0
    static TrackRoadRemoveQueryFlags queryRoadForRemoval(World::Pos3 pos, uint16_t tad, uint8_t roadObjId, CompanyId companyId)
    {
        using enum TrackRoadRemoveQueryFlags;
        auto* roadObj = ObjectManager::get<RoadObject>(roadObjId);
        if (roadObj->hasFlags(RoadObjectFlags::unk_03))
        {
            return roadTypeShouldNotBeRemoved;
        }

        auto roadStart = pos;
        if (tad & (1U << 2))
        {
            auto& roadSize = TrackData::getUnkRoad(tad);
            roadStart = pos + roadSize.pos;
            tad ^= (1U << 2);
            if (roadSize.rotationEnd < 12)
            {
                roadStart -= World::Pos3{ kRotationOffset[roadSize.rotationEnd], 0 };
            }
        }

        const auto rotation = tad & 0x3;
        const auto roadId = (tad >> 3) & 0xF;

        TrackRoadRemoveQueryFlags flags = none;
        auto& roadPieces = TrackData::getRoadPiece(roadObjId);
        for (auto& piece : roadPieces)
        {
            const auto rotatedPiece = World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, rotation), piece.z };
            const auto piecePos = roadStart + rotatedPiece;
            auto* elRoad = [piecePos, rotation, &piece, &flags, roadId, roadObjId, companyId]() -> const World::RoadElement* {
                const auto tile = World::TileManager::get(piecePos);
                for (const auto& el : tile)
                {
                    auto* elRoad = el.as<RoadElement>();
                    if (elRoad == nullptr)
                    {
                        continue;
                    }
                    if (elRoad->baseHeight() != piecePos.z)
                    {
                        continue;
                    }
                    if (elRoad->isGhost() || elRoad->isAiAllocated())
                    {
                        continue;
                    }
                    if (elRoad->roadObjectId() != roadObjId)
                    {
                        continue;
                    }

                    if (elRoad->rotation() != rotation)
                    {
                        flags |= isRoadJunction;
                        continue;
                    }
                    if (elRoad->sequenceIndex() != piece.index)
                    {
                        flags |= isRoadJunction;
                        continue;
                    }
                    if (elRoad->owner() != companyId)
                    {
                        flags |= isRoadJunction;
                        continue;
                    }
                    if (elRoad->roadId() != roadId)
                    {
                        flags |= isRoadJunction;
                        continue;
                    }
                    return elRoad;
                }
                return nullptr;
            }();

            if (elRoad == nullptr)
            {
                return malformed;
            }
            if (elRoad->hasStationElement())
            {
                flags |= hasStation;
            }
        }
        return flags;
    }

    // 0x004A83C0
    static TrackRoadRemoveQueryFlags queryTrackForRemoval(World::Pos3 pos, uint16_t tad, uint8_t trackObjId, CompanyId companyId)
    {
        using enum TrackRoadRemoveQueryFlags;

        auto trackStart = pos;
        if (tad & (1U << 2))
        {
            auto& trackSize = TrackData::getUnkTrack(tad);
            trackStart = pos + trackSize.pos;
            tad ^= (1U << 2);
            if (trackSize.rotationEnd < 12)
            {
                trackStart -= World::Pos3{ kRotationOffset[trackSize.rotationEnd], 0 };
            }
        }

        const auto rotation = tad & 0x3;
        const auto trackId = (tad >> 3) & 0x3F;

        TrackRoadRemoveQueryFlags flags = none;
        auto& trackPieces = TrackData::getTrackPiece(trackId);
        for (auto& piece : trackPieces)
        {
            const auto rotatedPiece = World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, rotation), piece.z };
            const auto piecePos = trackStart + rotatedPiece;
            auto* elTrack = [piecePos, rotation, &piece, trackId, trackObjId, companyId]() -> const World::TrackElement* {
                const auto tile = World::TileManager::get(piecePos);
                for (const auto& el : tile)
                {
                    auto* elTrack = el.as<TrackElement>();
                    if (elTrack == nullptr)
                    {
                        continue;
                    }
                    if (elTrack->baseHeight() != piecePos.z)
                    {
                        continue;
                    }
                    if (elTrack->isAiAllocated()) // Why no ghost?
                    {
                        continue;
                    }
                    if (elTrack->trackObjectId() != trackObjId)
                    {
                        continue;
                    }

                    if (elTrack->rotation() != rotation)
                    {
                        continue;
                    }
                    if (elTrack->sequenceIndex() != piece.index)
                    {
                        continue;
                    }
                    if (elTrack->owner() != companyId)
                    {
                        continue;
                    }
                    if (elTrack->trackId() != trackId)
                    {
                        continue;
                    }
                    return elTrack;
                }
                return nullptr;
            }();

            if (elTrack == nullptr)
            {
                return malformed;
            }
            if (elTrack->hasStationElement())
            {
                flags |= hasStation;
            }
        }
        return flags;
    }

    // 0x0047B238
    static void removeRoadStationAndRoad(World::Pos3 pos, uint8_t rotation, uint8_t roadObjId, CompanyId companyId)
    {
        auto args = GameCommands::RoadStationRemovalArgs{};
        args.index = 0;
        args.pos = pos;
        args.roadId = 0;
        args.roadObjectId = roadObjId;
        args.rotation = rotation;

        // Calling GC directly to match vanilla.
        // TODO change to use GameCommands::doCommand as this isn't handling costs
        auto regs = static_cast<registers>(args);
        regs.bl = GameCommands::Flags::apply;
        GameCommands::removeRoadStation(regs);
        if (static_cast<uint32_t>(regs.ebx) != GameCommands::FAILURE)
        {
            args.rotation ^= (1u << 1);
            auto regs2 = static_cast<registers>(args);
            regs2.bl = GameCommands::Flags::apply;
            GameCommands::removeRoadStation(regs2);
        }

        auto* roadObj = ObjectManager::get<RoadObject>(roadObjId);
        if (!roadObj->hasFlags(RoadObjectFlags::unk_03))
        {
            using enum TrackRoadRemoveQueryFlags;
            if ((queryRoadForRemoval(pos, (0 << 3) | rotation, roadObjId, companyId) & (malformed | roadTypeShouldNotBeRemoved)) == none)
            {
                auto roadArgs = GameCommands::RoadRemovalArgs{};
                roadArgs.pos = pos;
                roadArgs.objectId = roadObjId;
                roadArgs.roadId = 0;
                roadArgs.rotation = rotation;
                roadArgs.sequenceIndex = 0;
                GameCommands::doCommand(roadArgs, GameCommands::Flags::apply);
            }
        }
    }

    // 0x004A7E07
    static void removeTrainStationAndTrack(World::Pos3 pos, uint8_t rotation, uint8_t trackObjId, uint8_t stationLength)
    {
        auto piecePos = pos;
        for (auto i = 0U; i < stationLength; ++i)
        {
            auto args = GameCommands::TrackRemovalArgs{};
            args.index = 0;
            args.pos = piecePos;
            args.rotation = rotation;
            args.trackId = 0;
            args.trackObjectId = trackObjId;
            // Calling GC directly to match vanilla.
            // TODO change to use GameCommands::doCommand as this isn't handling costs
            auto regs = static_cast<registers>(args);
            regs.bl = GameCommands::Flags::apply;
            GameCommands::removeTrack(regs);

            piecePos += World::Pos3{ kRotationOffset[rotation], 0 };
        }
    }

    // 0x004836EB
    static bool sub_4836EB(AiThought& thought, CompanyId companyId)
    {
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased | ThoughtTypeFlags::waterBased))
        {
            return true;
        }

        auto operationalStationIdx = [&thought]() {
            for (auto i = 0; i < thought.numStations; ++i)
            {
                auto& aiStation = thought.stations[i];
                if (aiStation.hasFlags(AiThoughtStationFlags::operational))
                {
                    return i;
                }
            }
            return -1;
        }();
        if (operationalStationIdx == -1)
        {
            return true;
        }

        auto& aiStation = thought.stations[operationalStationIdx];
        if (thought.trackObjId & (1U << 7))
        {
            const auto roadObjId = thought.trackObjId & ~(1U << 7);

            removeRoadStationAndRoad(
                World::Pos3(aiStation.pos, aiStation.baseZ * kSmallZStep), aiStation.rotation, roadObjId, companyId);
        }
        else
        {
            removeTrainStationAndTrack(
                World::Pos3(aiStation.pos, aiStation.baseZ * kSmallZStep), aiStation.rotation, thought.trackObjId, thought.stationLength);
        }
        aiStation.var_02 &= ~AiThoughtStationFlags::operational;
        return false;
    }

    // 0x0048715C
    static bool sub_48715C(Company& company, AiThought& thought)
    {
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased | ThoughtTypeFlags::waterBased)
            || (company.var_85C3 & (1U << 2)))
        {
            return sub_4836EB(thought, company.id());
        }
        // 0x00487183
        company.var_85F0++;
        if (company.var_85F0 >= 1600)
        {
            company.var_85C3 |= (1U << 2);
            return false;
        }
        using enum TrackRoadRemoveQueryFlags;
        if (thought.trackObjId & (1U << 7))
        {
            const auto roadObjId = thought.trackObjId & ~(1U << 7);
            // 0x00487432
            if (company.var_85C3 & (1U << 1))
            {
                const auto pos = World::Pos3(company.var_85D0, company.var_85D4 * kSmallZStep);
                const auto tad = company.var_85D5;
                const auto roadFlags = queryRoadForRemoval(pos, tad, roadObjId, company.id());

                if ((roadFlags & (malformed | roadTypeShouldNotBeRemoved | isRoadJunction)) != none)
                {
                    company.var_85C3 &= ~(1U << 1);
                    return false;
                }

                // Get the next road (will be used after the removal)
                auto [nextPos, nextRotation] = Track::getRoadConnectionEnd(pos, tad);
                const auto rc = Track::getRoadConnections(nextPos, nextRotation, company.id(), roadObjId, 0, 0);

                auto roadStart = pos;
                auto roadStartTad = tad;
                if (roadStartTad & (1U << 2))
                {
                    auto& roadSize = TrackData::getUnkRoad(roadStartTad);
                    roadStart = pos + roadSize.pos;
                    roadStartTad ^= (1U << 2);
                    if (roadSize.rotationEnd < 12)
                    {
                        roadStart -= World::Pos3{ kRotationOffset[roadSize.rotationEnd], 0 };
                    }
                }

                const auto roadId = (roadStartTad >> 3) & 0xF;
                const auto rotation = roadStartTad & 0x3;
                roadStart.z += TrackData::getRoadPiece(roadId)[0].z;

                auto args = GameCommands::RoadRemovalArgs{};
                args.pos = roadStart;
                args.objectId = roadObjId;
                args.roadId = roadId;
                args.rotation = rotation;
                args.sequenceIndex = 0;
                const auto success = GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE;
                // If we fail for some reason or we are at the end of the road
                // we progress to the next phase of road removal
                if (!success || rc.connections.empty())
                {
                    company.var_85C3 &= ~(1U << 1);
                    return false;
                }

                company.var_85D0 = nextPos;
                company.var_85D4 = nextPos.z / kSmallZStep;
                company.var_85D5 = rc.connections[0] & Track::AdditionalTaDFlags::basicTaDMask;
                return false;
            }
            else
            {
                auto stationIdx = company.var_85C2;
                auto& aiStation = thought.stations[stationIdx];
                std::optional<World::Pos3> stationPos;
                uint8_t rotation = 0U;
                if (company.var_85C3 & (1U << 0))
                {
                    if (aiStation.var_A != 0xFFU)
                    {
                        stationPos = World::Pos3(aiStation.pos, aiStation.baseZ * kSmallZStep);
                        rotation = aiStation.rotation;
                    }
                }
                else
                {
                    if (aiStation.var_9 != 0xFFU)
                    {
                        stationPos = World::Pos3(aiStation.pos, aiStation.baseZ * kSmallZStep);
                        rotation = aiStation.rotation ^ (1U << 1);
                    }
                }
                if (stationPos.has_value())
                {
                    uint16_t tad = rotation | (0U << 3);
                    auto [nextPos, nextRotation] = Track::getRoadConnectionEnd(stationPos.value(), tad);
                    const auto rc = Track::getRoadConnections(nextPos, nextRotation, company.id(), roadObjId, 0, 0);
                    for (const auto connection : rc.connections)
                    {
                        const auto tadConnection = connection & Track::AdditionalTaDFlags::basicTaDMask;

                        const auto roadFlags = queryRoadForRemoval(nextPos, tadConnection, roadObjId, company.id());

                        if ((roadFlags & (roadTypeShouldNotBeRemoved | malformed | hasStation | isRoadJunction)) != none)
                        {
                            continue;
                        }

                        company.var_85D0 = nextPos;
                        company.var_85D4 = nextPos.z / kSmallZStep;
                        company.var_85D5 = tadConnection;
                        company.var_85C3 |= (1U << 1);
                        return false;
                    }
                }
                // 0x00487504
                if (company.var_85C3 & (1U << 0))
                {
                    company.var_85C3 &= ~(1U << 0);
                    company.var_85C2++;
                    if (company.var_85C2 >= thought.numStations)
                    {
                        company.var_85C3 |= (1U << 2); // All stations removed
                    }
                }
                else
                {
                    company.var_85C3 |= (1U << 0);
                }
                return false;
            }
        }
        else
        {
            // 0x0048718D
            if (company.var_85C3 & (1U << 1))
            {
                const auto pos = World::Pos3(company.var_85D0, company.var_85D4 * kSmallZStep);
                const auto tad = company.var_85D5;
                const auto roadFlags = queryTrackForRemoval(pos, tad, thought.trackObjId, company.id());

                if ((roadFlags & (malformed | hasStation)) != none)
                {
                    company.var_85C3 &= ~(1U << 1);
                    return false;
                }

                // Get the next track (will be used after the removal)
                auto [nextPos, nextRotation] = Track::getTrackConnectionEnd(pos, tad);
                const auto tc = Track::getTrackConnections(nextPos, nextRotation, company.id(), thought.trackObjId, 0, 0);

                auto trackStart = pos;
                auto trackStartTad = tad;
                if (trackStartTad & (1U << 2))
                {
                    auto& trackSize = TrackData::getUnkTrack(trackStartTad);
                    trackStart = pos + trackSize.pos;
                    trackStartTad ^= (1U << 2);
                    if (trackSize.rotationEnd < 12)
                    {
                        trackStart -= World::Pos3{ kRotationOffset[trackSize.rotationEnd], 0 };
                    }
                }

                const auto trackId = (trackStartTad >> 3) & 0x3F;
                const auto rotation = trackStartTad & 0x3;
                trackStart.z += TrackData::getTrackPiece(trackId)[0].z;

                auto args = GameCommands::TrackRemovalArgs{};
                args.pos = trackStart;
                args.trackObjectId = thought.trackObjId;
                args.trackId = trackId;
                args.rotation = rotation;
                args.index = 0;
                const auto success = GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE;
                // If we fail for some reason or we are at the end of the track
                // we progress to the next phase of track removal
                if (!success || tc.connections.empty())
                {
                    company.var_85C3 &= ~(1U << 1);
                    return false;
                }

                company.var_85D0 = nextPos;
                company.var_85D4 = nextPos.z / kSmallZStep;
                company.var_85D5 = tc.connections[0] & Track::AdditionalTaDFlags::basicTaDMask;
                return false;
            }
            else
            {
                auto stationIdx = company.var_85C2;
                auto& aiStation = thought.stations[stationIdx];
                std::optional<World::Pos3> stationPos;
                uint8_t rotation = 0U;
                if (company.var_85C3 & (1U << 0))
                {
                    if (aiStation.var_A != 0xFFU)
                    {
                        const auto stationLength = kRotationOffset[aiStation.rotation] * (thought.stationLength - 1);
                        stationPos = World::Pos3(aiStation.pos + stationLength, aiStation.baseZ * kSmallZStep);
                        rotation = aiStation.rotation;
                    }
                }
                else
                {
                    if (aiStation.var_9 != 0xFFU)
                    {
                        stationPos = World::Pos3(aiStation.pos, aiStation.baseZ * kSmallZStep);
                        rotation = aiStation.rotation ^ (1U << 1);
                    }
                }
                if (stationPos.has_value())
                {
                    uint16_t tad = rotation | (0U << 3);
                    auto [nextPos, nextRotation] = Track::getTrackConnectionEnd(stationPos.value(), tad);
                    const auto tc = Track::getTrackConnections(nextPos, nextRotation, company.id(), thought.trackObjId, 0, 0);
                    for (const auto connection : tc.connections)
                    {
                        const auto tadConnection = connection & Track::AdditionalTaDFlags::basicTaDMask;

                        const auto trackFlags = queryTrackForRemoval(nextPos, tadConnection, thought.trackObjId, company.id());

                        if ((trackFlags & (malformed | hasStation)) != none)
                        {
                            continue;
                        }

                        company.var_85D0 = nextPos;
                        company.var_85D4 = nextPos.z / kSmallZStep;
                        company.var_85D5 = tadConnection;
                        company.var_85C3 |= (1U << 1);
                        return false;
                    }
                }
                // 0x0048727C
                if (company.var_85C3 & (1U << 0))
                {
                    company.var_85C3 &= ~(1U << 0);
                    company.var_85C2++;
                    if (company.var_85C2 >= thought.numStations)
                    {
                        company.var_85C3 |= (1U << 2); // All stations removed
                    }
                }
                else
                {
                    company.var_85C3 |= (1U << 0);
                }
                return false;
            }
        }
    }

    // 0x0043112D
    static void sub_43112D(Company& company, AiThought& thought)
    {
        // identical to sub_4311B5 but separate so that
        // we can maybe enforce types for the state machine
        if (sellAiThoughtVehicle(thought))
        {
            company.var_4A5 = 1;
            sub_487144(company);
        }
    }

    // 0x00431142
    static void sub_431142(Company& company, AiThought& thought)
    {
        if (sub_48715C(company, thought))
        {
            company.var_4A5 = 2;
            company.var_85C4 = World::Pos2{ 0, 0 };
        }
    }

    // 0x00486224
    static void removeAiAllocatedCompanyTracksRoadsOnTile(const World::Pos2 pos)
    {
        auto tile = World::TileManager::get(pos);
        bool recheck = true;
        while (recheck)
        {
            recheck = false;
            for (auto& el : tile)
            {
                if (!el.isAiAllocated())
                {
                    continue;
                }
                auto* elRoad = el.as<RoadElement>();
                auto* elTrack = el.as<TrackElement>();
                if (elRoad != nullptr)
                {
                    if (elRoad->owner() != GameCommands::getUpdatingCompanyId())
                    {
                        continue;
                    }
                    if (elRoad->hasStationElement())
                    {
                        auto* station = getAiAllocatedStationElement(World::Pos3{ pos, elRoad->baseHeight() });
                        if (station != nullptr)
                        {
                            continue;
                        }
                    }

                    GameCommands::RoadRemovalArgs args{};
                    args.pos = World::Pos3{ pos, elRoad->baseHeight() };
                    args.objectId = elRoad->roadObjectId();
                    args.roadId = elRoad->roadId();
                    args.rotation = elRoad->rotation();
                    args.sequenceIndex = elRoad->sequenceIndex();
                    auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment);
                    if (res != GameCommands::FAILURE)
                    {
                        recheck = true;
                        break;
                    }
                }
                else if (elTrack != nullptr)
                {
                    if (elTrack->owner() != GameCommands::getUpdatingCompanyId())
                    {
                        continue;
                    }
                    if (elTrack->hasStationElement())
                    {
                        continue;
                    }

                    GameCommands::TrackRemovalArgs args{};
                    args.pos = World::Pos3{ pos, elTrack->baseHeight() };
                    args.trackObjectId = elTrack->trackObjectId();
                    args.trackId = elTrack->trackId();
                    args.rotation = elTrack->rotation();
                    args.index = elTrack->sequenceIndex();
                    auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment);
                    if (res != GameCommands::FAILURE)
                    {
                        recheck = true;
                        break;
                    }
                }
            }
        }
    }

    // 0x004861BF
    // returns false until the whole map has been iterated
    static bool removeAllAiAllocatedCompanyAssetsOnMapByChunk(Company& company, AiThought& thought)
    {
        auto pos = company.var_85C4;
        bool fullySearched = false;
        for (auto i = 0U; i < 1500; ++i)
        {
            removeAiAllocatedCompanyTracksRoadsOnTile(pos);

            pos.x += 32;
            if (pos.x < World::kMapWidth)
            {
                continue;
            }
            pos.x = 0;
            pos.y += 32;
            if (pos.y < World::kMapHeight)
            {
                continue;
            }

            fullySearched = true;
            break;
        }
        if (fullySearched)
        {
            for (auto i = 0U; i < thought.numStations; ++i)
            {
                auto& aiStation = thought.stations[i];
                aiStation.var_B &= ~((1U << 1) | (1U << 3));
                aiStation.var_C &= ~((1U << 1) | (1U << 3));
            }
            return true;
        }
        else
        {
            company.var_85C4 = pos;
            return false;
        }
    }

    // 0x00431164
    static void sub_431164(Company& company, AiThought& thought)
    {
        if (removeAllAiAllocatedCompanyAssetsOnMapByChunk(company, thought))
        {
            company.var_4A5 = 3;
        }
    }

    // 0x0047B107
    static void removeAiAllocatedRoadStation(World::Pos3 pos, uint8_t rotation, uint8_t objectId)
    {
        {
            GameCommands::RoadStationRemovalArgs args{};
            args.pos = pos;
            args.rotation = rotation;
            args.roadObjectId = objectId;
            args.roadId = 0;
            args.index = 0;

            auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment);
            if (res == GameCommands::FAILURE)
            {
                // Try remove facing the opposite rotation
                args.rotation ^= (1U << 1);
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment);
            }
        }

        auto tile = World::TileManager::get(pos);
        for (auto& el : tile)
        {
            auto* elRoad = el.as<RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }
            if (elRoad->baseHeight() != pos.z)
            {
                continue;
            }
            if (elRoad->rotation() != rotation)
            {
                continue;
            }
            if (elRoad->roadObjectId() != objectId)
            {
                continue;
            }
            if (elRoad->roadId() != 0)
            {
                continue;
            }
            if (elRoad->owner() != GameCommands::getUpdatingCompanyId())
            {
                continue;
            }
            if (elRoad->isAiAllocated())
            {
                GameCommands::RoadRemovalArgs args{};
                args.objectId = objectId;
                args.pos = pos;
                args.rotation = rotation;
                args.roadId = 0;
                args.sequenceIndex = 0;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment);
            }
            break;
        }
    }

    // 0x004A7CD2
    static void removeAiAllocatedTrainStation(const World::Pos3 pos, const uint8_t rotation, const uint8_t objectId, const uint8_t stationLength)
    {
        auto stationPos = pos;
        for (auto i = 0U; i < stationLength; ++i)
        {
            auto nonAiAllocated = 0U;
            auto tile = World::TileManager::get(stationPos);
            for (auto& el : tile)
            {
                auto* elTrack = el.as<TrackElement>();
                if (elTrack == nullptr)
                {
                    continue;
                }
                if (elTrack->baseHeight() != stationPos.z)
                {
                    continue;
                }
                if (elTrack->rotation() != rotation)
                {
                    continue;
                }
                if (elTrack->trackObjectId() != objectId)
                {
                    continue;
                }
                if (elTrack->sequenceIndex() != 0)
                {
                    continue;
                }
                if (elTrack->trackId() != 0)
                {
                    continue;
                }
                if (elTrack->owner() != GameCommands::getUpdatingCompanyId())
                {
                    continue;
                }
                if (elTrack->isAiAllocated())
                {
                    GameCommands::TrackRemovalArgs args{};
                    args.trackObjectId = objectId;
                    args.pos = stationPos;
                    args.rotation = rotation;
                    args.trackId = 0;
                    args.index = 0;
                    GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment);
                }
                else
                {
                    nonAiAllocated++;
                }
                break;
            }
            // Really? Seems a bit odd
            if (nonAiAllocated)
            {
                GameCommands::TrackRemovalArgs args{};
                args.trackObjectId = objectId;
                args.pos = stationPos;
                args.rotation = rotation;
                args.trackId = 0;
                args.index = 0;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noPayment);
            }

            stationPos += World::Pos3{ kRotationOffset[rotation], 0 };
        }
    }

    // 0x00483602
    static uint8_t removeAiAllocatedStations(AiThought& thought)
    {
        for (auto i = 0U; i < thought.numStations; ++i)
        {
            auto& aiStation = thought.stations[i];
            if (aiStation.hasFlags(AiThoughtStationFlags::operational))
            {
                continue;
            }
            if (!aiStation.hasFlags(AiThoughtStationFlags::aiAllocated))
            {
                continue;
            }
            const auto pos = World::Pos3(aiStation.pos, aiStation.baseZ * World::kSmallZStep);
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased))
            {
                GameCommands::AirportRemovalArgs args{};
                args.pos = pos;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment);
            }
            else if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::waterBased))
            {
                GameCommands::PortRemovalArgs args{};
                args.pos = pos;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::aiAllocated | GameCommands::Flags::noPayment);
            }
            else if (thought.trackObjId & (1U << 7))
            {
                removeAiAllocatedRoadStation(pos, aiStation.rotation, thought.trackObjId & ~(1U << 7));
            }
            else
            {
                removeAiAllocatedTrainStation(pos, aiStation.rotation, thought.trackObjId, thought.stationLength);
            }
            // Ai assumes removal was a success!
            aiStation.var_02 &= ~AiThoughtStationFlags::aiAllocated;
            return 0;
        }
        return 1;
    }

    // 0x00431174
    static void sub_431174(Company& company, AiThought& thought)
    {
        if (removeAiAllocatedStations(thought) != 0)
        {
            company.var_4A5 = 4;
        }
    }

    // 0x00431186
    static void sub_431186(Company& company, AiThought& thought)
    {
        clearThought(thought);
        company.var_4A4 = AiThinkState::unk0;
    }

    // 0x004F9510
    static constexpr std::array<AiThinkState4Function, 5> kFuncs4F9510 = {
        sub_43112D,
        sub_431142,
        sub_431164,
        sub_431174,
        sub_431186,
    };

    // 0x00431104
    static void aiThinkState6(Company& company)
    {
        // try sell a vehicle?

        company.var_85F6++;
        kFuncs4F9510[company.var_4A5](company, company.aiThoughts[company.activeThoughtId]);
    }

    // 0x004311CA
    static void sub_4311CA(Company& company, AiThought& thought)
    {
        if (sub_48715C(company, thought))
        {
            company.var_4A5 = 2;
        }
    }

    // 0x004311DA
    static void sub_4311DA(Company& company, AiThought& thought)
    {
        clearThought(thought);
        company.var_4A4 = AiThinkState::unk1;
    }

    using AiThinkState7Function = void (*)(Company&, AiThought&);

    static constexpr std::array<AiThinkState7Function, 3> _funcs_4F9524 = {
        sub_4311B5,
        sub_4311CA,
        sub_4311DA,
    };

    // 0x00431193
    static void aiThinkState7(Company& company)
    {
        _funcs_4F9524[company.var_4A5](company, company.aiThoughts[company.activeThoughtId]);
    }

    // 0x00487DAD
    static uint32_t tryPlaceTrackOrRoadMods(AiThought& thought, uint8_t flags)
    {
        if (thought.trackObjId & (1U << 7))
        {
            GameCommands::RoadModsPlacementArgs args{};
            args.pos = World::Pos3(thought.stations[0].pos, thought.stations[0].baseZ * World::kSmallZStep);
            args.rotation = thought.stations[0].rotation;
            args.roadObjType = thought.trackObjId & ~(1U << 7);
            args.index = 0;
            args.modSection = Track::ModSection::allConnected;
            args.roadId = 0;
            args.type = thought.mods;
            const auto cost = GameCommands::doCommand(args, flags);
            if (cost != GameCommands::FAILURE)
            {
                return cost;
            }
            return GameCommands::doCommand(args, flags | GameCommands::Flags::noPayment);
        }
        else
        {
            GameCommands::TrackModsPlacementArgs args{};
            args.pos = World::Pos3(thought.stations[0].pos, thought.stations[0].baseZ * World::kSmallZStep);
            args.rotation = thought.stations[0].rotation;
            args.trackObjType = thought.trackObjId;
            args.index = 0;
            args.modSection = Track::ModSection::allConnected;
            args.trackId = 0;
            args.type = thought.mods;
            const auto cost = GameCommands::doCommand(args, flags);
            if (cost != GameCommands::FAILURE)
            {
                return cost;
            }
            return GameCommands::doCommand(args, flags | GameCommands::Flags::noPayment);
        }
    }

    // 0x00487C83
    static void sub_487C83(AiThought& thought)
    {
        // Gets refund costs for vehicles and costs for track mods

        thought.var_76 = 0;
        if (thought.hasPurchaseFlags(AiPurchaseFlags::unk2))
        {
            for (auto i = 0U; i < thought.numVehicles; ++i)
            {
                auto* head = EntityManager::get<Vehicles::VehicleHead>(thought.vehicles[i]);
                if (head == nullptr)
                {
                    continue;
                }
                Vehicles::Vehicle train(*head);
                for (auto& car : train.cars)
                {
                    thought.var_76 -= car.front->refundCost;
                }
            }
        }
        currency32_t pendingVehicleCarCosts = 0;
        for (auto i = 0U; i < thought.var_45; ++i)
        {
            auto* vehicleObj = ObjectManager::get<VehicleObject>(thought.var_46[i]);
            auto objCost = Economy::getInflationAdjustedCost(vehicleObj->costFactor, vehicleObj->costIndex, 6);
            pendingVehicleCarCosts += objCost;
            if (vehicleObj->hasFlags(VehicleObjectFlags::mustHavePair))
            {
                pendingVehicleCarCosts += objCost;
            }
        }
        auto numPendingVehicles = thought.var_43;
        if (!thought.hasPurchaseFlags(AiPurchaseFlags::unk2))
        {
            numPendingVehicles -= thought.numVehicles;
        }
        thought.var_76 += pendingVehicleCarCosts * numPendingVehicles;

        if (thought.hasPurchaseFlags(AiPurchaseFlags::requiresMods))
        {
            thought.var_76 += tryPlaceTrackOrRoadMods(thought, 0);
        }
    }

    // 0x00431209
    static void sub_431209(Company& company, AiThought& thought)
    {
        sub_487C83(thought);
        company.var_4A5 = 1;
    }

    // 0x00431216
    static void sub_431216(Company& company, AiThought&)
    {
        // branch on sub_487E6D (which is a nop) would have made var_4A4 = 1
        company.var_4A5 = 2;
    }

    // 0x00487E74
    static bool sub_487E74(Company& company, AiThought& thought)
    {
        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            return true;
        }

        if (!thought.hasPurchaseFlags(AiPurchaseFlags::requiresMods))
        {
            return false;
        }

        return tryPlaceTrackOrRoadMods(thought, GameCommands::Flags::apply) == GameCommands::FAILURE;
    }

    // 0x0043122D
    static void sub_43122D(Company& company, AiThought& thought)
    {
        if (sub_487E74(company, thought))
        {
            company.var_4A4 = AiThinkState::unk1;
        }
        else
        {
            company.var_4A5 = 3;
        }
    }

    // 0x00487EA0
    // Sells one at a time
    // returns true when there are no vehicles left to sell
    static bool sellAiThoughtVehicleIfRequired(AiThought& thought)
    {
        if (!thought.hasPurchaseFlags(AiPurchaseFlags::unk2))
        {
            return true;
        }

        return sellAiThoughtVehicle(thought);
    }

    // 0x00431244
    static void sub_431244(Company& company, AiThought& thought)
    {
        if (sellAiThoughtVehicleIfRequired(thought))
        {
            company.var_4A5 = 4;
        }
    }

    // 0x00431254
    static void sub_431254(Company& company, AiThought& thought)
    {
        const auto res = purchaseVehicle(company, thought);
        if (res == PurchaseVehicleResult::failure)
        {
            company.var_4A4 = AiThinkState::unk7;
            company.var_4A5 = 0;
        }
        else if (res == PurchaseVehicleResult::allVehiclesPurchased)
        {
            // Move onto the next stage of this AiThinkState
            company.var_4A5 = 5;
        }
        // else will keep purchasing vehicles until allVehiclesPurchased
    }

    // 0x00431279
    static void sub_431279(Company& company, AiThought& thought)
    {
        sub_4876CB(thought);
        company.var_4A4 = AiThinkState::unk1;
    }

    using Unk4311E7ThinkFunction = void (*)(Company&, AiThought&);

    static constexpr std::array<Unk4311E7ThinkFunction, 6> _funcs_4F9530 = {
        sub_431209,
        sub_431216,
        sub_43122D,
        sub_431244,
        sub_431254,
        sub_431279,
    };

    // 0x004311E7
    static void aiThinkState8(Company& company)
    {
        _funcs_4F9530[company.var_4A5](company, company.aiThoughts[company.activeThoughtId]);
    }

    static void nullsub_4([[maybe_unused]] Company& company)
    {
    }

    // 0x00488563
    static void removeCompanyTracksRoadsOnTile(CompanyId id, World::TilePos2 tilePos)
    {
        bool reTryTile = true;
        while (reTryTile)
        {
            reTryTile = false;

            auto tile = World::TileManager::get(tilePos);
            for (auto& el : tile)
            {
                auto* elTrack = el.as<World::TrackElement>();
                auto* elRoad = el.as<World::RoadElement>();
                if (elTrack != nullptr)
                {
                    if (elTrack->owner() != id)
                    {
                        continue;
                    }
                    reTryTile = true;

                    GameCommands::TrackRemovalArgs args{};
                    args.pos = World::Pos3(World::toWorldSpace(tilePos), elTrack->baseHeight());
                    args.index = elTrack->sequenceIndex();
                    args.rotation = elTrack->rotation();
                    args.trackId = elTrack->trackId();
                    args.trackObjectId = elTrack->trackObjectId();
                    auto aiPreviewFlag = elTrack->isAiAllocated() ? GameCommands::Flags::aiAllocated : 0;
                    GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noPayment | aiPreviewFlag);
                    break;
                }
                if (elRoad != nullptr)
                {
                    if (elRoad->owner() != id)
                    {
                        continue;
                    }
                    reTryTile = true;

                    GameCommands::RoadRemovalArgs args{};
                    args.pos = World::Pos3(World::toWorldSpace(tilePos), elRoad->baseHeight());
                    args.sequenceIndex = elRoad->sequenceIndex();
                    args.rotation = elRoad->rotation();
                    args.roadId = elRoad->roadId();
                    args.objectId = elRoad->roadObjectId();
                    auto aiPreviewFlag = elRoad->isAiAllocated() ? GameCommands::Flags::aiAllocated : 0;
                    GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noPayment | aiPreviewFlag);
                    break;
                }
            }
        }
    }

    // 0x004884E7
    // returns false until the whole map has been iterated
    static bool removeAllCompanyAssetsOnMapByChunk(Company& company)
    {
        auto remainingRange = World::TilePosRangeView(
            World::toTileSpace(company.var_85C4),
            World::TilePos2{ World::kMapColumns - 1, World::kMapRows - 1 });

        auto count = 1500;
        for (auto& tilePos : remainingRange)
        {
            removeCompanyTracksRoadsOnTile(company.id(), tilePos);
            count--;
            // TODO: Remove when divergence from vanilla as this is silly
            if (tilePos.x == World::kMapColumns - 1)
            {
                count--;
            }
            if (count == 0)
            {
                company.var_85C4 = World::toWorldSpace(tilePos);
                return false;
            }
        }

        if (company.headquartersX != -1)
        {
            GameCommands::HeadquarterRemovalArgs args{};
            args.pos = World::Pos3(company.headquartersX, company.headquartersY, company.headquartersZ * World::kSmallZStep);
            GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noPayment);
        }
        return true;
    }

    // 0x00431287
    static void aiThinkEndCompany(Company& company)
    {
        if (!removeAllCompanyAssetsOnMapByChunk(company))
        {
            return;
        }
        CompanyManager::aiDestroy(company.id());
    }

    using UnknownThinkFunction = void (*)(Company&);

    static constexpr std::array<UnknownThinkFunction, 11> _funcs_430786 = {
        aiThinkState0,
        aiThinkState1,
        aiThinkState2,
        aiThinkState3,
        aiThinkState4,
        nullsub_3,
        aiThinkState6,
        aiThinkState7,
        aiThinkState8,
        nullsub_4,
        aiThinkEndCompany,
    };

    // 0x00430762
    void aiThink(const CompanyId id)
    {
        // const auto updatingCompanyId = GameCommands::getUpdatingCompanyId();

        // Ensure this is only used for Non-Player controlled companies.
        if (CompanyManager::isPlayerCompany(id))
        {
            return;
        }

        auto* company = CompanyManager::get(id);

        const auto thinkFunc1 = _funcs_430786[enumValue(company->var_4A4)];
        thinkFunc1(*company);

        if (company->empty())
        {
            return;
        }

        processVehiclePlaceStateMachine(*company);

        if (company->headquartersX != -1 || (company->challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none || (company->challengeFlags & CompanyFlags::unk0) == CompanyFlags::none)
        {
            return;
        }

        // Look for an entry with either town or industry assigned.
        auto index = std::size(company->aiThoughts);
        while (company->aiThoughts[--index].type == AiThoughtType::null)
        {
            if (index == 0)
            {
                return;
            }
        }

        auto& thought = company->aiThoughts[index];

        World::Pos2 pos;
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationAIsIndustry))
        {
            auto* industry = IndustryManager::get(static_cast<IndustryId>(thought.destinationA));
            pos = { industry->x, industry->y };
        }
        else
        {
            auto* town = TownManager::get(static_cast<TownId>(thought.destinationA));
            pos = { town->x, town->y };
        }

        auto& prng = gPrng1();
        const auto randPick = prng.randNext();
        // Random tile position 32x32 tiles centred on 0,0 i.e. +-16 tiles
        const auto randPos = World::Pos2{
            static_cast<coord_t>(randPick & 0x3E0),
            static_cast<coord_t>(std::rotr<uint32_t>(randPick, 5) & 0x3E0)
        } - World::toWorldSpace(World::TilePos2{ 16, 16 });

        const auto selectedPos = randPos + pos;
        if (World::validCoords(selectedPos))
        {
            auto tile = World::TileManager::get(selectedPos);
            auto* surface = tile.surface();

            coord_t z = surface->baseHeight();
            if (surface->slope() != 0)
            {
                z += 16;
            }

            const auto rot = (std::rotr<uint32_t>(randPick, 10)) & 0x3;
            const auto buildingType = CompanyManager::getHeadquarterBuildingType();

            GameCommands::HeadquarterPlacementArgs args;
            args.pos = World::Pos3(selectedPos, z);
            args.rotation = rot;
            args.type = buildingType;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }
    }

    // 0x0043821D
    void setAiObservation(CompanyId id)
    {
        auto* company = CompanyManager::get(id);
        if (company->var_4A4 == AiThinkState::unk3)
        {
            World::Pos2 pos{};
            auto& thought = company->aiThoughts[company->activeThoughtId];
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationAIsIndustry))
            {
                auto* industry = IndustryManager::get(static_cast<IndustryId>(thought.destinationA));
                pos = World::Pos2{ industry->x, industry->y };
            }
            else
            {
                // Interestingly var_01 isn't a uint16_t
                auto* town = TownManager::get(static_cast<TownId>(thought.destinationA));
                pos = World::Pos2{ town->x, town->y };
            }
            companySetObservation(id, ObservationStatus::surveyingLandscape, pos, EntityId::null, 0xFFFFU);
        }
        else
        {
            if (company->observationTimeout != 0)
            {
                return;
            }

            const auto totalVehicles = std::accumulate(std::begin(company->transportTypeCount), std::end(company->transportTypeCount), 0);

            const auto randVehicleNum = totalVehicles * static_cast<uint64_t>(gPrng1().randNext()) / (1ULL << 32);
            auto i = 0U;
            EntityId randVehicle = EntityId::null;
            for (auto* vehicle : VehicleManager::VehicleList())
            {
                if (vehicle->owner != id)
                {
                    continue;
                }
                if (vehicle->has38Flags(Vehicles::Flags38::isGhost))
                {
                    continue;
                }
                if (i == randVehicleNum)
                {
                    randVehicle = vehicle->id;
                    break;
                }
                i++;
            }
            if (randVehicle == EntityId::null)
            {
                return;
            }
            Vehicles::Vehicle train(randVehicle);
            if (train.veh2->position.x != Location::null)
            {
                companySetObservation(id, ObservationStatus::checkingServices, train.veh2->position, train.head->id, 0xFFFFU);
            }
        }
    }

    static void removeEntityFromThought(AiThought& thought, size_t index)
    {
        auto* end = std::end(thought.vehicles);
        auto* iter = std::begin(thought.vehicles) + index;
        while (iter != end)
        {
            if (iter + 1 == end)
            {
                // TODO: This is purely so we match vanilla but its just copying some rubbish into our array
                // change this when we diverge to EntityId::null
                *iter = static_cast<EntityId>(thought.var_76 & 0xFFFFU);
                break;
            }
            *iter = *(iter + 1);
            iter++;
        }
        thought.numVehicles--;
    }

    void removeEntityFromThought(AiThought& thought, EntityId id)
    {
        auto iter = std::find(std::begin(thought.vehicles), std::end(thought.vehicles), id);
        if (iter == std::end(thought.vehicles))
        {
            return;
        }

        removeEntityFromThought(thought, std::distance(std::begin(thought.vehicles), iter));
    }

    static Pos2 getDestinationPosition(bool isIndustry, uint8_t destination)
    {
        if (isIndustry)
        {
            const auto* industry = IndustryManager::get(static_cast<IndustryId>(destination));
            return Pos2{ industry->x, industry->y };
        }
        else
        {
            auto* town = TownManager::get(static_cast<TownId>(destination));
            return Pos2{ town->x, town->y };
        }
    }

    Pos2 AiThought::getDestinationPositionA() const
    {
        return getDestinationPosition(thoughtTypeHasFlags(type, ThoughtTypeFlags::destinationAIsIndustry), destinationA);
    }

    Pos2 AiThought::getDestinationPositionB() const
    {
        return getDestinationPosition(thoughtTypeHasFlags(type, ThoughtTypeFlags::destinationBIsIndustry), destinationB);
    }
}
