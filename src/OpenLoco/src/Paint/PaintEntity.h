#pragma once
#include "Map/Map.hpp"
#include "Types.hpp"

namespace OpenLoco::Paint
{
    struct PaintSession;
    void paintEntities(PaintSession& session, const Map::Pos2& loc);
    void paintEntities2(PaintSession& session, const Map::Pos2& loc);
}
