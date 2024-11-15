#include "Audio/Audio.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/LoadSaveQuit.h"
#include "Graphics/Colour.h"
#include "Input.h"
#include "Localisation/StringIds.h"
#include "SceneManager.h"
#include "Ui.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>

#include <array>
#include <cstring>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::PromptSaveWindow
{
    static loco_global<LoadOrQuitMode, 0x0050A002> _savePromptType;

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

    static constexpr auto _widgets = makeWidgets(
        Widgets::Panel({ 0, 0 }, { 260, 48 }, WindowColour::primary),
        makeWidget({ 1, 1 }, { 258, 13 }, WidgetType::caption_22, WindowColour::primary, StringIds::empty),
        Widgets::Button({ 247, 2 }, { 11, 11 }, WindowColour::primary, StringIds::close_window_cross, StringIds::tooltip_close_window),
        Widgets::Label({ 2, 17 }, { 256, 12 }, WindowColour::primary, ContentAlign::Center, StringIds::empty),
        Widgets::Button({ 8, 33 }, { 78, 12 }, WindowColour::primary, StringIds::label_button_save),
        Widgets::Button({ 91, 33 }, { 78, 12 }, WindowColour::primary, StringIds::label_button_dont_save),
        Widgets::Button({ 174, 33 }, { 78, 12 }, WindowColour::primary, StringIds::label_button_cancel)

    );

    static const WindowEventList& getEvents();

    // 0x0043C27E
    Window* open(LoadOrQuitMode savePromptType)
    {
        auto window = WindowManager::bringToFront(WindowType::saveGamePrompt);
        if (window == nullptr)
        {
            window = WindowManager::createWindowCentred(
                WindowType::saveGamePrompt,
                { 260, 48 },
                WindowFlags::notScrollView | WindowFlags::stickToFront,
                getEvents());

            if (window == nullptr)
            {
                return nullptr;
            }

            window->setWidgets(_widgets);
            window->enabledWidgets = (1 << widx::closeButton) | (1 << widx::saveButton) | (1 << widx::dontSaveButton) | (1 << widx::cancelButton);
            window->initScrollWidgets();
            window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedDarkRed).translucent());
            window->flags |= Ui::WindowFlags::transparent;

            setPauseFlag(1 << 1);
            Audio::pauseSound();
            WindowManager::invalidate(WindowType::timeToolbar);
        }

        _savePromptType = savePromptType;

        if (!isEditorMode())
        {
            static constexpr std::array<const StringId, 3> kTypeToType = {
                StringIds::title_load_game,
                StringIds::title_quit_game,
                StringIds::title_quit_game_alt,
            };

            window->widgets[widx::caption].text = kTypeToType.at(enumValue(savePromptType));
        }
        else
        {
            if (savePromptType == LoadOrQuitMode::loadGamePrompt)
            {
                window->widgets[widx::caption].text = StringIds::title_load_landscape;
            }
            else
            {
                window->widgets[widx::caption].text = StringIds::title_quit_scenario_editor;
            }
        }

        static constexpr std::array<const StringId, 3> kTypeToPrompt = {
            StringIds::prompt_save_before_loading,
            StringIds::prompt_save_before_quitting,
            StringIds::prompt_save_before_quitting_alt,
        };

        window->widgets[widx::promptLabel].text = kTypeToPrompt.at(enumValue(savePromptType));

        return window;
    }

    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        self.draw(drawingCtx);
    }

    // 0x0043C3F4
    static void onMouseUp([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::closeButton:
            case widx::cancelButton:
            {
                GameCommands::LoadSaveQuitGameArgs args{};
                args.option1 = GameCommands::LoadSaveQuitGameArgs::Options::closeSavePrompt;
                args.option2 = LoadOrQuitMode::loadGamePrompt;
                GameCommands::doCommand(args, GameCommands::Flags::apply);
                break;
            }

            case widx::saveButton:
            {
                Game::confirmSaveGame();
                break;
            }

            case widx::dontSaveButton:
            {
                GameCommands::LoadSaveQuitGameArgs args{};
                args.option1 = GameCommands::LoadSaveQuitGameArgs::Options::dontSave;
                args.option2 = LoadOrQuitMode::loadGamePrompt;
                GameCommands::doCommand(args, GameCommands::Flags::apply);
                break;
            }
        }
    }

    // 0x0043C577
    static void onClose([[maybe_unused]] Window& self)
    {
        unsetPauseFlag(2);
        Audio::unpauseSound();
        WindowManager::invalidate(WindowType::timeToolbar);
    }

    static constexpr WindowEventList kEvents = {
        .onClose = onClose,
        .onMouseUp = onMouseUp,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
