#pragma once

namespace OpenLoco::World
{
    struct TileElementEntry;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintTrainStation(PaintSession& session, const World::TileElementEntry& entry);
}
