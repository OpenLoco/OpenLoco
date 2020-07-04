#include "tutorial.h"
#include "config.h"
#include "environment.h"
#include "interop/interop.hpp"
#include "localisation/string_ids.h"
#include "scenario.h"
#include "ui.h"

using namespace openloco::interop;

namespace openloco::tutorial
{
    static loco_global<uint8_t, 0x00508F19> _state;

    static loco_global<uint16_t*, 0x009C86FC> _tutorialOffset;
    static loco_global<string_id, 0x009C8708> _tutorialString;
    static loco_global<uint16_t*, 0x009C8704> _tutorialEnd; // ?
    static loco_global<uint8_t, 0x009C870A> _tutorialNumber;

    tutorial_state state()
    {
        return (tutorial_state)*_state;
    }

    void registerHooks()
    {
        interop::write_nop(0x0043C626, 0x0043C62D - 0x0043C626);
        interop::write_nop(0x0043C66E, 0x0043C6CC - 0x0043C66E);

        register_hook(
            0x0043C6CC,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                sub_43C6CC();
                regs = backup;
                return 0;
            });

        register_hook(
            0x0043C7A2,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                uint16_t next = nextInput();
                regs = backup;
                regs.ax = next;
                return 0;
            });
    }

    static void getTutorialScenarioFilename();

    // 0x0043C590
    void start(int16_t tutorialNumber)
    {
        if (tutorialNumber < 0 || tutorialNumber > 3)
            return;

        // NB: only used by tutorial widget drawing after.
        _tutorialNumber = tutorialNumber;

        // Ensure that we're in windowed mode, using dimensions 1024x768.
        const auto& display = config::get_new().display;
        const config::resolution_t tutorialResolution = { 1024, 768 };
        if (display.mode != config::screen_mode::window || display.window_resolution != tutorialResolution)
        {
            if (!ui::setDisplayMode(config::screen_mode::window, tutorialResolution))
                return;
        }

        // TODO(avgeffen) Maybe the window should not be scaled, either?

        // Get the environment file for this tutorial.
        static const environment::path_id tutorialFileIds[] = {
            environment::path_id::tut1024_1,
            environment::path_id::tut1024_2,
            environment::path_id::tut1024_3,
        };

        auto fileId = tutorialFileIds[tutorialNumber];

        // Jump back into the original routine.
        registers regs;
        regs.eax = tutorialNumber;
        regs.ebx = static_cast<int32_t>(fileId);
        call(0x0043C590, regs);
    }

    // 0x0043C6CC
    void sub_43C6CC()
    {
        // Set the first string to show.
        static const string_id openingStringIds[] = {
            string_ids::tutorial_1_string_1,
            string_ids::tutorial_2_string_1,
            string_ids::tutorial_3_string_1,
        };

        *_state = static_cast<uint8_t>(tutorial_state::playing);
        *_tutorialString = openingStringIds[*_tutorialNumber];

        // Set up the scenario.
        getTutorialScenarioFilename();
        addr<0x0050AE83, uint32_t>() = 0x12345678;
        addr<0x0050AE87, uint32_t>() = 0x9ABCDEF0;
        scenario::start(-1);

        // Unreachable?
        stop();
    }

    // 0x0043C70E
    void stop()
    {
        call(0x0043C70E);
    }

    // 0x0043C7A2
    uint16_t nextInput()
    {
        uint16_t next = **_tutorialOffset;
        _tutorialOffset++;

        if (*_tutorialOffset == *_tutorialEnd)
        {
            stop();
            addr<0x00508F12, uint16_t>() = 0;
        }

        return next;
    }

    string_id nextString()
    {
        string_id currentString = *_tutorialString;
        _tutorialString = currentString + 1;
        return currentString;
    }

    // 0x00445A30
    static void getTutorialScenarioFilename()
    {
        static loco_global<char[512], 0x00112CE04> scenarioFilename;
        auto bbPath = environment::get_path(environment::path_id::boulder_breakers);

        strncpy(&*scenarioFilename, bbPath.make_preferred().u8string().c_str(), 512);
    }
}
