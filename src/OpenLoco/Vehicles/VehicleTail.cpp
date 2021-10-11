#include "../Map/AnimationManager.h"
#include "../Map/TileManager.h"
#include "../Map/Track/TrackData.h"
#include "../ViewportManager.h"
#include "Vehicle.h"
#include "VehicleManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
    static loco_global<int32_t, 0x0113612C> vehicleUpdate_var_113612C; // Speed
    static loco_global<uint32_t, 0x01136114> vehicleUpdate_var_1136114;
    static loco_global<Map::Pos2[16], 0x00503C6C> _503C6C;

    // 0x0048963F
    static uint8_t sub_48963F(const Map::Pos3& loc, const TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType, const uint32_t flags)
    {
        registers regs;
        regs.ax = loc.x;
        regs.cx = loc.y;
        regs.dx = loc.z;
        regs.bl = enumValue(company);
        regs.bh = trackType;
        regs.ebp = trackAndDirection.track._data;
        regs.edi = flags;
        call(0x0048963F, regs);
        return regs.al;
    }

    // 0x004A2AD7
    static void sub_4A2AD7(const Map::Pos3& loc, const TrackAndDirection trackAndDirection, const CompanyId company, const uint8_t trackType)
    {
        addr<0x001135F88, uint16_t>() = 0;
        registers regs;
        regs.ax = loc.x;
        regs.cx = loc.y;
        regs.dx = loc.z;
        regs.bl = enumValue(company);
        regs.bh = trackType;
        regs.ebp = trackAndDirection.track._data;
        regs.esi = 0x004A2AF0;
        regs.edi = 0x004A2CE7;
        call(0x004A2E46, regs);
    }

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
                levelCrossingLoc -= Map::Pos3{ _503C6C[trackSize.rotationEnd] };
            }
        }

        auto& trackPiece = Map::TrackData::getTrackPiece(trackAndDirection.id());
        levelCrossingLoc += Map::Pos3{ Math::Vector::rotate(Map::Pos2{ trackPiece[0].x, trackPiece[0].y }, trackAndDirection.cardinalDirection()) };
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
        const Map::Pos3 _oldTilePos = Map::Pos3(tile_x, tile_y, tile_base_z * 4);

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
                // Update signal state?
                sub_48963F(_oldTilePos, trackAndDirection, owner, track_type, 0);
            }

            const auto& trackSize = Map::TrackData::getUnkTrack(ref & 0x1FF);
            auto nextTile = _oldTilePos + trackSize.pos;
            if (trackSize.rotationEnd < 12)
            {
                nextTile -= Map::Pos3{ _503C6C[trackSize.rotationEnd] };
            }
            auto trackAndDirection2 = trackAndDirection;
            trackAndDirection2.track.setReversed(!trackAndDirection2.track.isReversed());
            sub_4A2AD7(nextTile, trackAndDirection2, owner, track_type);
            leaveLevelCrossing(_oldTilePos, trackAndDirection.track, 9);
        }
        return true;
    }
}
