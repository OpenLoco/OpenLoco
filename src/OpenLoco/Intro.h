#pragma once

namespace OpenLoco::Intro
{
    enum class State
    {
        none,
        begin,
        state_8 = 8,
        state_9 = 9,
        end = 254,
        done = 255,
    };

    bool isActive();
    State state();
    void state(State state);

    void update();
}
