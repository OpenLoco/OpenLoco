#pragma once

namespace OpenLoco::World
{
    struct StationElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintDocks(PaintSession& session, const World::StationElement& elStation);
}
