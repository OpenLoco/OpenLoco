#pragma once

#include <OpenLoco/Engine/Types.hpp>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace OpenLoco
{
    enum class CompanyId : uint8_t
    {
        neutral = 15,
        null = std::numeric_limits<uint8_t>::max(),
    };
    enum class StationId : uint16_t
    {
        null = std::numeric_limits<uint16_t>::max(),
    };
    enum class IndustryId : uint8_t
    {
        null = std::numeric_limits<uint8_t>::max(),
    };
    enum class EntityId : uint16_t
    {
        null = std::numeric_limits<uint16_t>::max(),
    };
    enum class TownId : uint16_t
    {
        null = std::numeric_limits<uint16_t>::max(),
    };
    enum class MessageId : uint16_t
    {
        null = std::numeric_limits<uint16_t>::max(),
    };
    using string_id = uint16_t;
    using SoundObjectId_t = uint8_t;

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

    enum class Colour : uint8_t;

    struct ColourScheme
    {
        Colour primary;
        Colour secondary;
    };

    constexpr uint8_t vehicleTypeCount = 6;
}
