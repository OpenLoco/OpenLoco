#include "CompanyAi.h"
#include "Company.h"
#include "CompanyManager.h"
#include "Date.h"
#include "Economy/Economy.h"
#include "GameCommands/Airports/CreateAirport.h"
#include "GameCommands/Airports/RemoveAirport.h"
#include "GameCommands/Company/BuildCompanyHeadquarters.h"
#include "GameCommands/Company/RemoveCompanyHeadquarters.h"
#include "GameCommands/Docks/CreatePort.h"
#include "GameCommands/Docks/RemovePort.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Road/CreateRoadMod.h"
#include "GameCommands/Road/RemoveRoad.h"
#include "GameCommands/Road/RemoveRoadStation.h"
#include "GameCommands/Track/CreateSignal.h"
#include "GameCommands/Track/CreateTrackMod.h"
#include "GameCommands/Track/RemoveTrack.h"
#include "GameCommands/Track/RemoveTrainStation.h"
#include "GameCommands/Vehicles/CreateVehicle.h"
#include "GameCommands/Vehicles/VehicleOrderInsert.h"
#include "GameCommands/Vehicles/VehicleOrderSkip.h"
#include "GameCommands/Vehicles/VehicleRefit.h"
#include "Industry.h"
#include "IndustryManager.h"
#include "Map/IndustryElement.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "MessageManager.h"
#include "Objects/BridgeObject.h"
#include "Objects/CargoObject.h"
#include "Objects/CompetitorObject.h"
#include "Objects/DockObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Objects/RoadStationObject.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Objects/TrainStationObject.h"
#include "Random.h"
#include "Station.h"
#include "StationManager.h"
#include "TownManager.h"
#include "Ui/Windows/Construction/Construction.h"
#include "Vehicles/Orders.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <bit>
#include <numeric>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco
{
    static loco_global<StationId, 0x0112C730> _lastPlacedTrackStationId;
    static loco_global<StationId, 0x0112C744> _lastPlacedAirportStationId;
    static loco_global<StationId, 0x0112C748> _lastPlacedPortStationId;
    static loco_global<EntityId, 0x0113642A> _lastCreatedVehicleId;

    enum class ThoughtTypeFlags : uint32_t
    {
        none = 0U,

        unk0 = 1U << 0,
        unk1 = 1U << 1,
        unk2 = 1U << 2,
        unk3 = 1U << 3,
        unk4 = 1U << 4,
        unk5 = 1U << 5,
        unk6 = 1U << 6,
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
        ThoughtTypeFlags::unk0 | ThoughtTypeFlags::unk3 | ThoughtTypeFlags::unk6 | ThoughtTypeFlags::unk11,
        ThoughtTypeFlags::unk0 | ThoughtTypeFlags::unk4 | ThoughtTypeFlags::unk14,
        ThoughtTypeFlags::unk0 | ThoughtTypeFlags::unk4 | ThoughtTypeFlags::unk6 | ThoughtTypeFlags::unk14,
        ThoughtTypeFlags::unk3 | ThoughtTypeFlags::unk11,
        ThoughtTypeFlags::unk3 | ThoughtTypeFlags::unk11 | ThoughtTypeFlags::unk17,
        ThoughtTypeFlags::unk0 | ThoughtTypeFlags::unk5 | ThoughtTypeFlags::unk10 | ThoughtTypeFlags::unk12,
        ThoughtTypeFlags::unk5 | ThoughtTypeFlags::unk8 | ThoughtTypeFlags::unk12,
        ThoughtTypeFlags::unk1 | ThoughtTypeFlags::unk2 | ThoughtTypeFlags::unk3 | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::unk8 | ThoughtTypeFlags::unk11,
        ThoughtTypeFlags::unk1 | ThoughtTypeFlags::unk2 | ThoughtTypeFlags::unk3 | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::unk8 | ThoughtTypeFlags::unk11 | ThoughtTypeFlags::unk17,
        ThoughtTypeFlags::unk1 | ThoughtTypeFlags::unk3 | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::unk8 | ThoughtTypeFlags::unk11,
        ThoughtTypeFlags::unk1 | ThoughtTypeFlags::unk3 | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::unk8 | ThoughtTypeFlags::unk11 | ThoughtTypeFlags::unk17,
        ThoughtTypeFlags::unk1 | ThoughtTypeFlags::unk2 | ThoughtTypeFlags::unk5 | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::unk8 | ThoughtTypeFlags::unk13,
        ThoughtTypeFlags::unk1 | ThoughtTypeFlags::unk5 | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::unk8 | ThoughtTypeFlags::unk13,
        ThoughtTypeFlags::airBased,
        ThoughtTypeFlags::unk1 | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::airBased,
        ThoughtTypeFlags::waterBased,
        ThoughtTypeFlags::unk1 | ThoughtTypeFlags::unk2 | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::waterBased,
        ThoughtTypeFlags::unk1 | ThoughtTypeFlags::unk7 | ThoughtTypeFlags::waterBased,
        ThoughtTypeFlags::unk1 | ThoughtTypeFlags::unk3 | ThoughtTypeFlags::unk11,
        ThoughtTypeFlags::unk1 | ThoughtTypeFlags::unk3 | ThoughtTypeFlags::unk11 | ThoughtTypeFlags::unk17,
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

    static bool thoughtTypeHasFlags(AiThoughtType type, ThoughtTypeFlags flags)
    {
        return (kThoughtTypeFlags[enumValue(type)] & flags) != ThoughtTypeFlags::none;
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
                && thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk1))
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
        if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk1))
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
                head->var_61 = thought.stations[0].pos.x;
                head->var_63 = thought.stations[0].pos.y;
                head->var_67 = thought.stations[0].baseZ;
                head->var_65 = thought.stations[0].rotation;
            }
            else
            {
                head->var_61 = thought.stations[0].pos.x;
                head->var_63 = thought.stations[0].pos.y;
                head->var_67 = thought.stations[0].baseZ;

                uint8_t rotation = thought.stations[0].rotation;
                if (thought.trackObjId & (1U << 7))
                {
                    if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk6))
                    {
                        if (i & 0b1)
                        {
                            rotation ^= (1U << 2);
                        }
                    }
                }
                head->var_65 = rotation;
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

    // 0x00488050
    static bool sub_488050(const Company& company, const AiThought& thought)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        regs.edi = X86Pointer(&thought);
        return call(0x00488050, regs) & X86_FLAG_CARRY;
    }

    // 0x00430971
    static void aiThinkState1(Company& company)
    {
        company.activeThoughtId++;
        if (company.activeThoughtId < kMaxAiThoughts)
        {
            const auto& thought = company.aiThoughts[company.activeThoughtId];
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

    // 0x004309FD
    static void aiThinkState2(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x004309FD, regs);
    }

    // 0x004834C0, 0x0048352E, 0x00493594
    template<typename Filter>
    static uint8_t sub_4834C0(const AiThought& thought, Filter&& bridgeFilter)
    {
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased | ThoughtTypeFlags::waterBased))
        {
            return 0;
        }

        std::array<uint8_t, 9> bridges{};
        if (thought.trackObjId & (1U << 7))
        {
            const uint8_t roadObjId = thought.trackObjId & ~(1U << 7);
            Ui::Windows::Construction::Common::refreshBridgeList(bridges.data(), roadObjId, TransportMode::road);
        }
        else
        {
            Ui::Windows::Construction::Common::refreshBridgeList(bridges.data(), thought.trackObjId, TransportMode::rail);
        }

        const auto chosenBridge = [&bridges, &bridgeFilter]() {
            Speed16 maxSpeed = kSpeedZero;
            uint8_t bestBridge = 0xFFU;
            for (auto bridgeObjId : bridges)
            {
                if (bridgeObjId == 0xFFU)
                {
                    break;
                }
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
        auto* airportObj = ObjectManager::get<AirportObject>(thought.var_89);
        auto [minPos, maxPos] = airportObj->getAirportExtents(newAirportTilePos, aiStation.rotation);
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

        const auto [acceptedCargo, producedCargo] = calcAcceptedCargoAi(minPos, maxPos);
        const bool shouldCreateAirport = [&thought, aiStationIdx, producedCargo, acceptedCargo]() {
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
        args.type = thought.var_89;
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

        // Same as air
        const auto [acceptedCargo, producedCargo] = calcAcceptedCargoAi(minPos, maxPos);
        const bool shouldCreatePort = [&thought, aiStationIdx, producedCargo, acceptedCargo]() {
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
        args.type = thought.var_89;
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
    static bool sub_482A00_road(Company& , AiThought& , uint8_t , const World::Pos3 ) { return true; }

    // 0x00482914
    static bool sub_482914_rail(Company& company, AiThought& thought, uint8_t aiStationIdx, const World::Pos3 newStationPos)
    {
        auto& aiStation = thought.stations[aiStationIdx];

        GameCommands::Unk51Args args{};
        args.pos = newStationPos;
        args.rotation = aiStation.rotation;
        args.trackObjectId = thought.trackObjId;
        args.stationObjectId = thought.var_89;
        args.stationLength = thought.var_04;
        if (aiStation.var_9 != 0xFFU)
        {
            args.unk1 |= (1U << 1);
        }
        if (aiStation.var_A != 0xFFU)
        {
            args.unk1 |= (1U << 1);
        }
        args.unk2 = company.var_259A;

        auto* trackObj = ObjectManager::get<TrackObject>(thought.trackObjId);
        for (auto i = 0U; i < 4; ++i)
        {
            if (trackObj->mods[i] != 0xFFU
                && (thought.mods & (1U << i)))
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

        const auto length = thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk3) ? thought.var_04 : 1;
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
        auto maxPos = minPos + toTileSpace(kRotationOffset[aiStation.rotation]) * checkLength;

        minPos.x = std::min(minPos.x, maxPos.x);
        minPos.y = std::min(minPos.y, maxPos.y);
        maxPos.x = std::max(minPos.x, maxPos.x);
        maxPos.y = std::max(minPos.y, maxPos.y);

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

        stationMin.x = std::min(stationMin.x, stationMax.x);
        stationMin.y = std::min(stationMin.y, stationMax.y);
        stationMax.x = std::max(stationMin.x, stationMax.x);
        stationMax.y = std::max(stationMin.y, stationMax.y);

        if (!World::validCoords(stationMax))
        {
            return true;
        }

        const auto [acceptedCargo, producedCargo] = calcAcceptedCargoAi(stationMin, stationMax);
        const bool shouldCreateStation = [&thought, aiStationIdx, producedCargo, acceptedCargo]() {
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
        if (thought.var_8B & (1U << 0))
        {
            company.var_85C3 |= (1U << 2);
        }
        if (thought.var_8B & (1U << 1))
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
                if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk3))
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
                if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk3))
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

        if (thought.var_8A == 0xFFU)
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
        const uint8_t signalType = thought.var_8A;
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

        if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk3))
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

    // 0x00481DE3
    static currency32_t getStationCostEstimate(const AiThought& thought)
    {
        currency32_t baseCost = 0;
        uint8_t costMultiplier = kThoughtTypeEstimatedCostMultiplier[enumValue(thought.type)];

        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::airBased))
        {
            auto* airportObj = ObjectManager::get<AirportObject>(thought.var_89);
            baseCost = Economy::getInflationAdjustedCost(airportObj->buildCostFactor, airportObj->costIndex, 6);
        }
        else if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::waterBased))
        {
            auto* dockObj = ObjectManager::get<DockObject>(thought.var_89);
            baseCost = Economy::getInflationAdjustedCost(dockObj->buildCostFactor, dockObj->costIndex, 7);
        }
        else
        {
            if (!thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk5 | ThoughtTypeFlags::unk6))
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
                    for (auto i = 0U; i < 32U; ++i)
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
                    auto* stationObj = ObjectManager::get<RoadStationObject>(thought.var_89);
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
                    for (auto i = 0U; i < 32U; ++i)
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
                    auto* stationObj = ObjectManager::get<TrainStationObject>(thought.var_89);
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

    // 0x00486668
    static void sub_486668(Company& company, AiThought& thought)
    {
        thought.var_76 += getStationCostEstimate(thought);
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

        if (company.var_257E * 2 < thought.var_76)
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

            auto townId = static_cast<TownId>(thought.var_01);
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk1))
            {
                townId = IndustryManager::get(static_cast<IndustryId>(thought.var_01))->town;
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

    // 0x0047BA2C
    static uint32_t sub_47BA2C(World::Pos3 pos, uint8_t rotation, uint8_t roadObjectId, uint32_t unk, uint8_t flags)
    {
        registers regs;
        regs.ax = pos.x;
        regs.cx = pos.y;
        regs.di = pos.z;
        regs.bl = flags;
        regs.bh = rotation;
        regs.bp = roadObjectId;
        regs.edx = unk;
        call(0x0047BA2C, regs);
        return regs.ebx;
    }

    // 0x004866C8
    static uint8_t sub_4866C8(const Company& company, AiThought& thought)
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
            placeArgs.type = thought.var_89;

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
            placeArgs.type = thought.var_89;

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
                if (sub_47BA2C(pos, aiStation.rotation, thought.trackObjId & ~(1U << 7), 0, aiStation.rotation) == GameCommands::FAILURE)
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
        const auto res = sub_4866C8(company, thought);
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
    static bool sub_487BA3(AiThought& thought)
    {
        // sell a vehicle ??
        registers regs;
        regs.edi = X86Pointer(&thought);
        return call(0x00487BA3, regs) & X86_FLAG_CARRY;
    }

    // 0x004311B5
    static void sub_4311B5(Company& company, AiThought& thought)
    {
        if (sub_487BA3(thought))
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
        if (sub_487BA3(thought))
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

    // 0x00483778
    static void clearThought(AiThought& thought)
    {
        thought.type = AiThoughtType::null;
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

    // 0x00487C83
    static void sub_487C83(AiThought& thought)
    {
        // Gets refund costs for vehicles and costs for track mods
        registers regs;
        regs.edi = X86Pointer(&thought);
        call(0x00487C83, regs);
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
            args.modSection = 2;
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
            args.modSection = 2;
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

    // 0x00487E74
    static bool sub_487E74(Company& company, AiThought& thought)
    {
        if ((company.challengeFlags & CompanyFlags::bankrupt) != CompanyFlags::none)
        {
            return true;
        }

        if (!(thought.var_8B & (1U << 3)))
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
    static bool sub_487EA0(AiThought& thought)
    {
        // some sort of sell of vehicle
        registers regs;
        regs.edi = X86Pointer(&thought);
        return call(0x00487EA0, regs) & X86_FLAG_CARRY;
    }

    // 0x00431244
    static void sub_431244(Company& company, AiThought& thought)
    {
        if (sub_487EA0(thought))
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

    // 0x00431295
    static void sub_431295(Company& company)
    {
        company.var_4A6 = 1;
    }

    // 0x0043129D
    static void sub_43129D(Company& company)
    {
        company.var_4A6 = 2;
        company.var_259E = 0;
    }

    // 0x00487784
    static bool tryPlaceVehicles(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        return call(0x00487784, regs) & X86_FLAG_CARRY;
    }

    // 0x004312AF
    static void sub_4312AF(Company& company)
    {
        if (tryPlaceVehicles(company))
        {
            company.var_4A6 = 3;
        }
    }

    // 0x004312BF
    static void sub_4312BF(Company& company)
    {
        company.var_4A6 = 0;
    }

    static void callThinkFunc2(Company& company)
    {
        switch (company.var_4A6)
        {
            case 0:
                sub_431295(company);
                break;
            case 1:
                sub_43129D(company);
                break;
            case 2:
                sub_4312AF(company);
                break;
            case 3:
                sub_4312BF(company);
                break;
            default:
                assert(false);
                return;
        }
    }

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

        callThinkFunc2(*company);

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
        if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk1))
        {
            auto* industry = IndustryManager::get(static_cast<IndustryId>(thought.var_01));
            pos = { industry->x, industry->y };
        }
        else
        {
            auto* town = TownManager::get(static_cast<TownId>(thought.var_01));
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
            if (thoughtTypeHasFlags(thought.type, ThoughtTypeFlags::unk1))
            {
                auto* industry = IndustryManager::get(static_cast<IndustryId>(thought.var_01));
                pos = World::Pos2{ industry->x, industry->y };
            }
            else
            {
                // Interestingly var_01 isn't a uint16_t
                auto* town = TownManager::get(static_cast<TownId>(thought.var_01));
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

    void removeEntityFromThought(AiThought& thought, EntityId id)
    {
        auto iter = std::find(std::begin(thought.vehicles), std::end(thought.vehicles), id);
        if (iter == std::end(thought.vehicles))
        {
            return;
        }
        // Original would copy the value from vehicles + 2 which
        // would mean it would copy currency var_76 if numVehicles was 7
        // I don't think that is possible but lets just add an assert.
        assert(thought.numVehicles < 7);

        *iter = thought.vehicles[std::size(thought.vehicles) - 1];
        std::rotate(iter, iter + 1, std::end(thought.vehicles));
        thought.numVehicles--;
    }
}
