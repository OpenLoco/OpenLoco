#include "../input.h"
#include "../interop/interop.hpp"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::construction
{
    namespace widx
    {
        enum
        {
            close = 2,
            tab_0 = 4,
            tab_1,
            tab_2,
            tab_3,
            construct = 28,
            remove,
            place,
        };
    }

    // 0x004A3B0D
    window* openWithFlags(const uint32_t flags)
    {
        registers regs;
        regs.ecx = flags;
        call(0x004A3B0D, regs);
        return (window*)regs.esi;
    }

    // 0x0049D3F6
    void on_mouse_up(window& w, const uint16_t widgetIndex)
    {
        // Allow shift key to repeat the action multiple times
        // This is useful for building very long tracks.
        int multiplier = 1;
        if (input::has_key_modifier(input::key_modifier::shift))
        {
            multiplier = 10;
        }

        registers regs;
        regs.edx = widgetIndex;
        regs.esi = (int32_t)&w;
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(&w);
                break;
            case widx::tab_0:
            case widx::tab_1:
            case widx::tab_2:
            case widx::tab_3:
                call(0x0049D93A, regs);
                break;
            case widx::construct:
                for (int i = 0; i < multiplier; i++)
                {
                    call(0x0049F92D, regs);
                }
                break;
            case widx::remove:
                for (int i = 0; i < multiplier; i++)
                {
                    call(0x004A0121, regs);
                }
                break;
            case widx::place:
                call(0x0049D7DC, regs);
                break;
        }
    }
}
