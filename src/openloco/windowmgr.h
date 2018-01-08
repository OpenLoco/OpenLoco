#pragma once

#include "window.h"

namespace openloco::ui::windowmgr
{
    enum class window_type
    {
        window_39 = 39
    };

    void resize();
    window * find(window_type type);
    window * find(window_type type, uint16_t index);
}
