#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace OpenLoco::Math::Vector
{
#pragma pack(push, 1)
    struct Vector2Tag
    {
    };

    template<typename T, typename TTypeTag = Vector2Tag>
    struct TVector2
    {
        using TypeTag = TTypeTag;

        T x{};
        T y{};

        constexpr TVector2() noexcept = default;
        constexpr TVector2(T _x, T _y) noexcept
            : x(_x)
            , y(_y)
        {
        }

        constexpr bool operator==(const TVector2& rhs) const noexcept
        {
            return x == rhs.x && y == rhs.y;
        }

        constexpr TVector2& operator+=(const TVector2& rhs) noexcept
        {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }

        constexpr TVector2& operator-=(const TVector2& rhs) noexcept
        {
            x -= rhs.x;
            y -= rhs.y;
            return *this;
        }

        constexpr TVector2& operator*=(const T rhs) noexcept
        {
            x *= rhs;
            y *= rhs;
            return *this;
        }

        constexpr TVector2& operator/=(const T rhs) noexcept
        {
            x /= rhs;
            y /= rhs;
            return *this;
        }

        constexpr const TVector2 operator+(const TVector2& rhs) const noexcept
        {
            return { static_cast<T>(x + rhs.x), static_cast<T>(y + rhs.y) };
        }

        constexpr const TVector2 operator-(const TVector2& rhs) const noexcept
        {
            return { static_cast<T>(x - rhs.x), static_cast<T>(y - rhs.y) };
        }

        constexpr const TVector2 operator*(const int32_t rhs) const noexcept
        {
            return { static_cast<T>(x * rhs), static_cast<T>(y * rhs) };
        }

        constexpr const TVector2 operator/(const int32_t rhs) const noexcept
        {
            return { static_cast<T>(x / rhs), static_cast<T>(y / rhs) };
        }

        constexpr const TVector2 operator<<(const uint8_t rhs) const noexcept
        {
            return { static_cast<T>(x << rhs), static_cast<T>(y << rhs) };
        }

        constexpr const TVector2 operator>>(const uint8_t rhs) const noexcept
        {
            return { static_cast<T>(x >> rhs), static_cast<T>(y >> rhs) };
        }
    };

    struct Vector3Tag
    {
    };

    template<typename T, typename TTypeTag = Vector3Tag>
    struct TVector3 : TVector2<T, TTypeTag>
    {
        using Base = TVector2<T, TTypeTag>;

        T z{};

        constexpr TVector3() noexcept = default;
        constexpr TVector3(T _x, T _y, T _z) noexcept
            : Base(_x, _y)
            , z(_z)
        {
        }

        constexpr TVector3(const Base& other, T z) noexcept
            : TVector3(other.x, other.y, z)
        {
        }

        constexpr bool operator==(const TVector3& rhs) const noexcept
        {
            return Base::operator==(rhs) && z == rhs.z;
        }

        constexpr TVector3& operator+=(const TVector3& rhs) noexcept
        {
            Base::operator+=(rhs);
            z += rhs.z;
            return *this;
        }

        constexpr TVector3& operator-=(const TVector3& rhs) noexcept
        {
            Base::operator-=(rhs);
            z -= rhs.z;
            return *this;
        }

        constexpr TVector3& operator*=(const T rhs) noexcept
        {
            Base::operator*=(rhs);
            z *= rhs.z;
            return *this;
        }

        constexpr TVector3& operator/=(const T rhs) noexcept
        {
            Base::operator/=(rhs);
            z /= rhs.z;
            return *this;
        }

        constexpr const TVector3 operator+(const TVector3& rhs) const noexcept
        {
            return { static_cast<T>(Base::x + rhs.x), static_cast<T>(Base::y + rhs.y), static_cast<T>(z + rhs.z) };
        }

        constexpr const TVector3 operator-(const TVector3& rhs) const noexcept
        {
            return { static_cast<T>(Base::x - rhs.x), static_cast<T>(Base::y - rhs.y), static_cast<T>(z - rhs.z) };
        }

        constexpr const TVector3 operator*(const T rhs) const noexcept
        {
            return { static_cast<T>(Base::x * rhs), static_cast<T>(Base::y * rhs), static_cast<T>(z * rhs) };
        }

        constexpr const TVector3 operator/(const T rhs) const noexcept
        {
            return { static_cast<T>(Base::x / rhs), static_cast<T>(Base::y / rhs), static_cast<T>(z / rhs) };
        }
    };
#pragma pack(pop)

    template<typename T, typename TTypeTag>
    static constexpr auto rotate(const TVector2<T, TTypeTag>& vec, int32_t direction) noexcept
    {
        TVector2<T, TTypeTag> res;
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

    // AKA taxicab distance
    template<typename T, typename TTypeTag>
    static constexpr auto manhattanDistance2D(const TVector2<T, TTypeTag>& lhs, const TVector2<T, TTypeTag>& rhs) noexcept
    {
        return std::abs(lhs.x - rhs.x) + std::abs(lhs.y - rhs.y);
    }

    // AKA taxicab distance
    template<typename T, typename TTypeTag>
    static constexpr auto manhattanDistance3D(const TVector3<T, TTypeTag>& lhs, const TVector3<T, TTypeTag>& rhs) noexcept
    {
        return std::abs(lhs.x - rhs.x) + std::abs(lhs.y - rhs.y) + std::abs(lhs.z - rhs.z);
    }

    // AKA maximum metric, chessboard distance
    template<typename T, typename TTypeTag>
    static constexpr auto chebyshevDistance2D(const TVector2<T, TTypeTag>& lhs, const TVector2<T, TTypeTag>& rhs) noexcept
    {
        return std::max(std::abs(lhs.x - rhs.x), std::abs(lhs.y - rhs.y));
    }

    // AKA maximum metric, chessboard distance
    template<typename T, typename TTypeTag>
    static constexpr auto chebyshevDistance3D(const TVector3<T, TTypeTag>& lhs, const TVector3<T, TTypeTag>& rhs) noexcept
    {
        return std::max({ std::abs(lhs.x - rhs.x), std::abs(lhs.y - rhs.y), std::abs(lhs.z - rhs.z) });
    }

    template<typename T, typename TTypeTag>
    static constexpr auto dot(const TVector2<T, TTypeTag>& lhs, const TVector2<T, TTypeTag>& rhs) noexcept
    {
        return lhs.x * rhs.x + lhs.y * rhs.y;
    }

    template<typename T, typename TTypeTag>
    static constexpr auto dot(const TVector3<T, TTypeTag>& lhs, const TVector3<T, TTypeTag>& rhs) noexcept
    {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    template<typename T, typename TTypeTag>
    static constexpr auto cross(const TVector3<T, TTypeTag>& lhs, const TVector3<T, TTypeTag>& rhs) noexcept
    {
        return TVector3<T>{
            lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x
        };
    }

    uint16_t fastSquareRoot(uint32_t distance);

    template<typename T, typename TTypeTag>
    auto distance2D(const TVector2<T, TTypeTag>& lhs, const TVector2<T, TTypeTag>& rhs) noexcept
    {
        auto x = lhs.x - rhs.x;
        auto y = lhs.y - rhs.y;
        return fastSquareRoot(x * x + y * y);
    }

    template<typename T, typename TTypeTag>
    auto distance3D(const TVector3<T, TTypeTag>& lhs, const TVector3<T, TTypeTag>& rhs) noexcept
    {
        auto x = lhs.x - rhs.x;
        auto y = lhs.y - rhs.y;
        auto z = lhs.z - rhs.z;
        return fastSquareRoot(x * x + y * y + z * z);
    }
}
