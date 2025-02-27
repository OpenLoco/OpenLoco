#pragma once
#include <cstddef>

namespace OpenLoco::S5::Limits
{
    constexpr size_t kMaxMessages = 199;
    constexpr size_t kMaxCompanies = 15;
    constexpr size_t kMinTowns = 1;
    constexpr size_t kMaxTowns = 80;
    constexpr size_t kMaxIndustries = 128;
    constexpr size_t kMaxStations = 1024;
    constexpr size_t kMaxEntities = 20000;
    constexpr size_t kMaxAnimations = 8192;
    constexpr size_t kMaxWaves = 64;
    constexpr size_t kMaxUserStrings = 2048;
    constexpr size_t kMaxVehicles = 1000;
    constexpr size_t kMaxRoutingsPerVehicle = 64;
    // The number of orders appears to be the number of routings minus a null byte (OrderEnd)
    constexpr size_t kMaxOrdersPerVehicle = kMaxRoutingsPerVehicle - 1;
    constexpr size_t kMaxOrders = 256000;
    constexpr size_t kNumEntityLists = 7;
    // There is a separate pool of 200 entities dedicated for money
    constexpr size_t kMaxMoneyEntities = 200;
    // This is the main pool for everything that isn't money
    constexpr size_t maxNormalEntities = kMaxEntities - kMaxMoneyEntities;
    // Money is not counted in this limit
    constexpr size_t kMaxMiscEntities = 4000;
    constexpr size_t kMaxStationCargoDensity = 15;

    constexpr size_t kMaxInterfaceObjects = 1;
    constexpr size_t kMaxSoundObjects = 128;
    constexpr size_t kMaxCurrencyObjects = 1;
    constexpr size_t kMaxSteamObjects = 32;
    constexpr size_t kMaxRockObjects = 8;
    constexpr size_t kMaxWaterObjects = 1;
    constexpr size_t kMaxSurfaceObjects = 32;
    constexpr size_t kMaxTownNamesObjects = 1;
    constexpr size_t kMaxCargoObjects = 32;
    constexpr size_t kMaxWallObjects = 32;
    constexpr size_t kMaxTrainSignalObjects = 16;
    constexpr size_t kMaxLevelCrossingObjects = 4;
    constexpr size_t kMaxStreetLightObjects = 1;
    constexpr size_t kMaxTunnelObjects = 16;
    constexpr size_t kMaxBridgeObjects = 8;
    constexpr size_t kMaxTrainStationObjects = 16;
    constexpr size_t kMaxTrackExtraObjects = 8;
    constexpr size_t kMaxTrackObjects = 8;
    constexpr size_t kMaxRoadStationObjects = 16;
    constexpr size_t kMaxRoadExtraObjects = 4;
    constexpr size_t kMaxRoadObjects = 8;
    constexpr size_t kMaxAirportObjects = 8;
    constexpr size_t kMaxDockObjects = 8;
    constexpr size_t kMaxVehicleObjects = 224;
    constexpr size_t kMaxTreeObjects = 64;
    constexpr size_t kMaxSnowObjects = 1;
    constexpr size_t kMaxClimateObjects = 1;
    constexpr size_t kMaxHillShapesObjects = 1;
    constexpr size_t kMaxBuildingObjects = 128;
    constexpr size_t kMaxScaffoldingObjects = 1;
    constexpr size_t kMaxIndustryObjects = 16;
    constexpr size_t kMaxRegionObjects = 1;
    constexpr size_t kMaxCompetitorObjects = 32;
    constexpr size_t kMaxScenarioTextObjects = 1;

}
