#pragma once
#include <cstdint>
#include <type_traits>

namespace OpenLoco
{
    template<typename TValue>
    struct SpeedTemplate
    {
        using ValueType = TValue;

        ValueType value;
        constexpr ValueType getRaw() const { return value; }
        constexpr explicit SpeedTemplate(ValueType val)
            : value(val)
        {
        }

        // Conversion function (only valid to more accuracy)
        template<typename T>
        constexpr operator SpeedTemplate<T>() const
        {
            static_assert(sizeof(T) > sizeof(ValueType));
            constexpr auto shift = (sizeof(T) - sizeof(ValueType)) * 8;
            return SpeedTemplate<T>(getRaw() * (1 << shift));
        }

        // Best accuracy between ThisType and OtherType
        template<typename OtherType>
        using BestValue = std::conditional_t<(sizeof(ValueType) > sizeof(OtherType)), ValueType, OtherType>;
        template<typename OtherType>
        using BestType = SpeedTemplate<BestValue<OtherType>>;

        template<typename RhsT>
        constexpr bool operator==(SpeedTemplate<RhsT> const& rhs) const
        {
            return BestType<RhsT>(*this).value == BestType<RhsT>(rhs).value;
        }
        template<typename RhsT>
        constexpr bool operator!=(SpeedTemplate<RhsT> const& rhs) const
        {
            return !(*this == rhs);
        }
        template<typename RhsT>
        constexpr bool operator>(SpeedTemplate<RhsT> const& rhs) const
        {
            return BestType<RhsT>(*this).value > BestType<RhsT>(rhs).value;
        }
        template<typename RhsT>
        constexpr bool operator<=(SpeedTemplate<RhsT> const& rhs) const
        {
            return !(*this > rhs);
        }
        template<typename RhsT>
        constexpr bool operator<(SpeedTemplate<RhsT> const& rhs) const
        {
            return BestType<RhsT>(*this).value < BestType<RhsT>(rhs).value;
        }
        template<typename RhsT>
        constexpr bool operator>=(SpeedTemplate<RhsT> const& rhs) const
        {
            return !(*this < rhs);
        }
        constexpr SpeedTemplate& operator-()
        {
            value = -value;
            return *this;
        }
        constexpr SpeedTemplate& operator+=(SpeedTemplate const& rhs)
        {
            value += rhs.value;
            return *this;
        }
        constexpr SpeedTemplate& operator-=(SpeedTemplate const& rhs)
        {
            value -= rhs.value;
            return *this;
        }
        template<typename RhsT>
        constexpr BestType<RhsT> operator+(SpeedTemplate<RhsT> const& rhs) const
        {
            return BestType<RhsT>(BestType<RhsT>(*this).value + BestType<RhsT>(rhs).value);
        }
        template<typename RhsT>
        constexpr BestType<RhsT> operator-(SpeedTemplate<RhsT> const& rhs) const
        {
            return BestType<RhsT>(BestType<RhsT>(*this).value - BestType<RhsT>(rhs).value);
        }
        template<typename RhsT>
        constexpr BestValue<RhsT> operator/(SpeedTemplate<RhsT> const& rhs) const
        {
            return BestType<RhsT>(*this).value / BestType<RhsT>(rhs).value;
        }
        constexpr SpeedTemplate operator/(ValueType rhs) const
        {
            return SpeedTemplate(value / rhs);
        }
        constexpr SpeedTemplate operator*(ValueType rhs) const
        {
            return SpeedTemplate(value * rhs);
        }
    };

    using Speed32 = SpeedTemplate<int32_t>;
    using Speed16 = SpeedTemplate<int16_t>;

    constexpr auto kSpeedZero = Speed16(0);
    constexpr auto kSpeed16Null = Speed16(-1);
    constexpr auto kSpeed16Max = Speed16(0x7FFF);
    // Truncates only use if safe to lose information
    constexpr Speed16 toSpeed16(Speed32 speed)
    {
        return Speed16(static_cast<Speed16::ValueType>(speed.getRaw() / 65536));
    }

    namespace Literals
    {
        // Note: Only valid for 5 decimal places.
        constexpr Speed32 operator"" _mph(long double speedMph)
        {
            const auto wholeNumber = static_cast<uint32_t>(speedMph);
            const auto fraction = static_cast<uint64_t>((speedMph - wholeNumber) * 100000);
            return Speed32((wholeNumber << 16) | static_cast<uint16_t>((fraction << 16) / 100000));
        }

        constexpr Speed16 operator""_mph(unsigned long long int speed)
        {
            return Speed16(static_cast<Speed16::ValueType>(speed));
        }

        static_assert(2.75_mph == Speed32(0x2C000));
        static_assert(4.0_mph == Speed32(0x40000));
        static_assert(14.0_mph == Speed32(0xE0000));
        static_assert(0.333333_mph == Speed32(0x5555));
        static_assert(2.0_mph == 2_mph);
        static_assert(2_mph + 4_mph == 6_mph);
        static_assert(2_mph + 4.0_mph == 6_mph);
        static_assert(2_mph - 4_mph == -2_mph);
        static_assert(2.0_mph - 4.0_mph == -2_mph);
        static_assert(6.0_mph / 2_mph == 3);
        static_assert(6.0_mph / 2.0_mph == 3);
        static_assert(6.0_mph / 2 == 3.0_mph);
        static_assert(6.0_mph * 2 == 12.0_mph);
        static_assert(6_mph * 2 == 12_mph);
        static_assert(toSpeed16(10000.9_mph) == 10000_mph);
        static_assert(0.21303_mph == Speed32(0x3689));
        static_assert(0.79798_mph == Speed32(0xCC48));
        static_assert(1.5_mph == Speed32(0x18000));
        static_assert(-1.5_mph == Speed32(0xFFFE8000));
        static_assert(0.18311_mph == Speed32(0x2EE0));
        static_assert(32000.5_mph / 2 == 16000.25_mph);
    }
}
