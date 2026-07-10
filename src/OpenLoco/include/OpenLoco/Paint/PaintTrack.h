#pragma once

namespace OpenLoco::World
{
    struct TrackElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintTrack(PaintSession& session, const World::TrackElement& elTrack);
}
