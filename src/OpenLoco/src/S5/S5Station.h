#include "S5LabelFrame.h"
#include <OpenLoco/Engine/World.hpp>
#include <cstdint>

namespace OpenLoco
{
    struct Station;
}

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct StationCargoStats
    {
        uint16_t quantity;      // 0x2E
        uint16_t origin;        // 0x30
        uint8_t flags;          // 0x32
        uint8_t age;            // 0x33
        uint8_t rating;         // 0x34
        uint8_t enrouteAge;     // 0x35
        int16_t vehicleSpeed;   // 0x36 max speed of vehicle that transported the cargo
        uint8_t vehicleAge;     // 0x38 age of the vehicle (car) that transported the cargo
        uint8_t industryId;     // 0x39
        uint8_t densityPerTile; // 0x3A amount of cargo visible per tile of station
    };
    static_assert(sizeof(StationCargoStats) == 0xD);

    struct Station
    {
        uint16_t name;                    // 0x00
        coord_t x;                        // 0x02
        coord_t y;                        // 0x04
        coord_t z;                        // 0x06
        LabelFrame labelFrame;            // 0x08
        uint8_t owner;                    // 0x28
        uint8_t noTilesTimeout;           // 0x29 measured in days
        uint16_t flags;                   // 0x2A
        uint16_t town;                    // 0x2C
        StationCargoStats cargoStats[32]; // 0x2E
        uint16_t stationTileSize;         // 0x1CE
        World::Pos3 stationTiles[80];     // 0x1D0 Note: z coordinate also contains rotation so always floor
        uint8_t var_3B0;
        uint8_t var_3B1;
        uint8_t var_3B2;
        uint8_t airportRotation;               // 0x3B3
        World::Pos3 airportStartPos;           // 0x3B4
        uint32_t airportMovementOccupiedEdges; // 0x3BA
        uint8_t pad_3BE[0x3D2 - 0x3BE];
    };
    static_assert(sizeof(Station) == 0x3D2);
#pragma pack(pop)

    S5::Station exportStation(const OpenLoco::Station& src);
    OpenLoco::Station importStation(const S5::Station& src);
}
