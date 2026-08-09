#pragma once

#include "GameCommands/GameCommands.h"
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

    struct PlaceResult
    {
        uint32_t cost = 0;
        bool failure = false;
        bool hasPlaced = false;
    };

    template<typename FilterFunction, typename ActionFunction>
    static PlaceResult autoPlaceSignals(const World::Pos3& trackStart, uint16_t tad, const uint8_t trackObjType, const uint16_t sides, const uint8_t step, const uint8_t initialStep, const uint8_t flags, FilterFunction&& filterFunc, ActionFunction&& actionFunc)
    {
        PlaceResult result{};
        int32_t currentStep = initialStep;
        if (currentStep >= step)
        {
            currentStep = 0;
        }

        World::Track::iterateTrackToJunction(
            trackStart,
            tad,
            trackObjType,
            GameCommands::getUpdatingCompanyId(),
            [trackObjType, step, sides, flags, &currentStep, &result, &filterFunc, &actionFunc](const World::Pos3& pos, uint16_t tad) {
                auto iterationTrackStart = pos;
                const auto rotation = tad & 0x3;
                const auto trackId = (tad >> 3) & 0x3F;
                auto& trackPiece0 = World::TrackData::getTrackPiece(trackId)[0];
                iterationTrackStart += World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece0.x, trackPiece0.y }, rotation), trackPiece0.z };

                auto elTrack = getElTrackAt(iterationTrackStart, rotation, 0, trackObjType, trackId);
                if (elTrack == nullptr)
                {
                    result.failure = true;
                    return false;
                }
                // If we have additional filtering
                if (filterFunc(*elTrack))
                {
                    return false;
                }

                if (auto res = World::Track::validateTrackIsSignalCompatible(pos, rotation, trackId, trackObjType); res.has_value())
                {
                    setErrorText(res.value());
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

                    auto cost = actionFunc(iterationTrackStart, tad, adjustedSides, trackObjType, flags);

                    if (cost == GameCommands::kFailure)
                    {
                        result.failure = true;
                        return false;
                    }
                    result.hasPlaced = true;
                    result.cost += cost;
                }
                currentStep++;
                if (currentStep >= step)
                {
                    currentStep = 0;
                }
                return true;
            });
        return result;
    }

    template<typename FilterFunction, typename ActionFunction>
    static uint32_t autoSignalsWalk(const World::Pos3& pos, const uint8_t trackId, const uint8_t rotation, const uint8_t index, const uint8_t trackObjType, const uint16_t sides, const uint8_t step, const uint8_t flags, FilterFunction&& filterFunc, ActionFunction&& actionFunc)
    {
        if (step == 0)
        {
            return actionFunc(pos, (rotation & 0x3) | (trackId << 3), sides, trackObjType, flags);
        }
        const auto trackPieces = World::TrackData::getTrackPiece(trackId);
        if (trackPieces.size() < index)
        {
            return GameCommands::kFailure;
        }

        auto& trackPiece = trackPieces[index];
        const auto trackStart = pos - World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, rotation), trackPiece.z };

        uint32_t totalCost = 0;
        bool hasPlaced = false;

        // Perform a forward walk along the track placing signals every arg.step tiles
        {
            const auto startTad = rotation | (trackId << 3);
            auto result = autoPlaceSignals(trackStart, startTad, trackObjType, sides, step, 0, flags, filterFunc, actionFunc);
            if (result.failure)
            {
                return GameCommands::kFailure;
            }
            totalCost += result.cost;
            hasPlaced |= result.hasPlaced;
        }

        // Perform a backward walk along the track placing signals every arg.step tiles
        {
            auto reverseStart = trackStart;
            auto& trackSize = World::TrackData::getUnkTrack((trackId << 3) | rotation);
            if (trackSize.rotationBegin < 12)
            {
                reverseStart -= World::Pos3{ World::kRotationOffset[trackSize.rotationBegin], 0 };
            }

            const auto reverseStartRotation = World::kReverseRotation[trackSize.rotationBegin];
            auto tc = World::Track::getTrackConnections(reverseStart, reverseStartRotation, GameCommands::getUpdatingCompanyId(), trackObjType, 0, 0);
            if (tc.connections.size() == 1)
            {
                const auto reverseTad = tc.connections[0] & World::Track::AdditionalTaDFlags::basicTaDMask;

                // As we are walking backwards we need to reverse the sides we are placing signals on, except when they are double sided signals.
                auto reverseSides = sides;
                if (sides != ((1U << 15) | (1U << 14)))
                {
                    reverseSides ^= (1U << 15) | (1U << 14);
                }
                // Start at step 1 so we don't place a signal on the first tile
                auto result = autoPlaceSignals(reverseStart, reverseTad, trackObjType, reverseSides, step, 1, flags, filterFunc, actionFunc);
                if (result.failure)
                {
                    return GameCommands::kFailure;
                }
                totalCost += result.cost;
                hasPlaced |= result.hasPlaced;
            }
        }

        // If we haven't performed any signal action then we return failure
        if (!hasPlaced)
        {
            return GameCommands::kFailure;
        }

        // Set the position as the initial tile we placed as that is usually what we want
        // we could also have chosen the middle point of all the tiles we placed on but
        // that would be annoying to calculate and I'm not sure if it would be better.
        setPosition(pos + World::Pos3{ 16, 16, 0 });
        return totalCost;
    }
}
