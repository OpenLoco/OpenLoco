#include "Intro.h"
#include "Audio/Audio.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/RenderTarget.h"
#include "Gui.h"
#include "OpenLoco.h"
#include "Title.h"
#include "ViewportManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Intro
{
    loco_global<uint32_t, 0x0050C190> _50C190;
    loco_global<State, 0x0050C195> _state;
    loco_global<bool, 0x0050C196> _50C196;

    bool isActive()
    {
        return state() != State::none;
    }

    State state()
    {
        return *_state;
    }

    void state(State state)
    {
        _state = state;
    }

    // 0x0046AE0C
    void update()
    {
        addr<0x0005252E0, int32_t>() = 1;
        if (_state == State::end)
        {
            Gfx::getDrawingEngine().getDrawingContext().clearSingle(Gfx::getScreenRT(), 0x0A);
            if (!_50C196)
            {
                // Audio::stopIntro(); TODO: I think we might have broken audio in the intro...
                _50C196 = false;
            }
            _state = State::end2;
            _50C190 = 0;
            sub_431695(0);
            return;
        }
        else if (_state == State::end2)
        {
            _state = State::none;
            Gfx::loadPalette();
            addr<0x0005252E0, int32_t>() = 0;
            Gfx::invalidateScreen();
            initialiseViewports();
            Gui::init();
            Title::reset();
            sub_431695(0);
            return;
        }
        call(0x0046AE0C);
    }
}
