#pragma once

#include <OpenLoco/Core/Reflection.hpp>
#include <OpenLoco/S5/S5Station.h>

namespace OpenLoco
{
    template<>
    struct Reflection<S5::StationCargoStats>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::StationCargoStats, quantity),
            REFL_FIELD(S5::StationCargoStats, origin),
            REFL_FIELD(S5::StationCargoStats, flags),
            REFL_FIELD(S5::StationCargoStats, age),
            REFL_FIELD(S5::StationCargoStats, rating),
            REFL_FIELD(S5::StationCargoStats, enrouteAge),
            REFL_FIELD(S5::StationCargoStats, vehicleSpeed),
            REFL_FIELD(S5::StationCargoStats, vehicleAge),
            REFL_FIELD(S5::StationCargoStats, industryId),
            REFL_FIELD(S5::StationCargoStats, densityPerTile)>;
    };

    template<>
    struct Reflection<S5::Station>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::Station, name),
            REFL_FIELD(S5::Station, x),
            REFL_FIELD(S5::Station, y),
            REFL_FIELD(S5::Station, z),
            REFL_FIELD(S5::Station, labelFrame),
            REFL_FIELD(S5::Station, owner),
            REFL_FIELD(S5::Station, noTilesTimeout),
            REFL_FIELD(S5::Station, flags),
            REFL_FIELD(S5::Station, town),
            REFL_FIELD(S5::Station, cargoStats),
            REFL_FIELD(S5::Station, stationTileSize),
            REFL_FIELD(S5::Station, stationTiles),
            REFL_FIELD(S5::Station, var_3B0),
            REFL_FIELD(S5::Station, var_3B1),
            REFL_FIELD(S5::Station, var_3B2),
            REFL_FIELD(S5::Station, airportRotation),
            REFL_FIELD(S5::Station, airportStartPos),
            REFL_FIELD(S5::Station, airportMovementOccupiedEdges),
            REFL_FIELD(S5::Station, pad_3BE)>;
    };
}
