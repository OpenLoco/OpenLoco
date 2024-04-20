#include "RemoveTrainStation.h"
#include "Economy/Economy.h"
#include "Map/StationElement.h"
#include "Map/TileElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrainStationObject.h"
#include "ViewportManager.h"
#include "World/Station.h"
#include "World/StationManager.h"

namespace OpenLoco::GameCommands
{
    // TODO: based on CreateTrainStation.cpp
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
            if (elTrack->unkDirection() != rotation)
            {
                continue;
            }
            if (elTrack->sequenceIndex() != index)
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
            return elTrack;
        }
        return nullptr;
    }

    // 0x0048C402
    static currency32_t removeTrainStation(const TrainStationRemovalArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3(16, 16, 0));
        bool updateStationTileRegistration = true;

        auto* initialElTrack = getElTrack(args.pos, args.rotation, args.type, args.trackId, args.index);
        if (initialElTrack == nullptr)
        {
            return FAILURE;
        }

        if (!sub_431E6A(initialElTrack->owner(), reinterpret_cast<const World::TileElement*>(initialElTrack)))
        {
            return FAILURE;
        }

        const auto& trackPieces = World::TrackData::getTrackPiece(args.trackId);
        const auto& argPiece = trackPieces[args.index];
        const auto trackStart = args.pos - World::Pos3(Math::Vector::rotate(World::Pos2(argPiece.x, argPiece.y), args.rotation), argPiece.z);

        StationId foundStationId = StationId::null;
        currency32_t totalCost = 0;

        for (auto& piece : trackPieces)
        {
            const auto trackLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };

            auto* elTrack = getElTrack(trackLoc, args.rotation, args.type, args.trackId, piece.index);
            if (elTrack == nullptr)
                return FAILURE;

            auto* nextEl = elTrack->next();
            auto* stationEl = nextEl->as<World::StationElement>();
            if (stationEl == nullptr)
                return FAILURE;

            if (stationEl->isGhost())
                updateStationTileRegistration = false;

            foundStationId = stationEl->stationId();
            auto* stationObj = ObjectManager::get<TrainStationObject>(stationEl->objectId());

            if (piece.index == 0)
            {
                auto removeCostBase = Economy::getInflationAdjustedCost(stationObj->sellCostFactor, stationObj->costIndex, 8);
                totalCost += (removeCostBase * World::TrackData::getTrackMiscData(args.trackId).costFactor) / 256;
            }

            if ((flags & Flags::apply) != 0)
            {
                elTrack->setClearZ(elTrack->clearZ() - stationObj->height);
                Ui::ViewportManager::invalidate(World::Pos2(trackLoc), stationEl->baseHeight(), stationEl->clearHeight(), ZoomLevel::eighth);
                elTrack->setHasStationElement(false);
                World::TileManager::removeElement(*nextEl);
            }
        }

        if (updateStationTileRegistration && (flags & Flags::apply) != 0)
        {
            auto* station = StationManager::get(foundStationId);
            removeTileFromStationAndRecalcCargo(foundStationId, trackStart, args.rotation);
            station->invalidate();

            recalculateStationModes(foundStationId);
            recalculateStationCenter(foundStationId);
            station->updateLabel();
            station->invalidate();

            sub_48D794(*station);
        }

        return totalCost;
    }

    void removeTrainStation(registers& regs)
    {
        regs.ebx = removeTrainStation(TrainStationRemovalArgs(regs), regs.bl);
    }
}
