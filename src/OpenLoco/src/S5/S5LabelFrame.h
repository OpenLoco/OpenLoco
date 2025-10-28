#pragma once

#include <cstdint>

namespace OpenLoco
{
    struct LabelFrame;
}

namespace OpenLoco::S5
{
    struct LabelFrame
    {
        int16_t left[4]{};
        int16_t right[4]{};
        int16_t top[4]{};
        int16_t bottom[4]{};
    };

    S5::LabelFrame exportLabelFrame(OpenLoco::LabelFrame& src);
}
