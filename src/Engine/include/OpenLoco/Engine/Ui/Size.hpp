#pragma once

#include <cstdint>

namespace OpenLoco::Ui
{
    struct Size
    {
        int32_t width = 0;
        int32_t height = 0;

        constexpr Size() = default;

        constexpr Size(const int32_t _width, const int32_t _height)
            : width(_width)
            , height(_height)
        {
        }

        constexpr Size operator-(const Size& rhs) const
        {
            return { width - rhs.width, height - rhs.height };
        }

        constexpr Size operator+(const Size& rhs) const
        {
            return { width + rhs.width, height + rhs.height };
        }
    };
}
