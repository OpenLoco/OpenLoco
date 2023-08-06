#include "Economy/Expenditures.h"
#include "GameCommands.h"
#include "Localisation/StringIds.h"
#include "Map/SignalElement.h"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"

namespace OpenLoco::GameCommands
{
    // 0x00488BDB
    uint32_t createSignal(const SignalPlacementArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3{ 16, 16, 0 });
        if (!World::TileManager::checkFreeElementsAndReorganise())
        {
            return FAILURE;
        }

        auto tile = World::TileManager::get(args.pos);
        World::TrackElement* elTrack = nullptr;
        for (auto& el : tile)
        {
            elTrack = el.as<World::TrackElement>();
            if (elTrack == nullptr)
            {
                continue;
            }
            if (elTrack->baseHeight() != args.pos.z)
            {
                continue;
            }
            if (elTrack->unkDirection() != args.rotation)
            {
                continue;
            }
            if (elTrack->sequenceIndex() != args.index)
            {
                continue;
            }
            if (elTrack->trackObjectId() != args.trackObjType)
            {
                continue;
            }
            if (elTrack->trackId() != args.trackId)
            {
                continue;
            }
            break;
        }

        if (elTrack == nullptr)
        {
            return FAILURE;
        }

        if (elTrack->hasLevelCrossing())
        {
            setErrorText(StringIds::level_crossing_in_the_way);
            return FAILURE;
        }

        if (!sub_431E6A(elTrack->owner(), reinterpret_cast<World::TileElement*>(elTrack)))
        {
            return FAILURE;
        }

        const auto trackPieces = World::TrackData::getTrackPiece(args.trackId);
        auto& trackPiece = trackPieces[args.index];

        const auto trackStart = args.pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, args.rotation), trackPiece.z };

        for (auto& piece : trackPieces)
        {
            const auto trackLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, args.rotation), trackPiece.z };
            // 0x00488D5A
        }
        // 0x00488D14
    }

    void createSignal(registers& regs)
    {
        regs.ebx = createSignal(SignalPlacementArgs(regs), regs.bl);
    }
}
