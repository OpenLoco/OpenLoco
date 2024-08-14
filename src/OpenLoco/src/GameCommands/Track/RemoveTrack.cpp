#include "RemoveTrack.h"
#include "Economy/Economy.h"
#include "Map/RoadElement.h"
#include "Map/SignalElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/BridgeObject.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Random.h"
#include "RemoveSignal.h"
#include "RemoveTrainStation.h"
#include "Vehicles/Vehicle.h"
#include "ViewportManager.h"

namespace OpenLoco::GameCommands
{
    static World::TrackElement* getElTrackAt(const TrackRemovalArgs& args, const uint8_t flags, const World::Pos3 pos, const uint8_t sequenceIndex)
    {
        auto tile = World::TileManager::get(pos);
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
            if (elTrack->sequenceIndex() != sequenceIndex)
            {
                continue;
            }
            if (elTrack->trackObjectId() != args.trackObjectId)
            {
                continue;
            }
            if (elTrack->isGhost() != (((flags & Flags::ghost) != 0)))
            {
                continue;
            }
            if (elTrack->isAiAllocated() != (((flags & Flags::aiAllocated) != 0)))
            {
                continue;
            }
            if (elTrack->owner() != getUpdatingCompanyId())
            {
                continue;
            }
            return elTrack;
        }
        return nullptr;
    };

    // 0x0049CC23
    static currency32_t trackRemoveCost(const TrackRemovalArgs& args, const World::TrackData::PreviewTrack trackPiece0, const World::Pos3 trackStart, const uint8_t flags)
    {
        const auto trackLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece0.x, trackPiece0.y }, args.rotation), trackPiece0.z };
        auto* pieceElTrack = getElTrackAt(args, flags, trackLoc, trackPiece0.index);
        if (pieceElTrack == nullptr)
        {
            return 0;
        }

        currency32_t totalCost = 0;

        const auto* trackObj = ObjectManager::get<TrackObject>(pieceElTrack->trackObjectId());
        totalCost += Economy::getInflationAdjustedCost(trackObj->sellCostFactor, trackObj->costIndex, 10);

        // Check mod removal costs
        for (auto i = 0U; i < 4; i++)
        {
            if (pieceElTrack->hasMod(i))
            {
                const auto* trackExtraObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[i]);
                totalCost += Economy::getInflationAdjustedCost(trackExtraObj->sellCostFactor, trackExtraObj->costIndex, 10);
            }
        }

        return totalCost;
    }

    // 0x004795D1
    static void setLevelCrossingFlags(const World::Pos3 pos)
    {
        auto findLevelTrackAndRoad = [pos](auto&& trackFunction, auto&& roadFunction) {
            auto tile = World::TileManager::get(pos);
            for (auto& el : tile)
            {
                if (el.baseHeight() != pos.z)
                {
                    continue;
                }
                if (el.isAiAllocated())
                {
                    continue;
                }
                auto* elTrack = el.as<World::TrackElement>();
                if (elTrack != nullptr)
                {
                    if (elTrack->trackId() == 0)
                    {
                        trackFunction(*elTrack);
                    }
                }
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad != nullptr)
                {
                    if (elRoad->roadId() == 0)
                    {
                        roadFunction(*elRoad);
                    }
                }
            }
        };

        bool hasRoad = false;
        bool hasTrack = false;
        findLevelTrackAndRoad(
            [&hasTrack](World::TrackElement& elTrack) { hasTrack |= elTrack.hasLevelCrossing(); },
            [&hasRoad](World::RoadElement& elRoad) { hasRoad |= elRoad.hasLevelCrossing(); });

        if (hasRoad ^ hasTrack)
        {
            findLevelTrackAndRoad(
                [hasTrack](World::TrackElement& elTrack) { if (hasTrack) { elTrack.setHasLevelCrossing(false); } },
                [hasRoad](World::RoadElement& elRoad) { if (hasRoad) {
                    elRoad.setHasLevelCrossing(false);
                    elRoad.setUnk7_10(false);
                    elRoad.setLevelCrossingObjectId(0);
                } });
        }
    }

    // 0x0048B04E
    static void playTrackRemovalSound(const World::Pos3 pos)
    {
        const auto frequency = gPrng1().randNext(8191, 17955);
        Audio::playSound(Audio::SoundId::demolish, pos, 0, frequency);
    }

    // 0x0049C7F2
    static currency32_t removeTrack(const TrackRemovalArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3{ 16, 16, 0 });

        currency32_t totalRemovalCost = 0;

        auto* elTrack = getElTrackAt(args, flags, args.pos, args.index);
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
        elTrack = getElTrackAt(args, flags, args.pos, args.index);

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
        elTrack = getElTrackAt(args, flags, args.pos, args.index);

        // 0x0049CAC4
        const auto trackPieces = World::TrackData::getTrackPiece(args.trackId);
        auto& trackPiece = trackPieces[args.index];

        const auto trackStart = args.pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, args.rotation), trackPiece.z };

        // NB: moved out of the loop below (was at 0x0049CC1B)
        const currency32_t pieceRemovalCost = trackRemoveCost(args, trackPieces[0], trackStart, flags);

        bool trackHadBridge = false; // 0x0113605B
        int8_t trackBridgeId = -1;   // 0x0113605C

        // 0x0049CB27
        for (auto& piece : trackPieces)
        {
            const auto trackLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };

            if (!(flags & Flags::aiAllocated))
            {
                World::TileManager::mapInvalidateTileFull(trackLoc); // shift piece.z?
            }

            // 0x0049CB84
            auto* pieceElTrack = getElTrackAt(args, flags, trackLoc, piece.index);
            if (pieceElTrack == nullptr)
            {
                return FAILURE;
            }

            // 0x0049CC02
            if (pieceElTrack->hasBridge())
            {
                trackHadBridge = true;
                trackBridgeId = pieceElTrack->bridge();
            }

            if (!(flags & Flags::apply))
            {
                continue;
            }

            auto baseHeight = pieceElTrack->baseHeight();
            World::TileManager::removeElement(*reinterpret_cast<World::TileElement*>(pieceElTrack));
            setLevelCrossingFlags(World::Pos3{ trackLoc.x, trackLoc.y, baseHeight });
        }

        totalRemovalCost += pieceRemovalCost;

        if (trackHadBridge)
        {
            const auto* bridgeObj = ObjectManager::get<BridgeObject>(trackBridgeId);
            totalRemovalCost += Economy::getInflationAdjustedCost(bridgeObj->sellCostFactor, bridgeObj->costIndex, 10);
        }

        if (flags & Flags::apply)
        {
            if (!(flags & (Flags::aiAllocated | Flags::ghost)))
            {
                playTrackRemovalSound(args.pos);
            }
        }

        return totalRemovalCost;
    }

    void removeTrack(registers& regs)
    {
        regs.ebx = removeTrack(TrackRemovalArgs(regs), regs.bl);
    }
}
