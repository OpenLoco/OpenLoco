#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringManager.h"
#include "../StationManager.h"
#include "../Ui.h"
#include "../Ui/Scrollview.h"
#include "../Window.h"
#include "WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::viewport_interaction
{
    // 0x004CD658
    InteractionItem getItemLeft(int16_t tempX, int16_t tempY, InteractionArg* arg)
    {
        registers regs;
        regs.ax = tempX;
        regs.bx = tempY;

        call(0x004CD658, regs);
        if (arg != nullptr)
        {
            arg->value = regs.edx;
            arg->x = regs.ax;
            arg->y = regs.cx;
        }
        return static_cast<InteractionItem>(regs.bl);
    }

    // 0x004CDB2B
    InteractionItem rightOver(int16_t x, int16_t y, InteractionArg* arg)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;

        call(0x004CDB2B, regs);
        if (arg != nullptr)
        {
            arg->value = regs.edx;
            arg->x = regs.ax;
            arg->y = regs.cx;
            arg->unkBh = regs.bh;
        }
        return static_cast<InteractionItem>(regs.bl);
    }
}
