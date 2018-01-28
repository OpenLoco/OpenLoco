#pragma once

#include "utility/prng.hpp"
#include <cstdint>
#include <functional>

namespace openloco
{
    namespace screen_flags
    {
        constexpr uint8_t title = 1 << 0;     // 1
        constexpr uint8_t editor = 1 << 1;    // 2
        constexpr uint8_t unknown_2 = 1 << 2; // 4
        constexpr uint8_t unknown_3 = 1 << 3; // 8
        constexpr uint8_t unknown_4 = 1 << 4; // 0x10
        constexpr uint8_t unknown_5 = 1 << 5; // 0x20
        constexpr uint8_t unknown_6 = 1 << 6; // 0x40
    }

    void* hInstance();
    const char* lpCmdLine();
    void lpCmdLine(const char* path);
    uint8_t get_screen_flags();
    bool is_editor_mode();
    bool is_title_mode();
    bool is_paused();
    uint32_t scenario_ticks();
    utility::prng& gprng();

    void main();
    void prompt_tick_loop(std::function<bool()> tickAction);
    void sub_48A18C();
}
