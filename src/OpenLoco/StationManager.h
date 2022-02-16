#pragma once

#include "Core/LocoFixedVector.hpp"
#include "Limits.h"
#include "Station.h"
#include <array>
#include <cstddef>

namespace OpenLoco::StationManager
{
    void reset();
    FixedVector<Station, Limits::kMaxStations> stations();
    Station* get(StationId id);
    void update();
    void updateLabels();
    void updateDaily();
    void sub_437F29(CompanyId cid, uint8_t arg1);
    string_id generateNewStationName(StationId stationId, TownId townId, Map::Pos3 position, uint8_t mode);
    void zeroUnused();
    void registerHooks();
    uint16_t deliverCargoToNearbyStations(const uint8_t cargoType, const uint8_t cargoQty, const Map::Pos2& pos, const Map::TilePos2& size);
}
