#pragma once
#include <cstdint>

namespace OpenLoco::Intro
{
    enum class State : uint8_t
    {
        none,
        begin,
        state2,
        state3,
        state4,
        state5,
        state6,
        state7,
        state8,
        state9,
        state10,
        end = 254,
        end2 = 255,
    };

    bool isActive();
    State state();
    void state(State state);

    void update();
}
