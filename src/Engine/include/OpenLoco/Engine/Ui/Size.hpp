#pragma once

#include <cstdint>

namespace OpenLoco::Ui
{
    struct Size
    {
        uint16_t width = 0;
        uint16_t height = 0;

        constexpr Size(const uint16_t _width, const uint16_t _height)
            : width(_width)
            , height(_height)
        {
        }
    };
}
