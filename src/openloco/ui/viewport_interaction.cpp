#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/stringmgr.h"
#include "../stationmgr.h"
#include "../ui.h"
#include "../ui/scrollview.h"
#include "../window.h"
#include "WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::viewport_interaction
{
    // 0x004CD658
    uint8_t get_item_left(int16_t x, int16_t y, void* arg)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;

        call(0x004CD658, regs);
        return regs.bl;
    }

    // 0x004CDB2B
    void right_over(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;

        call(0x004CDB2B, regs);
    }
}
