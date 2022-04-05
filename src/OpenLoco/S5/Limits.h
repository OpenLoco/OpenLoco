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
    constexpr size_t kMaxOrders = 256000;
    constexpr size_t kNumEntityLists = 7;
    // There is a seperate pool of 200 entities dedicated for money
    constexpr size_t kMaxMoneyEntities = 200;
    // This is the main pool for everything that isn't money
    constexpr size_t maxNormalEntities = kMaxEntities - kMaxMoneyEntities;
    // Money is not counted in this limit
    constexpr size_t kMaxMiscEntities = 4000;
}
