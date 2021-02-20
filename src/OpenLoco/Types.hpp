#pragma once

#include <cstdint>
#include <type_traits>

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

    template<typename Value>
    struct SpeedTemplate
    {
        Value value;
        constexpr Value getRaw() const { return value; }
        constexpr explicit SpeedTemplate(Value val)
            : value(val)
        {
        }

        // Conversion function (only valid to more accuracy)
        template<typename T>
        constexpr operator SpeedTemplate<T>() const
        {
            static_assert(sizeof(T) > sizeof(Value));
            constexpr auto shift = (sizeof(T) - sizeof(Value)) * 8;
            return SpeedTemplate<T>(getRaw() << shift);
        }

        // Best accuracy between ThisType and OtherType
        template<typename OtherType>
        using BestType = std::conditional_t<(sizeof(Value) > sizeof(OtherType)), SpeedTemplate<Value>, SpeedTemplate<OtherType>>;

        template<typename RhsT>
        constexpr bool operator==(SpeedTemplate<RhsT> const& rhs)
        {
            return BestType<RhsT>(*this).value == BestType<RhsT>(rhs).value;
        }
        template<typename RhsT>
        constexpr bool operator!=(SpeedTemplate<RhsT> const& rhs)
        {
            return !(*this == rhs);
        }
        template<typename RhsT>
        constexpr bool operator>(SpeedTemplate<RhsT> const& rhs)
        {
            return BestType<RhsT>(*this).value > BestType<RhsT>(rhs).value;
        }
        template<typename RhsT>
        constexpr bool operator<=(SpeedTemplate<RhsT> const& rhs)
        {
            return !(*this > rhs);
        }
        template<typename RhsT>
        constexpr bool operator<(SpeedTemplate<RhsT> const& rhs)
        {
            return BestType<RhsT>(*this).value < BestType<RhsT>(rhs).value;
        }
        template<typename RhsT>
        constexpr bool operator>=(SpeedTemplate<RhsT> const& rhs)
        {
            return !(*this < rhs);
        }
    };

    using Speed32 = SpeedTemplate<int32_t>;
    using Speed16 = SpeedTemplate<int16_t>;

    constexpr auto speed16Null = Speed16(-1);
    constexpr Speed16 toSpeed16(Speed32 speed)
    {
        return Speed16(speed.getRaw() / 65535);
    }

    namespace Literals
    {
        // Note: Only valid for 5 decimal places.
        constexpr Speed32 operator"" _mph(long double speedMph)
        {
            uint16_t wholeNumber = speedMph;
            uint64_t fraction = (speedMph - wholeNumber) * 100000;
            return Speed32((static_cast<uint32_t>(wholeNumber) << 16) | static_cast<uint16_t>((fraction << 16) / 100000));
        }

        constexpr Speed16 operator""_mph(unsigned long long int speed)
        {
            return Speed16(speed);
        }
        static_assert(2.75_mph == Speed32(0x2C000));
        static_assert(4.0_mph == Speed32(0x40000));
        static_assert(14.0_mph == Speed32(0xE0000));
        static_assert(0.333333_mph == Speed32(0x5555));
        static_assert(2.0_mph == 2_mph);
    }
}
