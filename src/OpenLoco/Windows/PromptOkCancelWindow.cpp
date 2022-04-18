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
    static loco_global<uint8_t, 0x009D1C9A> _result;

    static char _descriptionBuffer[512];

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
        makeWidget({ 1, 1 }, { 278, 13 }, WidgetType::caption_22, WindowColour::primary),
        makeWidget({ 267, 2 }, { 11, 11 }, WidgetType::button, WindowColour::primary, StringIds::close_window_cross, StringIds::tooltip_close_window),
        makeWidget({ 20, 77 }, { 100, 12 }, WidgetType::button, WindowColour::primary, StringIds::label_ok),
        makeWidget({ 160, 77 }, { 100, 12 }, WidgetType::button, WindowColour::primary, StringIds::label_button_cancel),
        widgetEnd(),
    };

    static WindowEventList _events;
    static void initEvents();

    // 0x00446F6B
    // eax: okButtonStringId
    // eax: {return}
    bool open(string_id captionId, string_id descriptionId, FormatArguments& descriptionArgs, string_id okButtonStringId)
    {
        initEvents();
        auto window = WindowManager::createWindowCentred(
            WindowType::confirmationPrompt,
            { 280, 92 },
            Ui::WindowFlags::flag_12 | Ui::WindowFlags::stickToFront,
            &_events);

        if (window == nullptr)
            return false;

        window->widgets = _widgets;
        window->widgets[widx::caption].text = captionId;
        window->widgets[widx::okButton].text = okButtonStringId;

        // Prepare description buffer for drawing
        StringManager::formatString(_descriptionBuffer, descriptionId, &descriptionArgs);

        window->enabledWidgets = (1 << widx::closeButton) | (1 << widx::okButton) | (1 << widx::cancelButton);
        window->initScrollWidgets();
        window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedDarkRed).translucent());
        window->setColour(WindowColour::secondary, AdvancedColour(Colour::mutedDarkRed).translucent());
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
        // Prepare description string for drawing.
        char* buffer_2039 = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
        strncpy(&buffer_2039[0], _descriptionBuffer, 512);
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

        FormatArguments args{};
        args.push(StringIds::buffer_2039);

        auto origin = Ui::Point(self->x + self->width / 2, self->y + 41);
        Gfx::drawStringCentredWrapped(*context, origin, self->width, Colour::black, StringIds::wcolour2_stringid, &args);
    }

    static void initEvents()
    {
        _events.draw = draw;
        _events.onMouseUp = onMouseUp;
        _events.prepareDraw = prepareDraw;
    }
}
