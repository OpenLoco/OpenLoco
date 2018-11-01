#pragma once

#include "utility/prng.hpp"
#include <cstdint>
#include <functional>
#include <string>

namespace openloco
{
    namespace screen_flags
    {
        constexpr uint8_t title = 1 << 0;
        constexpr uint8_t editor = 1 << 1;
        constexpr uint8_t unknown_2 = 1 << 2;
        constexpr uint8_t unknown_3 = 1 << 3;
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
    bool is_unknown_4_mode();
    bool is_paused();
    uint32_t scenario_ticks();
    utility::prng& gprng();
    void initialise_viewports();

    void main();
    void prompt_tick_loop(std::function<bool()> tickAction);
}
