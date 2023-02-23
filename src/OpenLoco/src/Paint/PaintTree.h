#pragma once

namespace OpenLoco::World
{
    struct TreeElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintTree(PaintSession& session, const World::TreeElement& elTree);
}
