#pragma once

#include "Types.hpp"

namespace OpenLoco::Tutorial
{
    enum class State : uint8_t
    {
        none,
        initialising,
        standby,
        playing,
        recording,
    };

    State state();

    void initialise(int16_t tutorialNumber);
    void start();
    void stop();

    int32_t nextInput();
    StringId nextString();

    uint8_t getTutorialNumber();
}
