#include "../Audio/Audio.h"
#include "../Graphics/Colour.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include <cstring>

using namespace OpenLoco::interop;

namespace OpenLoco::ui::windows
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

    loco_global<char[512], 0x0112CC04> byte_112CC04;
    loco_global<char[512], 0x0112CE04> byte_112CE04;

    // 0x00446F6B
    // eax: okButtonStringId
    // eax: {return}
    bool promptOkCancel(string_id okButtonStringId)
    {
        text_buffers_t buffers;
        _ok_button_string_id = okButtonStringId;
        std::memcpy(buffers.title, byte_112CC04, 512);
        std::memcpy(buffers.description, byte_112CE04, 512);
        _text_buffers = &buffers;

        auto window = WindowManager::createWindowCentred(
            WindowType::confirmationPrompt,
            { 280, 92 },
            ui::window_flags::flag_12 | ui::window_flags::stick_to_front,
            (ui::window_event_list*)0x004FB37C);
        if (window != nullptr)
        {
            window->widgets = (widget_t*)0x0050AE00;
            window->enabled_widgets = (1 << 2) | (1 << 3) | (1 << 4);
            window->initScrollWidgets();
            window->colours[0] = colour::translucent(colour::salmon_pink);
            window->colours[1] = colour::translucent(colour::salmon_pink);
            window->flags |= ui::window_flags::transparent;
            _result = 0;

            auto originalModal = WindowManager::getCurrentModalType();
            WindowManager::setCurrentModalType(WindowType::confirmationPrompt);
            promptTickLoop(
                []() {
                    input::handleKeyboard();
                    audio::updateSounds();
                    WindowManager::dispatchUpdateAll();
                    input::processKeyboardInput();
                    WindowManager::update();
                    ui::minimalHandleInput();
                    gfx::render();
                    return WindowManager::find(WindowType::confirmationPrompt) != nullptr;
                });
            WindowManager::setCurrentModalType(originalModal);
            return _result != 0;
        }
        return false;
    }
}
