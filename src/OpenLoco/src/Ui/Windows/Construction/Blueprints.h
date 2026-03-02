#pragma once
#include <vector>

namespace OpenLoco
{
    namespace GameCommands
    {

        struct TrackPlacementArgs;
        struct SignalPlacementArgs;
        struct TrainStationPlacementArgs;
    }

    namespace Ui::Windows::Construction::Construction
    {
        struct CopiedTrack
        {
            std::vector<GameCommands::TrackPlacementArgs> trackArgs;
            std::vector<GameCommands::SignalPlacementArgs> signalArgs;
            std::vector<GameCommands::TrainStationPlacementArgs> stationArgs;
        };
    }
}
