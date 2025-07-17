#include "Entities/EntityManager.h"
#include "Map/AnimationManager.h"
#include "Map/RoadElement.h"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "RoutingManager.h"
#include "Vehicle.h"
#include "ViewportManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;
using namespace OpenLoco::World;

namespace OpenLoco::Vehicles
{
    static loco_global<int32_t, 0x0113612C> _vehicleUpdate_var_113612C; // Speed
    static loco_global<uint32_t, 0x01136114> _vehicleUpdate_var_1136114;

    // 0x004794BC
    void leaveLevelCrossing(const World::Pos3& loc, const TrackAndDirection::_TrackAndDirection trackAndDirection, const uint16_t unk)
    {
        auto levelCrossingLoc = loc;
        if (trackAndDirection.isReversed())
        {
            auto& trackSize = World::TrackData::getUnkTrack(trackAndDirection._data);
            levelCrossingLoc += trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                levelCrossingLoc -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }
        }

        auto& trackPiece = World::TrackData::getTrackPiece(trackAndDirection.id());
        levelCrossingLoc += World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece[0].x, trackPiece[0].y }, trackAndDirection.cardinalDirection()), 0 };
        levelCrossingLoc.z += trackPiece[0].z;
        auto tile = World::TileManager::get(levelCrossingLoc);
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

            World::AnimationManager::createAnimation(1, levelCrossingLoc, levelCrossingLoc.z / 4);
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
        const World::Pos3 _oldTilePos = World::Pos3(tileX, tileY, tileBaseZ * World::kSmallZStep);

        _vehicleUpdate_var_1136114 = 0;
        updateTrackMotion(*_vehicleUpdate_var_113612C);

        if (*_vehicleUpdate_var_1136114 & (1 << 1))
        {
            destroyTrain();
            return false;
        }

        if (_oldRoutingHandle == routingHandle)
        {
            return true;
        }

        const auto ref = RoutingManager::getRouting(_oldRoutingHandle);
        TrackAndDirection trackAndDir((ref & 0x1F8) >> 3, ref & 0x7);
        RoutingManager::freeRouting(_oldRoutingHandle);

        if (mode == TransportMode::road)
        {
            sub_47D959(_oldTilePos, trackAndDir.road, false);
        }
        else
        {
            if (ref & (1 << 15))
            {
                setSignalState(_oldTilePos, trackAndDir.track, trackType, 0);
            }

            const auto& trackSize = World::TrackData::getUnkTrack(ref & 0x1FF);
            auto nextTile = _oldTilePos + trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                nextTile -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
            }
            auto trackAndDirection2 = trackAndDir;
            trackAndDirection2.track.setReversed(!trackAndDirection2.track.isReversed());
            sub_4A2AD7(nextTile, trackAndDirection2.track, owner, trackType);
            leaveLevelCrossing(_oldTilePos, trackAndDir.track, 9);
        }
        return true;
    }

    // 0x004B090F
    void liftUpTail(VehicleTail& tail)
    {
        if (tail.mode == TransportMode::road)
        {
            const auto tailPos = World::Pos3(tail.tileX, tail.tileY, tail.tileBaseZ * World::kSmallZStep);

            auto pos = tailPos;

            RoutingManager::RingView ring(tail.routingHandle);
            for (const auto& handle : ring)
            {
                const auto routing = RoutingManager::getRouting(handle);
                auto tad = TrackAndDirection::_RoadAndDirection(0, 0);
                tad._data = routing & Track::AdditionalTaDFlags::basicTaDMask;
                tail.sub_47D959(pos, tad, false);

                pos += World::TrackData::getUnkRoad(tad._data).pos;
            }
        }
        else
        {
            const auto tailPos = World::Pos3(tail.tileX, tail.tileY, tail.tileBaseZ * World::kSmallZStep);
            {
                TrackAndDirection::_TrackAndDirection tad{ 0, 0 };
                tad._data = RoutingManager::getRouting(tail.routingHandle) & Track::AdditionalTaDFlags::basicTaDMask;
                auto& trackSize = World::TrackData::getUnkTrack(tad._data);
                auto pos = tailPos + trackSize.pos;
                if (trackSize.rotationEnd < 12)
                {
                    pos -= World::Pos3{ World::kRotationOffset[trackSize.rotationEnd], 0 };
                }
                tad.setReversed(!tad.isReversed());
                sub_4A2AD7(pos, tad, tail.owner, tail.trackType);
            }

            auto pos = tailPos;

            RoutingManager::RingView ring(tail.routingHandle);
            for (const auto& handle : ring)
            {
                const auto routing = RoutingManager::getRouting(handle);
                auto tad = TrackAndDirection::_TrackAndDirection(0, 0);
                tad._data = routing & Track::AdditionalTaDFlags::basicTaDMask;
                sub_4A2AD7(pos, tad, tail.owner, tail.trackType);

                if (routing & Track::AdditionalTaDFlags::hasSignal)
                {
                    setSignalState(pos, tad, tail.trackType, 0);
                }

                leaveLevelCrossing(pos, tad, 9);

                pos += World::TrackData::getUnkTrack(tad._data).pos;
            }
        }
    }
}
