#include "../input.h"
#include "../things/vehicle.h"
#include "../ui.h"
#include "../windowmgr.h"
#include "interop.hpp"

void register_hooks()
{
    using namespace openloco::ui::windows;

    // Replace ui::update() with our own
    register_hook(0x004524C1,
        [](registers &regs) -> uint8_t
        {
            openloco::ui::update();
            return 0;
        });

    register_hook(0x00445AB9,
        [](registers &regs) -> uint8_t
        {
            auto result = prompt_browse(
                (browse_type)regs.al,
                (char *)regs.ecx,
                (const char *)regs.edx,
                (const char *)regs.ebx);
            regs.eax = result ? 1 : 0;
            return 0;
        });

    register_hook(0x00446F6B,
        [](registers &regs) -> uint8_t
        {
            auto result = prompt_ok_cancel(regs.eax);
            regs.eax = result ? 1 : 0;
            return 0;
        });

    register_hook(0x00407218,
        [](registers &regs) -> uint8_t
        {
            openloco::input::sub_407218();
            return 0;
        });
    register_hook(0x00407231,
        [](registers &regs) -> uint8_t
        {
            openloco::input::sub_407231();
            return 0;
        });

    register_hook(0x00498E9B,
        [](registers &regs) -> uint8_t
        {
            openloco::ui::windows::sub_498E9B((openloco::ui::window *)regs.esi);
            return 0;
        });

    register_hook(0x004BA8D4,
        [](registers &regs) -> uint8_t
        {
            auto v = (openloco::vehicle *)regs.esi;
            v->sub_4BA8D4();
            return 0;
        });

    // Remove the set window pos function, we do not want it as it
    // keeps moving the process window to 0, 0
    // Can be removed when windowmgr:update() is hooked
    write_ret(0x00406520);
}
