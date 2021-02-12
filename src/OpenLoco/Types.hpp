#pragma once

#include <cstdint>

namespace OpenLoco
{
    using coord_t = int16_t;
    using company_id_t = uint8_t;
    using currency32_t = int32_t;
    using station_id_t = uint16_t;
    using industry_id_t = uint8_t;
    using string_id = uint16_t;
    using thing_id_t = uint16_t;
    using tile_coord_t = int16_t;
    using sound_object_id_t = uint8_t;

#pragma pack(push, 1)
    struct currency48_t
    {
        int32_t var_00 = 0;
        int16_t var_04 = 0;

        currency48_t(int32_t currency)
            : currency48_t(static_cast<int64_t>(currency))
        {
        }

        currency48_t(int64_t currency)
        {
            var_00 = currency & 0xFFFFFFFF;
            var_04 = (currency >> 32) & 0xFFFF;
        }

        int64_t asInt64()
        {
            return var_00 | (static_cast<int64_t>(var_04) << 32);
        }

        bool operator==(const currency48_t rhs)
        {
            return var_00 == rhs.var_00 && var_04 == rhs.var_04;
        }

        bool operator!=(const currency48_t rhs)
        {
            return !(var_00 == rhs.var_00 && var_04 == rhs.var_04);
        }

        currency48_t operator+(currency48_t& rhs)
        {
            return currency48_t(asInt64() + rhs.asInt64());
        }

        currency48_t& operator+=(currency48_t& rhs)
        {
            auto sum = currency48_t(asInt64() + rhs.asInt64());
            return *this = sum;
        }

        bool operator<(currency48_t& rhs)
        {
            return asInt64() < rhs.asInt64();
        }

        bool operator<(int rhs)
        {
            return var_00 < rhs;
        }
    };
#pragma pack(pop)
    static_assert(sizeof(currency48_t) == 6);

    template<typename T>
    struct location2
    {
        T x = 0;
        T y = 0;

        location2() = default;
        location2(T locX, T locY)
            : x(locX)
            , y(locY)
        {
        }
    };

    template<typename T>
    struct location3
    {
        T x = 0;
        T y = 0;
        T z = 0;

        location3() = default;
        location3(T locX, T locY, T locZ)
            : x(locX)
            , y(locY)
            , z(locZ)
        {
        }
    };

    using xy32 = location2<int32_t>;
    using loc16 = location3<int16_t>;

    namespace Location
    {
        constexpr int16_t null = (int16_t)0x8000u;
    }

    class FormatArguments;

    enum class ZoomLevel : uint8_t
    {
        full = 0,
        half = 1,
        quarter = 2,
        eighth = 3,
    };

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

    namespace ZoomLevels
    {
        constexpr uint8_t max = 4;
    }

    namespace Literals
    {
        // Note: Only valid for 5 decimal places.
        constexpr uint32_t operator"" _mph(long double speedMph)
        {
            uint16_t wholeNumber = speedMph;
            uint64_t fraction = (speedMph - wholeNumber) * 100000;
            return (static_cast<uint32_t>(wholeNumber) << 16) | static_cast<uint16_t>((fraction << 16) / 100000);
        }

        static_assert(0.0_mph == 0x0000);
        static_assert(1.0_mph == 0x10000);
        static_assert(2.75_mph == 0x2C000);
        static_assert(4.0_mph == 0x40000);
        static_assert(6.0_mph == 0x60000);
        static_assert(9.0_mph == 0x90000);
        static_assert(13.0_mph == 0xD0000);
        static_assert(14.0_mph == 0xE0000);
        static_assert(15.0_mph == 0xF0000);
        static_assert(25.0_mph == 0x190000);
        static_assert(35.0_mph == 0x230000);
        static_assert(0.333333_mph == 0x5555);
    }
}
