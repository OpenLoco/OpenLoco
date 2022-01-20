#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

namespace OpenLoco::Math::Vector
{
#pragma pack(push, 1)
    template<typename T, T TResolution, bool TIsGameSpace = true>
    struct TVector2
    {
        static constexpr auto Resolution = TResolution;
        static constexpr auto IsGameSpace = TIsGameSpace;

        T x = 0;
        T y = 0;

        constexpr TVector2() = default;
        constexpr TVector2(T _x, T _y)
            : x(_x)
            , y(_y)
        {
        }

        template<T TResolution2>
        constexpr TVector2(const TVector2<T, TResolution2>& other)
            : TVector2(other.x, other.y)
        {
            if constexpr (TResolution < TVector2<T, TResolution2>::Resolution)
            {
                x *= TVector2<T, TResolution2>::Resolution;
                y *= TVector2<T, TResolution2>::Resolution;
            }
            if constexpr (TResolution > TVector2<T, TResolution2>::Resolution)
            {
                x /= TResolution;
                y /= TResolution;
            }
        }

        constexpr bool operator==(const TVector2& rhs) const
        {
            return x == rhs.x && y == rhs.y;
        }

        constexpr bool operator!=(const TVector2& rhs) const
        {
            return !(*this == rhs);
        }

        constexpr TVector2& operator+=(const TVector2& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }

        constexpr TVector2& operator-=(const TVector2& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            return *this;
        }

        constexpr TVector2& operator*=(const T rhs)
        {
            x *= rhs;
            y *= rhs;
            return *this;
        }

        constexpr TVector2& operator/=(const T rhs)
        {
            x /= rhs;
            y /= rhs;
            return *this;
        }

        constexpr const TVector2 operator+(const TVector2& rhs) const
        {
            return { static_cast<T>(x + rhs.x), static_cast<T>(y + rhs.y) };
        }

        constexpr const TVector2 operator-(const TVector2& rhs) const
        {
            return { static_cast<T>(x - rhs.x), static_cast<T>(y - rhs.y) };
        }

        constexpr const TVector2 operator*(const int32_t rhs) const
        {
            return { static_cast<T>(x * rhs), static_cast<T>(y * rhs) };
        }

        constexpr const TVector2 operator/(const int32_t rhs) const
        {
            return { static_cast<T>(x / rhs), static_cast<T>(y / rhs) };
        }

        constexpr const TVector2 operator<<(const uint8_t rhs) const
        {
            return { static_cast<T>(x << rhs), static_cast<T>(y << rhs) };
        }

        constexpr const TVector2 operator>>(const uint8_t rhs) const
        {
            return { static_cast<T>(x >> rhs), static_cast<T>(y >> rhs) };
        }
    };

    template<typename T, T TResolution = 1>
    struct TVector3 : TVector2<T, TResolution>
    {
        static constexpr auto NumberBase = TResolution;

        using Base = TVector2<T, TResolution>;

        T z = 0;

        constexpr TVector3() = default;
        constexpr TVector3(T _x, T _y, T _z)
            : Base(_x, _y)
            , z(_z)
        {
        }

        template<T TResolution2>
        constexpr TVector3(const TVector3<T, TResolution2>& other)
            : TVector3(other.x, other.y, other.z)
        {
            if constexpr (TResolution < TVector3<T, TResolution2>::Resolution)
            {
                Base::x *= TVector3<T, TResolution2>::Resolution;
                Base::y *= TVector3<T, TResolution2>::Resolution;
                z *= TResolution;
            }
            if constexpr (TResolution > TVector3<T, TResolution2>::Resolution)
            {
                Base::x /= TResolution;
                Base::y /= TResolution;
                z /= TResolution;
            }
        }

        template<T TResolution2>
        explicit constexpr TVector3(const TVector2<T, TResolution2>& other, T _z)
            : TVector3(other.x, other.y, _z)
        {
            if constexpr (TResolution < TVector2<T, TResolution2>::Resolution)
            {
                Base::x *= TVector3<T, TResolution2>::Resolution;
                Base::y *= TVector3<T, TResolution2>::Resolution;
                z *= TResolution;
            }
            if constexpr (TResolution > TVector2<T, TResolution2>::Resolution)
            {
                Base::x /= TResolution;
                Base::y /= TResolution;
                z /= TResolution;
            }
        }

        constexpr bool operator==(const TVector3& rhs) const
        {
            return Base::operator==(rhs) && z == rhs.z;
        }

        constexpr bool operator!=(const TVector3& rhs) const
        {
            return !(*this == rhs);
        }

        constexpr TVector3& operator+=(const TVector3& rhs)
        {
            Base::operator+=(rhs);
            z += rhs.z;
            return *this;
        }

        constexpr TVector3& operator-=(const TVector3& rhs)
        {
            Base::operator-=(rhs);
            z -= rhs.z;
            return *this;
        }

        constexpr TVector3& operator*=(const T rhs)
        {
            Base::operator*=(rhs);
            z *= rhs.z;
            return *this;
        }

        constexpr TVector3& operator/=(const T rhs)
        {
            Base::operator/=(rhs);
            z /= rhs.z;
            return *this;
        }

        constexpr const TVector3 operator+(const TVector3& rhs) const
        {
            return { static_cast<T>(Base::x + rhs.x), static_cast<T>(Base::y + rhs.y), static_cast<T>(z + rhs.z) };
        }

        constexpr const TVector3 operator-(const TVector3& rhs) const
        {
            return { static_cast<T>(Base::x - rhs.x), static_cast<T>(Base::y - rhs.y), static_cast<T>(z - rhs.z) };
        }

        constexpr const TVector3 operator*(const T rhs) const
        {
            return { static_cast<T>(Base::x * rhs), static_cast<T>(Base::y * rhs), static_cast<T>(z * rhs) };
        }

        constexpr const TVector3 operator/(const T rhs) const
        {
            return { static_cast<T>(Base::x / rhs), static_cast<T>(Base::y / rhs), static_cast<T>(z / rhs) };
        }
    };
#pragma pack(pop)

    template<typename T, T TResolution>
    static constexpr auto rotate(const TVector2<T, TResolution>& vec, int32_t direction)
    {
        TVector2<T, TResolution> res;
        switch (direction & 3)
        {
            default:
            case 0:
                res.x = vec.x;
                res.y = vec.y;
                break;
            case 1:
                res.x = vec.y;
                res.y = -vec.x;
                break;
            case 2:
                res.x = -vec.x;
                res.y = -vec.y;
                break;
            case 3:
                res.x = -vec.y;
                res.y = vec.x;
                break;
        }
        return res;
    }

    template<typename T, T TResolution, bool TIsGameSpace>
    static constexpr auto manhattanDistance(const TVector2<T, TResolution, TIsGameSpace>& lhs, const TVector2<T, TResolution, TIsGameSpace>& rhs)
    {
        return std::abs(lhs.x - rhs.x) + std::abs(lhs.y - rhs.y);
    }

    template<typename T, T TResolution>
    static constexpr auto manhattanDistance(const TVector3<T, TResolution>& lhs, const TVector3<T, TResolution>& rhs)
    {
        return std::abs(lhs.x - rhs.x) + std::abs(lhs.y - rhs.y) + std::abs(lhs.z - rhs.z);
    }

    template<typename T, T TResolution>
    static constexpr auto dot(const TVector2<T, TResolution>& lhs, const TVector2<T, TResolution>& rhs)
    {
        return lhs.x * rhs.x + lhs.y * rhs.y;
    }

    template<typename T, T TResolution>
    static constexpr auto dot(const TVector3<T, TResolution>& lhs, const TVector3<T, TResolution>& rhs)
    {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    template<typename T, T TResolution>
    static constexpr auto cross(const TVector3<T, TResolution>& lhs, const TVector3<T, TResolution>& rhs)
    {
        return TVector3<T, TResolution>{
            lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x
        };
    }

    uint16_t fastSquareRoot(uint32_t distance);

    template<typename T, T TResolution>
    auto distance(const TVector2<T, TResolution>& lhs, const TVector2<T, TResolution>& rhs)
    {
        auto x = lhs.x - rhs.x;
        auto y = lhs.y - rhs.y;
        return fastSquareRoot(x * x + y * y);
    }
}
