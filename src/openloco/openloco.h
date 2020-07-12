#pragma once

#include "utility/prng.hpp"
#include <cstdint>
#include <functional>
#include <string>

namespace openloco
{
    using string_id = uint16_t;
    namespace screen_flags
    {
        constexpr uint8_t title = 1 << 0;
        constexpr uint8_t editor = 1 << 1;
        constexpr uint8_t networked = 1 << 2;
        constexpr uint8_t trackUpgrade = 1 << 3;
        constexpr uint8_t unknown_4 = 1 << 4;
        constexpr uint8_t unknown_5 = 1 << 5;
        constexpr uint8_t unknown_6 = 1 << 6;
    }

    extern const char version[];

    std::string get_version_info();

    void* hInstance();
    const char* lpCmdLine();
    void lpCmdLine(const char* path);
    uint8_t get_screen_flags();
    bool is_editor_mode();
    bool is_title_mode();
    bool isNetworked();
    bool isTrackUpgradeMode();
    bool is_unknown_4_mode();
    bool is_unknown_5_mode();
    bool is_paused();
    uint8_t get_pause_flags();
    void toggle_paused(bool value);
    uint32_t scenario_ticks();
    utility::prng& gprng();
    void initialise_viewports();

    void sub_431695(uint16_t var_F253A0);
    void main();
    void prompt_tick_loop(std::function<bool()> tickAction);
    void exit_with_error(openloco::string_id message, uint32_t errorCode);
}
