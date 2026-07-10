#pragma once

namespace OpenLoco::World
{
    struct StationElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintAirport(PaintSession& session, const World::StationElement& elStation);
}
