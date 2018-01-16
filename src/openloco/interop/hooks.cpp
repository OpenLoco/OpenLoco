#include "../environment.h"
#include "../graphics/gfx.h"
#include "../input.h"
#include "../station.h"
#include "../things/vehicle.h"
#include "../ui.h"
#include "../windowmgr.h"
#include "interop.hpp"

using namespace openloco;

void openloco::interop::register_hooks()
{
    using namespace openloco::ui::windows;

    register_hook(0x004416B5,
        [](registers &regs) -> uint8_t
        {
            using namespace openloco::environment;

            auto buffer = (char *)0x009D0D72;
            auto path = get_path((path_id)regs.ebx);
            std::strcpy(buffer, path.make_preferred().u8string().c_str());
            regs.ebx = (int32_t)buffer;
            return 0;
        });

    // Replace ui::update() with our own
    register_hook(0x004524C1,
        [](registers &regs) -> uint8_t
        {
            ui::update();
            return 0;
        });

    register_hook(0x00407BA3,
        [](registers &regs) -> uint8_t
        {
            auto cursor = (ui::cursor_id)regs.edx;
            ui::set_cursor(cursor);
            return 0;
        });
    register_hook(0x004CF142,
        [](registers &regs) -> uint8_t
        {
            ui::set_cursor(ui::cursor_id::blank);
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

    register_hook(0x00492793,
        [](registers &regs) -> uint8_t
        {
            auto station = (openloco::station *)regs.esi;
            regs.al = (station->update_cargo() != 0);
            return 0;
        });

    register_hook(0x0049D3F6,
        [](registers &regs) -> uint8_t
        {
            ui::windows::construction_mouse_up(*((ui::window *)regs.esi), regs.dx);
            return 0;
        });

    register_hook(0x0048ED2F,
        [](registers &regs) -> uint8_t
        {
            ui::windows::station_2_scroll_paint(
                *((ui::window *)regs.esi),
                *((gfx::drawpixelinfo_t *)regs.edi));
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

    // Remove check for is road in use when removing roads. It is
    // quite annoying when it's sometimes only the player's own
    // vehicles that are using it.
    write_nop(0x004776DD, 6);
}
