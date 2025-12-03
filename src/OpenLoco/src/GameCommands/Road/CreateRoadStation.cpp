#include "CreateRoadStation.h"
#include "Economy/Economy.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileClearance.h"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/RoadStationObject.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"

namespace OpenLoco::GameCommands
{
    // 0x004900B8
    static StationManager::NearbyStation findNearbyStationOnRoad(World::Pos3 pos, uint16_t tad, uint8_t roadObjectId)
    {
        {
            auto [nextPos, nextRotation] = World::Track::getRoadConnectionEnd(pos, tad);
            const auto rc = World::Track::getRoadConnections(nextPos, nextRotation, getUpdatingCompanyId(), roadObjectId, 0, 0);
            if (rc.stationId != StationId::null)
            {
                if (StationManager::get(rc.stationId)->owner == getUpdatingCompanyId())
                {
                    return StationManager::NearbyStation{ rc.stationId, true };
                }
            }
        }
        {
            auto tailTaD = tad;
            const auto& roadSize = World::TrackData::getUnkRoad(tailTaD);
            auto tailPos = pos + roadSize.pos;
            if (roadSize.rotationEnd < 12)
            {
                tailPos -= World::Pos3{ World::kRotationOffset[roadSize.rotationEnd], 0 };
            }
            tailTaD ^= (1U << 2); // Reverse
            auto [nextTailPos, nextTailRotation] = World::Track::getRoadConnectionEnd(tailPos, tailTaD);
            const auto tailRc = World::Track::getRoadConnections(nextTailPos, nextTailRotation, getUpdatingCompanyId(), roadObjectId, 0, 0);
            if (tailRc.stationId != StationId::null)
            {
                if (StationManager::get(tailRc.stationId)->owner == getUpdatingCompanyId())
                {
                    return StationManager::NearbyStation{ tailRc.stationId, true };
                }
            }
        }

        return StationManager::findNearbyStation(pos, getUpdatingCompanyId());
    }

    enum class NearbyStationValidation
    {
        okay,
        requiresNewStation,
        failure,
    };

    // 0x0048CA3E & 0x0048C9C8
    static std::pair<NearbyStationValidation, StationId> validateNearbyStation(const World::Pos3 pos, const uint16_t tad, const uint8_t trackObjectId)
    {
        auto nearbyStation = findNearbyStationOnRoad(pos, tad, trackObjectId);
        if (nearbyStation.id == StationId::null)
        {
            return std::make_pair(NearbyStationValidation::requiresNewStation, StationId::null);
        }

        // _lastPlacedTrackStationId = nearbyStation.id; set in callers
        auto* station = StationManager::get(nearbyStation.id);
        if (station->stationTileSize >= std::size(station->stationTiles))
        {
            if (nearbyStation.isPhysicallyAttached)
            {
                setErrorText(StringIds::station_too_large);
                return std::make_pair(NearbyStationValidation::failure, StationId::null);
            }
            return std::make_pair(NearbyStationValidation::requiresNewStation, StationId::null);
        }
        else
        {
            if (StationManager::exceedsStationSize(*station, pos))
            {
                if (nearbyStation.isPhysicallyAttached)
                {
                    setErrorText(StringIds::station_too_large);
                    return std::make_pair(NearbyStationValidation::failure, StationId::null);
                }
                return std::make_pair(NearbyStationValidation::requiresNewStation, StationId::null);
            }
        }
        return std::make_pair(NearbyStationValidation::okay, nearbyStation.id);
    }

    static World::RoadElement* getElRoad(World::Pos3 pos, uint8_t rotation, uint8_t roadObjectId, uint8_t roadId, uint8_t index)
    {
        auto tile = World::TileManager::get(pos);
        for (auto& el : tile)
        {
            auto* elRoad = el.as<World::RoadElement>();
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
            if (elRoad->roadId() != roadId)
            {
                continue;
            }
            if (elRoad->roadObjectId() != roadObjectId)
            {
                continue;
            }
            if (elRoad->sequenceIndex() != index)
            {
                continue;
            }
            return elRoad;
        }
        return nullptr;
    }

    static bool elRoadIsJunctionAi(World::Pos3 pos, uint8_t rotation)
    {
        auto tile = World::TileManager::get(pos);
        for (auto& el : tile)
        {
            auto* elRoad = el.as<World::RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }
            if (elRoad->baseHeight() != pos.z)
            {
                continue;
            }
            if (elRoad->isGhost())
            {
                continue;
            }
            if (elRoad->roadId() != 0)
            {
                return true;
            }
            if ((elRoad->rotation() - rotation) & 0b01)
            {
                return true;
            }
        }
        return false;
    }

    // 0x0048BAF5
    static World::TileClearance::ClearFuncResult clearFunc0(World::TileElement& el, World::RoadElement* elReferenceRoad0, World::RoadElement* elReferenceRoad1)
    {
        auto* elStation = el.as<World::StationElement>();
        auto* elRoad = el.as<World::RoadElement>();
        if (elStation != nullptr)
        {
            if (elStation->stationType() == StationType::roadStation)
            {
                return World::TileClearance::ClearFuncResult::noCollision;
            }
        }
        else if (elRoad != nullptr && elReferenceRoad0 != nullptr && elReferenceRoad1 != nullptr && (elRoad > elReferenceRoad0 && elRoad <= elReferenceRoad1))
        {
            return World::TileClearance::ClearFuncResult::noCollision;
        }
        return World::TileClearance::ClearFuncResult::collision;
    };

    // TODO THIS IS A DUPLICATE MOVE TO SOMEWHERE ELSE
    // 0x0048BAE5
    static World::TileClearance::ClearFuncResult clearFuncCollideWithSurface(World::TileElement& el)
    {
        auto* elSurface = el.as<World::SurfaceElement>();
        if (elSurface != nullptr)
        {
            return World::TileClearance::ClearFuncResult::collision;
        }
        return World::TileClearance::ClearFuncResult::noCollision;
    };

    // 0x0048C708
    static currency32_t createRoadStation(const RoadStationPlacementArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3(16, 16, 0));

        auto& returnState = getLegacyReturnState();
        returnState.lastPlacedTrackRoadStationId = StationId::null;
        returnState.lastConstructedAdjoiningStationPos = World::Pos2(-1, -1);
        returnState.lastConstructedAdjoiningStation = StationId::null;

        auto* stationObj = ObjectManager::get<RoadStationObject>(args.type);

        const auto trackIdCompatFlags = World::TrackData::getRoadMiscData(args.roadId).compatibleFlags;
        const auto compatibleTrack = stationObj->roadPieces & trackIdCompatFlags;
        if (compatibleTrack != trackIdCompatFlags)
        {
            setErrorText(StringIds::track_road_unsuitable_for_station);
            return FAILURE;
        }

        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            return FAILURE;
        }

        if (!(flags & Flags::apply))
        {
            if (stationObj->hasFlags(RoadStationFlags::roadEnd))
            {
                const uint16_t tad = (0 << 3) | args.rotation;
                const auto roadEnd = World::Track::getRoadConnectionEnd(args.pos, tad);
                const auto rc = World::Track::getRoadConnections(roadEnd.nextPos, roadEnd.nextRotation, getUpdatingCompanyId(), args.roadObjectId, 0, 0);

                if (!rc.connections.empty())
                {
                    // Check other side of road
                    const uint16_t tad2 = (1 << 2) | args.rotation;
                    const auto roadEnd2 = World::Track::getRoadConnectionEnd(args.pos, tad2);
                    const auto rc2 = World::Track::getRoadConnections(roadEnd2.nextPos, roadEnd2.nextRotation, getUpdatingCompanyId(), args.roadObjectId, 0, 0);

                    if (!rc2.connections.empty())
                    {
                        setErrorText(StringIds::station_type_can_only_be_built_at_road_ends);
                        return FAILURE;
                    }
                }
            }
        }

        auto* initialElRoad = getElRoad(args.pos, args.rotation, args.roadObjectId, args.roadId, args.index);

        auto index = args.index;

        bool unk112C7F3 = false;
        if (initialElRoad == nullptr)
        {
            if (flags & Flags::apply)
            {
                return FAILURE;
            }
            if (!(flags & Flags::aiAllocated))
            {
                return FAILURE;
            }
            // Why???
            index = 0;
            unk112C7F3 = true;
        }
        else
        {
            if ((flags & Flags::aiAllocated) && initialElRoad->hasStationElement())
            {
                setErrorText(StringIds::empty);
                return FAILURE;
            }

            if (initialElRoad->hasStationElement())
            {
                auto* elStation = getStationElement(args.pos);
                // Will happen if its an aiAllocated station
                if (elStation == nullptr)
                {
                    setErrorText(StringIds::empty);
                    return FAILURE;
                }
                // Do not allow replacing station elements owned by other companies
                if (!sub_431E6A(elStation->owner(), reinterpret_cast<const World::TileElement*>(elStation)))
                {
                    return FAILURE;
                }
                // Otherwise, allow replacement
            }
        }
        auto& roadPieces = World::TrackData::getRoadPiece(args.roadId);
        auto& argPiece = roadPieces[index];
        const auto roadStart = args.pos - World::Pos3(Math::Vector::rotate(World::Pos2(argPiece.x, argPiece.y), args.rotation), argPiece.z);

        if ((flags & Flags::ghost) && (flags & Flags::apply))
        {
            returnState.lastConstructedAdjoiningStationPos = roadStart;
            uint16_t tad = (args.roadId << 3) | args.rotation;
            auto nearbyStation = findNearbyStationOnRoad(roadStart, tad, args.roadObjectId);
            returnState.lastConstructedAdjoiningStation = nearbyStation.id;
        }

        bool isNewStationTile = false; // 0x0112C7A9

        if (!(flags & Flags::ghost))
        {
            if (flags & Flags::apply)
            {
                auto [result, nearbyStationId] = validateNearbyStation(roadStart, (args.roadId << 3) | args.rotation, args.roadObjectId);
                switch (result)
                {
                    case NearbyStationValidation::failure:
                        // Odd???
                        break;
                    case NearbyStationValidation::requiresNewStation:
                    {
                        const auto newStationId = StationManager::allocateNewStation(roadStart, getUpdatingCompanyId(), 0);
                        if (newStationId != StationId::null)
                        {
                            returnState.lastPlacedTrackRoadStationId = newStationId;
                            auto* station = StationManager::get(newStationId);
                            station->updateLabel();
                        }
                    }
                    break;
                    case NearbyStationValidation::okay:
                        returnState.lastPlacedTrackRoadStationId = nearbyStationId;
                        break;
                }
                isNewStationTile = true;
            }
            else
            {
                // Same as the other branch but deallocate after allocating and return failure on failure
                auto [result, nearbyStationId] = validateNearbyStation(roadStart, (args.roadId << 3) | args.rotation, args.roadObjectId);
                switch (result)
                {
                    case NearbyStationValidation::failure:
                        return FAILURE;
                    case NearbyStationValidation::requiresNewStation:
                    {
                        const auto newStationId = StationManager::allocateNewStation(roadStart, getUpdatingCompanyId(), 0);
                        if (newStationId == StationId::null)
                        {
                            return FAILURE;
                        }
                        StationManager::deallocateStation(newStationId);
                        // _lastPlacedTrackStationId not set but that's fine since this is the no apply side
                    }
                    break;
                    case NearbyStationValidation::okay:
                        returnState.lastPlacedTrackRoadStationId = nearbyStationId;
                        break;
                }
            }
        }

        currency32_t totalCost = 0;

        for (auto& piece : roadPieces)
        {
            const auto roadLoc = roadStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };

            if (unk112C7F3)
            {
                if (elRoadIsJunctionAi(roadLoc, args.rotation))
                {
                    setErrorText(StringIds::station_cannot_be_built_on_a_junction);
                    return FAILURE;
                }
            }
            else
            {

                auto* elRoad = getElRoad(roadLoc, args.rotation, args.roadObjectId, args.roadId, piece.index);
                if (elRoad == nullptr)
                {
                    return FAILURE;
                }
                if (elRoad->hasSignalElement())
                {
                    setErrorText(StringIds::signal_in_the_way);
                    return FAILURE;
                }
                if (elRoad->hasLevelCrossing())
                {
                    setErrorText(StringIds::level_crossing_in_the_way);
                    return FAILURE;
                }
                if (elRoad->roadId() == 0)
                {
                    if (elRoadIsJunctionAi(roadLoc, args.rotation))
                    {
                        setErrorText(StringIds::station_cannot_be_built_on_a_junction);
                        return FAILURE;
                    }
                }
                else
                { // Connect flags validation
                    const auto connectFlags = piece.connectFlags[elRoad->rotation()];
                    auto tile = World::TileManager::get(roadLoc);
                    for (auto& el : tile)
                    {
                        auto* elConnectRoad = el.as<World::RoadElement>();
                        if (elConnectRoad == nullptr)
                        {
                            continue;
                        }
                        if (elConnectRoad == elRoad)
                        {
                            continue;
                        }
                        if (elConnectRoad->baseHeight() != roadLoc.z)
                        {
                            continue;
                        }
                        if (elConnectRoad->isGhost())
                        {
                            continue;
                        }
                        auto& connectPiece = World::TrackData::getRoadPiece(elConnectRoad->roadId())[elConnectRoad->sequenceIndex()];
                        if (connectFlags & connectPiece.connectFlags[elConnectRoad->rotation()])
                        {
                            setErrorText(StringIds::station_cannot_be_built_on_a_junction);
                            return FAILURE;
                        }
                    }
                }
            }

            bool unk112C7F4 = false;
            // 0x0112C738 & 0x0112C73C
            std::array<World::RoadElement*, 2> elRoads = {
                nullptr,
                nullptr,
            };

            {
                auto tile = World::TileManager::get(roadLoc);
                for (auto& el : tile)
                {
                    auto* elRoad = el.as<World::RoadElement>();
                    if (elRoad == nullptr)
                    {
                        continue;
                    }
                    if (elRoad->baseHeight() != roadLoc.z)
                    {
                        continue;
                    }
                    auto* roadObj2 = ObjectManager::get<RoadObject>(elRoad->roadObjectId());
                    if (stationObj->hasFlags(RoadStationFlags::roadEnd) && !roadObj2->hasFlags(RoadObjectFlags::isRoad))
                    {
                        auto formatter = FormatArguments::common();
                        formatter.push(roadObj2->name);
                        setErrorText(StringIds::station_not_compatible_with_string_id);
                        return FAILURE;
                    }
                    if (roadObj2->hasFlags(RoadObjectFlags::unk_03)
                        || roadObj2->hasFlags(RoadObjectFlags::unk_07)
                        || sub_431E6A(elRoad->owner(), &el))
                    {
                        unk112C7F4 = true;
                    }
                    bool isCompatible = false;
                    for (auto i = 0U; i < stationObj->numCompatible; ++i)
                    {
                        if (elRoad->roadObjectId() == stationObj->mods[i])
                        {
                            elRoads[1] = elRoad;
                            if (elRoads[0] == nullptr)
                            {
                                elRoads[0] = elRoad;
                            }
                            isCompatible = true;
                            break;
                        }
                    }
                    if (isCompatible)
                    {
                        continue;
                    }
                    for (auto i = 0U; i < roadObj2->numStations; ++i)
                    {
                        if (roadObj2->stations[i] == args.type)
                        {
                            elRoads[1] = elRoad;
                            if (elRoads[0] == nullptr)
                            {
                                elRoads[0] = elRoad;
                            }
                            isCompatible = true;
                            break;
                        }
                    }
                    if (isCompatible)
                    {
                        continue;
                    }
                    // if neither above for loop
                    auto formatter = FormatArguments::common();
                    formatter.push(roadObj2->name);
                    setErrorText(StringIds::wrong_type_of_station_for_string_id);
                    return FAILURE;
                }
                if (!unk112C7F4 && !unk112C7F3)
                {
                    return FAILURE;
                }
            }
            // Calculate station costs
            if (piece.index == 0)
            {
                bool calculateCost = true;

                // Replace station if it already exists
                if (!unk112C7F3 && elRoads[1]->hasStationElement())
                {
                    auto* elStation = elRoads[1]->next()->as<World::StationElement>();
                    if (elStation == nullptr)
                    {
                        return FAILURE;
                    }
                    if (elStation->objectId() == args.type)
                    {
                        calculateCost = false;
                    }
                    else
                    {
                        auto* oldStationObj = ObjectManager::get<RoadStationObject>(elStation->objectId());
                        auto removeCostBase = Economy::getInflationAdjustedCost(oldStationObj->sellCostFactor, oldStationObj->costIndex, 8);
                        const auto cost = (removeCostBase * World::TrackData::getRoadMiscData(args.roadId).costFactor) / 256;
                        totalCost += cost;
                    }
                }

                if (calculateCost)
                {
                    auto placementCostBase = Economy::getInflationAdjustedCost(stationObj->buildCostFactor, stationObj->costIndex, 8);
                    const auto cost = (placementCostBase * World::TrackData::getRoadMiscData(args.roadId).costFactor) / 256;
                    totalCost += cost;
                }
            }

            // Perform clearance
            const auto baseZ = (roadStart.z / World::kSmallZStep); // Vanilla bug? should really be roadLoc.z
            const auto clearZ = baseZ + 8 + stationObj->height / World::kSmallZStep;
            World::QuarterTile qt(0b1111, 0);

            auto clearFunc = [&elRoads](World::TileElement& el) {
                return clearFunc0(el, elRoads[0], elRoads[1]);
            };
            // Perform clearance at just the road clear height to road clear height + station height
            if (!World::TileClearance::applyClearAtStandardHeight(roadLoc, baseZ + 8, clearZ, qt, clearFunc))
            {
                return FAILURE;
            }

            if (!(flags & Flags::aiAllocated))
            {
                // Perform clearance at full station height (only checks for surface collisions)
                if (!World::TileClearance::applyClearAtStandardHeight(roadLoc, baseZ, clearZ, qt, clearFuncCollideWithSurface))
                {
                    return FAILURE;
                }
            }

            // elTrack is still valid as applyClearAtStandardHeight set to not remove anything
            // this will need changed if ever a different clear function is used
            if (!unk112C7F3 && elRoads[1]->hasStationElement() && (flags & Flags::ghost))
            {
                // ?????
                setErrorText(StringIds::empty);
                return FAILURE;
            }

            if (!(flags & Flags::apply))
            {
                continue;
            }

            if (CompanyManager::isPlayerCompany(getUpdatingCompanyId()))
            {
                companyEmotionEvent(getUpdatingCompanyId(), Emotion::thinking);
            }

            World::StationElement* newStationElement = nullptr;
            // Actually place the new station
            if (elRoads[1]->hasStationElement())
            {
                auto* elStation = elRoads[1]->next()->as<World::StationElement>();
                if (elStation == nullptr)
                {
                    return FAILURE;
                }
                auto* oldStationObj = ObjectManager::get<RoadStationObject>(elStation->objectId());
                for (auto elRoad = elRoads[0]; elRoad <= elRoads[1]; ++elRoad)
                {
                    elRoad->setClearZ(elRoad->clearZ() - oldStationObj->height / World::kSmallZStep);
                }
                isNewStationTile = false; // We are just replacing so not a new tile
                Ui::ViewportManager::invalidate(roadLoc, elStation->baseHeight(), elStation->clearHeight());
                newStationElement = elStation;
            }
            else
            {
                // elTrack pointer will be invalid after this call
                newStationElement = World::TileManager::insertElementAfterNoReorg<World::StationElement>(
                    reinterpret_cast<World::TileElement*>(elRoads[1]),
                    roadLoc,
                    elRoads[1]->baseZ(),
                    elRoads[1]->occupiedQuarter());
                if (newStationElement == nullptr)
                {
                    return FAILURE;
                }
                elRoads[0] = nullptr;
                elRoads[1] = nullptr;
                auto tile = World::TileManager::get(roadLoc);
                for (auto& el : tile)
                {
                    auto* elRoad = el.as<World::RoadElement>();
                    if (elRoad == nullptr)
                    {
                        continue;
                    }
                    if (elRoad->baseHeight() != roadLoc.z)
                    {
                        continue;
                    }
                    elRoads[1] = elRoad;
                    if (elRoads[0] == nullptr)
                    {
                        elRoads[0] = elRoad;
                    }
                }
                newStationElement->setRotation(elRoads[1]->rotation());
                newStationElement->setGhost(flags & Flags::ghost);
                newStationElement->setAiAllocated(flags & Flags::aiAllocated);
                newStationElement->setSequenceIndex(0);
                newStationElement->setUnk4SLR4(0);
                newStationElement->setStationType(StationType::roadStation);
                newStationElement->setBuildingType(0);
                if (!(flags & Flags::ghost))
                {
                    newStationElement->setStationId(returnState.lastPlacedTrackRoadStationId);
                }
                else
                {
                    newStationElement->setStationId(static_cast<StationId>(0));
                }
                for (auto elRoad = elRoads[0]; elRoad <= elRoads[1]; ++elRoad)
                {
                    elRoad->setHasStationElement(true);
                }
            }
            newStationElement->setObjectId(args.type);
            for (auto elRoad = elRoads[0]; elRoad <= elRoads[1]; ++elRoad)
            {
                elRoad->setClearZ(elRoad->clearZ() + stationObj->height / World::kSmallZStep);
            }
            newStationElement->setClearZ(elRoads[0]->clearZ());
            newStationElement->setOwner(getUpdatingCompanyId());
            Ui::ViewportManager::invalidate(roadLoc, newStationElement->baseHeight(), newStationElement->clearHeight());
        }

        if (!(flags & Flags::ghost) && (flags & Flags::apply))
        {
            if (isNewStationTile)
            {
                addTileToStation(returnState.lastPlacedTrackRoadStationId, roadStart, args.rotation);
            }
            auto* station = StationManager::get(returnState.lastPlacedTrackRoadStationId);
            station->invalidate();
            recalculateStationModes(returnState.lastPlacedTrackRoadStationId);
            recalculateStationCenter(returnState.lastPlacedTrackRoadStationId);
            station->updateLabel();
            station->invalidate();
        }
        return totalCost;
    }

    void createRoadStation(registers& regs)
    {
        regs.ebx = createRoadStation(RoadStationPlacementArgs(regs), regs.bl);
    }
}
