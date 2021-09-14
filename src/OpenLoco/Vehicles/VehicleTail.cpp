#include "../TrackData.h"
#include "Vehicle.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

namespace OpenLoco::Vehicles
{
    constexpr auto max_num_vehicles = 1000;
    constexpr auto max_num_routing_steps = 64;
    constexpr uint16_t allocated_but_free_routing_station = -2;        // Indicates that this array is allocated to a vehicle but no station has been set.
    constexpr uint16_t routingNull = -1;                               // Indicates that this array is allocated to a vehicle but no station has been set.
    static loco_global<int32_t, 0x0113612C> vehicleUpdate_var_113612C; // Speed
    static loco_global<uint32_t, 0x01136114> vehicleUpdate_var_1136114;
    static loco_global<uint16_t[max_num_vehicles][max_num_routing_steps], 0x0096885C> _96885C; // Likely routing related
    static loco_global<Map::Pos2[16], 0x00503C6C> _503C6C;

    // 0x0048963F
    static uint8_t sub_48963F(const Map::Pos3& loc, const TrackAndDirection trackAndDirection, const CompanyId_t company, const uint8_t trackType, const uint32_t flags)
    {
        registers regs;
        regs.ax = loc.x;
        regs.cx = loc.y;
        regs.dx = loc.z;
        regs.bl = company;
        regs.bh = trackType;
        regs.ebp = trackAndDirection.track._data;
        regs.edi = flags;
        call(0x0048963F, regs);
        return regs.al;
    }

    // 0x004A2AD7
    static void sub_4A2AD7(const Map::Pos3& loc, const TrackAndDirection trackAndDirection, const CompanyId_t company, const uint8_t trackType)
    {
        addr<0x001135F88, uint16_t>() = 0;
        registers regs;
        regs.ax = loc.x;
        regs.cx = loc.y;
        regs.dx = loc.z;
        regs.bl = company;
        regs.bh = trackType;
        regs.ebp = trackAndDirection.track._data;
        regs.esi = 0x004A2AF0;
        regs.edi = 0x004A2CE7;
        call(0x004A2E46, regs);
    }

    // 0x004794BC
    static uint8_t sub_4794BC(const Map::Pos3& loc, const TrackAndDirection trackAndDirection, const CompanyId_t company, const uint8_t trackType, const uint16_t unk)
    {
        registers regs;
        regs.ax = loc.x;
        regs.cx = loc.y;
        regs.dx = loc.z;
        regs.bl = company;
        regs.bh = trackType;
        regs.ebp = trackAndDirection.track._data;
        regs.di = unk;
        call(0x004794BC, regs);
        return regs.al;
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

        const auto ref = _96885C[_oldRoutingHandle.getVehicleRef()][_oldRoutingHandle.getIndex()];
        TrackAndDirection trackAndDirection((ref & 0x1F8) >> 3, ref & 0x7);
        const auto vehId = _oldRoutingHandle.getVehicleRef();
        const auto routeId = _oldRoutingHandle.getIndex();
        _96885C[vehId][routeId] = allocated_but_free_routing_station;

        if (mode == TransportMode::road)
        {
            sub_47D959(_oldTilePos, trackAndDirection.road);
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
                nextTile -= _503C6C[trackSize.rotationEnd];
            }
            auto trackAndDirection2 = trackAndDirection;
            trackAndDirection2.track.setReversed(!trackAndDirection2.track.isReversed());
            sub_4A2AD7(nextTile, trackAndDirection2, owner, track_type);
            // Update level crossing
            sub_4794BC(_oldTilePos, trackAndDirection, owner, track_type, 9);
        }
        return true;
    }
}
