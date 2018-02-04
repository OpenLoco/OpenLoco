#include "../graphics/colours.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui.h"
#include "../windowmgr.h"
#include <cstring>

using namespace openloco::interop;

namespace openloco::ui::windows
{
#pragma pack(push, 1)
    struct text_buffers_t
    {
        char title[512];
        char description[512];
    };
#pragma pack(pop)

    loco_global<string_id, 0x0050AE3A> _ok_button_string_id;
    loco_global<text_buffers_t*, 0x009D1078> _text_buffers;
    loco_global<uint8_t, 0x009D1C9A> _result;

    loco_global_array<char, 512, 0x0112CC04> byte_112CC04;
    loco_global_array<char, 512, 0x0112CE04> byte_112CE04;

    // 0x00446F6B
    // eax: okButtonStringId
    // eax: {return}
    bool prompt_ok_cancel(string_id okButtonStringId)
    {
        text_buffers_t buffers;
        _ok_button_string_id = okButtonStringId;
        std::memcpy(buffers.title, byte_112CC04, 512);
        std::memcpy(buffers.description, byte_112CE04, 512);
        _text_buffers = &buffers;

        auto window = windowmgr::create_window_centred(window_type::prompt_ok_cancel, 280, 92, ui::window_flags::flag_12 | ui::window_flags::flag_1, (void*)0x004FB37C);
        if (window != nullptr)
        {
            window->widgets = (widget*)0x0050AE00;
            window->enabled_widgets = (1 << 2) | (1 << 3) | (1 << 4);
            window->sub_4CA17F();
            window->colours[0] = colour::translucent(colour::salmon_pink);
            window->colours[1] = colour::translucent(colour::salmon_pink);
            window->flags |= ui::window_flags::flag_4;
            _result = 0;

            auto originalModal = windowmgr::current_modal_type();
            windowmgr::current_modal_type(window_type::prompt_ok_cancel);
            prompt_tick_loop(
                []() {
                    input::handle_keyboard();
                    sub_48A18C();
                    call(0x004CD3D0);
                    call(0x004BEC5B);
                    windowmgr::update();
                    call(0x004C98CF);
                    call(0x004CF63B);
                    return windowmgr::find(window_type::prompt_ok_cancel) != nullptr;
                });
            windowmgr::current_modal_type(originalModal);
            return _result != 0;
        }
        return false;
    }
}
