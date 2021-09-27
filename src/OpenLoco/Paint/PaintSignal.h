#pragma once

namespace OpenLoco::Map
{
    struct SignalElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintSignal(PaintSession& session, const Map::SignalElement& elSignal);
}
