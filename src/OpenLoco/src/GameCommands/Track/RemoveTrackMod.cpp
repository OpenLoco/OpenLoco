#include "RemoveTrackMod.h"
#include "Economy/Expenditures.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Math/Vector.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x004A668A
    static currency32_t removeTrackMod(const TrackModsRemovalArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3(16, 16, 0));

        auto* elTrack = [&args]() -> const World::TrackElement* {
            const auto tile = World::TileManager::get(args.pos);
            for (const auto& el : tile)
            {
                auto* elTrack = el.as<World::TrackElement>();
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

                return elTrack;
            }
            return nullptr;
        }();

        if (elTrack == nullptr)
        {
            return FAILURE;
        }

        if (!sub_431E6A(elTrack->owner(), reinterpret_cast<const World::TileElement*>(elTrack)))
        {
            return FAILURE;
        }

        const auto& piece = World::TrackData::getTrackPiece(elTrack->trackId())[elTrack->sequenceIndex()];
        const auto offsetToFirstTile = World::Pos3{
            Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, elTrack->unkDirection()),
            piece.z
        };
        const auto firstTilePos = args.pos - offsetToFirstTile;
        const auto tad = Vehicles::TrackAndDirection::_TrackAndDirection(elTrack->trackId(), elTrack->unkDirection());

        auto cost = Vehicles::removeTrackModsToTrackNetwork(firstTilePos, tad, elTrack->owner(), args.trackObjType, flags, args.modSection, args.type);

        return cost;
    }

    void removeTrackMod(registers& regs)
    {
        regs.ebx = removeTrackMod(TrackModsRemovalArgs(regs), regs.bl);
    }
}
