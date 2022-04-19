#pragma once

namespace OpenLoco::Map
{
    struct StationElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintStation(PaintSession& session, const Map::StationElement& elStation);
}
