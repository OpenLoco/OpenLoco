#include "GameCommands/Track/CreateSignalsAuto.h"
#include "GameCommands/Track/CreateSignal.h"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Map/Track/TrackIteration.h"
#include "Map/TrackElement.h"

namespace OpenLoco::GameCommands
{
    static World::TrackElement* getElTrackAt(const World::Pos3 pos, const uint8_t rotation, const uint8_t index, const uint8_t trackObjType, const uint8_t trackId)
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
            return elTrack;
        }
        return nullptr;
    };

    static uint32_t placeSignal(const World::Pos3& pos, const uint16_t tad, const uint16_t sides, const uint8_t trackObjType, const uint8_t signalType, const uint8_t flags)
    {
        GameCommands::SignalPlacementArgs sargs{};
        sargs.pos = pos;
        sargs.rotation = tad & 0x3;
        sargs.trackId = (tad >> 3) & 0x3F;
        sargs.index = 0;
        sargs.sides = sides;
        sargs.trackObjType = trackObjType;
        sargs.type = signalType;

        return GameCommands::doCommand(sargs, flags);
    }

    static uint32_t AutoPlaceSignals(const World::Pos3& trackStart, uint16_t tad, const uint8_t trackObjType, const uint8_t signalType, const uint16_t sides, const uint8_t step, const uint8_t initialStep, const uint8_t flags)
    {
        int32_t currentStep = initialStep;
        if (currentStep >= step)
        {
            currentStep = 0;
        }
        uint32_t totalCost = 0;

        World::Track::iterateTrackToJunction(
            trackStart,
            tad,
            trackObjType,
            GameCommands::getUpdatingCompanyId(),
            [trackObjType, step, sides, signalType, flags, &currentStep, &totalCost](const World::Pos3& pos, uint16_t tad) {
                auto iterationTrackStart = pos;
                const auto rotation = tad & 0x3;
                const auto trackId = (tad >> 3) & 0x3F;
                auto& trackPiece0 = World::TrackData::getTrackPiece(trackId)[0];
                iterationTrackStart += World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece0.x, trackPiece0.y }, rotation), trackPiece0.z };

                auto elTrack = getElTrackAt(iterationTrackStart, rotation, 0, trackObjType, trackId);
                if (elTrack == nullptr)
                {
                    totalCost = GameCommands::kFailure;
                    return false;
                }
                // If we have a signal or station stop the auto placement
                if (elTrack->hasSignal() || elTrack->hasStationElement())
                {
                    return false;
                }

                if (World::Track::validateTrackIsSignalCompatible(pos, rotation, trackId, trackObjType).has_value())
                {
                    return false;
                }

                // If we have a level crossing we skip this tile for placement
                if (currentStep == 0 && !elTrack->hasLevelCrossing())
                {
                    // If we have a reverse connection then we need to reverse the sides!
                    // Except when they are double sided signals.
                    auto adjustedSides = sides;
                    if ((tad & (1U << 2)) && sides != ((1U << 15) | (1U << 14)))
                    {
                        adjustedSides ^= (1U << 15) | (1U << 14);
                    }

                    auto cost = placeSignal(iterationTrackStart, tad, adjustedSides, trackObjType, signalType, flags);

                    if (cost == GameCommands::kFailure)
                    {
                        totalCost = GameCommands::kFailure;
                        return false;
                    }
                    totalCost += cost;
                }
                currentStep++;
                if (currentStep >= step)
                {
                    currentStep = 0;
                }
                return true;
            });
        return totalCost;
    }

    static uint32_t createSignalsAuto(const SignalsPlacementAutoArgs& args, const uint8_t flags)
    {
        const auto trackPieces = World::TrackData::getTrackPiece(args.trackId);
        if (trackPieces.size() < args.index)
        {
            return GameCommands::kFailure;
        }

        auto& trackPiece = trackPieces[args.index];
        const auto trackStart = args.pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, args.rotation), trackPiece.z };

        uint32_t totalCost = 0;

        // Perform a forward walk along the track placing signals every arg.step tiles
        {
            const auto startTad = args.rotation | (args.trackId << 3);
            auto cost = AutoPlaceSignals(trackStart, startTad, args.trackObjType, args.type, args.sides, args.step, 0, flags);
            if (cost == GameCommands::kFailure)
            {
                return GameCommands::kFailure;
            }
            totalCost += cost;
        }

        // Perform a backward walk along the track placing signals every arg.step tiles
        {
            auto reverseStart = trackStart;
            auto& trackSize = World::TrackData::getUnkTrack((args.trackId << 3) | args.rotation);
            if (trackSize.rotationBegin < 12)
            {
                reverseStart -= World::Pos3{ World::kRotationOffset[trackSize.rotationBegin], 0 };
            }

            const auto reverseStartRotation = World::kReverseRotation[trackSize.rotationBegin];
            auto tc = World::Track::getTrackConnections(reverseStart, reverseStartRotation, GameCommands::getUpdatingCompanyId(), args.trackObjType, 0, 0);
            if (tc.connections.size() == 1)
            {
                const auto reverseTad = tc.connections[0] & World::Track::AdditionalTaDFlags::basicTaDMask;

                // As we are walking backwards we need to reverse the sides we are placing signals on, except when they are double sided signals.
                auto reverseSides = args.sides;
                if (args.sides != ((1U << 15) | (1U << 14)))
                {
                    reverseSides ^= (1U << 15) | (1U << 14);
                }
                // Start at step 1 so we don't place a signal on the first tile
                auto cost = AutoPlaceSignals(reverseStart, reverseTad, args.trackObjType, args.type, reverseSides, args.step, 1, flags);
                if (cost == GameCommands::kFailure)
                {
                    return GameCommands::kFailure;
                }
                totalCost += cost;
            }
        }
        return totalCost;
    }

    void createSignalsAuto(registers& regs, const uint8_t flags)
    {
        regs.ebx = createSignalsAuto(SignalsPlacementAutoArgs(regs), flags);
    }
}
