#pragma once

namespace openloco::tutorial
{
    enum class tutorial_state
    {
        none,
        playing,
        recording,
    };

    tutorial_state state();

    void stop();
}
