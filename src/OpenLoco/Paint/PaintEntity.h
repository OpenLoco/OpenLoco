#pragma once
#include "../Map/Map.hpp"
#include "../Types.hpp"

namespace OpenLoco::Paint
{
    struct PaintSession;
    void paintEntities(PaintSession& session, const Map::map_pos& loc);
    void paintEntities2(PaintSession& session, const Map::map_pos& loc);
}
