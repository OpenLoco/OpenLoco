#pragma once
#include "../Map/Map.hpp"

namespace OpenLoco::Paint
{
    struct PaintSession;
    void paintTileElements(PaintSession& session, const Map::Pos2& loc);
    void paintTileElements2(PaintSession& session, const Map::Pos2& loc);
}
