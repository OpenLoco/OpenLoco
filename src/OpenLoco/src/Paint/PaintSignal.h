#pragma once

namespace OpenLoco::World
{
    struct SignalElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintSignal(PaintSession& session, const World::SignalElement& elSignal);
}
