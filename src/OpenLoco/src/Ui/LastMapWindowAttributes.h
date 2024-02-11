#pragma once

#include "Window.h"
#include <cstdint>

namespace OpenLoco::Ui
{
#pragma pack(push, 1)
    struct LastMapWindowAttributes
    {
        Ui::WindowFlags flags; // 0x00526284
        Ui::Size size;         // 0x00526288
        uint16_t var88A;       // 0x0052628C
        uint16_t var88C;       // 0x0052628E
    };
#pragma pack(pop)
    static_assert(sizeof(LastMapWindowAttributes) == 0x0C);
    LastMapWindowAttributes& getLastMapWindowAttributes();
}
