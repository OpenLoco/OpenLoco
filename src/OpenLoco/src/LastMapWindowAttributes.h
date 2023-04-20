#pragma once

#include <cstdint>
#include "Window.h"

namespace OpenLoco::LastMapWindow
{
#pragma pack(push, 1)
    struct LastMapWindowAttributes
    {
        Ui::WindowFlags lastMapWindowFlags; // 0x00526284
        Ui::Size lastMapWindowSize;         // 0x00526288
        uint16_t lastMapWindowVar88A;       // 0x0052628C
        uint16_t lastMapWindowVar88C;       // 0x0052628E
    };
#pragma pack(pop)
    static_assert(sizeof(LastMapWindowAttributes) == 0x0C);
    LastMapWindowAttributes& getLastMapWindowAttributes();
}
