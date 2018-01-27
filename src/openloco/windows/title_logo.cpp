#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{

    // 0x00439298
    static void draw(window* window)
    {
        // draw image(uint32_t image@<ebx>, 16 x@<cx>, 16 y@<dx>)
        registers regs;
        regs.ebx = 3624;
        regx.cx = window->x;
        regx.dx = window->y;
        call(0x00448C79, regs);
    }

    // 0x004392AD
    static void on_mouse_up(window* window, uint16_t widget)
    {
        switch (widget)
        {
            case 0:
                open_about_window();
                break;
        }
    }
}
