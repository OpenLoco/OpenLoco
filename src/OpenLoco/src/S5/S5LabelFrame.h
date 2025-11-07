#pragma once

#include <cstdint>

namespace OpenLoco
{
    struct LabelFrame;
}

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct LabelFrame
    {
        int16_t left[4]{};
        int16_t right[4]{};
        int16_t top[4]{};
        int16_t bottom[4]{};
    };
    static_assert(sizeof(LabelFrame) == 0x20);
#pragma pack(pop)

    S5::LabelFrame exportLabelFrame(const OpenLoco::LabelFrame& src);
    OpenLoco::LabelFrame importLabelFrame(const S5::LabelFrame& src);
}
