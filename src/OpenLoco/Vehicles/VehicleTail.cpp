#include "../Entities/EntityManager.h"
#include "../Map/AnimationManager.h"
#include "../Map/TileManager.h"
#include "../Map/Track/Track.h"
#include "../Map/Track/TrackData.h"
#include "../ViewportManager.h"
#include "Vehicle.h"
#include "VehicleManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;
using namespace OpenLoco::Map;

namespace OpenLoco::Vehicles
{
    static loco_global<int32_t, 0x0113612C> vehicleUpdate_var_113612C; // Speed
    static loco_global<uint32_t, 0x01136114> vehicleUpdate_var_1136114;

    static loco_global<Map::Pos2[16], 0x00503C6C> _503C6C;

    // 0x004794BC
    static void leaveLevelCrossing(const Map::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const uint16_t unk)
    {
        auto levelCrossingLoc = loc;
        if (trackAndDirection.isReversed())
        {
            auto& trackSize = Map::TrackData::getUnkTrack(trackAndDirection._data);
            levelCrossingLoc += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                levelCrossingLoc -= Map::Pos3{ _503C6C[trackSize.rotationEnd], 0 };
            }
        }

        auto& trackPiece = Map::TrackData::getTrackPiece(trackAndDirection.id());
        levelCrossingLoc += Map::Pos3{ Math::Vector::rotate(Map::Pos2{ trackPiece[0].x, trackPiece[0].y }, trackAndDirection.cardinalDirection()), 0 };
        levelCrossingLoc.z += trackPiece[0].z;
        auto tile = Map::TileManager::get(levelCrossingLoc);
        for (auto& el : tile)
        {
            if (el.baseZ() != levelCrossingLoc.z / 4)
            {
                continue;
            }

            auto* road = el.as<RoadElement>();
            if (road == nullptr)
            {
                continue;
            }

            if (road->roadId() != 0)
            {
                continue;
            }

            road->setUnk7_10(false);
            if (unk != 8)
            {
                continue;
            }

            Map::AnimationManager::createAnimation(1, levelCrossingLoc, levelCrossingLoc.z / 4);
        }

        Ui::ViewportManager::invalidate(levelCrossingLoc, levelCrossingLoc.z, levelCrossingLoc.z + 32, ZoomLevel::full);
    }

    // 0x004AA24A
    bool VehicleTail::update()
    {
        if (mode == TransportMode::air || mode == TransportMode::water)
        {
            return true;
        }

        const auto _oldRoutingHandle = routingHandle;
        const Map::Pos3 _oldTilePos = Map::Pos3(tileX, tileY, tileBaseZ * 4);

        vehicleUpdate_var_1136114 = 0;
        sub_4B15FF(*vehicleUpdate_var_113612C);

        if (*vehicleUpdate_var_1136114 & (1 << 1))
        {
            sub_4AA464();
            return false;
        }

        if (_oldRoutingHandle == routingHandle)
        {
            return true;
        }

        const auto ref = RoutingManager::getRouting(_oldRoutingHandle);
        TrackAndDirection trackAndDirection((ref & 0x1F8) >> 3, ref & 0x7);
        RoutingManager::freeRouting(_oldRoutingHandle);

        if (mode == TransportMode::road)
        {
            sub_47D959(_oldTilePos, trackAndDirection.road, false);
        }
        else
        {
            if (ref & (1 << 15))
            {
                setSignalState(_oldTilePos, trackAndDirection.track, trackType, 0);
            }

            const auto& trackSize = Map::TrackData::getUnkTrack(ref & 0x1FF);
            auto nextTile = _oldTilePos + trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                nextTile -= Map::Pos3{ _503C6C[trackSize.rotationEnd], 0 };
            }
            auto trackAndDirection2 = trackAndDirection;
            trackAndDirection2.track.setReversed(!trackAndDirection2.track.isReversed());
            sub_4A2AD7(nextTile, trackAndDirection2.track, owner, trackType);
            leaveLevelCrossing(_oldTilePos, trackAndDirection.track, 9);
        }
        return true;
    }
}
