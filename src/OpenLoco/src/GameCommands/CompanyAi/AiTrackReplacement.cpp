#include "AiTrackReplacement.h"
#include "Economy/Economy.h"
#include "GameState.h"
#include "Map/BuildingElement.h"
#include "Map/RoadElement.h"
#include "Map/SignalElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/Track/TrackEnum.h"
#include "Map/TrackElement.h"
#include "Map/TreeElement.h"
#include "Objects/BridgeObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Objects/TrainSignalObject.h"
#include "Objects/TrainStationObject.h"
#include "Ui/WindowManager.h"
#include "World/Station.h"
#include "World/StationManager.h"
#include "World/Town.h"
#include "World/TownManager.h"

namespace OpenLoco::GameCommands
{
    namespace
    {
        struct ClearFunctionArgs
        {
            World::Pos3 pos;
            uint8_t trackId;
            uint8_t bridgeId;
            uint8_t trackObjectId;
            uint8_t flags;
        };
    }

    static World::TrackElement* getTrackElement(const World::Pos3 pos, const uint8_t rotation, const uint8_t trackObjectId, const uint8_t trackId, const uint8_t sequenceIndex, const CompanyId companyId)
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
            if (elTrack->sequenceIndex() != sequenceIndex)
            {
                continue;
            }

            if (elTrack->trackObjectId() != trackObjectId)
            {
                continue;
            }

            if (elTrack->trackId() != trackId)
            {
                continue;
            }

            if (elTrack->owner() != companyId)
            {
                return nullptr;
            }

            return elTrack;
        }
        return nullptr;
    }

    // 0x004A7BE8
    static World::TileClearance::ClearFuncResult clearRoad(World::RoadElement& elRoad, const ClearFunctionArgs& args, bool& hasLevelCrossing)
    {
        if (elRoad.hasBridge())
        {
            getLegacyReturnState().byte_1136075 = elRoad.bridge();
            auto* bridgeObj = ObjectManager::get<BridgeObject>(elRoad.bridge());
            if ((bridgeObj->disabledTrackCfg & World::Track::CommonTraitFlags::junction) != World::Track::CommonTraitFlags::none)
            {
                setErrorText(StringIds::bridge_not_suitable_for_junction);
                return World::TileClearance::ClearFuncResult::collisionErrorSet;
            }
        }

        auto* roadObj = ObjectManager::get<RoadObject>(elRoad.roadObjectId());
        auto* trackObj = ObjectManager::get<TrackObject>(args.trackObjectId);

        if (!(roadObj->compatibleTracks & (1U << args.trackObjectId))
            && !(trackObj->compatibleRoads & (1U << elRoad.roadObjectId())))
        {
            FormatArguments::common(roadObj->name);
            setErrorText(StringIds::unable_to_cross_or_create_junction_with_string);
            return World::TileClearance::ClearFuncResult::collisionErrorSet;
        }

        if (elRoad.hasSignalElement())
        {
            setErrorText(StringIds::signal_in_the_way);
            return World::TileClearance::ClearFuncResult::collisionErrorSet;
        }

        if (elRoad.hasStationElement())
        {
            setErrorText(StringIds::station_in_the_way);
            return World::TileClearance::ClearFuncResult::collisionErrorSet;
        }

        if (elRoad.hasBridge())
        {
            if (elRoad.bridge() != args.bridgeId)
            {
                setErrorText(StringIds::bridge_types_must_match);
                return World::TileClearance::ClearFuncResult::collisionErrorSet;
            }
        }

        hasLevelCrossing = true;
        getLegacyReturnState().flags_1136073 |= (1U << 2);

        if (args.flags & Flags::apply)
        {
            elRoad.setHasLevelCrossing(true);
            elRoad.setLevelCrossingObjectId(getGameState().currentDefaultLevelCrossingType);
            elRoad.setUnk7_10(false);
            elRoad.setUnk6l(0);
        }

        return World::TileClearance::ClearFuncResult::noCollision;
    }

    // 0x004A7A7F
    static World::TileClearance::ClearFuncResult clearFunction(
        World::TileElement& el,
        currency32_t& totalCost,
        bool& hasLevelCrossing,
        World::TileClearance::RemovedBuildings& removedBuildings,
        const ClearFunctionArgs& args)
    {
        switch (el.type())
        {
            case World::ElementType::track:
            {
                return World::TileClearance::ClearFuncResult::noCollision;
            }
            case World::ElementType::station:
            {
                auto* elStation = el.as<World::StationElement>();
                if (elStation->stationType() == StationType::trainStation)
                {
                    return World::TileClearance::ClearFuncResult::noCollision;
                }
                return World::TileClearance::ClearFuncResult::collision;
            }
            case World::ElementType::signal:
                return World::TileClearance::ClearFuncResult::noCollision;
            case World::ElementType::building:
            {
                auto* elBuilding = el.as<World::BuildingElement>();
                if (elBuilding == nullptr)
                {
                    return World::TileClearance::ClearFuncResult::noCollision;
                }
                return World::TileClearance::clearBuildingCollision(*elBuilding, args.pos, removedBuildings, args.flags | Flags::flag_7, totalCost);
            }
            case World::ElementType::tree:
            {
                auto* elTree = el.as<World::TreeElement>();
                if (elTree == nullptr)
                {
                    return World::TileClearance::ClearFuncResult::noCollision;
                }
                return World::TileClearance::clearTreeCollision(*elTree, args.pos, args.flags, totalCost);
            }
            case World::ElementType::road:
            {
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad != nullptr)
                {
                    return clearRoad(*elRoad, args, hasLevelCrossing);
                }
                break;
            }
            case World::ElementType::surface:
                return World::TileClearance::ClearFuncResult::noCollision;

            case World::ElementType::wall:
            case World::ElementType::industry:
                return World::TileClearance::ClearFuncResult::collision;
        }
        return World::TileClearance::ClearFuncResult::collision;
    }

    static currency32_t aiTrackReplacement(const AiTrackReplacementArgs& args, const uint8_t flags)
    {
        getLegacyReturnState().flags_1136072 = World::TileManager::ElementPositionFlags::none;
        GameCommands::setExpenditureType(ExpenditureType::Construction);
        const auto companyId = GameCommands::getUpdatingCompanyId();
        if (flags & GameCommands::Flags::apply)
        {
            const auto center = World::Pos2(args.pos) + World::Pos2{ 16, 16 };
            companySetObservation(companyId, ObservationStatus::buildingTrackRoad, center, EntityId::null, args.trackObjectId);
        }

        auto* elTrackSeq = getTrackElement(args.pos, args.rotation, args.trackObjectId, args.trackId, args.sequenceIndex, companyId);

        if (elTrackSeq == nullptr)
        {
            return GameCommands::FAILURE;
        }

        auto* trackObj = ObjectManager::get<TrackObject>(args.trackObjectId);

        currency32_t totalCost = 0;
        const auto trackIdCostFactor = World::TrackData::getTrackMiscData(args.trackId).costFactor;
        if (elTrackSeq->isAiAllocated())
        {
            {
                const auto trackBaseCost = Economy::getInflationAdjustedCost(trackObj->buildCostFactor, trackObj->costIndex, 10);
                const auto cost = (trackBaseCost * trackIdCostFactor) / 256;
                totalCost += cost;
            }
            for (auto i = 0U; i < 4; ++i)
            {
                if (elTrackSeq->hasMod(i))
                {
                    auto* extraObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[i]);
                    const auto trackExtraBaseCost = Economy::getInflationAdjustedCost(extraObj->buildCostFactor, extraObj->costIndex, 10);
                    const auto cost = (trackExtraBaseCost * trackIdCostFactor) / 256;
                    totalCost += cost;
                }
            }
        }

        if (elTrackSeq->hasSignal())
        {
            auto* elSignal = elTrackSeq->next()->as<World::SignalElement>();
            if (elSignal != nullptr && elSignal->isAiAllocated())
            {
                auto getSignalCost = [](const World::SignalElement::Side& side) {
                    auto* signalObj = ObjectManager::get<TrainSignalObject>(side.signalObjectId());
                    return Economy::getInflationAdjustedCost(signalObj->costFactor, signalObj->costIndex, 10);
                };
                if (elSignal->getLeft().hasSignal())
                {
                    totalCost += getSignalCost(elSignal->getLeft());
                }
                if (elSignal->getRight().hasSignal())
                {
                    totalCost += getSignalCost(elSignal->getRight());
                }
            }
        }

        if (elTrackSeq->hasStationElement())
        {
            auto* elStation = elTrackSeq->next()->as<World::StationElement>();
            if (elStation != nullptr && elStation->isAiAllocated())
            {
                auto* stationObj = ObjectManager::get<TrainStationObject>(elStation->objectId());
                const auto stationBaseCost = Economy::getInflationAdjustedCost(stationObj->buildCostFactor, stationObj->costIndex, 8);
                const auto cost = (stationBaseCost * trackIdCostFactor) / 256;
                totalCost += cost;
            }
        }

        const auto& trackPieces = World::TrackData::getTrackPiece(args.trackId);
        const auto& trackPieceSeq = trackPieces[args.sequenceIndex];
        const auto trackLoc0 = args.pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPieceSeq.x, trackPieceSeq.y }, args.rotation), trackPieceSeq.z };
        World::TileClearance::RemovedBuildings removedBuildings;

        // 0x0113605B
        bool hasBridge = false;
        uint8_t bridgeType = 0xFFU;
        bool overWater = false;

        for (auto& piece : trackPieces)
        {
            const auto trackLoc = trackLoc0 + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };

            // 0x004A76E0
            auto tile = World::TileManager::get(trackLoc);

            auto* elSurface = tile.surface();
            if (elSurface->water() != 0)
            {
                overWater = true;
            }
            auto* elTrack = getTrackElement(trackLoc, args.rotation, args.trackObjectId, args.trackId, piece.index, companyId);
            if (elTrack == nullptr)
            {
                continue;
            }
            if (elTrack->hasBridge())
            {
                hasBridge = true;
                bridgeType = elTrack->bridge();
            }
            if (elTrack->isAiAllocated())
            {
                if (flags & GameCommands::Flags::apply)
                {
                    elTrack->setAiAllocated(false);
                    World::TileManager::mapInvalidateTileFull(trackLoc);
                }
            }
            if (elTrack->hasSignal())
            {
                auto* elSignal = elTrack->next()->as<World::SignalElement>();
                if (elSignal != nullptr && (flags & GameCommands::Flags::apply))
                {
                    elSignal->setAiAllocated(false);
                    World::TileManager::mapInvalidateTileFull(trackLoc);
                }
            }
            if (elTrack->hasStationElement())
            {
                auto* elStation = elTrack->next()->as<World::StationElement>();
                if (elStation != nullptr && (flags & GameCommands::Flags::apply))
                {
                    elStation->setAiAllocated(false);
                    World::TileManager::mapInvalidateTileFull(trackLoc);
                    const auto stationId = elStation->stationId();
                    getLegacyReturnState().lastPlacedTrackRoadStationId = stationId;
                    auto* station = StationManager::get(stationId);
                    if ((station->flags & StationFlags::flag_5) != StationFlags::none)
                    {
                        station->flags &= ~StationFlags::flag_5;
                        auto* town = TownManager::get(station->town);
                        town->numStations++;
                        Ui::WindowManager::invalidate(Ui::WindowType::town, enumValue(town->id()));
                    }
                    station->invalidate();
                    recalculateStationModes(stationId);
                    recalculateStationCenter(stationId);
                    station->updateLabel();
                    station->invalidate();
                    sub_48D794(*station);
                }
            }

            ClearFunctionArgs clearArgs;
            clearArgs.pos = trackLoc;
            clearArgs.bridgeId = bridgeType;
            clearArgs.trackId = args.trackId;
            clearArgs.trackObjectId = args.trackObjectId;
            clearArgs.flags = flags;

            bool hasLevelCrossing = false;

            auto clearFunc = [&totalCost, &hasLevelCrossing, &removedBuildings, &clearArgs](World::TileElement& el) {
                return clearFunction(el, totalCost, hasLevelCrossing, removedBuildings, clearArgs);
            };
            if (!World::TileClearance::applyClearAtStandardHeight(trackLoc, elTrack->baseZ(), elTrack->clearZ(), World::QuarterTile(elTrack->occupiedQuarter(), 0), clearFunc))
            {
                return GameCommands::FAILURE;
            }

            const auto posFlags = World::TileClearance::getPositionFlags();
            // Abridged flags for just above/underground
            const auto newGroundFlags = posFlags & (World::TileManager::ElementPositionFlags::aboveGround | World::TileManager::ElementPositionFlags::underground);
            getLegacyReturnState().flags_1136072 = newGroundFlags;

            if (hasLevelCrossing && (flags & GameCommands::Flags::apply))
            {
                // elTrack is invalid after clearFunction
                elTrack = getTrackElement(trackLoc, args.rotation, args.trackObjectId, args.trackId, piece.index, companyId);
                elTrack->setHasLevelCrossing(true);
            }

            if (flags & Flags::apply)
            {
                World::TileManager::removeAllWallsOnTileBelow(World::toTileSpace(trackLoc), trackLoc.z / World::kSmallZStep);

                World::TileManager::removeSurfaceIndustryAtHeight(trackLoc);
                World::TileManager::setTerrainStyleAsClearedAtHeight(trackLoc);
            }
        }
        // 0x004A7999
        if (hasBridge)
        {
            auto* bridgeObj = ObjectManager::get<BridgeObject>(bridgeType);
            const auto heightCost = 1 * bridgeObj->heightCostFactor; // Why 1???
            const auto bridgeBaseCost = Economy::getInflationAdjustedCost(bridgeObj->baseCostFactor + heightCost, bridgeObj->costIndex, 10);
            auto cost = (bridgeBaseCost * trackIdCostFactor) / 256;
            if (overWater)
            {
                cost *= 2;
            }
            totalCost += cost;
        }

        if ((getLegacyReturnState().flags_1136072 & World::TileManager::ElementPositionFlags::underground) != World::TileManager::ElementPositionFlags::none)
        {
            const auto tunnelBaseCost = Economy::getInflationAdjustedCost(trackObj->tunnelCostFactor, 2, 8);
            auto cost = (tunnelBaseCost * trackIdCostFactor) / 256;

            totalCost += cost;
        }

        if (flags & Flags::apply)
        {
            playConstructionPlacementSound(getPosition());
        }

        return totalCost;
    }

    // 0x004A734F
    void aiTrackReplacement(registers& regs)
    {
        regs.ebx = aiTrackReplacement(AiTrackReplacementArgs(regs), regs.bl);
    }
}
