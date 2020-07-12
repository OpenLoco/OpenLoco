#include "title.h"
#include "companymgr.h"
#include "gui.h"
#include "interop/interop.hpp"
#include "openloco.h"
#include "ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::title
{
    static loco_global<uint16_t, 0x00508F12> _screenAge;
    static loco_global<uint8_t, 0x00508F14> _screenFlags;
    static loco_global<uint8_t, 0x00508F1A> _gameSpeed;

    static void sub_473A95(int32_t eax);

    void registerHooks()
    {
        register_hook(
            0x0046AD7D,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                start();
                return 0;
            });
    }

    // 0x0046AD7D
    void start()
    {
        companymgr::updating_company_id(companymgr::get_controlling_id());
        if (is_paused())
        {
            toggle_paused(true);
        }

        auto screenFlags = _screenFlags;
        _screenFlags = screenFlags & ~screen_flags::networked;
        ui::WindowManager::closeAllFloatingWindows();
        _screenFlags = screenFlags;
        _screenFlags |= screen_flags::title;
        _gameSpeed = 0;
        call(0x00472031); // unload all objects
        sub_473A95(1);
        call(0x00474874); // load selected objects
        call(0x00473B91); // object flags free 0
        call(0x0047237D); // reset loaded objects
        call(0x004748D4);
        call(0x0043C88C); // reset all
        initialise_viewports();
        call(0x004284C8);
        gui::init();
        call(0x00444357);
        gfx::invalidate_screen();
        _screenAge = 0;
    }

    static void sub_473A95(int32_t eax)
    {
        registers regs;
        regs.eax = eax;
        call(0x00473A95, regs);
    }
}
