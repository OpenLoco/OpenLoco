#include "Graphics/ImageIds.h"
#include "Input/Shortcuts.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Engine/Input/ShortcutManager.h>
#include <SDL3/SDL_keycode.h>

using namespace OpenLoco::Input;

namespace OpenLoco::Ui::Windows::EditKeyboardShortcut
{
    static constexpr Ui::Size kWindowSize = { 280, 78 };

    static Shortcut _editingShortcutId;
    static KeyModifier _pressedModifiers;

    namespace Widx
    {
        constexpr WidgetId kClose{ "close" };
        constexpr WidgetId kDescription{ "description" };
        constexpr WidgetId kShortcutName{ "shortcutName" };
        constexpr WidgetId kPressedKeys{ "pressedKeys" };
    }

    static constexpr auto _widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, kWindowSize, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { kWindowSize.width - 2, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, StringIds::change_keyboard_shortcut),
        Widgets::ImageButton(Widx::kClose, { 265, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { kWindowSize.width, kWindowSize.height - 15 }, WindowColour::secondary),
        Widgets::Label(Widx::kDescription, { 4, 20 }, { kWindowSize.width - 8, 12 }, WindowColour::secondary, ContentAlign::center, StringIds::change_keyboard_shortcut_desc),
        Widgets::Label(Widx::kShortcutName, { 4, 34 }, { kWindowSize.width - 8, 12 }, WindowColour::secondary, ContentAlign::center, StringIds::black_quoted_stringid),
        Widgets::Label(Widx::kPressedKeys, { 4, 54 }, { kWindowSize.width - 8, 12 }, WindowColour::secondary, ContentAlign::center, StringIds::black_stringid));

    static const WindowEventList& getEvents();

    enum widx
    {
        frame,
        caption,
        close,
        panel,
        description,
        shortcutName,
        pressedKeys,
    };

    // 0x004BF7B9
    Window* open(const Shortcut shortcutId)
    {
        WindowManager::close(WindowType::editKeyboardShortcut);
        _editingShortcutId = shortcutId;
        _pressedModifiers = KeyModifier::none;

        auto window = WindowManager::createWindowCentred(WindowType::editKeyboardShortcut, kWindowSize, WindowFlags::none, getEvents());

        window->setWidgets(_widgets);
        window->initScrollWidgets();

        const auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->windowTitlebarColour);
        window->setColour(WindowColour::secondary, skin->windowOptionsColour);

        return window;
    }

    static void editShortcut([[maybe_unused]] const uint32_t charCode, const uint32_t keyCode, const KeyModifier keyModifier)
    {
        if (keyCode == SDLK_UP)
        {
            return;
        }
        if (keyCode == SDLK_DOWN)
        {
            return;
        }
        if (keyCode == SDLK_LEFT)
        {
            return;
        }
        if (keyCode == SDLK_RIGHT)
        {
            return;
        }
        if (keyCode == SDLK_NUMLOCKCLEAR)
        {
            return;
        }
        if (keyCode == SDLK_LGUI)
        {
            return;
        }
        if (keyCode == SDLK_RGUI)
        {
            return;
        }

        Input::Shortcuts::setBinding(_editingShortcutId, keyCode, keyModifier);

        WindowManager::close(WindowType::editKeyboardShortcut);
        WindowManager::invalidate(WindowType::keyboardShortcuts);
    }

    static void onUpdate(Window& self)
    {
        const auto modifiers = Input::getKeyModifier() & ~KeyModifier::cheat;
        if (modifiers == _pressedModifiers)
        {
            return;
        }

        _pressedModifiers = modifiers;
        self.invalidate();
    }

    static void prepareDraw(Window& self)
    {
        {
            auto args = FormatArguments(self.widgets[widx::description].textArgs);
            args.push(StringIds::empty);
        }

        if (auto def = ShortcutManager::getDefinition(_editingShortcutId))
        {
            auto args = FormatArguments(self.widgets[widx::shortcutName].textArgs);
            args.push(def->displayName);
        }

        {
            auto args = FormatArguments(self.widgets[widx::pressedKeys].textArgs);
            Shortcuts::pushModifierStrings(args, _pressedModifiers);
        }
    }

    // 0x004BE8DF
    static void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
    {
        self.draw(drawingCtx);
    }

    // 0x004BE821
    static void onMouseUp(Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, const WidgetId id)
    {
        switch (id)
        {
            case Widx::kClose:
                WindowManager::close(&self);
                return;
        }
    }

    static bool onKeyUp([[maybe_unused]] Window& self, const uint32_t charCode, const uint32_t keyCode)
    {
        editShortcut(charCode, keyCode, Input::getKeyModifier());

        return true;
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .onUpdate = onUpdate,
        .prepareDraw = prepareDraw,
        .draw = draw,
        .keyUp = onKeyUp,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
