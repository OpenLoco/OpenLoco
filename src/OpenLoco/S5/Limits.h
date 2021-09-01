#pragma once
#include <cstddef>

namespace OpenLoco::S5::Limits
{
    constexpr size_t maxMessages = 199;
    constexpr size_t maxCompanies = 15;
    constexpr size_t maxTowns = 80;
    constexpr size_t maxIndustries = 128;
    constexpr size_t maxStations = 1024;
    constexpr size_t maxEntities = 20000;
    constexpr size_t maxAnimations = 8192;
    constexpr size_t maxWaves = 64;
    constexpr size_t maxUserStrings = 2048;
    constexpr size_t maxVehicles = 1000;
    constexpr size_t maxRoutingsPerVehicle = 64;
    constexpr size_t maxOrders = 256000;
    constexpr size_t numEntityLists = 7;
    // There is a seperate pool of 200 entities dedicated for money
    constexpr size_t maxMoneyEntities = 200;
    // This is the main pool for everything that isn't money
    constexpr size_t maxNormalEntities = maxEntities - maxMoneyEntities;
    // Money is not counted in this limit
    constexpr size_t maxMiscEntities = 4000;
}
