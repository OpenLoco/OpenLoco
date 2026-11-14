#pragma once
#include <cstdint>

namespace OpenLoco::Gfx
{
    class DrawingContext;
}

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
    };

    bool isActive();
    State state();
    void state(State state);
    void skip();

    void tick();
    void draw(Gfx::DrawingContext& drawingCtx);
}
