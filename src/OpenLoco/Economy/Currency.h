#pragma once

#include <cstdint>

namespace OpenLoco
{
    using currency32_t = int32_t;

#pragma pack(push, 1)
    struct currency48_t
    {
        uint32_t var_00 = 0;
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

        int64_t asInt64() const
        {
            return var_00 | (static_cast<int64_t>(var_04) << 32);
        }

        bool operator==(const currency48_t rhs) const
        {
            return var_00 == rhs.var_00 && var_04 == rhs.var_04;
        }

        bool operator!=(const currency48_t rhs) const
        {
            return !(var_00 == rhs.var_00 && var_04 == rhs.var_04);
        }

        currency48_t operator+(const currency32_t& rhs)
        {
            return currency48_t(asInt64() + rhs);
        }

        currency48_t operator+(const currency48_t& rhs)
        {
            return currency48_t(asInt64() + rhs.asInt64());
        }

        currency48_t& operator+=(const currency32_t& rhs)
        {
            auto sum = currency48_t(asInt64() + rhs);
            return *this = sum;
        }

        currency48_t& operator+=(const currency48_t& rhs)
        {
            auto sum = currency48_t(asInt64() + rhs.asInt64());
            return *this = sum;
        }

        currency48_t operator-(const currency32_t& rhs)
        {
            return currency48_t(asInt64() - rhs);
        }

        currency48_t operator-(const currency48_t& rhs)
        {
            return currency48_t(asInt64() - rhs.asInt64());
        }

        currency48_t& operator-=(const currency32_t& rhs)
        {
            auto sum = currency48_t(asInt64() - rhs);
            return *this = sum;
        }

        currency48_t& operator-=(const currency48_t& rhs)
        {
            auto sum = currency48_t(asInt64() - rhs.asInt64());
            return *this = sum;
        }

        bool operator<(const currency48_t& rhs) const
        {
            return asInt64() < rhs.asInt64();
        }

        bool operator<(const int64_t rhs) const
        {
            return asInt64() < rhs;
        }

        bool operator>(const currency48_t& rhs) const
        {
            return asInt64() > rhs.asInt64();
        }

        bool operator>(const int64_t rhs) const
        {
            return asInt64() > rhs;
        }
    };
#pragma pack(pop)
    static_assert(sizeof(currency48_t) == 6);

}
