#pragma once

#include <cstdint>

namespace OpenLoco::Gfx
{
    enum class Font : int16_t
    {
        m1 = -1,
        m2 = -2,

        medium_normal = 0,
        medium_bold = 224,
        small = 448,
        large = 672,
    };

}
