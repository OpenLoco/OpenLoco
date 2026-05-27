#pragma once
#include "Types.hpp"
#include <OpenLoco/Engine/World.hpp>

namespace OpenLoco::Paint
{
    struct PaintSession;
    void paintEntities(PaintSession& session, const World::Pos2& loc);
    void paintEntities2(PaintSession& session, const World::Pos2& loc);
}
