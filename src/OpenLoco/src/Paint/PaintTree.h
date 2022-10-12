#pragma once

namespace OpenLoco::Map
{
    struct TreeElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintTree(PaintSession& session, const Map::TreeElement& elTree);
}
