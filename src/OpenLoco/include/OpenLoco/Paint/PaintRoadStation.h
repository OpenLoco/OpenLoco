#pragma once

namespace OpenLoco::World
{
    struct StationElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintRoadStation(PaintSession& session, const World::StationElement& elStation);
}
