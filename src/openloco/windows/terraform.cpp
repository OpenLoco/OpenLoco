#include "../interop/interop.hpp"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::terraform
{
    window* open()
    {
        registers regs;
        call(0x004BB4A3, regs);
        return (window*)regs.esi;
    }

    // 0x004BB566
    void open_clear_area()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(4);
    }

    // 0x004BB546
    void open_adjust_land()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(5);
    }

    // 0x004BB556
    void open_adjust_water()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(6);
    }

    // 0x004BB4A3
    void open_plant_trees()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(7);
    }

    // 0x004BB576
    void open_build_walls()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(8);
    }
}
