#include "../interop/interop.hpp"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::terraform
{
    namespace common
    {
        enum widx
        {
            frame = 0,
            caption = 1,
            close_button = 2,
            panel = 3,
            tab_clear_area,
            tab_adjust_land,
            tab_adjust_water,
            tab_plant_trees,
            tab_build_walls,
        };
    }

    window* open()
    {
        registers regs;
        call(0x004BB4A3, regs);
        return (window*)(uintptr_t)regs.esi;
    }

    // 0x004BB566
    void open_clear_area()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(common::widx::tab_clear_area);
    }

    // 0x004BB546
    void open_adjust_land()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(common::widx::tab_adjust_land);
    }

    // 0x004BB556
    void open_adjust_water()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(common::widx::tab_adjust_water);
    }

    // 0x004BB4A3
    void open_plant_trees()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(common::widx::tab_plant_trees);
    }

    // 0x004BB576
    void open_build_walls()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(common::widx::tab_build_walls);
    }
}
