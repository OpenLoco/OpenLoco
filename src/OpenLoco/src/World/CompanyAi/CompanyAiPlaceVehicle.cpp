#include "CompanyAiPlaceVehicle.h"
#include "Date.h"
#include "GameCommands/Vehicles/VehicleChangeRunningMode.h"
#include "GameCommands/Vehicles/VehiclePlace.h"
#include "GameCommands/Vehicles/VehiclePlaceAir.h"
#include "GameCommands/Vehicles/VehiclePlaceWater.h"
#include "GameCommands/Vehicles/VehicleSell.h"
#include "GameState.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Random.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include "World/Company.h"
#include "World/StationManager.h"

namespace OpenLoco::CompanyAi
{
    using namespace World;

    // 0x00431295
    static void beginPlacement(Company& company)
    {
        company.var_4A6 = AiPlaceVehicleState::resetList;
    }

    // 0x0043129D
    static void resetPlacementList(Company& company)
    {
        company.var_4A6 = AiPlaceVehicleState::place;
        company.aiPlaceVehicleIndex = 0;
    }

    // 0x00497AC6
    static void tryPlaceVehicleFailure(Company& company, Vehicles::VehicleHead& head)
    {
        if (head.hasBreakdownFlags(Vehicles::BreakdownFlags::brokenDown))
        {
            return;
        }
        auto train = Vehicles::Vehicle(head);
        const auto firstCarAge = getCurrentDay() - train.cars.firstCar.front->creationDay;
        if (firstCarAge < 42)
        {
            return;
        }
        if (head.aiThoughtId == 0xFFU)
        {
            return;
        }
        auto& thought = company.aiThoughts[head.aiThoughtId];
        removeEntityFromThought(thought, head.id);
        thought.var_43--;
        thought.purchaseFlags |= AiPurchaseFlags::unk4;

        GameCommands::VehicleSellArgs args{};
        args.car = head.id;

        const auto oldUpdatingCompanyId = GameCommands::getUpdatingCompanyId();
        GameCommands::setUpdatingCompanyId(CompanyId::neutral); // Why?

        GameCommands::doCommand(args, GameCommands::Flags::apply);

        GameCommands::setUpdatingCompanyId(oldUpdatingCompanyId);
    }

    // 0x00487B77
    static void tryPlaceVehicleSuccess(Vehicles::VehicleHead& head)
    {
        head.breakdownFlags |= Vehicles::BreakdownFlags::brokenDown; // Why?

        GameCommands::VehicleChangeRunningModeArgs args{};
        args.head = head.id;
        args.mode = GameCommands::VehicleChangeRunningModeArgs::Mode::startVehicle;

        const auto oldUpdatingCompanyId = GameCommands::getUpdatingCompanyId();
        GameCommands::setUpdatingCompanyId(CompanyId::neutral); // Why?

        GameCommands::doCommand(args, GameCommands::Flags::apply);

        GameCommands::setUpdatingCompanyId(oldUpdatingCompanyId);
    }

    static std::optional<GameCommands::VehiclePlacementArgs> generateBackupRoadPlacement(const World::Pos2 pos, const uint8_t roadObjectId, const CompanyId companyId)
    {
        const auto randVal = gPrng1().randNext();
        const auto randX = ((randVal & 0xFU) - 7) * 32;
        const auto randY = (((randVal >> 4) & 0xFU) - 7) * 32;
        const auto randPos = World::Pos2(pos.x + randX, pos.y + randY);
        if (!TileManager::validCoords(randPos))
        {
            return std::nullopt;
        }

        const auto tile = World::TileManager::get(randPos);
        for (const auto& el : tile)
        {
            auto* elRoad = el.as<RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }
            if (elRoad->sequenceIndex() != 0)
            {
                continue;
            }
            if (roadObjectId == 0xFFU)
            {
                if (!(getGameState().roadObjectIdIsNotTram & (1U << elRoad->roadObjectId())))
                {
                    continue;
                }
            }
            else
            {
                if (elRoad->roadObjectId() != roadObjectId)
                {
                    continue;
                }
                if (elRoad->owner() != companyId)
                {
                    continue;
                }
            }
            GameCommands::VehiclePlacementArgs args{};
            args.convertGhost = false;
            args.pos = World::Pos3(randPos, elRoad->baseHeight() - World::TrackData::getRoadPiece(elRoad->roadId())[0].z);
            args.trackAndDirection = (elRoad->roadId() << 3) | elRoad->rotation();
            args.trackProgress = 0;
            return args;
        }
        return std::nullopt;
    }

    // 0x0047C371
    static uint8_t validateRoadPlacement(const World::Pos3 pos, const uint8_t roadObjectId, const uint16_t tad)
    {
        const auto rotation = tad & 0x3;
        const auto roadId = tad >> 3;
        World::Pos3 initialPos = pos;
        if (tad & (1U << 2))
        {
            const auto& trackSize = World::TrackData::getUnkRoad(tad);
            initialPos += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                initialPos.x -= kRotationOffset[trackSize.rotationEnd].x;
                initialPos.y -= kRotationOffset[trackSize.rotationEnd].y;
            }
        }

        auto& roadPieces = World::TrackData::getRoadPiece(roadId);
        for (auto& piece : roadPieces)
        {
            const auto rotatedPiece = World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, rotation), piece.z };
            const auto piecePos = initialPos + rotatedPiece;
            auto* elRoad = [piecePos, rotation, &piece, roadId, roadObjectId]() -> const World::RoadElement* {
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
                    if (elRoad->rotation() != rotation)
                    {
                        continue;
                    }
                    if (elRoad->sequenceIndex() != piece.index)
                    {
                        continue;
                    }
                    if (elRoad->roadId() != roadId)
                    {
                        continue;
                    }
                    if (elRoad->roadObjectId() != roadObjectId)
                    {
                        if (!(getGameState().roadObjectIdIsNotTram & (1U << elRoad->roadObjectId())))
                        {
                            continue;
                        }
                    }
                    return elRoad;
                }
                return nullptr;
            }();
            if (elRoad == nullptr)
            {
                return 2;
            }
        }
        return 0;
    }

    // 0x004A868A
    static std::optional<GameCommands::VehiclePlacementArgs> generateBackupTrackPlacement(const World::Pos2 pos, const uint8_t trackObjectId, const CompanyId companyId)
    {
        const auto randVal = gPrng1().randNext();
        const auto randX = ((randVal & 0xFU) - 7) * 32;
        const auto randY = (((randVal >> 4) & 0xFU) - 7) * 32;
        const auto randPos = World::Pos2(pos.x + randX, pos.y + randY);
        if (!TileManager::validCoords(randPos))
        {
            return std::nullopt;
        }

        const auto tile = World::TileManager::get(randPos);
        for (const auto& el : tile)
        {
            auto* elTrack = el.as<TrackElement>();
            if (elTrack == nullptr)
            {
                continue;
            }
            if (elTrack->sequenceIndex() != 0)
            {
                continue;
            }
            if (elTrack->trackObjectId() != trackObjectId)
            {
                continue;
            }
            if (elTrack->owner() != companyId)
            {
                continue;
            }

            GameCommands::VehiclePlacementArgs args{};
            args.convertGhost = false;
            args.pos = World::Pos3(randPos, elTrack->baseHeight() - World::TrackData::getTrackPiece(elTrack->trackId())[0].z);
            args.trackAndDirection = (elTrack->trackId() << 3) | elTrack->rotation();
            args.trackProgress = 0;
            return args;
        }
        return std::nullopt;
    }

    // 0x004A852B
    static uint8_t validateTrackPlacement(const World::Pos3 pos, const uint8_t trackObjectId, const uint16_t tad, const CompanyId companyId)
    {
        const auto rotation = tad & 0x3;
        const auto trackId = tad >> 3;
        World::Pos3 initialPos = pos;
        if (tad & (1U << 2))
        {
            const auto& trackSize = World::TrackData::getUnkTrack(tad);
            initialPos += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                initialPos.x -= kRotationOffset[trackSize.rotationEnd].x;
                initialPos.y -= kRotationOffset[trackSize.rotationEnd].y;
            }
        }

        auto& trackPieces = World::TrackData::getTrackPiece(trackId);
        for (auto& piece : trackPieces)
        {
            const auto rotatedPiece = World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, rotation), piece.z };
            const auto piecePos = initialPos + rotatedPiece;
            auto* elTrack = [piecePos, rotation, &piece, trackId, trackObjectId, companyId]() -> const World::TrackElement* {
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
                    if (elTrack->isAiAllocated())
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
                    if (elTrack->trackId() != trackId)
                    {
                        continue;
                    }
                    if (elTrack->trackObjectId() != trackObjectId)
                    {
                        continue;
                    }
                    if (elTrack->owner() != companyId)
                    {
                        continue;
                    }
                    return elTrack;
                }
                return nullptr;
            }();
            if (elTrack == nullptr)
            {
                return 2;
            }
        }
        return 0;
    }

    // 0x004878E3
    // args is expected to not have head set yet
    static void tryPlaceSurfaceVehicle(Company& company, Vehicles::VehicleHead& head, GameCommands::VehiclePlacementArgs args)
    {
        args.head = head.id;
        const auto oldUpdatingCompanyId = GameCommands::getUpdatingCompanyId();
        GameCommands::setUpdatingCompanyId(CompanyId::neutral); // Why?

        const auto res = GameCommands::doCommand(args, GameCommands::Flags::apply);

        GameCommands::setUpdatingCompanyId(oldUpdatingCompanyId);
        if (res == GameCommands::FAILURE)
        {
            tryPlaceVehicleFailure(company, head);
            return;
        }
        tryPlaceVehicleSuccess(head);
    }

    // 0x00487A27
    static void tryPlaceWaterVehicle(Company& company, Vehicles::VehicleHead& head, World::Pos3 pos)
    {
        auto* elStation = [&pos, &head]() -> const StationElement* {
            auto tile = World::TileManager::get(pos);
            for (const auto& el : tile)
            {
                auto* elStation = el.as<StationElement>();
                if (elStation == nullptr)
                {
                    continue;
                }
                if (elStation->baseHeight() != pos.z)
                {
                    continue;
                }
                if (elStation->isAiAllocated() || elStation->isGhost())
                {
                    continue;
                }
                if (elStation->stationType() != StationType::docks)
                {
                    continue;
                }
                if (elStation->owner() != head.owner)
                {
                    continue;
                }
                if (elStation->isFlag6())
                {
                    continue;
                }
                return elStation;
            }
            return nullptr;
        }();
        if (elStation == nullptr)
        {
            tryPlaceVehicleFailure(company, head);
            return;
        }

        GameCommands::VehicleWaterPlacementArgs args{};
        args.head = head.id;
        args.pos = pos;
        args.convertGhost = false;

        const auto oldUpdatingCompanyId = GameCommands::getUpdatingCompanyId();
        GameCommands::setUpdatingCompanyId(CompanyId::neutral); // Why?

        const auto res = GameCommands::doCommand(args, GameCommands::Flags::apply);

        GameCommands::setUpdatingCompanyId(oldUpdatingCompanyId);
        if (res == GameCommands::FAILURE)
        {
            tryPlaceVehicleFailure(company, head);
            return;
        }
        tryPlaceVehicleSuccess(head);
    }

    // 0x00487926
    static void tryPlaceAirVehicle(Company& company, Vehicles::VehicleHead& head, World::Pos3 pos)
    {
        auto* elStation = [&pos, &head]() -> const StationElement* {
            auto tile = World::TileManager::get(pos);
            for (const auto& el : tile)
            {
                auto* elStation = el.as<StationElement>();
                if (elStation == nullptr)
                {
                    continue;
                }
                if (elStation->baseHeight() != pos.z)
                {
                    continue;
                }
                if (elStation->isAiAllocated() || elStation->isGhost())
                {
                    continue;
                }
                if (elStation->stationType() != StationType::airport)
                {
                    continue;
                }
                if (elStation->owner() != head.owner)
                {
                    continue;
                }
                return elStation;
            }
            return nullptr;
        }();
        if (elStation == nullptr)
        {
            tryPlaceVehicleFailure(company, head);
            return;
        }

        auto* station = StationManager::get(elStation->stationId());
        auto* airportObj = ObjectManager::get<AirportObject>(elStation->objectId());
        const auto movementNodes = airportObj->getMovementNodes();
        const auto movementEdges = airportObj->getMovementEdges();

        for (auto nodeIndex = 0U; nodeIndex < airportObj->numMovementNodes; ++nodeIndex)
        {
            auto& node = movementNodes[nodeIndex];
            if (!node.hasFlags(AirportMovementNodeFlags::terminal))
            {
                continue;
            }
            const auto mustBeClearEdges = [nodeIndex, &airportObj, &movementEdges]() {
                for (auto edgeIndex = 0U; edgeIndex < airportObj->numMovementEdges; ++edgeIndex)
                {
                    auto& edge = movementEdges[edgeIndex];
                    if (edge.nextNode == nodeIndex)
                    {
                        return edge.mustBeClearEdges;
                    }
                }
                return 0U;
            }();
            if (station->airportMovementOccupiedEdges & mustBeClearEdges)
            {
                continue;
            }

            GameCommands::VehicleAirPlacementArgs args{};
            args.head = head.id;
            args.airportNode = nodeIndex;
            args.stationId = elStation->stationId();
            args.convertGhost = false;

            const auto oldUpdatingCompanyId = GameCommands::getUpdatingCompanyId();
            GameCommands::setUpdatingCompanyId(CompanyId::neutral); // Why?

            const auto res = GameCommands::doCommand(args, GameCommands::Flags::apply);

            GameCommands::setUpdatingCompanyId(oldUpdatingCompanyId);
            if (res == GameCommands::FAILURE)
            {
                tryPlaceVehicleFailure(company, head);
            }
            else
            {
                tryPlaceVehicleSuccess(head);
            }
            return;
        }
        tryPlaceVehicleFailure(company, head);
    }

    // 0x00487818
    static void tryPlaceRoadVehicle(Company& company, Vehicles::VehicleHead& head, World::Pos3 pos)
    {
        if (validateRoadPlacement(pos, head.trackType, head.aiPlacementTaD) & (1U << 1))
        {
            // 0x0048788E
            auto args = generateBackupRoadPlacement(pos, head.trackType, company.id());
            if (args.has_value())
            {
                tryPlaceSurfaceVehicle(company, head, args.value());
            }
            return;
        }

        if (!head.hasBreakdownFlags(Vehicles::BreakdownFlags::brokenDown))
        {
            auto train = Vehicles::Vehicle(head);
            const auto firstCarAge = getCurrentDay() - train.cars.firstCar.front->creationDay;
            if (firstCarAge >= 28 && head.aiThoughtId != 0xFFU)
            {
                // 0x0048788E
                auto args = generateBackupRoadPlacement(pos, head.trackType, company.id());
                if (args.has_value())
                {
                    tryPlaceSurfaceVehicle(company, head, args.value());
                }
                return;
            }
        }
        GameCommands::VehiclePlacementArgs args{};
        args.convertGhost = false;
        args.pos = pos;
        args.trackAndDirection = head.aiPlacementTaD;
        args.trackProgress = 0;

        if (!head.hasBreakdownFlags(Vehicles::BreakdownFlags::brokenDown))
        {
            if (head.trackType == 0xFFU)
            {
                head.aiPlacementTaD ^= (1U << 2);
            }
        }
        tryPlaceSurfaceVehicle(company, head, args);
    }

    // 0x00487807
    static void tryPlaceTrackVehicle(Company& company, Vehicles::VehicleHead& head, World::Pos3 pos)
    {
        GameCommands::VehiclePlacementArgs args{};
        args.convertGhost = false;
        args.pos = pos;
        args.trackAndDirection = head.aiPlacementTaD;
        args.trackProgress = 0;
        if (validateTrackPlacement(pos, head.trackType, head.aiPlacementTaD, company.id()) & (1U << 1))
        {
            auto res = generateBackupTrackPlacement(pos, head.trackType, company.id());
            if (!res.has_value())
            {
                return;
            }
            args = res.value();
        }

        tryPlaceSurfaceVehicle(company, head, args);
    }

    // 0x00487784
    static bool tryPlaceVehicles(Company& company)
    {
        company.aiPlaceVehicleIndex++;
        auto* head = [&company]() -> Vehicles::VehicleHead* {
            auto i = company.aiPlaceVehicleIndex;
            for (auto* v : VehicleManager::VehicleList())
            {
                if (v->owner != company.id())
                {
                    continue;
                }
                i--;
                if (i == 0)
                {
                    return v;
                }
            }
            return nullptr;
        }();
        if (head == nullptr)
        {
            return true;
        }
        if (head->tileX != -1)
        {
            return false;
        }
        if (!head->hasBreakdownFlags(Vehicles::BreakdownFlags::breakdownPending))
        {
            return false;
        }
        const auto pos = World::Pos3(head->aiPlacementPos, head->aiPlacementBaseZ * World::kSmallZStep);
        if (head->mode == TransportMode::air)
        {
            tryPlaceAirVehicle(company, *head, pos);
        }
        else if (head->mode == TransportMode::water)
        {
            tryPlaceWaterVehicle(company, *head, pos);
        }
        else if (head->mode == TransportMode::road)
        {
            tryPlaceRoadVehicle(company, *head, pos);
        }
        else // head->mode == TransportMode::rail
        {
            tryPlaceTrackVehicle(company, *head, pos);
        }
        return false;
    }

    // 0x004312AF
    // Places all vehicles that are owned by the company
    // one vehicle a tick
    // transitions to next state after all vehicles processed
    static void placeVehicles(Company& company)
    {
        if (tryPlaceVehicles(company))
        {
            company.var_4A6 = AiPlaceVehicleState::restart;
        }
    }

    // 0x004312BF
    static void restartPlacement(Company& company)
    {
        company.var_4A6 = AiPlaceVehicleState::begin;
    }

    void processVehiclePlaceStateMachine(Company& company)
    {
        // TODO: In the future we could cut
        // this FSM down into just two states
        // a reset and a placement state
        using enum AiPlaceVehicleState;
        switch (company.var_4A6)
        {
            case begin:
                beginPlacement(company);
                break;
            case resetList:
                resetPlacementList(company);
                break;
            case place:
                placeVehicles(company);
                break;
            case restart:
                restartPlacement(company);
                break;
            default:
                assert(false);
                return;
        }
    }
}
