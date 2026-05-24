#include "S5/S5Industry.h"
#include "World/Industry.h"
#include <cstring>

namespace OpenLoco::S5
{
    S5::Industry exportIndustry(const OpenLoco::Industry& src)
    {
        S5::Industry dst{};
        dst.name = src.name;
        dst.x = src.x;
        dst.y = src.y;
        dst.flags = enumValue(src.flags);
        dst.prng0 = src.prng.srand_0();
        dst.prng1 = src.prng.srand_1();
        dst.objectId = src.objectId;
        dst.under_construction = src.under_construction;
        dst.foundingYear = src.foundingYear;
        dst.numTiles = src.numTiles;
        std::ranges::copy(src.tiles, dst.tiles);
        dst.town = enumValue(src.town);
        dst.tileLoop = src.tileLoop.current();
        dst.numFarmTiles = src.numFarmTiles;
        dst.numIdleFarmTiles = src.numIdleFarmTiles;
        dst.productionRate = src.productionRate;
        dst.owner = enumValue(src.owner);

        // Copy stations in range bitset to uint32_t array
        for (auto i = 0U; i < Limits::kMaxStations; ++i)
        {
            dst.stationsInRange[i / 32] |= src.stationsInRange[i] << (i % 32);
        }

        for (auto i = 0U; i < 2; ++i)
        {
            for (auto j = 0U; j < 4; ++j)
            {
                dst.producedCargoStatsStation[i][j] = enumValue(src.producedCargoStatsStation[i][j]);
            }
        }

        std::memcpy(dst.producedCargoStatsRating, src.producedCargoStatsRating, sizeof(dst.producedCargoStatsRating));
        std::ranges::copy(src.dailyProductionTarget, dst.dailyProductionTarget);
        std::ranges::copy(src.dailyProduction, dst.dailyProduction);
        std::ranges::copy(src.outputBuffer, dst.outputBuffer);
        std::ranges::copy(src.producedCargoQuantityMonthlyTotal, dst.producedCargoQuantityMonthlyTotal);
        std::ranges::copy(src.producedCargoQuantityPreviousMonth, dst.producedCargoQuantityPreviousMonth);
        std::ranges::copy(src.receivedCargoQuantityMonthlyTotal, dst.receivedCargoQuantityMonthlyTotal);
        std::ranges::copy(src.receivedCargoQuantityPreviousMonth, dst.receivedCargoQuantityPreviousMonth);
        std::ranges::copy(src.receivedCargoQuantityDailyTotal, dst.receivedCargoQuantityDailyTotal);
        std::ranges::copy(src.producedCargoQuantityDeliveredMonthlyTotal, dst.producedCargoQuantityDeliveredMonthlyTotal);
        std::ranges::copy(src.producedCargoQuantityDeliveredPreviousMonth, dst.producedCargoQuantityDeliveredPreviousMonth);
        std::ranges::copy(src.producedCargoPercentTransportedPreviousMonth, dst.producedCargoPercentTransportedPreviousMonth);
        std::ranges::copy(src.producedCargoMonthlyHistorySize, dst.producedCargoMonthlyHistorySize);
        std::ranges::copy(src.producedCargoMonthlyHistory1, dst.producedCargoMonthlyHistory1);
        std::ranges::copy(src.producedCargoMonthlyHistory2, dst.producedCargoMonthlyHistory2);
        std::ranges::copy(src.history_min_production, dst.history_min_production);

        return dst;
    }

    OpenLoco::Industry importIndustry(const S5::Industry& src)
    {
        OpenLoco::Industry dst{};
        dst.name = src.name;
        dst.x = src.x;
        dst.y = src.y;
        dst.flags = static_cast<IndustryFlags>(src.flags);
        dst.prng = Core::Prng{ src.prng0, src.prng1 };
        dst.objectId = src.objectId;
        dst.under_construction = src.under_construction;
        dst.foundingYear = src.foundingYear;
        dst.numTiles = src.numTiles;
        std::ranges::copy(src.tiles, dst.tiles);
        dst.town = static_cast<TownId>(src.town);
        dst.tileLoop = World::TileLoop{ src.tileLoop };
        dst.numFarmTiles = src.numFarmTiles;
        dst.numIdleFarmTiles = src.numIdleFarmTiles;
        dst.productionRate = src.productionRate;
        dst.owner = static_cast<CompanyId>(src.owner);
        // Copy uint32_t array to stations in range bitset
        for (auto i = 0U; i < Limits::kMaxStations; ++i)
        {
            dst.stationsInRange[i] = (src.stationsInRange[i / 32] >> (i % 32)) & 0x1;
        }
        for (auto i = 0U; i < 2; ++i)
        {
            for (auto j = 0U; j < 4; ++j)
            {
                dst.producedCargoStatsStation[i][j] = static_cast<StationId>(src.producedCargoStatsStation[i][j]);
            }
        }
        std::memcpy(dst.producedCargoStatsRating, src.producedCargoStatsRating, sizeof(dst.producedCargoStatsRating));
        std::ranges::copy(src.dailyProductionTarget, dst.dailyProductionTarget);
        std::ranges::copy(src.dailyProduction, dst.dailyProduction);
        std::ranges::copy(src.outputBuffer, dst.outputBuffer);
        std::ranges::copy(src.producedCargoQuantityMonthlyTotal, dst.producedCargoQuantityMonthlyTotal);
        std::ranges::copy(src.producedCargoQuantityPreviousMonth, dst.producedCargoQuantityPreviousMonth);
        std::ranges::copy(src.receivedCargoQuantityMonthlyTotal, dst.receivedCargoQuantityMonthlyTotal);
        std::ranges::copy(src.receivedCargoQuantityPreviousMonth, dst.receivedCargoQuantityPreviousMonth);
        std::ranges::copy(src.receivedCargoQuantityDailyTotal, dst.receivedCargoQuantityDailyTotal);
        std::ranges::copy(src.producedCargoQuantityDeliveredMonthlyTotal, dst.producedCargoQuantityDeliveredMonthlyTotal);
        std::ranges::copy(src.producedCargoQuantityDeliveredPreviousMonth, dst.producedCargoQuantityDeliveredPreviousMonth);
        std::ranges::copy(src.producedCargoPercentTransportedPreviousMonth, dst.producedCargoPercentTransportedPreviousMonth);
        std::ranges::copy(src.producedCargoMonthlyHistorySize, dst.producedCargoMonthlyHistorySize);
        std::ranges::copy(src.producedCargoMonthlyHistory1, dst.producedCargoMonthlyHistory1);
        std::ranges::copy(src.producedCargoMonthlyHistory2, dst.producedCargoMonthlyHistory2);
        std::ranges::copy(src.history_min_production, dst.history_min_production);

        return dst;
    }
}
