#include "viewportmgr.h"
#include "interop/interop.hpp"
#include "things/thing.h"
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

    /* 0x004CA2D0
     * ax : x
     * eax >> 16 : y
     * bx : width
     * ebx >> 16 : height
     * cl : zoom
     * edx >> 14 : flags (bit 30 zoom related, bit 31 set if thing_id used)
     * Optional one of 2
     * 1.
     * ecx >> 16 : tile_z
     * dx : tile_x
     * edx >> 16 : tile_y
     * 2.
     * dx : thing_id
     */
    void create(window* window, int16_t x, int16_t y, uint16_t width, uint16_t height, bool zoom_flag, uint8_t zoom, uint16_t thing_id)
    {
        registers regs;
        regs.dx = (zoom_flag ? ((1 << 30) | (1 << 31)) : (1 << 31)) | thing_id;
        regs.eax = (y << 16) | x;
        regs.ebx = (height << 16) | width;
        regs.cl = zoom;
        regs.esi = (uint32_t)window;
        call(0x004ca2d0, regs);
    }

    /* 0x004CA2D0
     * ax : x
     * eax >> 16 : y
     * bx : width
     * ebx >> 16 : height
     * cl : zoom
     * edx >> 14 : flags (bit 30 zoom related, bit 31 set if thing_id used)
     * Optional one of 2
     * 1.
     * ecx >> 16 : tile_z
     * dx : tile_x
     * edx >> 16 : tile_y
     * 2.
     * dx : thing_id
     */
    void create(window* window, int16_t x, int16_t y, uint16_t width, uint16_t height, bool zoom_flag, uint8_t zoom, uint16_t tile_x, uint16_t tile_y, uint16_t tile_z)
    {
        registers regs;
        regs.edx = (tile_y << 16) | tile_x;
        regs.ecx = (tile_z << 16) | zoom;

        regs.eax = (y << 16) | x;
        regs.ebx = (height << 16) | width;
        regs.edx |= zoom_flag ? (1 << 30) : 0;
        regs.esi = (uint32_t)window;
        call(0x004ca2d0, regs);
    }
}
