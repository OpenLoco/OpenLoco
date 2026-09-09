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
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/WindowManager.h"

#include <array>
#include <cstring>

namespace OpenLoco::Ui::Windows::PromptSaveWindow
{
    static LoadOrQuitMode _savePromptType; // 0x0050A002

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

    namespace Widx
    {
        constexpr WidgetId kCaption{ "caption" };
        constexpr WidgetId kCloseButton{ "closeButton" };
        constexpr WidgetId kPromptLabel{ "promptLabel" };
        constexpr WidgetId kSaveButton{ "saveButton" };
        constexpr WidgetId kDontSaveButton{ "dontSaveButton" };
        constexpr WidgetId kCancelButton{ "cancelButton" };
    }

    static constexpr auto _widgets = makeWidgets(
        Widgets::Panel({ 0, 0 }, { 260, 48 }, WindowColour::primary),
        Widgets::Caption(Widx::kCaption, { 1, 1 }, { 258, 13 }, Widgets::Caption::Style::boxed, WindowColour::primary, StringIds::empty),
        Widgets::Button(Widx::kCloseButton, { 247, 2 }, { 11, 11 }, WindowColour::primary, StringIds::close_window_cross, StringIds::tooltip_close_window),
        Widgets::Label(Widx::kPromptLabel, { 2, 17 }, { 256, 12 }, WindowColour::primary, ContentAlign::center, StringIds::empty),
        Widgets::Button(Widx::kSaveButton, { 8, 33 }, { 78, 12 }, WindowColour::primary, StringIds::label_button_save),
        Widgets::Button(Widx::kDontSaveButton, { 91, 33 }, { 78, 12 }, WindowColour::primary, StringIds::label_button_dont_save),
        Widgets::Button(Widx::kCancelButton, { 174, 33 }, { 78, 12 }, WindowColour::primary, StringIds::label_button_cancel)

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
            window->initScrollWidgets();
            window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedDarkRed).translucent());
            window->flags |= Ui::WindowFlags::transparent;

            SceneManager::setPauseFlag(PauseFlags::promptSave);
        }

        _savePromptType = savePromptType;

        if (!SceneManager::isEditorMode())
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
    static void onMouseUp([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, const WidgetId id)
    {
        switch (id)
        {
            case Widx::kCloseButton:
            case Widx::kCancelButton:
            {
                GameCommands::LoadSaveQuitGameArgs args{};
                args.loadQuitMode = _savePromptType;
                args.saveMode = GameCommands::LoadSaveQuitGameArgs::SaveMode::closeSavePrompt;
                GameCommands::doCommand(args, GameCommands::Flags::apply);
                break;
            }

            case Widx::kSaveButton:
            {
                Game::confirmSaveGame(_savePromptType);
                break;
            }

            case Widx::kDontSaveButton:
            {
                GameCommands::LoadSaveQuitGameArgs args{};
                args.loadQuitMode = _savePromptType;
                args.saveMode = GameCommands::LoadSaveQuitGameArgs::SaveMode::dontSave;
                GameCommands::doCommand(args, GameCommands::Flags::apply);
                break;
            }
        }
    }

    // 0x0043C577
    static void onClose([[maybe_unused]] Window& self)
    {
        SceneManager::unsetPauseFlag(PauseFlags::promptSave);
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
