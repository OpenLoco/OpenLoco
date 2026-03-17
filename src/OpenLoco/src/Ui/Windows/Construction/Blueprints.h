#pragma once
#include "Economy/Currency.h"
#include <OpenLoco/Engine/World.hpp>
#include <vector>

namespace OpenLoco
{
    namespace GameCommands
    {
        struct TrackPlacementArgs;
        struct SignalPlacementArgs;
        struct TrainStationPlacementArgs;
    }

    struct CopiedTrack
    {
        std::vector<GameCommands::TrackPlacementArgs> trackArgs;
        std::vector<GameCommands::SignalPlacementArgs> signalArgs;
        std::vector<GameCommands::TrainStationPlacementArgs> stationArgs;
    };

    currency32_t removeBlueprint(const CopiedTrack& copiedTrack, const World::Pos3& ghostBPPos, const uint8_t flags);
    currency32_t placeBlueprint(const CopiedTrack& _copiedTrack, const World::Pos3& ghostBPPos, const uint8_t flags);
    CopiedTrack copyTrackToBlueprint(const World::TilePos2 posA, const World::TilePos2 posB);
}
