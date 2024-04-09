#include "RemoveTrainStation.h"
#include "Map/TileElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "World/Station.h"
#include "World/StationManager.h"

namespace OpenLoco::GameCommands
{
    static loco_global<bool, 0x0112C7A9> _112C7A9;

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

    static currency32_t removeTrainStation(const TrainStationRemovalArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3(16, 16, 0));
        _112C7A9 = true;

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

        // sub dx, [ebp+edi+5] ???

        StationId foundStationId = StationId::null;
        currency32_t totalCost = 0;

        for (auto& piece : trackPieces)
        {
            const auto trackLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };

            auto* elTrack = getElTrack(trackLoc, args.rotation, args.type, args.trackId, piece.index);

            if (elTrack == nullptr)
            {
                if (_112C7A9 && (flags & Flags::apply) != 0)
                {
                    auto* station = StationManager::get(foundStationId);
                    removeTileFromStation(foundStationId, trackStart, args.rotation);
                    station->invalidate();

                    recalculateStationCenter(foundStationId);
                    recalculateStationModes(foundStationId);
                    station->updateLabel();
                    station->invalidate();

                    sub_48D794(*station);
                }
                break;
            }
            else
            {
                // loc_48C572

                // add dx, [ebp+edi+5] ???

            }
        }

        return totalCost;
    }

    void removeTrainStation(registers& regs)
    {
        regs.ebx = removeTrainStation(TrainStationRemovalArgs(regs), regs.bl);
    }
}
