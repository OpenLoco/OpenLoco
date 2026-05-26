#pragma once
#include <OpenLoco/Engine/World.hpp>

namespace OpenLoco::Paint
{
    struct PaintSession;
    void paintTileElements(PaintSession& session, const World::Pos2& loc);
    void paintTileElements2(PaintSession& session, const World::Pos2& loc);
}
