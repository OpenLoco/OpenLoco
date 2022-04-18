#include "../Audio/Audio.h"
#include "../Game.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

#include <array>
#include <cstring>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::PromptSaveWindow
{
    static loco_global<uint16_t, 0x0050A002> _savePromptType;

    enum widx
    {
        frame,
        caption,
        closeButton,
        promptLabel,
        saveButton,
        dontSaveButton,
        cancelButton,
    };

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 260, 48 }, WidgetType::panel, WindowColour::primary),
        makeWidget({ 1, 1 }, { 258, 13 }, WidgetType::caption_22, WindowColour::primary, StringIds::empty),
        makeWidget({ 247, 2 }, { 11, 11 }, WidgetType::button, WindowColour::primary, StringIds::close_window_cross, StringIds::tooltip_close_window),
        makeWidget({ 2, 17 }, { 256, 12 }, WidgetType::wt_13, WindowColour::primary, StringIds::empty),
        makeWidget({ 8, 33 }, { 78, 12 }, WidgetType::button, WindowColour::primary, StringIds::label_button_save),
        makeWidget({ 91, 33 }, { 78, 12 }, WidgetType::button, WindowColour::primary, StringIds::label_button_dont_save),
        makeWidget({ 174, 33 }, { 78, 12 }, WidgetType::button, WindowColour::primary, StringIds::label_button_cancel),
        widgetEnd(),
    };

    static WindowEventList _events;
    static void initEvents();

    // 0x0043C27E
    Window* open(uint16_t savePromptType)
    {
        auto window = WindowManager::bringToFront(WindowType::saveGamePrompt);
        if (window == nullptr)
        {
            window = WindowManager::createWindowCentred(
                WindowType::saveGamePrompt,
                { 260, 48 },
                WindowFlags::notScrollView | WindowFlags::stickToFront,
                &_events);

            if (window == nullptr)
                return nullptr;

            window->widgets = _widgets;
            window->enabledWidgets = (1 << widx::closeButton) | (1 << widx::saveButton) | (1 << widx::dontSaveButton) | (1 << widx::cancelButton);
            window->initScrollWidgets();
            window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedDarkRed).translucent());
            window->flags |= Ui::WindowFlags::transparent;

            setPauseFlag(1 << 1);
            Audio::pauseSound();
            WindowManager::invalidate(WindowType::timeToolbar);
        }

        _savePromptType = savePromptType;

        initEvents();

        if (!isEditorMode())
        {
            static constexpr std::array<const string_id, 3> typeToType = {
                StringIds::title_load_game,
                StringIds::title_quit_game,
                StringIds::title_quit_game_alt,
            };

            window->widgets[widx::caption].text = typeToType.at(savePromptType);
        }
        else
        {
            if (savePromptType == 0)
                window->widgets[widx::caption].text = StringIds::title_load_landscape;
            else
                window->widgets[widx::caption].text = StringIds::title_quit_scenario_editor;
        }

        static constexpr std::array<const string_id, 3> typeToPrompt = {
            StringIds::prompt_save_before_loading,
            StringIds::prompt_save_before_quitting,
            StringIds::prompt_save_before_quitting_alt,
        };

        window->widgets[widx::promptLabel].text = typeToPrompt.at(savePromptType);

        return window;
    }

    static void draw(Window* const self, Gfx::Context* const context)
    {
        self->draw(context);
    }

    // 0x0043C3F4
    static void onMouseUp(Window* const self, const WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::closeButton:
            case widx::cancelButton:
            {
                GameCommands::do_21(1, 0);
                break;
            }

            case widx::saveButton:
            {
                Game::confirmSaveGame();
                break;
            }

            case widx::dontSaveButton:
            {
                GameCommands::do_21(2, 0);
                break;
            }
        }
    }

    // 0x0043C577
    static void onClose(Window* const self)
    {
        unsetPauseFlag(2);
        Audio::unpauseSound();
        WindowManager::invalidate(WindowType::timeToolbar);
    }

    static void initEvents()
    {
        _events.draw = draw;
        _events.onClose = onClose;
        _events.onMouseUp = onMouseUp;
    }
}
