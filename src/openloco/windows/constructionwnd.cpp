#include "../input.h"
#include "../interop/interop.hpp"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{
    namespace widget_idx
    {
        constexpr uint16_t close = 2;
        constexpr uint16_t tab_0 = 4;
        constexpr uint16_t tab_1 = 5;
        constexpr uint16_t tab_2 = 6;
        constexpr uint16_t tab_3 = 7;
        constexpr uint16_t construct = 28;
        constexpr uint16_t remove = 29;
        constexpr uint16_t place = 30;
    }

    // 0x0049D3F6
    void construction_mouse_up(window& w, uint16_t widgetIndex)
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
            case widget_idx::close:
                WindowManager::close(&w);
                break;
            case widget_idx::tab_0:
            case widget_idx::tab_1:
            case widget_idx::tab_2:
            case widget_idx::tab_3:
                call(0x0049D93A, regs);
                break;
            case widget_idx::construct:
                for (int i = 0; i < multiplier; i++)
                {
                    call(0x0049F92D, regs);
                }
                break;
            case widget_idx::remove:
                for (int i = 0; i < multiplier; i++)
                {
                    call(0x004A0121, regs);
                }
                break;
            case widget_idx::place:
                call(0x0049D7DC, regs);
                break;
        }
    }
}
