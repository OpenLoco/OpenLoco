#include "../Audio/Audio.h"
#include "../Graphics/Colour.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

#include <SDL2/SDL.h>
#include <cstring>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::PromptOkCancel
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

    enum widx
    {
        frame,
        caption,
        closeButton,
        okButton,
        cancelButton,
    };

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 280, 92 }, WidgetType::panel, WindowColour::primary),
        makeWidget({ 1, 1 }, { 278, 13 }, WidgetType::caption_22, WindowColour::primary, StringIds::buffer_2039),
        makeWidget({ 267, 2 }, { 11, 11 }, WidgetType::wt_11, WindowColour::primary, StringIds::close_window_cross, StringIds::tooltip_close_window),
        makeWidget({ 20, 77 }, { 100, 12 }, WidgetType::wt_11, WindowColour::primary, StringIds::label_ok),
        makeWidget({ 160, 77 }, { 100, 12 }, WidgetType::wt_11, WindowColour::primary, StringIds::label_button_cancel),
        widgetEnd(),
    };

    static WindowEventList _events;
    static void initEvents();

    // 0x00446F6B
    // eax: okButtonStringId
    // eax: {return}
    bool open(string_id okButtonStringId)
    {
        text_buffers_t buffers;
        std::memcpy(buffers.title, byte_112CC04, 512);
        std::memcpy(buffers.description, byte_112CE04, 512);
        _text_buffers = &buffers;

        initEvents();
        auto window = WindowManager::createWindowCentred(
            WindowType::confirmationPrompt,
            { 280, 92 },
            Ui::WindowFlags::flag_12 | Ui::WindowFlags::stick_to_front,
            &_events);

        if (window == nullptr)
            return false;

        window->widgets = _widgets;
        window->widgets[okButton].text = okButtonStringId;

        window->enabled_widgets = (1 << widx::closeButton) | (1 << widx::okButton) | (1 << widx::cancelButton);
        window->initScrollWidgets();
        window->setColour(WindowColour::primary, Colour::translucent(Colour::salmon_pink));
        window->setColour(WindowColour::secondary, Colour::translucent(Colour::salmon_pink));
        window->flags |= Ui::WindowFlags::transparent;

        _result = 0;

        auto originalModal = WindowManager::getCurrentModalType();
        WindowManager::setCurrentModalType(WindowType::confirmationPrompt);
        promptTickLoop(
            []() {
                Input::handleKeyboard();
                Audio::updateSounds();
                WindowManager::dispatchUpdateAll();
                Input::processKeyboardInput();
                WindowManager::update();
                Ui::minimalHandleInput();
                Gfx::render();
                return WindowManager::find(WindowType::confirmationPrompt) != nullptr;
            });
        WindowManager::setCurrentModalType(originalModal);

        return _result != 0;
    }

    // 0x00447125
    void handleInput(uint32_t charCode, uint32_t keyCode)
    {
        auto window = WindowManager::find(WindowType::confirmationPrompt);
        if (window == nullptr)
            return;

        if (keyCode == SDLK_ESCAPE)
            window->callOnMouseUp(widx::closeButton);
    }

    // 0x00447093
    static void prepareDraw(Window* const self)
    {
        // Prepare title string for drawing.
        char* buffer_2039 = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
        strncpy(&buffer_2039[0], (*_text_buffers)->title, 512);
    }

    // 0x004470FD
    static void onMouseUp(Window* const self, const WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::closeButton:
            case widx::cancelButton:
                WindowManager::close(self->type);
                break;

            case widx::okButton:
                _result = 1;
                WindowManager::close(self->type);
                break;
        }
    }

    // 0x004470AA
    static void draw(Window* const self, Gfx::Context* const context)
    {
        self->draw(context);

        // Prepare description string for drawing.
        char* buffer_2039 = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
        strncpy(&buffer_2039[0], (*_text_buffers)->description, 512);

        FormatArguments args{};
        args.push(StringIds::buffer_2039);

        auto origin = Gfx::point_t(self->x + self->width / 2, self->y + 41);
        Gfx::drawStringCentredWrapped(context, &origin, self->width, Colour::black, StringIds::wcolour2_stringid, &args);
    }

    static void initEvents()
    {
        _events.draw = draw;
        _events.on_mouse_up = onMouseUp;
        _events.prepare_draw = prepareDraw;
    }
}
