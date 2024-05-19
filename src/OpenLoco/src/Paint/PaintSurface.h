#pragma once

namespace OpenLoco::World
{
    struct SurfaceElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintSurface(PaintSession& session, World::SurfaceElement& elSurface);
}
