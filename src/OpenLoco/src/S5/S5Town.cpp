#include "S5/S5Town.h"
#include "World/Town.h"
#include <cstring>

namespace OpenLoco::S5
{
    S5::Town exportTown(OpenLoco::Town& src)
    {
        S5::Town dst{};
        dst.name = src.name;
        dst.x = src.x;
        dst.y = src.y;
        dst.flags = enumValue(src.flags);
        dst.labelFrame = exportLabelFrame(src.labelFrame);
        dst.prng0 = src.prng.srand_0();
        dst.prng1 = src.prng.srand_1();
        dst.population = src.population;
        dst.populationCapacity = src.populationCapacity;
        dst.numBuildings = src.numBuildings;
        std::ranges::copy(src.companyRatings, dst.companyRatings);
        dst.companiesWithRating = src.companiesWithRating;
        dst.size = enumValue(src.size);
        dst.historySize = src.historySize;
        std::ranges::copy(src.history, dst.history);
        dst.historyMinPopulation = src.historyMinPopulation;
        std::ranges::copy(src.var_150, dst.var_150);
        std::ranges::copy(src.monthlyCargoDelivered, dst.monthlyCargoDelivered);
        dst.cargoInfluenceFlags = src.cargoInfluenceFlags;
        std::memcpy(dst.var_19C, src.var_19C, sizeof(uint16_t) * sizeof(dst.var_19C));
        dst.buildSpeed = src.buildSpeed;
        dst.numberOfAirports = src.numberOfAirports;
        dst.numStations = src.numStations;
        dst.var_1A8 = src.var_1A8;
        return dst;
    }
}
