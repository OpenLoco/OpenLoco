#pragma once

namespace OpenLoco::Map
{
    struct TrackElement;
}
namespace OpenLoco::Paint
{
    struct PaintSession;

    void paintTrack(PaintSession& session, const Map::TrackElement& elTrack);

    void registerTrackHooks();
}
