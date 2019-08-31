#pragma once

#include <cstdint>

namespace openloco::gfx
{
    struct point_t
    {
        int16_t x = 0;
        int16_t y = 0;

        constexpr point_t(){};

        constexpr point_t(int16_t x, int16_t y)
            : x(x)
            , y(y)
        {
        }

        bool operator==(const point_t& rhs)
        {
            return x == rhs.x && y == rhs.y;
        }

        bool operator==(const int16_t rhs)
        {
            return x == rhs && y == rhs;
        }

        point_t& operator+=(const point_t& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }

        point_t& operator-=(const point_t& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            return *this;
        }

        friend point_t operator+(point_t lhs, const point_t& rhs)
        {
            lhs += rhs;
            return lhs;
        }

        friend point_t operator-(point_t lhs, const point_t& rhs)
        {
            lhs -= rhs;
            return lhs;
        }
    };

    struct ui_size_t
    {
        uint16_t width = 0;
        uint16_t height = 0;

        constexpr ui_size_t(uint16_t width, uint16_t height)
            : width(width)
            , height(height)
        {
        }
    };
}
