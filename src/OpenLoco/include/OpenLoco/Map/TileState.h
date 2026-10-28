#pragma once

#include "BuildingElement.h"
#include "IndustryElement.h"
#include "RoadElement.h"
#include "SignalElement.h"
#include "StationElement.h"
#include "SurfaceElement.h"
#include "TileElementEntry.h"
#include "TrackElement.h"
#include "TreeElement.h"
#include "WallElement.h"
#include <OpenLoco/Core/Store.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <array>
#include <cstdint>
#include <vector>

namespace OpenLoco::World
{
    struct TileState
    {
        Store<SurfaceElement> surface;
        Store<TrackElement> track;
        Store<StationElement> station;
        Store<SignalElement> signal;
        Store<BuildingElement> building;
        Store<TreeElement> tree;
        Store<WallElement> wall;
        Store<RoadElement> road;
        Store<IndustryElement> industry;

        // The number of tiles depends on the current map dimensions, so this is resized when the map size changes.
        std::vector<TileElementEntry*> tiles{};
        std::vector<TileElementEntry> entries;
        std::ptrdiff_t entriesEnd = 0;
    };
}
