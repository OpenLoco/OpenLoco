#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{

    // 0x00439236
    static void draw(window* window)
    {
        window->draw_widgets();

        // draw_string_centred_wrapped(uint16_t string_id@<bx>, 16 center_x@<cx>, 16 center_y@<cx>, void* args@<esi>)
        {
            registers regs;
            regx.cx = window->x + window->width / 2;
            regx.dx = window->x + window->widgets[0].top + 8;
            regs.bx = string_ids::title_exit_game;
            // al = 0
            regx.esi = 0x112c826; // common format args
            call(0x00494ECF, regs);
        }
    }

    // 0x00439268
    static void on_mouse_up(window* window, uint16_t widget_index)
    {
        if (intro::state() != intro_state::none)
        {
            return;
        }

        switch (widget_index)
        {
            case 0:
                // do_game_command
                {
                    regs.bl = 1;
                    regs.dl = 0;
                    regs.di = 2;
                    regx.esi = 21;
                    call(0x00431315, regs);
                }
                break;
        }
    }
}
