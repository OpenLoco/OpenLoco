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

    // TODO: Remove the Size struct and rename this one to Size once interop is gone.
    struct Size32
    {
        int32_t width = 0;
        int32_t height = 0;

        constexpr Size32(const int32_t _width, const int32_t _height)
            : width(_width)
            , height(_height)
        {
        }
    };
}
