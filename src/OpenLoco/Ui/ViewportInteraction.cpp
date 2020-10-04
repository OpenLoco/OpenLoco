#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringManager.h"
#include "../StationManager.h"
#include "../Ui.h"
#include "../Ui/ScrollView.h"
#include "../Window.h"
#include "WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::ViewportInteraction
{
    // 0x004CD658
    InteractionArg getItemLeft(int16_t tempX, int16_t tempY)
    {
        registers regs;
        regs.ax = tempX;
        regs.bx = tempY;

        call(0x004CD658, regs);
        InteractionArg result;
        result.value = regs.edx;
        result.x = regs.ax;
        result.y = regs.cx;
        result.unkBh = regs.bh;
        result.type = static_cast<InteractionItem>(regs.bl);

        return result;
    }

    // 0x004CDB2B
    InteractionArg rightOver(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;

        call(0x004CDB2B, regs);
        InteractionArg result;
        result.value = regs.edx;
        result.x = regs.ax;
        result.y = regs.cx;
        result.unkBh = regs.bh;
        result.type = static_cast<InteractionItem>(regs.bl);

        return result;
    }
}
