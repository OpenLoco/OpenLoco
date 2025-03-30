#include "CompanyAi.h"
#include "CompanyAiPlaceVehicle.h"
#include "Date.h"
#include "Economy/Economy.h"
#include "GameCommands/Airports/CreateAirport.h"
#include "GameCommands/Airports/RemoveAirport.h"
#include "GameCommands/Company/BuildCompanyHeadquarters.h"
#include "GameCommands/Company/RemoveCompanyHeadquarters.h"
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

    static loco_global<StationId, 0x0112C730> _lastPlacedTrackStationId;
    static loco_global<StationId, 0x0112C744> _lastPlacedAirportStationId;
    static loco_global<StationId, 0x0112C748> _lastPlacedPortStationId;
    static loco_global<EntityId, 0x0113642A> _lastCreatedVehicleId;

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
    static constexpr std::array<uint8_t, kAiThoughtTypeCount> kThoughtTypeEstimatedCostMultiplier = {
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
                trainHeadId = _lastCreatedVehicleId;
            }
            // There was some broken code that would try read the head as a body here

            // auto* veh = EntityManager::get<Vehicles::VehicleBogie>(_lastCreatedVehicleId); lol no this wouldn't work
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

    struct VehiclePurchaseRequest
    {
        uint8_t numVehicleObjects; // cl
        uint8_t dl;                // dl
        currency32_t ebx;          // ebx
        currency32_t eax;          // eax
    };

    // 0x004802D0
    static VehiclePurchaseRequest aiGenerateVehiclePurchaseRequest(AiThought& thought, uint16_t* requestBuffer)
    {
        registers regs;
        regs.esi = X86Pointer(requestBuffer);
        regs.edi = X86Pointer(&thought);
        call(0x004802D0, regs);
        VehiclePurchaseRequest res{};
        res.numVehicleObjects = regs.cl;
        res.dl = regs.dl;
        res.ebx = regs.ebx;
        res.eax = regs.eax;
        return res;
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
                    const auto purchaseRequest = aiGenerateVehiclePurchaseRequest(thought, thought.var_46);
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

        const auto purchaseRequest = aiGenerateVehiclePurchaseRequest(thought, thought.var_46);
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
                        return distance < 33 * 32;
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

        auto destinationPosition = [](bool isIndustry, uint8_t destination) {
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
        };
        const auto destAPos = destinationPosition(thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationAIsIndustry), thought.destinationA);

        World::Pos2 destBPos = destAPos;
        if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::singleDestination))
        {
            destBPos = destinationPosition(thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationBIsIndustry), thought.destinationB);
        }

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
            const auto otherDestAPos = destinationPosition(thoughtTypeHasFlags(otherThought.type, ThoughtTypeFlags::destinationAIsIndustry), otherThought.destinationA);
            const auto distAA = Math::Vector::distance2D(destAPos, otherDestAPos);
            if (distAA <= 60 * 32)
            {
                otherThoughtNearby = true;
                break;
            }
            const auto distBA = Math::Vector::distance2D(destBPos, otherDestAPos);
            if (distBA <= 60 * 32)
            {
                otherThoughtNearby = true;
                break;
            }
            if (thoughtTypeHasFlags(otherThought.type, ThoughtTypeFlags::singleDestination))
            {
                continue;
            }
            const auto otherDestBPos = destinationPosition(thoughtTypeHasFlags(otherThought.type, ThoughtTypeFlags::destinationBIsIndustry), otherThought.destinationB);
            const auto distAB = Math::Vector::distance2D(destAPos, otherDestBPos);
            if (distAB <= 60 * 32)
            {
                otherThoughtNearby = true;
                break;
            }
            const auto distBB = Math::Vector::distance2D(destBPos, otherDestBPos);
            if (distBB <= 60 * 32)
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

    // 0x00430BDA
    static void sub_430BDA(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x00430BDA, regs);
    }

    struct DestinationPositions
    {
        Pos2 posA;
        std::optional<Pos2> posB;
    };
    static DestinationPositions getDestinationPositions(const AiThought& thought)
    {
        DestinationPositions destPos{};
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationAIsIndustry))
        {
            const auto* industry = IndustryManager::get(static_cast<IndustryId>(thought.destinationA));
            destPos.posA = { industry->x, industry->y };
        }
        else
        {
            const auto* town = TownManager::get(static_cast<TownId>(thought.destinationA));
            destPos.posA = { town->x, town->y };
        }
        if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::singleDestination))
        {
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::destinationBIsIndustry))
            {
                const auto* industry = IndustryManager::get(static_cast<IndustryId>(thought.destinationB));
                destPos.posB = { industry->x, industry->y };
            }
            else
            {
                const auto* town = TownManager::get(static_cast<TownId>(thought.destinationB));
                destPos.posB = { town->x, town->y };
            }
        }
        return destPos;
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
        const auto request = aiGenerateVehiclePurchaseRequest(thought, thought.var_46);
        if (request.numVehicleObjects == 0)
        {
            state2ClearActiveThought(company);
            return;
        }
        thought.var_45 = request.numVehicleObjects;
        thought.var_43 = request.dl;
        thought.var_7C = request.dl * request.ebx;
        thought.var_76 += request.eax;
        company.var_85F2 = request.eax;
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
        uint8_t costMultiplier = kThoughtTypeEstimatedCostMultiplier[enumValue(thought.type)];

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
                costMultiplier *= thought.var_04;
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

        GameCommands::Unk53Args args{};
        args.pos = newStationPos;
        args.rotation = aiStation.rotation;
        args.roadObjectId = thought.trackObjId & ~(1U << 7);
        args.stationObjectId = thought.stationObjId;
        args.stationLength = thought.var_04;
        if (aiStation.var_9 != 0xFFU)
        {
            args.unk1 |= (1U << 1);
        }
        if (aiStation.var_A != 0xFFU)
        {
            args.unk1 |= (1U << 0);
        }
        args.unk2 = company.var_259A;

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

        GameCommands::Unk51Args args{};
        args.pos = newStationPos;
        args.rotation = aiStation.rotation;
        args.trackObjectId = thought.trackObjId;
        args.stationObjectId = thought.stationObjId;
        args.stationLength = thought.var_04;
        if (aiStation.var_9 != 0xFFU)
        {
            args.unk1 |= (1U << 1);
        }
        if (aiStation.var_A != 0xFFU)
        {
            args.unk1 |= (1U << 0);
        }
        args.unk2 = company.var_259A;

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

        const auto length = thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::railBased) ? thought.var_04 : 1;
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

    // 0x00483FBA
    static bool sub_483FBA(Company& company, AiThought& thought)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        regs.edi = X86Pointer(&thought);
        return call(0x00483FBA, regs) & X86_FLAG_CARRY;
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
                    const auto stationEndDiff = kRotationOffset[rotation] * (thought.var_04 - 1);
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
                    const auto stationEndDiff = kRotationOffset[rotation] * (thought.var_04 - 1);
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
            if (sub_483FBA(company, thought))
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
        const uint16_t minSignalSpacing = thought.var_04 * 32;

        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk6))
        {
            // 0x0048639B
            if (company.var_85C2 >= thought.numStations)
            {
                return true;
            }

            const auto& aiStation = thought.stations[company.var_85C2];

            const auto stationEnd = World::Pos3(
                aiStation.pos + World::Pos3{ kRotationOffset[aiStation.rotation], 0 } * (thought.var_04 - 1),
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
    // Replaces AiAllocated assets with real assets
    static uint8_t replaceAiAllocated(const Company& company, AiThought& thought)
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

            if (_lastPlacedAirportStationId != StationId::null)
            {
                aiStation.id = _lastPlacedAirportStationId;
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

            if (_lastPlacedPortStationId != StationId::null)
            {
                aiStation.id = _lastPlacedPortStationId;
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

                if (_lastPlacedTrackStationId != StationId::null)
                {
                    aiStation.id = _lastPlacedTrackStationId;
                }
            }
            else
            {
                // Track
                GameCommands::Unk52Args placeArgs{};
                placeArgs.rotation = aiStation.rotation;
                placeArgs.trackObjectId = thought.trackObjId;
                placeArgs.unk = 0;
                placeArgs.pos = pos;

                for (auto j = 0U; j < thought.var_04; ++j)
                {
                    if (GameCommands::doCommand(placeArgs, GameCommands::Flags::apply) == GameCommands::FAILURE)
                    {
                        if (GameCommands::doCommand(placeArgs, GameCommands::Flags::apply | GameCommands::Flags::noPayment) == GameCommands::FAILURE)
                        {
                            return 2;
                        }
                    }

                    if (_lastPlacedTrackStationId != StationId::null)
                    {
                        aiStation.id = _lastPlacedTrackStationId;
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
        const auto res = replaceAiAllocated(company, thought);
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

    // 0x004869F7
    static uint8_t sub_4869F7(const Company& company, const AiThought& thought)
    {
        // gc_unk_52?
        registers regs;
        regs.esi = X86Pointer(&company);
        regs.edi = X86Pointer(&thought);
        call(0x004869F7, regs);
        return regs.al;
    }

    // 0x0043109A
    static void sub_43109A(Company& company, AiThought& thought)
    {
        const auto res = sub_4869F7(company, thought);
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

    // 0x0048715C
    static bool sub_48715C(Company& company, AiThought& thought)
    {
        // fair amount of logic
        // removes roads/tracks
        registers regs;
        regs.esi = X86Pointer(&company);
        regs.edi = X86Pointer(&thought);
        return call(0x0048715C, regs) & X86_FLAG_CARRY;
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
                removeAiAllocatedTrainStation(pos, aiStation.rotation, thought.trackObjId, thought.var_04);
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
}
