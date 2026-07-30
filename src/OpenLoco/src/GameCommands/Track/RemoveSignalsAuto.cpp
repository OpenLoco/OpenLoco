#include "GameCommands/Track/RemoveSignalsAuto.h"
#include "GameCommands/Track/RemoveSignal.h"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"

namespace OpenLoco::GameCommands
{
    struct TrackLookup
    {
        World::TileElementEntry* entry;
        World::TrackElement* element;
    };

    static TrackLookup getElTrackAt(const World::Pos3 pos, const uint8_t index, const uint8_t rotation, const uint8_t trackObjType, const uint8_t trackId)
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
            if (elTrack->sequenceIndex() != index)
            {
                continue;
            }
            if (elTrack->trackObjectId() != trackObjType)
            {
                continue;
            }
            if (elTrack->trackId() != trackId)
            {
                continue;
            }
            return { &el, elTrack };
        }
        return { nullptr, nullptr };
    };

    static uint32_t AutoRemoveSignals(const World::Pos3& trackStart, uint16_t tad, const uint8_t trackObjType, const uint16_t sides, const uint8_t step, const uint8_t initialStep, const uint8_t flags)
    {
        auto pos = World::Pos3(trackStart);
        int32_t currentStep = initialStep;
        if (currentStep >= step)
        {
            currentStep = 0;
        }
        uint32_t totalCost = 0;
        while (true)
        {
            const auto rotation = tad & 0x3;
            const auto trackId = (tad >> 3) & 0x3F;
            auto elTrack = getElTrackAt(pos, 0, rotation, trackObjType, trackId);
            if (elTrack.element == nullptr)
            {
                return GameCommands::kFailure;
            }

            // If we have a station stop the auto removal
            if (elTrack.element->hasStationElement())
            {
                break;
            }

            const auto [trackEndLoc, trackEndRotation] = World::Track::getTrackConnectionEnd(pos, tad);
            auto tc = World::Track::getTrackConnections(trackEndLoc, trackEndRotation, GameCommands::getUpdatingCompanyId(), trackObjType, 0, 0);

            // If there is a junction or no connection we stop the auto removal
            if (tc.connections.size() != 1)
            {
                break;
            }

            // If we have a level crossing we skip this tile for removal
            if (currentStep == 0 && !elTrack.element->hasLevelCrossing())
            {
                GameCommands::SignalRemovalArgs sargs{};
                sargs.pos = pos;
                sargs.rotation = rotation;
                sargs.trackId = trackId;
                sargs.index = 0;
                sargs.flags = sides;
                sargs.trackObjType = trackObjType;

                auto res = GameCommands::doCommand(sargs, flags);
                if (res == GameCommands::kFailure)
                {
                    return GameCommands::kFailure;
                }
                totalCost += res;
            }

            // Now we move to the next track piece
            pos = trackEndLoc;
            tad = tc.connections[0] & World::Track::AdditionalTaDFlags::basicTaDMask;
            currentStep++;
            if (currentStep >= step)
            {
                currentStep = 0;
            }

            // If we have looped back to the start we stop the auto removal
            if (pos == trackStart)
            {
                break;
            }
        }
        return totalCost;
    }

    static uint32_t AutoRemoveSignalsBackwards(const World::Pos3& trackStart, uint16_t tad, const uint8_t trackObjType, const uint16_t sides, const uint8_t step, const uint8_t initialStep, const uint8_t flags)
    {
        auto pos = World::Pos3(trackStart);
        int32_t currentStep = initialStep;
        if (currentStep >= step)
        {
            currentStep = 0;
        }
        uint32_t totalCost = 0;
        while (true)
        {
            const auto rotation = tad & 0x3;
            const auto trackId = (tad >> 3) & 0x3F;
            auto elTrack = getElTrackAt(pos, 0, rotation, trackObjType, trackId);
            if (elTrack.element == nullptr)
            {
                return GameCommands::kFailure;
            }

            // If we have a station stop the auto removal
            if (elTrack.element->hasStationElement())
            {
                break;
            }

            auto reverseStart = pos;
            auto& trackSize = World::TrackData::getUnkTrack(tad);
            if (trackSize.rotationEnd < 12)
            {
                reverseStart -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }

            const auto reverseStartRotation = World::kReverseRotation[trackSize.rotationEnd];
            auto tc = World::Track::getTrackConnections(reverseStart, reverseStartRotation, GameCommands::getUpdatingCompanyId(), trackObjType, 0, 0);

            // If there is a junction or no connection we stop the auto removal
            if (tc.connections.size() != 1)
            {
                break;
            }

            // If we have a level crossing we skip this tile for removal
            if (currentStep == 0 && !elTrack.element->hasLevelCrossing())
            {
                GameCommands::SignalRemovalArgs sargs{};
                sargs.pos = pos;
                sargs.rotation = rotation;
                sargs.trackId = trackId;
                sargs.index = elTrack.element->sequenceIndex();
                sargs.flags = sides;
                sargs.trackObjType = trackObjType;

                auto res = GameCommands::doCommand(sargs, flags);
                if (res == GameCommands::kFailure)
                {
                    return GameCommands::kFailure;
                }
                totalCost += res;
            }

            // Now we move to the next track piece
            pos = reverseStart;
            tad = tc.connections[0] & World::Track::AdditionalTaDFlags::basicTaDMask;
            tad ^= (1U << 2);
            currentStep++;
            if (currentStep >= step)
            {
                currentStep = 0;
            }

            // If we have looped back to the start we stop the auto removal
            if (pos == trackStart)
            {
                break;
            }
        }
        return totalCost;
    }

    static uint32_t removeSignalsAuto(const SignalsRemovalAutoArgs& args, const uint8_t flags)
    {
        const auto trackPieces = World::TrackData::getTrackPiece(args.trackId);
        if (trackPieces.size() < args.index)
        {
            return GameCommands::kFailure;
        }

        auto& trackPiece = trackPieces[args.index];
        const auto trackStart = args.pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, args.rotation), trackPiece.z };

        uint32_t totalCost = 0;

        // Perform a forward walk along the track removing signals every arg.step tiles
        {
            const auto startTad = args.rotation | (args.trackId << 3);
            auto cost = AutoRemoveSignals(trackStart, startTad, args.trackObjType, args.flags, args.step, 0, flags);
            if (cost == GameCommands::kFailure)
            {
                return GameCommands::kFailure;
            }
            totalCost += cost;
        }

        // Perform a backward walk along the track removing signals every arg.step tiles
        {
            auto reverseStart = trackStart;
            auto& trackSize = World::TrackData::getUnkTrack((args.trackId << 3) | args.rotation);
            if (trackSize.rotationEnd < 12)
            {
                reverseStart -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }

            const auto reverseStartRotation = World::kReverseRotation[trackSize.rotationEnd];
            auto tc = World::Track::getTrackConnections(reverseStart, reverseStartRotation, GameCommands::getUpdatingCompanyId(), args.trackObjType, 0, 0);
            if (tc.connections.size() == 1)
            {
                const auto reverseTad = (tc.connections[0] & World::Track::AdditionalTaDFlags::basicTaDMask) ^ (1U << 2);
                // Start at step 1 so we don't remove a signal on the first tile
                auto cost = AutoRemoveSignalsBackwards(reverseStart, reverseTad, args.trackObjType, args.flags, args.step, 1, flags);
                if (cost == GameCommands::kFailure)
                {
                    return GameCommands::kFailure;
                }
                totalCost += cost;
            }
        }
        return totalCost;
    }

    void removeSignalsAuto(registers& regs, const uint8_t flags)
    {
        regs.ebx = removeSignalsAuto(SignalsRemovalAutoArgs(regs), flags);
    }
}
