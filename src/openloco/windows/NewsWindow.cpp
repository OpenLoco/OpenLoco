#include "../game_commands.h"
#include "../interop/interop.hpp"
#include "../message.h"
#include "../messagemgr.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"

namespace openloco::ui::NewsWindow
{
    // 0x00428F8B
    void open(uint16_t messageIndex)
    {
        registers regs;
        regs.ax = messageIndex;
        call(0x00428F8B, regs);
    }
}
