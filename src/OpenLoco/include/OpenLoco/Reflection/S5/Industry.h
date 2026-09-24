#pragma once

#include <OpenLoco/Core/Reflection.hpp>
#include <OpenLoco/S5/S5Industry.h>

namespace OpenLoco
{
    template<>
    struct Reflection<S5::Industry>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::Industry, name),
            REFL_FIELD(S5::Industry, x),
            REFL_FIELD(S5::Industry, y),
            REFL_FIELD(S5::Industry, flags),
            REFL_FIELD(S5::Industry, prng0),
            REFL_FIELD(S5::Industry, prng1),
            REFL_FIELD(S5::Industry, objectId),
            REFL_FIELD(S5::Industry, under_construction),
            REFL_FIELD(S5::Industry, foundingYear),
            REFL_FIELD(S5::Industry, numTiles),
            REFL_FIELD(S5::Industry, tiles),
            REFL_FIELD(S5::Industry, town),
            REFL_FIELD(S5::Industry, tileLoop),
            REFL_FIELD(S5::Industry, numFarmTiles),
            REFL_FIELD(S5::Industry, numIdleFarmTiles),
            REFL_FIELD(S5::Industry, productionRate),
            REFL_FIELD(S5::Industry, owner),
            REFL_FIELD(S5::Industry, stationsInRange),
            REFL_FIELD(S5::Industry, producedCargoStatsStation),
            REFL_FIELD(S5::Industry, producedCargoStatsRating),
            REFL_FIELD(S5::Industry, dailyProductionTarget),
            REFL_FIELD(S5::Industry, dailyProduction),
            REFL_FIELD(S5::Industry, outputBuffer),
            REFL_FIELD(S5::Industry, producedCargoQuantityMonthlyTotal),
            REFL_FIELD(S5::Industry, producedCargoQuantityPreviousMonth),
            REFL_FIELD(S5::Industry, receivedCargoQuantityMonthlyTotal),
            REFL_FIELD(S5::Industry, receivedCargoQuantityPreviousMonth),
            REFL_FIELD(S5::Industry, receivedCargoQuantityDailyTotal),
            REFL_FIELD(S5::Industry, producedCargoQuantityDeliveredMonthlyTotal),
            REFL_FIELD(S5::Industry, producedCargoQuantityDeliveredPreviousMonth),
            REFL_FIELD(S5::Industry, producedCargoPercentTransportedPreviousMonth),
            REFL_FIELD(S5::Industry, producedCargoMonthlyHistorySize),
            REFL_FIELD(S5::Industry, producedCargoMonthlyHistory1),
            REFL_FIELD(S5::Industry, producedCargoMonthlyHistory2),
            REFL_FIELD(S5::Industry, history_min_production),
            REFL_FIELD(S5::Industry, pad_393)>;
    };
}
