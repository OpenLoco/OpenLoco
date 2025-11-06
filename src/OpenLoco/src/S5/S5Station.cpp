#include "S5Station.h"
#include "World/Station.h"
#include <ranges>

namespace OpenLoco::S5
{
    static S5::StationCargoStats exportCargoStats(const OpenLoco::StationCargoStats& src)
    {
        S5::StationCargoStats dst{};
        dst.quantity = src.quantity;
        dst.origin = enumValue(src.origin);
        dst.flags = enumValue(src.flags);
        dst.age = src.age;
        dst.rating = src.rating;
        dst.enrouteAge = src.enrouteAge;
        dst.vehicleSpeed = src.vehicleSpeed.getRaw();
        dst.vehicleAge = src.vehicleAge;
        dst.industryId = enumValue(src.industryId);
        dst.densityPerTile = src.densityPerTile;
        return dst;
    }

    S5::Station exportStation(OpenLoco::Station& src)
    {
        S5::Station dst{};
        dst.name = src.name;
        dst.x = src.x;
        dst.y = src.y;
        dst.z = src.z;
        dst.labelFrame = exportLabelFrame(src.labelFrame);
        dst.owner = enumValue(src.owner);
        dst.noTilesTimeout = src.noTilesTimeout;
        dst.flags = enumValue(src.flags);
        dst.town = enumValue(src.town);
        for (auto i = 0; i < std::size(dst.cargoStats); ++i)
        {
            dst.cargoStats[i] = exportCargoStats(src.cargoStats[i]);
        }
        dst.stationTileSize = src.stationTileSize;
        std::ranges::copy(src.stationTiles, dst.stationTiles);
        dst.var_3B0 = src.var_3B0;
        dst.var_3B1 = src.var_3B1;
        dst.var_3B2 = src.var_3B2;
        dst.airportRotation = src.airportRotation;
        dst.airportStartPos = src.airportStartPos;
        dst.airportMovementOccupiedEdges = src.airportMovementOccupiedEdges;

        return dst;
    }
}
