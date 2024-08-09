#pragma once

namespace OpenLoco::World
{
    struct RoadElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintRoad(PaintSession& session, const World::RoadElement& elRoad);
}
