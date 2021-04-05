#pragma once

#include <cstdint>

namespace OpenLoco
{
    using coord_t = int16_t;
    using company_id_t = uint8_t;
    using station_id_t = uint16_t;
    using IndustryId_t = uint8_t;
    using string_id = uint16_t;
    using EntityId_t = uint16_t;
    using tile_coord_t = int16_t;
    using TownId_t = uint16_t;
    using sound_object_id_t = uint8_t;

    class FormatArguments;

    enum class VehicleType : uint8_t
    {
        train = 0,
        bus,
        truck,
        tram,
        aircraft,
        ship
    };

    struct ColourScheme
    {
        uint8_t primary;
        uint8_t secondary;
    };

    constexpr uint8_t vehicleTypeCount = 6;
}
