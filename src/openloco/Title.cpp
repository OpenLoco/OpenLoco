#include "Title.h"
#include "CompanyManager.h"
#include "Gui.h"
#include "OpenLoco.h"
#include "audio/audio.h"
#include "interop/interop.hpp"
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
        registerHook(
            0x0046AD7D,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                start();
                return 0;
            });
    }

    // 0x00472031
    // ?unload all objects?
    static void sub_472031()
    {
        call(0x00472031);
    }

    // 0x00474874
    // ?load selected objects?
    static void sub_474874()
    {
        call(0x00474874);
    }

    // 0x00473B91
    // object flags free 0
    static void sub_473B91()
    {
        call(0x00473B91);
    }

    // 0x0047237D
    // ?reset loaded objects?
    static void sub_47237D()
    {
        call(0x0047237D);
    }

    // 0x004748D4
    // ?Set default types for building. Like the initial track type and vehicle type and such.?
    static void sub_4748D4()
    {
        call(0x004748D4);
    }

    // 0x0043C88C
    // ?reset all?
    static void sub_43C88C()
    {
        call(0x0043C88C);
    }

    // 0x004284C8
    static void sub_4284C8()
    {
        call(0x004284C8);
    }

    // 0x00444357
    static void sub_444357()
    {
        call(0x00444357);
    }

    // 0x0046AD7D
    void start()
    {
        companymgr::updatingCompanyId(companymgr::getControllingId());
        if (isPaused())
        {
            togglePause(true);
        }

        auto screenFlags = _screenFlags;
        _screenFlags = screenFlags & ~screen_flags::networked;
        ui::WindowManager::closeAllFloatingWindows();
        _screenFlags = screenFlags;
        _screenFlags |= screen_flags::title;
        _gameSpeed = 0;
        sub_472031();
        sub_473A95(1);
        sub_474874();
        sub_473B91();
        sub_47237D();
        sub_4748D4();
        sub_43C88C();
        initialiseViewports();
        sub_4284C8();
        gui::init();
        sub_444357();
        gfx::invalidateScreen();
        _screenAge = 0;

        audio::playTitleScreenMusic();
    }

    // 0x00473A95
    static void sub_473A95(int32_t eax)
    {
        registers regs;
        regs.eax = eax;
        call(0x00473A95, regs);
    }
}
