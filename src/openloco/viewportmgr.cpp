#include "viewportmgr.h"
#include "interop/interop.hpp"
#include "ui.h"
#include "window.h"
#include <algorithm>

using namespace openloco::ui;
using namespace openloco::interop;

namespace openloco::ui::viewportmgr
{
    loco_global_array<viewport*, max_viewports, 0x0113D820> _viewports;

    std::array<viewport*, max_viewports> viewports()
    {
        auto arr = (std::array<viewport*, max_viewports>*)_viewports.get();
        return *arr;
    }

    void create(window* window, int16_t x, int16_t y, uint16_t width, uint16_t height)
    {
        registers regs;
        regs.edx = 0x17FF17FF;
        regs.eax = (y << 16) | x;
        regs.ebx = (height << 16) | width;
        regs.ecx = 0x1e00000;
        regs.esi = (uint32_t)window;
        addr<0x00e3f0b8, int32_t>() = 0; // gCurrentRotation?
        call(0x004ca2d0, regs);
    }
}
