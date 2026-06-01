#pragma once

namespace OpenLoco::World
{
    struct TileElementEntry;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintSignal(PaintSession& session, const World::TileElementEntry& entry);
}
