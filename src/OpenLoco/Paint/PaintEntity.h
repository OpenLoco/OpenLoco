#pragma once
#include "../Types.hpp"

namespace OpenLoco::Map
{
    struct map_pos;
}

namespace OpenLoco::Paint
{
    struct PaintSession;
    void paintEntities(PaintSession& session, const Map::map_pos& loc);
    void paintEntities2(PaintSession& session, const Map::map_pos& loc);
}
