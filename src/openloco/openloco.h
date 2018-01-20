#pragma once

#include <cstdint>
#include <functional>

namespace openloco
{
    namespace screen_flags
    {
        constexpr uint8_t title = 1 << 0;
        constexpr uint8_t editor = 1 << 1;
        constexpr uint8_t unknown_2 = 1 << 2;
    }

    void* hInstance();
    const char* lpCmdLine();
    void lpCmdLine(const char* path);
    bool is_editor_mode();
    bool is_title_mode();
    bool is_paused();
    uint32_t scenario_ticks();
    void main();

    void prompt_tick_loop(std::function<bool()> tickAction);
    void sub_48A18C();
    uint32_t rand_next();
    int32_t rand_next(int32_t high);
    int32_t rand_next(int32_t low, int32_t high);
}
