#pragma once

namespace OpenLoco::World
{
    struct StationElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintStation(PaintSession& session, const World::StationElement& elStation);
}
