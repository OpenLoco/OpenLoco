#pragma once

#include "Localisation/StringManager.h"
#include "Map/Tile.h"
#include "Town.h"
#include "Types.hpp"
#include "Utility/Numeric.hpp"
#include <cstdint>
#include <limits>

namespace OpenLoco
{
    using namespace OpenLoco::Map;

    namespace StationId
    {
        constexpr station_id_t null = std::numeric_limits<station_id_t>::max();
    }

#pragma pack(push, 1)
    struct station_cargo_stats
    {
        uint16_t quantity;   // 0x2E
        station_id_t origin; // 0x30
        uint8_t flags;       // 0x32
        uint8_t age;         // 0x33
        uint8_t rating;      // 0x34
        uint8_t enroute_age; // 0x35
        uint16_t var_36;     // 0x36
        uint8_t var_38;
        industry_id_t industry_id; // 0x39
        uint8_t pad_40;

        bool empty() const
        {
            return origin == StationId::null;
        }

        bool isAccepted() const
        {
            return flags & 1;
        }

        void isAccepted(bool value)
        {
            flags = Utility::setMask<uint8_t>(flags, 1, value);
        }
    };

    constexpr size_t max_cargo_stats = 32;

    enum stationType
    {
        trainStation = 0,
        roadStation,
        airport,
        docks,
    };

    enum station_flags : uint16_t
    {
        transport_mode_rail = (1 << 0),
        transport_mode_road = (1 << 1),
        transport_mode_air = (1 << 2),
        transport_mode_water = (1 << 3),
        flag_4 = (1 << 4),
        flag_5 = (1 << 5),
        flag_6 = (1 << 6),
        flag_7 = (1 << 7),
        flag_8 = (1 << 8),
    };

    constexpr uint16_t station_mask_all_modes = station_flags::transport_mode_rail | station_flags::transport_mode_road | station_flags::transport_mode_air | station_flags::transport_mode_water;

    struct CargoSearchState;

    struct station
    {
        string_id name; // 0x00
        coord_t x;      // 0x02
        coord_t y;      // 0x04
        coord_t z;      // 0x06
        int16_t label_left[4];
        int16_t label_right[4];
        int16_t label_top[4];
        int16_t label_bottom[4];
        company_id_t owner; // 0x28
        uint8_t var_29;
        uint16_t flags;
        town_id_t town;                                   // 0x2C
        station_cargo_stats cargo_stats[max_cargo_stats]; // 0x2E
        uint16_t stationTileSize;                         // 0x1CE
        map_pos3 stationTiles[80];                        // 0x1D0
        uint8_t var_3B0;
        uint8_t var_3B1;
        uint8_t pad_3B2[0x3D2 - 0x3B2];

        bool empty() const { return name == StringIds::null; }
        station_id_t id() const;
        void update();
        uint32_t calcAcceptedCargo(CargoSearchState& cargoSearchState, const map_pos& location = { -1, -1 }, const uint32_t filter = 0);
        void sub_48F7D1();
        void getStatusString(const char* buffer);
        bool updateCargo();
        int32_t calculateCargoRating(const station_cargo_stats& cargo) const;
        void invalidate();
        void invalidateWindow();
        void setCatchmentDisplay(uint8_t flags);

    private:
        void updateCargoAcceptance();
        void alertCargoAcceptanceChange(uint32_t oldCargoAcc, uint32_t newCargoAcc);
        void sub_4929DB();
    };
#pragma pack(pop)
}
