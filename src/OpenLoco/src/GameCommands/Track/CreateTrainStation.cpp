#include "CreateTrainStation.h"
#include "Economy/Economy.h"
#include "Localisation/StringIds.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileClearance.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrackObject.h"
#include "Objects/TrainStationObject.h"
#include "ViewportManager.h"
#include "World/StationManager.h"

namespace OpenLoco::GameCommands
{
    // 0x0048FF36
    static StationManager::NearbyStation findNearbyStationOnTrack(World::Pos3 pos, uint16_t tad, uint8_t trackObjectId)
    {
        {
            auto [nextPos, nextRotation] = World::Track::getTrackConnectionEnd(pos, tad);
            const auto tc = World::Track::getTrackConnections(nextPos, nextRotation, getUpdatingCompanyId(), trackObjectId, 0, 0);
            if (tc.stationId != StationId::null)
            {
                return StationManager::NearbyStation{ tc.stationId, true };
            }
        }
        {
            auto tailTaD = tad;
            const auto& trackSize = World::TrackData::getUnkTrack(tailTaD);
            auto tailPos = pos + trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                tailPos -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }
            tailTaD ^= (1U << 2); // Reverse
            auto [nextTailPos, nextTailRotation] = World::Track::getTrackConnectionEnd(tailPos, tailTaD);
            const auto tailTc = World::Track::getTrackConnections(nextTailPos, nextTailRotation, getUpdatingCompanyId(), trackObjectId, 0, 0);
            if (tailTc.stationId != StationId::null)
            {
                return StationManager::NearbyStation{ tailTc.stationId, true };
            }
        }

        return StationManager::findNearbyStation(pos, getUpdatingCompanyId());
    }

    // 0x0048FFF7
    static StationManager::NearbyStation findNearbyStationOnTrackAi(World::Pos3 pos, uint16_t tad, uint8_t trackObjectId)
    {
        {
            auto [nextPos, nextRotation] = World::Track::getTrackConnectionEnd(pos, tad);
            const auto tc = World::Track::getTrackConnectionsAi(nextPos, nextRotation, getUpdatingCompanyId(), trackObjectId, 0, 0);
            if (tc.stationId != StationId::null)
            {
                return StationManager::NearbyStation{ tc.stationId, true };
            }
        }
        {
            auto tailTaD = tad;
            const auto& trackSize = World::TrackData::getUnkTrack(tailTaD);
            auto tailPos = pos + trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                tailPos -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }
            tailTaD ^= (1U << 2); // Reverse
            auto [nextTailPos, nextTailRotation] = World::Track::getTrackConnectionEnd(tailPos, tailTaD);
            const auto tailTc = World::Track::getTrackConnectionsAi(nextTailPos, nextTailRotation, getUpdatingCompanyId(), trackObjectId, 0, 0);
            if (tailTc.stationId != StationId::null)
            {
                return StationManager::NearbyStation{ tailTc.stationId, true };
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

    // 0x0048BDCE & 0x0048BD40
    static std::pair<NearbyStationValidation, StationId> validateNearbyStation(const World::Pos3 pos, const uint16_t tad, const uint8_t trackObjectId, const uint8_t flags)
    {
        auto func = (flags & Flags::aiAllocated) ? &findNearbyStationOnTrackAi : &findNearbyStationOnTrack;
        auto nearbyStation = func(pos, tad, trackObjectId);
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
            if (!(flags & Flags::aiAllocated))
            {
                if (StationManager::exceedsStationSize(*station, pos))
                {
                    if (nearbyStation.isPhysicallyAttached)
                    {
                        setErrorText(StringIds::station_too_spread_out);
                        return std::make_pair(NearbyStationValidation::failure, StationId::null);
                    }
                    return std::make_pair(NearbyStationValidation::requiresNewStation, StationId::null);
                }
            }
        }
        return std::make_pair(NearbyStationValidation::okay, nearbyStation.id);
    }

    static World::TrackElement* getElTrack(World::Pos3 pos, uint8_t rotation, uint8_t trackObjectId, uint8_t trackId, uint8_t index)
    {
        auto tile = World::TileManager::get(pos);
        for (auto& el : tile)
        {
            auto* elTrack = el.as<World::TrackElement>();
            if (elTrack == nullptr)
            {
                continue;
            }
            if (elTrack->baseHeight() != pos.z)
            {
                continue;
            }
            if (elTrack->rotation() != rotation)
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
            if (elTrack->sequenceIndex() != index)
            {
                continue;
            }
            return elTrack;
        }
        return nullptr;
    }

    // 0x0048BAC2
    static World::TileClearance::ClearFuncResult clearFuncAiReservation(World::TileElement& el, World::TrackElement& elReferenceTrack)
    {
        auto* elStation = el.as<World::StationElement>();
        auto* elTrack = el.as<World::TrackElement>();
        if (elStation != nullptr)
        {
            if (elStation->stationType() == StationType::trainStation)
            {
                return World::TileClearance::ClearFuncResult::noCollision;
            }
        }
        else if (elTrack != nullptr && elTrack == &elReferenceTrack)
        {
            return World::TileClearance::ClearFuncResult::noCollision;
        }
        return World::TileClearance::ClearFuncResult::collision;
    };

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

    // 0x0048BB20
    static currency32_t createTrainStation(const TrainStationPlacementArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3(16, 16, 0));
        bool updateStationTileRegistration = false;

        auto& returnState = getLegacyReturnState();
        returnState.lastPlacedTrackRoadStationId = StationId::null;
        returnState.lastConstructedAdjoiningStationPos = World::Pos2(-1, -1);
        returnState.lastConstructedAdjoiningStation = StationId::null;

        auto* trackObj = ObjectManager::get<TrackObject>(args.trackObjectId);
        auto* stationObj = ObjectManager::get<TrainStationObject>(args.type);

        const auto trackIdCompatFlags = World::TrackData::getTrackMiscData(args.trackId).compatibleFlags;
        const auto compatibleTrack = trackObj->stationTrackPieces & stationObj->trackPieces & trackIdCompatFlags;
        if (compatibleTrack != trackIdCompatFlags)
        {
            setErrorText(StringIds::track_road_unsuitable_for_station);
            return FAILURE;
        }

        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            return FAILURE;
        }

        auto* initialElTrack = getElTrack(args.pos, args.rotation, args.trackObjectId, args.trackId, args.index);

        auto index = args.index;

        if (initialElTrack == nullptr)
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
        }
        else
        {
            if (!sub_431E6A(initialElTrack->owner(), reinterpret_cast<World::TileElement*>(initialElTrack)))
            {
                return FAILURE;
            }
        }
        auto& trackPieces = World::TrackData::getTrackPiece(args.trackId);
        auto& argPiece = trackPieces[index];
        const auto trackStart = args.pos - World::Pos3(Math::Vector::rotate(World::Pos2(argPiece.x, argPiece.y), args.rotation), argPiece.z);

        if ((flags & Flags::ghost) && (flags & Flags::apply))
        {
            returnState.lastConstructedAdjoiningStationPos = trackStart;
            uint16_t tad = (args.trackId << 3) | args.rotation;
            auto nearbyStation = findNearbyStationOnTrack(trackStart, tad, args.trackObjectId);
            returnState.lastConstructedAdjoiningStation = nearbyStation.id;
        }

        if (!(flags & Flags::ghost))
        {
            if (flags & Flags::apply)
            {
                auto [result, nearbyStationId] = validateNearbyStation(trackStart, (args.trackId << 3) | args.rotation, args.trackObjectId, flags);
                switch (result)
                {
                    case NearbyStationValidation::failure:
                        // Odd???
                        break;
                    case NearbyStationValidation::requiresNewStation:
                    {
                        const auto newStationId = StationManager::allocateNewStation(trackStart, getUpdatingCompanyId(), 0);
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
                updateStationTileRegistration = true;
            }
            else
            {
                // Same as the other branch but deallocate after allocating and return failure on failure
                auto [result, nearbyStationId] = validateNearbyStation(trackStart, (args.trackId << 3) | args.rotation, args.trackObjectId, flags);
                switch (result)
                {
                    case NearbyStationValidation::failure:
                        return FAILURE;
                    case NearbyStationValidation::requiresNewStation:
                    {
                        const auto newStationId = StationManager::allocateNewStation(trackStart, getUpdatingCompanyId(), 0);
                        if (newStationId == StationId::null)
                        {
                            return FAILURE;
                        }
                        StationManager::deallocateStation(newStationId);
                        //  returnState.lastPlacedTrackRoadStationId not set but that's fine since this is the no apply side
                    }
                    break;
                    case NearbyStationValidation::okay:
                        returnState.lastPlacedTrackRoadStationId = nearbyStationId;
                        break;
                }
            }
        }

        currency32_t totalCost = 0;

        for (auto& piece : trackPieces)
        {
            const auto trackLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };

            auto* elTrack = getElTrack(trackLoc, args.rotation, args.trackObjectId, args.trackId, piece.index);

            if (elTrack == nullptr)
            {
                // 0x0048BEC7
                // The following code is only used for aiPlacement when querying to get a rough idea of costs and
                // if there is clearance. It will never get here in a execute.

                // Common with below code. Extract out
                // Calculate station costs
                if (piece.index == 0)
                {
                    auto placementCostBase = Economy::getInflationAdjustedCost(stationObj->buildCostFactor, stationObj->costIndex, 8);
                    const auto cost = (placementCostBase * World::TrackData::getTrackMiscData(args.trackId).costFactor) / 256;
                    totalCost += cost;
                }

                // Perform clearance
                // Subtly different to below (baseZ and qt stuff)
                const auto baseZ = trackLoc.z / World::kSmallZStep;
                const auto clearZ = baseZ + 8 + stationObj->height / World::kSmallZStep;

                // Vanilla did the following code wrong so we have taken a best guess as to what it should do.
                // Vanilla had the following issues:
                // - Kept a pointer to 1 past the end of the tile
                // - Used that pointer to get the quarter tile (which most of the time would mean
                //   the first tileElement in the next tile in the x direction)
                // - Used that pointer to compare tiles on this tile (which would do nothing)
                // - Performed a clearance on the wrong clearZ due to adding 8 twice.
                const auto qt = World::QuarterTile(World::TrackData::getTrackPiece(args.trackId)[index].subTileClearance.getBaseQuarterOccupied(), 0);

                if (!World::TileClearance::applyClearAtStandardHeight(trackLoc, baseZ, clearZ, qt, clearFuncCollideWithSurface))
                {
                    return FAILURE;
                }
                continue;
            }
            else
            {
                if (elTrack->hasSignal())
                {
                    setErrorText(StringIds::signal_in_the_way);
                    return FAILURE;
                }
                if (elTrack->hasLevelCrossing())
                {
                    setErrorText(StringIds::level_crossing_in_the_way);
                    return FAILURE;
                }
                // Connect flags validation
                const auto connectFlags = piece.connectFlags[elTrack->rotation()];
                auto tile = World::TileManager::get(trackLoc);
                for (auto& el : tile)
                {
                    auto* elConnectTrack = el.as<World::TrackElement>();
                    if (elConnectTrack == nullptr)
                    {
                        continue;
                    }
                    if (elConnectTrack == elTrack)
                    {
                        continue;
                    }
                    if (elConnectTrack->baseHeight() != trackLoc.z)
                    {
                        continue;
                    }
                    if (elConnectTrack->isGhost())
                    {
                        continue;
                    }
                    auto& connectPiece = World::TrackData::getTrackPiece(elConnectTrack->trackId())[elConnectTrack->sequenceIndex()];
                    if (connectFlags & connectPiece.connectFlags[elConnectTrack->rotation()])
                    {
                        setErrorText(StringIds::station_cannot_be_built_on_a_junction);
                        return FAILURE;
                    }
                }

                // Calculate station costs
                if (piece.index == 0)
                {
                    bool calculateCost = true;

                    // Replace station if it already exists
                    if (elTrack->hasStationElement())
                    {
                        auto* elStation = elTrack->next()->as<World::StationElement>();
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
                            auto* oldStationObj = ObjectManager::get<TrainStationObject>(elStation->objectId());
                            auto removeCostBase = Economy::getInflationAdjustedCost(oldStationObj->sellCostFactor, oldStationObj->costIndex, 8);
                            const auto cost = (removeCostBase * World::TrackData::getTrackMiscData(args.trackId).costFactor) / 256;
                            totalCost += cost;
                        }
                    }

                    if (calculateCost)
                    {
                        auto placementCostBase = Economy::getInflationAdjustedCost(stationObj->buildCostFactor, stationObj->costIndex, 8);
                        const auto cost = (placementCostBase * World::TrackData::getTrackMiscData(args.trackId).costFactor) / 256;
                        totalCost += cost;
                    }
                }

                // Perform clearance
                const auto baseZ = elTrack->baseZ();
                const auto clearZ = baseZ + 8 + stationObj->height / World::kSmallZStep;
                World::QuarterTile qt(elTrack->occupiedQuarter(), 0);
                if (!(flags & Flags::aiAllocated))
                {
                    auto clearFunc = [&elTrack](World::TileElement& el) {
                        return clearFuncAiReservation(el, *elTrack);
                    };
                    if (!World::TileClearance::applyClearAtStandardHeight(trackLoc, baseZ + 8, clearZ, qt, clearFunc))
                    {
                        return FAILURE;
                    }
                }
                if (!World::TileClearance::applyClearAtStandardHeight(trackLoc, baseZ, clearZ, qt, clearFuncCollideWithSurface))
                {
                    return FAILURE;
                }

                // elTrack is still valid as applyClearAtStandardHeight set to not remove anything
                // this will need changed if ever a different clear function is used
                if (elTrack->hasStationElement() && (flags & Flags::ghost))
                {
                    // ?????
                    setErrorText(StringIds::empty);
                    return FAILURE;
                }

                if (!(flags & Flags::apply))
                {
                    continue;
                }

                World::StationElement* newStationElement = nullptr;
                // Actually place the new station
                if (elTrack->hasStationElement())
                {
                    auto* elStation = elTrack->next()->as<World::StationElement>();
                    if (elStation == nullptr)
                    {
                        return FAILURE;
                    }
                    auto* oldStationObj = ObjectManager::get<TrainStationObject>(elStation->objectId());
                    elTrack->setClearZ(elTrack->clearZ() - oldStationObj->height / World::kSmallZStep);
                    elStation->setSequenceIndex(0);
                    updateStationTileRegistration = false;
                    Ui::ViewportManager::invalidate(trackLoc, elStation->baseHeight(), elStation->clearHeight());
                    newStationElement = elStation;
                }
                else
                {
                    // elTrack pointer will be invalid after this call
                    newStationElement = World::TileManager::insertElementAfterNoReorg<World::StationElement>(
                        reinterpret_cast<World::TileElement*>(elTrack),
                        trackLoc,
                        elTrack->baseZ(),
                        elTrack->occupiedQuarter());
                    if (newStationElement == nullptr)
                    {
                        return FAILURE;
                    }
                    elTrack = newStationElement->prev()->as<World::TrackElement>();
                    if (elTrack == nullptr)
                    {
                        return FAILURE;
                    }
                    newStationElement->setRotation(elTrack->rotation());
                    newStationElement->setGhost(flags & Flags::ghost);
                    newStationElement->setAiAllocated(flags & Flags::aiAllocated);
                    newStationElement->setSequenceIndex(0);
                    newStationElement->setUnk4SLR4(0);
                    newStationElement->setStationType(StationType::trainStation);
                    newStationElement->setBuildingType(0);
                    if (!(flags & Flags::ghost))
                    {
                        newStationElement->setStationId(returnState.lastPlacedTrackRoadStationId);
                    }
                    else
                    {
                        newStationElement->setStationId(static_cast<StationId>(0));
                    }
                    elTrack->setHasStationElement(true);
                }
                newStationElement->setObjectId(args.type);
                elTrack->setClearZ(elTrack->clearZ() + stationObj->height / World::kSmallZStep);
                newStationElement->setClearZ(elTrack->clearZ());
                newStationElement->setOwner(getUpdatingCompanyId());
                Ui::ViewportManager::invalidate(trackLoc, newStationElement->baseHeight(), newStationElement->clearHeight());
            }
        }

        if (!(flags & Flags::ghost) && (flags & Flags::apply))
        {
            if (updateStationTileRegistration)
            {
                addTileToStation(returnState.lastPlacedTrackRoadStationId, trackStart, args.rotation);
            }
            auto* station = StationManager::get(returnState.lastPlacedTrackRoadStationId);
            station->invalidate();
            recalculateStationModes(returnState.lastPlacedTrackRoadStationId);
            recalculateStationCenter(returnState.lastPlacedTrackRoadStationId);
            station->updateLabel();
            station->invalidate();
            sub_48D794(*station);
        }
        return totalCost;
    }

    void createTrainStation(registers& regs)
    {
        regs.ebx = createTrainStation(TrainStationPlacementArgs(regs), regs.bl);
    }
}
