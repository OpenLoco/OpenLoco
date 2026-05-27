#pragma once
#include <cstdint>

namespace OpenLoco::Intro
{
    enum class State : uint8_t
    {
        none,
        begin,
        displayAtari,
        displayCS,
        state4, // unused
        state5, // unused
        state6, // unused
        state7, // unused
        displayNoticeBegin,
        displayNotice,
        displayNoticeBeginReset,
        end = 254,
        end2 = 255,
    };

    bool isActive();
    State state();
    void state(State state);

    void update();
}
