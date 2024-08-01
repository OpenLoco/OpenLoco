#include "RemoveTrack.h"
#include "Economy/Economy.h"
#include "Map/SignalElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/TrackObject.h"
#include "RemoveSignal.h"
#include "RemoveTrainStation.h"
#include "Vehicles/Vehicle.h"
#include "ViewportManager.h"

namespace OpenLoco::GameCommands
{
    static World::TrackElement* getTrack(const TrackRemovalArgs& args, const uint8_t trackFlags)
    {
        auto tile = World::TileManager::get(args.pos);
        for (auto& el : tile)
        {
            auto* elTrack = el.as<World::TrackElement>();
            if (elTrack == nullptr)
            {
                continue;
            }
            if (elTrack->rotation() != args.rotation)
            {
                continue;
            }
            if (elTrack->sequenceIndex() != args.index)
            {
                continue;
            }
            if (elTrack->trackObjectId() != args.trackObjectId)
            {
                continue;
            }
            if ((elTrack->flags() & 0x30) != trackFlags)
            {
                continue;
            }
            if ((trackFlags & 0x10) != 0 && elTrack->owner() != getUpdatingCompanyId())
            {
                continue;
            }
            return elTrack;
        }
        return nullptr;
    };

    // 0x0049C7F2
    static currency32_t removeTrack(const TrackRemovalArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3{ 16, 16, 0 });

        currency32_t totalRemovalCost = 0;

        uint8_t trackFlags = 0; // 0x01136083
        if (flags & Flags::ghost)
        {
            trackFlags |= 0x10;
        }
        if (flags & Flags::aiAllocated)
        {
            trackFlags |= 0x20;
        }

        auto* elTrack = getTrack(args, trackFlags);
        if (elTrack == nullptr)
        {
            return FAILURE;
        }

        if ((flags & Flags::aiAllocated) == 0 && !sub_431E6A(elTrack->owner(), reinterpret_cast<World::TileElement*>(elTrack)))
        {
            return FAILURE;
        }

        // 0x0049C902
        if (elTrack->hasSignal())
        {
            GameCommands::SignalRemovalArgs srArgs;
            srArgs.pos = args.pos;
            srArgs.rotation = args.rotation;
            srArgs.index = args.index;
            srArgs.trackId = args.trackId;
            srArgs.trackObjType = args.trackObjectId;
            srArgs.flags = 0;

            auto* elSignal = elTrack->next()->as<World::SignalElement>();
            if (elSignal != nullptr)
            {
                if (elSignal->getLeft().hasSignal())
                {
                    srArgs.flags |= 1U << 15;
                }
                if (elSignal->getRight().hasSignal())
                {
                    srArgs.flags |= 1U << 14;
                }
            }

            if (auto cost = GameCommands::doCommand(srArgs, flags); cost != FAILURE)
            {
                totalRemovalCost += cost;
            }
            else
            {
                return FAILURE;
            }
        }

        // 0x0049C967
        // Fetch track element again; signal removal above might have invalidated the pointer.
        elTrack = getTrack(args, trackFlags);

        // 0x0049C9F0
        if (elTrack->hasStationElement())
        {
            GameCommands::TrainStationRemovalArgs tsArgs;
            tsArgs.pos = args.pos;
            tsArgs.rotation = args.rotation;
            tsArgs.trackId = args.trackId;
            tsArgs.index = args.index;
            tsArgs.type = args.trackObjectId;

            if (auto cost = GameCommands::doCommand(tsArgs, flags); cost != FAILURE)
            {
                totalRemovalCost += cost;
            }
            else
            {
                return FAILURE;
            }
        }

        // 0x0049CA3B
        // Fetch track element again; station removal above might have invalidated the pointer.
        elTrack = getTrack(args, trackFlags);

        // 0x0049CAC4
        return totalRemovalCost;
    }

    void removeTrack(registers& regs)
    {
        regs.ebx = removeTrack(TrackRemovalArgs(regs), regs.bl);
    }
}
