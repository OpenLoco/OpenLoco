#include "../Audio/Audio.h"
#include "../Game.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
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

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, { 260, 48 }, widget_type::panel, 0),
        makeWidget({ 1, 1 }, { 258, 13 }, widget_type::caption_22, 0, StringIds::empty),
        makeWidget({ 247, 2 }, { 11, 11 }, widget_type::wt_11, 0, StringIds::close_window_cross, StringIds::tooltip_close_window),
        makeWidget({ 2, 17 }, { 256, 12 }, widget_type::wt_13, 0, StringIds::empty),
        makeWidget({ 8, 33 }, { 78, 12 }, widget_type::wt_11, 0, StringIds::label_button_save),
        makeWidget({ 91, 33 }, { 78, 12 }, widget_type::wt_11, 0, StringIds::label_button_dont_save),
        makeWidget({ 174, 33 }, { 78, 12 }, widget_type::wt_11, 0, StringIds::label_button_cancel),
        widgetEnd(),
    };

    static window_event_list _events;
    static void initEvents();

    // 0x0043C27E
    window* open(uint16_t savePromptType)
    {
        auto window = WindowManager::bringToFront(WindowType::saveGamePrompt);
        if (window == nullptr)
        {
            window = WindowManager::createWindowCentred(
                WindowType::saveGamePrompt,
                { 260, 48 },
                WindowFlags::not_scroll_view | WindowFlags::stick_to_front,
                &_events);

            if (window == nullptr)
                return nullptr;

            window->widgets = _widgets;
            window->enabled_widgets = (1 << widx::closeButton) | (1 << widx::saveButton) | (1 << widx::dontSaveButton) | (1 << widx::cancelButton);
            window->initScrollWidgets();
            window->colours[0] = Colour::translucent(Colour::salmon_pink);
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

    static void draw(window* const self, Gfx::drawpixelinfo_t* const dpi)
    {
        self->draw(dpi);
    }

    // 0x0043C3F4
    static void onMouseUp(window* const self, const widget_index widgetIndex)
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
    static void onClose(window* const self)
    {
        unsetPauseFlag(2);
        Audio::unpauseSound();
        WindowManager::invalidate(WindowType::timeToolbar);
    }

    static void initEvents()
    {
        _events.draw = draw;
        _events.on_close = onClose;
        _events.on_mouse_up = onMouseUp;
    }
}
