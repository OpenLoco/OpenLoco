#pragma once

#include <OpenLoco/Core/Reflection.hpp>
#include <OpenLoco/S5/S5Town.h>

namespace OpenLoco
{
    template<>
    struct Reflection<S5::Town>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::Town, name),
            REFL_FIELD(S5::Town, x),
            REFL_FIELD(S5::Town, y),
            REFL_FIELD(S5::Town, flags),
            REFL_FIELD(S5::Town, labelFrame),
            REFL_FIELD(S5::Town, prng0),
            REFL_FIELD(S5::Town, prng1),
            REFL_FIELD(S5::Town, population),
            REFL_FIELD(S5::Town, populationCapacity),
            REFL_FIELD(S5::Town, numBuildings),
            REFL_FIELD(S5::Town, companyRatings),
            REFL_FIELD(S5::Town, companiesWithRating),
            REFL_FIELD(S5::Town, size),
            REFL_FIELD(S5::Town, historySize),
            REFL_FIELD(S5::Town, history),
            REFL_FIELD(S5::Town, historyMinPopulation),
            REFL_FIELD(S5::Town, amenityCounts),
            REFL_FIELD(S5::Town, monthlyCargoDelivered),
            REFL_FIELD(S5::Town, cargoInfluenceFlags),
            REFL_FIELD(S5::Town, var_19C),
            REFL_FIELD(S5::Town, buildSpeed),
            REFL_FIELD(S5::Town, numberOfAirports),
            REFL_FIELD(S5::Town, numStations),
            REFL_FIELD(S5::Town, var_1A8),
            REFL_FIELD(S5::Town, pad_1AC)>;
    };
}
