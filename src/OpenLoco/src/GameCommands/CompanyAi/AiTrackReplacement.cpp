#include "AiTrackReplacement.h"
#include "Economy/Economy.h"
#include "Map/SignalElement.h"
#include "Map/StationElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Objects/TrainSignalObject.h"
#include "Objects/TrainStationObject.h"

namespace OpenLoco::GameCommands
{
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

    static currency32_t aiTrackReplacement(const AiTrackReplacementArgs& args, const uint8_t flags)
    {
        // Structured very like a game command
        GameCommands::setExpenditureType(ExpenditureType::Construction);
        const auto companyId = GameCommands::getUpdatingCompanyId();
        if (flags & GameCommands::Flags::apply)
        {
            const auto center = World::Pos2(args.pos) + World::Pos2{ 16, 16 };
            companySetObservation(companyId, ObservationStatus::buildingTrackRoad, center, EntityId::null, args.trackObjectId);
        }

        auto* elTrack = getTrackElement(args.pos, args.rotation, args.trackObjectId, args.trackId, args.sequenceIndex, companyId);

        if (elTrack == nullptr)
        {
            return GameCommands::FAILURE;
        }

        auto* trackObj = ObjectManager::get<TrackObject>(args.trackObjectId);

        currency32_t totalCost = 0;
        const auto trackIdCostFactor = World::TrackData::getTrackMiscData(args.trackId).costFactor;
        if (elTrack->isAiAllocated())
        {
            const auto trackBaseCost = Economy::getInflationAdjustedCost(trackObj->buildCostFactor, trackObj->costIndex, 10);
            const auto cost = (trackBaseCost * trackIdCostFactor) / 256;
            totalCost += cost;

            for (auto i = 0U; i < 4; ++i)
            {
                if (elTrack->hasMod(i))
                {
                    auto* extraObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[i]);
                    const auto trackExtraBaseCost = Economy::getInflationAdjustedCost(extraObj->buildCostFactor, extraObj->costIndex, 10);
                    const auto cost = (trackExtraBaseCost * trackIdCostFactor) / 256;
                    totalCost += cost;
                }
            }
        }

        if (elTrack->hasSignal())
        {
            auto* elSignal = elTrack->next()->as<World::SignalElement>();
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

        if (elTrack->hasStationElement())
        {
            auto* elStation = elTrack->next()->as<World::StationElement>();
            if (elStation != nullptr && elStation->isAiAllocated())
            {
                auto* stationObj = ObjectManager::get<TrainStationObject>(elStation->objectId());
                const auto stationBaseCost = Economy::getInflationAdjustedCost(stationObj->buildCostFactor, stationObj->costIndex, 8);
                const auto cost = (stationBaseCost * trackIdCostFactor) / 256;
            }
        }

        const auto& trackPieces = World::TrackData::getTrackPiece(args.trackId);
        const auto& trackPieceSeq = trackPieces[args.sequenceIndex];
        const auto trackLoc0 = args.pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPieceSeq.x, trackPieceSeq.y }, args.rotation), trackPieceSeq.z };
        World::TileClearance::RemovedBuildings removedBuildings;

        // 0x0113605B
        bool hasBridge = false;

        for (auto& piece : trackPieces)
        {
            const auto trackLoc = trackLoc0 + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };

            // 0x004A76E0
        }
        // 0x004A7999

        return totalCost;
    }

    // 0x004A734F
    void aiTrackReplacement(registers& regs)
    {
        regs.ebx = aiTrackReplacement(AiTrackReplacementArgs(regs), regs.bl);
    }
}
