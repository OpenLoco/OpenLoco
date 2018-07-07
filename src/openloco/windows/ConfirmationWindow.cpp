#include "../graphics/colours.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../ui.h"
#include "../ui/WindowManager.h"
#include <cstring>

using namespace openloco::interop;
using namespace openloco::ui;

namespace openloco::windows::ConfirmationWindow
{
#pragma pack(push, 1)
    struct text_buffers_t
    {
        char title[512];
        char description[512];
    };
#pragma pack(pop)

    loco_global<ui::widget_t[6], 0x0050AE00> _widgets;

    loco_global<text_buffers_t*, 0x009D1078> _text_buffers;
    loco_global<uint8_t, 0x009D1C9A> _result;

    loco_global<char[512], 0x0112CC04> byte_112CC04;
    loco_global<char[512], 0x0112CE04> byte_112CE04;

    // 0x00446F6B
    // eax: okButtonStringId
    // eax: {return}
    bool open(string_id okButtonStringId)
    {
        text_buffers_t buffers;
        _widgets[3].text = okButtonStringId;
        std::memcpy(buffers.title, byte_112CC04, 512);
        std::memcpy(buffers.description, byte_112CE04, 512);
        _text_buffers = &buffers;

        auto window = WindowManager::createWindowCentred(WindowType::confirmation, 280, 92, ui::WindowFlags::flag_12 | ui::WindowFlags::stickToFront, (ui::window_event_list*)0x004FB37C);
        if (window != nullptr)
        {
            window->widgets = &_widgets[0];
            window->setEnabledWidgets(2, 3, 4);
            window->initScrollWidgets();
            window->colours[0] = colour::translucent(colour::salmon_pink);
            window->colours[1] = colour::translucent(colour::salmon_pink);
            window->flags |= ui::WindowFlags::transparent;
            _result = 0;

            auto originalModal = WindowManager::getCurrentModalType();
            WindowManager::setCurrentModalType(WindowType::confirmation);
            prompt_tick_loop(
                []() {
                    input::handle_keyboard();
                    sub_48A18C();
                    WindowManager::dispatchUpdateAll();
                    call(0x004BEC5B);
                    WindowManager::update();
                    call(0x004C98CF);
                    call(0x004CF63B);
                    return WindowManager::find(WindowType::confirmation) != nullptr;
                });
            WindowManager::setCurrentModalType(originalModal);
            return _result != 0;
        }
        return false;
    }
}
