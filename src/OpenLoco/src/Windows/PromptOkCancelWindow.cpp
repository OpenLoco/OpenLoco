#include "Audio/Audio.h"
#include "Graphics/Colour.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "OpenLoco.h"
#include "Ui.h"
#include "Ui/Widget.h"
#include "Ui/WindowManager.h"

#include <SDL2/SDL.h>
#include <cstring>

namespace OpenLoco::Ui::Windows::PromptOkCancel
{
    static char _descriptionBuffer[512];
    static bool _result; // 0x009D1C9A

    enum widx
    {
        frame,
        caption,
        closeButton,
        okButton,
        cancelButton,
    };

    static constexpr Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 280, 92 }, WidgetType::panel, WindowColour::primary),
        makeWidget({ 1, 1 }, { 278, 13 }, WidgetType::caption_22, WindowColour::primary),
        makeWidget({ 267, 2 }, { 11, 11 }, WidgetType::button, WindowColour::primary, StringIds::close_window_cross, StringIds::tooltip_close_window),
        makeWidget({ 20, 77 }, { 100, 12 }, WidgetType::button, WindowColour::primary, StringIds::label_ok),
        makeWidget({ 160, 77 }, { 100, 12 }, WidgetType::button, WindowColour::primary, StringIds::label_button_cancel),
        widgetEnd(),
    };

    static const WindowEventList& getEvents();

    // 0x00446F6B
    // eax: okButtonStringId
    // eax: {return}
    bool open(StringId captionId, StringId descriptionId, FormatArguments& descriptionArgs, StringId okButtonStringId)
    {
        auto window = WindowManager::createWindowCentred(
            WindowType::confirmationPrompt,
            { 280, 92 },
            Ui::WindowFlags::flag_12 | Ui::WindowFlags::stickToFront,
            getEvents());

        if (window == nullptr)
            return false;

        window->setWidgets(_widgets);
        window->widgets[widx::caption].text = captionId;
        window->widgets[widx::okButton].text = okButtonStringId;

        // Prepare description buffer for drawing
        StringManager::formatString(_descriptionBuffer, descriptionId, descriptionArgs);

        window->enabledWidgets = (1 << widx::closeButton) | (1 << widx::okButton) | (1 << widx::cancelButton);
        window->initScrollWidgets();
        window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedDarkRed).translucent());
        window->setColour(WindowColour::secondary, AdvancedColour(Colour::mutedDarkRed).translucent());
        window->flags |= Ui::WindowFlags::transparent;

        _result = false;

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
                Gfx::renderAndUpdate();
                return WindowManager::find(WindowType::confirmationPrompt) != nullptr;
            });
        WindowManager::setCurrentModalType(originalModal);

        return _result;
    }

    // 0x00447125
    static bool keyUp(Window& w, [[maybe_unused]] uint32_t charCode, uint32_t keyCode)
    {
        if (keyCode == SDLK_ESCAPE)
        {
            w.callOnMouseUp(widx::closeButton);
            return true;
        }
        return false;
    }

    // 0x00447093
    static void prepareDraw([[maybe_unused]] Window& self)
    {
        // Prepare description string for drawing.
        char* buffer_2039 = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
        strncpy(&buffer_2039[0], _descriptionBuffer, 512);
    }

    // 0x004470FD
    static void onMouseUp(Window& self, const WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::closeButton:
            case widx::cancelButton:
                WindowManager::close(self.type);
                break;

            case widx::okButton:
                _result = true;
                WindowManager::close(self.type);
                break;
        }
    }

    // 0x004470AA
    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        self.draw(drawingCtx);

        FormatArguments args{};
        args.push(StringIds::buffer_2039);

        auto origin = Ui::Point(self.x + self.width / 2, self.y + 41);
        tr.drawStringCentredWrapped(origin, self.width, Colour::black, StringIds::wcolour2_stringid, args);
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .prepareDraw = prepareDraw,
        .draw = draw,
        .keyUp = keyUp,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
