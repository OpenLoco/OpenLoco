#include "tutorial.h"
#include "config.h"
#include "environment.h"
#include "gui.h"
#include "interop/interop.hpp"
#include "localisation/string_ids.h"
#include "scenario.h"
#include "ui.h"

#include <fstream>
#include <iterator>
#include <vector>

using namespace openloco::interop;

namespace openloco::tutorial
{
    static loco_global<uint8_t, 0x00508F19> _state;

    // The following two globals are unused, but left here for documentation purposes.
    static loco_global<uint16_t*, 0x009C86FC> _tutorialOffset;
    static loco_global<uint16_t*, 0x009C8704> _tutorialEnd;

    static loco_global<string_id, 0x009C8708> _tutorialString;
    static loco_global<uint8_t, 0x009C870A> _tutorialNumber;

    static std::vector<uint16_t> _tutorialData;
    static std::vector<uint16_t>::const_iterator _tutorialIt;

    constexpr config::resolution_t tutorialResolution = { 1024, 768 };

    tutorial_state state()
    {
        return (tutorial_state)*_state;
    }

    void registerHooks()
    {
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

    static std::vector<uint16_t> readTutorialFile(fs::path filename)
    {
        std::ifstream file(filename, std::ios::in | std::ios::binary);
        file.unsetf(std::ios::skipws);
        if (!file)
            throw;

        std::vector<uint16_t> tutorial;
        auto start = std::istream_iterator<uint8_t>(file);
        auto const end = std::istream_iterator<uint8_t>();

        for (auto it = start; it != end; ++it)
        {
            auto const first_byte = *it;
            ++it;

            // We expect an even number of bytes
            if (it == end)
                throw;

            auto const second_byte = *it;
            tutorial.push_back(second_byte << 8 | first_byte);
        }

        return tutorial;
    }

    // 0x0043C590
    void start(int16_t tutorialNumber)
    {
        if (tutorialNumber < 0 || tutorialNumber > 3)
            return;

        // NB: only used by tutorial widget drawing after.
        _tutorialNumber = tutorialNumber;

        // All destructors must be called prior to calling scenario::start due to the interaction of longjmp.
        // This can be removed when scenerio::start has been implemented.
        {
            // Figure out what dimensions to use for the tutorial, and whether we can continue using scaling.
            const auto& config = config::get_new();
            config::resolution_t newResolution = tutorialResolution;
            if (config.scale_factor > 1.0)
            {
                newResolution *= config.scale_factor;
                config::resolution_t desktopResolution = ui::getDesktopResolution();

                // Don't scale if it means the new window won't fit the desktop.
                if (newResolution > desktopResolution)
                {
                    ui::setWindowScaling(1.0);
                    newResolution = tutorialResolution;
                }
            }

            // Ensure that we're in windowed mode, using dimensions 1024x768.
            if (config.display.mode != config::screen_mode::window || config.display.window_resolution != tutorialResolution)
            {
                if (!ui::setDisplayMode(config::screen_mode::window, tutorialResolution))
                    return;
            }

            // Get the environment file for this tutorial.
            static const environment::path_id tutorialFileIds[] = {
                environment::path_id::tut1024_1,
                environment::path_id::tut1024_2,
                environment::path_id::tut1024_3,
            };

            auto fileId = tutorialFileIds[tutorialNumber];

            auto tutPath = environment::get_path(fileId);
            _tutorialData = readTutorialFile(tutPath);
            _tutorialIt = _tutorialData.cbegin();

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
        }

        scenario::start(-1);

        // Unreachable?
        stop();
    }

    // 0x0043C70E
    void stop()
    {
        *_state = static_cast<uint8_t>(tutorial_state::none);
        gfx::invalidate_screen();
        gui::resize();
    }

    // 0x0043C7A2
    uint16_t nextInput()
    {
        uint16_t next = *_tutorialIt;
        _tutorialIt++;

        if (_tutorialIt == _tutorialData.end())
        {
            stop();
            addr<0x00508F12, uint16_t>() = 0; // screen_age
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
