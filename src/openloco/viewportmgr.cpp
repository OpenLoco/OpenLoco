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
    loco_global<viewport[max_viewports], 0x0113D758> _viewports;
    loco_global<viewport * [max_viewports], 0x0113D820> _viewportPointers;

    void init()
    {
        for (size_t i = 0; i < max_viewports; i++)
        {
            _viewports[i].width = 0;
        }

        _viewportPointers[0] = nullptr;
    }

    std::array<viewport*, max_viewports> viewports()
    {
        auto arr = (std::array<viewport*, max_viewports>*)_viewportPointers.get();
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
