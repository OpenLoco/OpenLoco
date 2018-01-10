#pragma once

namespace openloco::input
{
    enum class mouse_button
    {
        left_down = 1,
        right_down,
        left_up,
        right_up,
    };

    void handle_keyboard();
    void enqueue_mouse_button(mouse_button button);
}
