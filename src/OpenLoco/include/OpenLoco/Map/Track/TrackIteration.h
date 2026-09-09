#include "OpenLoco/Map/Tile.h"
#include "Track.h"
#include "TrackData.h"

namespace OpenLoco::World::Track
{
    template<typename Function>
    static void iterateTrackToJunction(const World::Pos3& trackStart, uint16_t tad, const uint8_t trackObjType, const CompanyId companyId, Function&& filter)
    {
        auto pos = World::Pos3(trackStart);

        while (true)
        {
            auto connectionTrackStart = pos;
            if (tad & (1U << 2))
            {
                auto& trackSize = World::TrackData::getUnkTrack(tad);
                connectionTrackStart += trackSize.pos;
                if (trackSize.rotationEnd < 12)
                {
                    connectionTrackStart -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
                }
            }

            if (!filter(connectionTrackStart, tad))
            {
                break;
            }

            const auto [trackEndLoc, trackEndRotation] = World::Track::getTrackConnectionEnd(pos, tad);
            auto tc = World::Track::getTrackConnections(trackEndLoc, trackEndRotation, companyId, trackObjType, 0, 0);

            // If there is a junction or no connection we stop
            if (tc.connections.size() != 1)
            {
                break;
            }

            // Now we move to the next track piece
            pos = trackEndLoc;
            tad = tc.connections[0] & World::Track::AdditionalTaDFlags::basicTaDMask;

            // If we have looped back to the start we stop
            if (pos == trackStart)
            {
                break;
            }
        }
    }
}
