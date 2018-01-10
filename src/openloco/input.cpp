#include "input.h"
#include "interop/interop.hpp"

namespace openloco::input
{
    // 0x004BE92A
    void handle_keyboard()
    {
        LOCO_CALLPROC_X(0x004BE92A);
    }

    // 0x00406FEC
    void enqueue_mouse_button(mouse_button button)
    {
        ((void(*)(int))0x00406FEC)((int32_t)button);
    }
}
