#pragma once
#include <cstdint>

namespace OpenLoco::Intro
{
    enum class State : uint8_t
    {
        none,
        begin,
        state_8 = 8,
        state_9 = 9,
        end = 254,
    };

    bool isActive();
    State state();
    void state(State state);

    void update();
}
