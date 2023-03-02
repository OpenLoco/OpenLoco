#pragma once

#include "LabelFrame.h"
#include "Localisation/StringManager.h"
#include "Map/Tile.h"
#include "Speed.hpp"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Utility/Numeric.hpp>
#include <cstdint>
#include <limits>

namespace OpenLoco
{
#pragma pack(push, 1)
    enum class StationCargoStatsFlags : uint8_t
    {
        none = 0U,
        flag0 = (1U << 0),
        flag1 = (1U << 1),
        flag2 = (1U << 2),
        flag3 = (1U << 3),
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(StationCargoStatsFlags);

    struct StationCargoStats
    {
        uint16_t quantity{};                // 0x2E
        StationId origin = StationId::null; // 0x30
        StationCargoStatsFlags flags{};     // 0x32
        uint8_t age{};                      // 0x33
        uint8_t rating{};                   // 0x34
        uint8_t enrouteAge{};               // 0x35
        Speed16 vehicleSpeed{ 0 };          // 0x36 max speed of vehicle that transported the cargo
        uint8_t vehicleAge{};               // 0x38 age of the vehicle (car) that transported the cargo
        IndustryId industryId{};            // 0x39
        uint8_t densityPerTile{};           // 0x40 amount of cargo visible per tile of station

        bool empty() const
        {
            return origin == StationId::null;
        }

        bool isAccepted() const
        {
            return (flags & StationCargoStatsFlags::flag0) != StationCargoStatsFlags::none;
        }

        void isAccepted(bool value)
        {
            flags = Utility::setMask<StationCargoStatsFlags>(flags, StationCargoStatsFlags::flag0, value);
        }
    };

    constexpr size_t kMaxCargoStats = 32;

    enum class StationType : uint8_t
    {
        trainStation = 0,
        roadStation,
        airport,
        docks,
    };

    enum class StationFlags : uint16_t
    {
        none = 0U,
        transportModeRail = (1U << 0),
        transportModeRoad = (1U << 1),
        transportModeAir = (1U << 2),
        transportModeWater = (1U << 3),
        flag_4 = (1U << 4),
        flag_5 = (1U << 5),
        flag_6 = (1U << 6),
        flag_7 = (1U << 7),
        flag_8 = (1U << 8),
        allModes = StationFlags::transportModeRail | StationFlags::transportModeRoad | StationFlags::transportModeAir | StationFlags::transportModeWater,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(StationFlags);

    string_id getTransportIconsFromStationFlags(const StationFlags flags);

    struct CargoSearchState;

    enum class CatchmentFlags : uint8_t
    {
        flag_0 = 0U,
        flag_1 = 1U,
    };

    struct Station
    {
        string_id name = StringIds::null; // 0x00
        coord_t x{};                      // 0x02
        coord_t y{};                      // 0x04
        coord_t z{};                      // 0x06
        LabelFrame labelFrame;            // 0x08
        CompanyId owner{};                // 0x28
        uint8_t var_29{};
        StationFlags flags{};                         // 0x2A
        TownId town{};                                // 0x2C
        StationCargoStats cargoStats[kMaxCargoStats]; // 0x2E
        uint16_t stationTileSize{};                   // 0x1CE
        World::Pos3 stationTiles[80];                 // 0x1D0
        uint8_t var_3B0{};
        uint8_t var_3B1{};
        uint16_t var_3B2{};
        coord_t unk_tile_x{};                    // 0x3B4
        coord_t unk_tile_y{};                    // 0x3B6
        coord_t unk_tile_z{};                    // 0x3B8
        uint32_t airportMovementOccupiedEdges{}; // 0x3BA
        uint8_t pad_3BE[0x3D2 - 0x3BE]{};

        bool empty() const { return name == StringIds::null; }
        StationId id() const;
        void update();
        uint32_t calcAcceptedCargo(CargoSearchState& cargoSearchState) const;
        void sub_48F7D1();
        char* getStatusString(char* buffer);
        bool updateCargo();
        int32_t calculateCargoRating(const StationCargoStats& cargo) const;
        void updateLabel();
        void invalidate();
        void invalidateWindow();

        void deliverCargoToStation(const uint8_t cargoType, const uint8_t cargoQuantity);
        void deliverCargoToTown(uint8_t cargoType, uint16_t cargoQuantity);
        void updateCargoDistribution();

    private:
        void updateCargoAcceptance();
        void alertCargoAcceptanceChange(uint32_t oldCargoAcc, uint32_t newCargoAcc);
    };
    static_assert(sizeof(Station) == 0x3D2);
#pragma pack(pop)

    void setCatchmentDisplay(const Station* station, const CatchmentFlags flags);
    struct PotentialCargo
    {
        uint32_t accepted;
        uint32_t produced;
    };
    PotentialCargo calcAcceptedCargoTrackStationGhost(const Station* ghostStation, const World::Pos2& location, const uint32_t filter);
}
