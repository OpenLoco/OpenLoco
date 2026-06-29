#include "GameCommands/Track/RemoveSignal.h"
#include "Economy/Economy.h"
#include "Map/SignalElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrainSignalObject.h"
#include "Vehicles/Vehicle.h"
#include "ViewportManager.h"

namespace OpenLoco::GameCommands
{
    struct TrackLookup
    {
        World::TileElementEntry* entry;
        World::TrackElement* element;
    };

    static TrackLookup getElTrackAt(const SignalRemovalArgs& args, const World::Pos3 pos, const uint8_t sequenceIndex)
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
            if (elTrack->rotation() != args.rotation)
            {
                continue;
            }
            if (elTrack->sequenceIndex() != sequenceIndex)
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
            return { &el, elTrack };
        }
        return { nullptr, nullptr };
    };

    static currency32_t signalRemoveCost(const SignalRemovalArgs& args, const World::TrackData::PreviewTrack trackPiece0, const World::Pos3 trackStart)
    {
        const auto trackLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece0.x, trackPiece0.y }, args.rotation), trackPiece0.z };
        auto [trackEntry, pieceElTrack] = getElTrackAt(args, trackLoc, trackPiece0.index);
        if (trackEntry == nullptr)
        {
            return 0;
        }

        if (!pieceElTrack->hasSignal())
        {
            return 0;
        }

        auto* elSignal = trackEntry->next()->as<World::SignalElement>();
        if (elSignal == nullptr)
        {
            return kFailure;
        }

        currency32_t totalCost = 0;
        if ((args.flags & (1U << 15)) && elSignal->getLeft().hasSignal())
        {
            const auto* signalObj = ObjectManager::get<TrainSignalObject>(elSignal->getLeft().signalObjectId());
            totalCost += Economy::getInflationAdjustedCost(signalObj->sellCostFactor, signalObj->costIndex, 10);
        }
        if ((args.flags & (1U << 14)) && elSignal->getRight().hasSignal())
        {
            const auto* signalObj = ObjectManager::get<TrainSignalObject>(elSignal->getRight().signalObjectId());
            totalCost += Economy::getInflationAdjustedCost(signalObj->sellCostFactor, signalObj->costIndex, 10);
        }
        return totalCost;
    }

    // 0x004891E4
    static currency32_t removeSignal(const SignalRemovalArgs& args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(args.pos + World::Pos3{ 16, 16, 0 });

        auto [trackEntry, elTrack] = getElTrackAt(args, args.pos, args.index);

        if (trackEntry == nullptr)
        {
            return kFailure;
        }

        if (!checkCompanyCompatibility(elTrack->owner(), *elTrack))
        {
            return kFailure;
        }

        const auto trackPieces = World::TrackData::getTrackPiece(args.trackId);
        auto& trackPiece = trackPieces[args.index];

        const auto trackStart = args.pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, args.rotation), trackPiece.z };

        currency32_t cost = signalRemoveCost(args, trackPieces[0], trackStart);

        if (static_cast<uint32_t>(cost) == kFailure)
        {
            return kFailure;
        }

        for (auto& piece : trackPieces)
        {
            const auto trackLoc = trackStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, args.rotation), piece.z };
            auto [pieceEntry, pieceElTrack] = getElTrackAt(args, trackLoc, piece.index);
            if (pieceEntry == nullptr)
            {
                return kFailure;
            }

            if (!(flags & Flags::apply))
            {
                continue;
            }

            if (!pieceElTrack->hasSignal())
            {
                return 0;
            }

            auto* signalEntry = pieceEntry->next();
            auto* elSignal = signalEntry->as<World::SignalElement>();
            if (elSignal == nullptr)
            {
                return kFailure;
            }

            if (args.flags & (1U << 15))
            {
                if (!(flags & Flags::ghost) || elSignal->isLeftGhost())
                {
                    auto& left = elSignal->getLeft();
                    left.setHasSignal(false);
                    left.setAllLights(0);
                    left.setFrame(0);
                    elSignal->setLeftGhost(false);
                }
            }
            if (args.flags & (1U << 14))
            {
                if (!(flags & Flags::ghost) || elSignal->isRightGhost())
                {
                    auto& right = elSignal->getRight();
                    right.setHasSignal(false);
                    right.setAllLights(0);
                    right.setFrame(0);
                    elSignal->setRightGhost(false);
                }
            }
            Ui::ViewportManager::invalidate(trackLoc, elSignal->baseHeight(), elSignal->baseHeight() + 32, ZoomLevel::half);

            elSignal->setGhost(false); // Why??

            // No signals anymore so delete the tile!
            if (!elSignal->getLeft().hasSignal() && !elSignal->getRight().hasSignal())
            {
                pieceElTrack->setHasSignal(false);
                World::TileManager::removeElement(*signalEntry);
            }
        }

        if (flags & Flags::apply)
        {
            if (!(flags & (Flags::aiAllocated | Flags::ghost)))
            {
                const Vehicles::TrackAndDirection::_TrackAndDirection tad(args.trackId, args.rotation);

                Vehicles::sub_4A2AD7(trackStart, tad, getUpdatingCompanyId(), args.trackObjType);
            }
        }
        return cost;
    }

    void removeSignal(registers& regs, const uint8_t flags)
    {
        regs.ebx = removeSignal(SignalRemovalArgs(regs), flags);
    }
}
