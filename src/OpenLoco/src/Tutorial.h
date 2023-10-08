#pragma once

#include "Types.hpp"

namespace OpenLoco::Tutorial
{
    enum class State : uint8_t
    {
        none,
        playing,
        recording,
    };

    State state();

    void registerHooks();

    void start(int16_t tutorialNumber);
    void stop();

    uint16_t nextInput();
    StringId nextString();

    uint8_t getTutorialNumber();
}
