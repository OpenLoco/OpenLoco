#pragma once

#include "LabelFrame.h"
#include "Localisation/StringManager.h"
#include "Map/Tile.h"
#include "Speed.hpp"
#include "Types.hpp"
#include "Utility/Numeric.hpp"
#include <cstdint>
#include <limits>

namespace OpenLoco
{
#pragma pack(push, 1)
    struct StationCargoStats
    {
        uint16_t quantity{};                // 0x2E
        StationId origin = StationId::null; // 0x30
        uint8_t flags{};                    // 0x32
        uint8_t age{};                      // 0x33
        uint8_t rating{};                   // 0x34
        uint8_t enrouteAge{};               // 0x35
        Speed16 vehicleSpeed{ 0 };          // 0x36 max speed of vehicle that transported the cargo
        uint8_t vehicleAge{};               // 0x38 age of the vehicle (car) that transported the cargo
        IndustryId industryId{};            // 0x39
        uint8_t var_40{};

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

    constexpr size_t kMaxCargoStats = 32;

    enum class StationType : uint8_t
    {
        trainStation = 0,
        roadStation,
        airport,
        docks,
    };

    namespace StationFlags
    {
        constexpr uint16_t transportModeRail = (1 << 0);
        constexpr uint16_t transportModeRoad = (1 << 1);
        constexpr uint16_t transportModeAir = (1 << 2);
        constexpr uint16_t transportModeWater = (1 << 3);
        constexpr uint16_t flag_4 = (1 << 4);
        constexpr uint16_t flag_5 = (1 << 5);
        constexpr uint16_t flag_6 = (1 << 6);
        constexpr uint16_t flag_7 = (1 << 7);
        constexpr uint16_t flag_8 = (1 << 8);
        constexpr uint16_t allModes = StationFlags::transportModeRail | StationFlags::transportModeRoad | StationFlags::transportModeAir | StationFlags::transportModeWater;
    }

    string_id getTransportIconsFromStationFlags(const uint16_t flags);

    struct CargoSearchState;

    struct Station
    {
        string_id name = StringIds::null; // 0x00
        coord_t x{};                      // 0x02
        coord_t y{};                      // 0x04
        coord_t z{};                      // 0x06
        LabelFrame labelFrame;            // 0x08
        CompanyId owner{};                // 0x28
        uint8_t var_29{};
        uint16_t flags{};                             // 0x2A
        TownId town{};                                // 0x2C
        StationCargoStats cargoStats[kMaxCargoStats]; // 0x2E
        uint16_t stationTileSize{};                   // 0x1CE
        Map::Pos3 stationTiles[80];                   // 0x1D0
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
        uint32_t calcAcceptedCargo(CargoSearchState& cargoSearchState, const Map::Pos2& location = { -1, -1 }, const uint32_t filter = 0);
        void sub_48F7D1();
        char* getStatusString(char* buffer);
        bool updateCargo();
        int32_t calculateCargoRating(const StationCargoStats& cargo) const;
        void updateLabel();
        void invalidate();
        void invalidateWindow();
        void setCatchmentDisplay(uint8_t flags);
        void deliverCargoToStation(const uint8_t cargoType, const uint8_t cargoQuantity);
        void deliverCargoToTown(uint8_t cargoType, uint16_t cargoQuantity);
        void updateCargoDistribution();

    private:
        void updateCargoAcceptance();
        void alertCargoAcceptanceChange(uint32_t oldCargoAcc, uint32_t newCargoAcc);
    };
    static_assert(sizeof(Station) == 0x3D2);
#pragma pack(pop)
}
