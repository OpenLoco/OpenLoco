#pragma once

namespace openloco::intro
{
    enum class intro_state
    {
        none,
        begin,
        state_8 = 8,
        state_9 = 9,
        end = 254,
    };

    bool is_active();
    intro_state state();
    void state(intro_state state);

    void update();
}
