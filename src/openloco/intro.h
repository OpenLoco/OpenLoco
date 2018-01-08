#pragma once

namespace openloco::intro
{
    enum class intro_state
    {
        none,
        begin,
        end = 254,
    };

    bool is_active();
    intro_state state();
    void state(intro_state state);

    void update();
}
