#pragma once

#include "Types.hpp"

namespace OpenLoco::Tutorial
{
    enum class TutorialState : uint8_t
    {
        none,
        playing,
        recording,
    };

    TutorialState state();

    void registerHooks();

    void start(int16_t tutorialNumber);
    void stop();

    uint16_t nextInput();
    string_id nextString();

    uint8_t getTutorialNumber();
}
