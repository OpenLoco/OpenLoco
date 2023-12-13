#pragma once

namespace OpenLoco::World
{
    struct WallElement;
}

namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintWall(PaintSession& session, World::WallElement& elWall);
}
