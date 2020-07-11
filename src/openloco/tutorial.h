#pragma once

#include "types.hpp"

namespace openloco::tutorial
{
    enum class tutorial_state
    {
        none,
        playing,
        recording,
    };

    tutorial_state state();

    void registerHooks();

    void start(int16_t tutorialNumber);
    void stop();

    uint16_t nextInput();
    string_id nextString();
}
