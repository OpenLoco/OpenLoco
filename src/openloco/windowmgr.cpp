#include "windowmgr.h"
#include "interop/interop.hpp"
#include "ui.h"
#include <algorithm>

using namespace openloco::interop;

namespace openloco::ui::windowmgr
{
    namespace find_flag
    {
        constexpr uint16_t by_type = 1 << 7;
    }

    loco_global<uint8_t, 0x005233B6> _current_modal_type;
    loco_global<window[12], 0x011370AC> _windows;
    loco_global<window*, 0x0113D754> _windows_end;

    window* get(size_t index)
    {
        return &_windows.get()[index];
    }

    size_t num_windows()
    {
        return ((uintptr_t)*_windows_end - (uintptr_t)_windows.get()) / sizeof(window);
    }

    window_type current_modal_type()
    {
        return (window_type)*_current_modal_type;
    }

    void current_modal_type(window_type type)
    {
        _current_modal_type = (uint8_t)type;
    }

    // 0x004C6118
    void update()
    {
        call(0x004C6118);
    }

    // 0x004392BD
    void resize()
    {
        call(0x004392BD);
    }

    // 0x004C9B56
    window* find(window_type type)
    {
        registers regs;
        regs.cx = (uint8_t)type | find_flag::by_type;
        call(0x004C9B56, regs);
        return (window*)regs.esi;
    }

    // 0x004C9B56
    window* find(window_type type, uint16_t id)
    {
        registers regs;
        regs.cl = (uint8_t)type;
        regs.dx = id;
        call(0x004C9B56, regs);
        return (window*)regs.esi;
    }

    // 0x004CC692
    void close(window_type type)
    {
        registers regs;
        regs.cx = (uint8_t)type | find_flag::by_type;
        call(0x004CC692, regs);
    }

    // 0x004CC692
    void close(window_type type, uint16_t id)
    {
        registers regs;
        regs.cl = (uint8_t)type;
        regs.dx = id;
        call(0x004CC692, regs);
    }

    // 0x004C9F5D
    window* create_window(
        window_type type,
        int32_t x,
        int32_t y,
        int32_t width,
        int32_t height,
        int32_t flags,
        void* events)
    {
        registers regs;
        regs.eax = (y << 16) | (x & 0xFFFF);
        regs.ebx = (height << 16) | (width & 0xFFFF);
        regs.ecx = (uint8_t)type | (flags << 8);
        regs.edx = (int32_t)events;
        call(0x004C9F5D, regs);
        return (window*)regs.esi;
    }

    window* create_window_centred(window_type type, int32_t width, int32_t height, int32_t flags, void* events)
    {
        auto x = (ui::width() / 2) - (width / 2);
        auto y = std::max(28, (ui::height() / 2) - (height / 2));
        return create_window(type, x, y, width, height, flags, events);
    }
}
