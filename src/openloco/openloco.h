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
        constexpr uint8_t networked = 1 << 2;
        constexpr uint8_t unknown_3 = 1 << 3;
        constexpr uint8_t unknown_4 = 1 << 4;
        constexpr uint8_t unknown_5 = 1 << 5;
        constexpr uint8_t unknown_6 = 1 << 6;
    }

#pragma pack(push, 1)
    struct loco_ptr
    {
        uintptr_t _ptr;
        loco_ptr(const void* x)
        {
            _ptr = reinterpret_cast<uintptr_t>(x);
        }
        loco_ptr(int32_t x)
        {
            _ptr = x;
        }
        operator int32_t() const { assert(_ptr < UINT32_MAX); return (int32_t)_ptr; }
        operator uint32_t() const { assert(_ptr < UINT32_MAX);  return (uint32_t)_ptr; }
        operator uintptr_t() const { return _ptr; }
    };
#pragma pack(pop)

    extern const char version[];

    std::string get_version_info();

    void* hInstance();
    const char* lpCmdLine();
    void lpCmdLine(const char* path);
    uint8_t get_screen_flags();
    bool is_editor_mode();
    bool is_title_mode();
    bool isNetworked();
    bool is_unknown_4_mode();
    bool is_paused();
    uint8_t get_pause_flags();
    uint32_t scenario_ticks();
    utility::prng& gprng();
    void initialise_viewports();

    void sub_431695(uint16_t var_F253A0);
    void main();
    void prompt_tick_loop(std::function<bool()> tickAction);
}
