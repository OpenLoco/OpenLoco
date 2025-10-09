#include "Config.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input/Shortcuts.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Engine/Input/ShortcutManager.h>
#include <OpenLoco/Interop/Interop.hpp>
#include <SDL2/SDL_keyboard.h>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Input;

namespace OpenLoco::Ui::Windows::EditKeyboardShortcut
{
    static constexpr Ui::Size32 kWindowSize = { 280, 72 };

    static uint8_t _editingShortcutIndex;

    static constexpr auto _widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, kWindowSize, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { kWindowSize.width - 2, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, StringIds::change_keyboard_shortcut),
        Widgets::ImageButton({ 265, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { kWindowSize.width, 57 }, WindowColour::secondary));

    static const WindowEventList& getEvents();

    namespace Widx
    {
        enum
        {
            frame,
            caption,
            close,
            panel,
        };
    }

    // 0x004BF7B9
    Window* open(const uint8_t shortcutIndex)
    {
        WindowManager::close(WindowType::editKeyboardShortcut);
        _editingShortcutIndex = shortcutIndex;

        auto window = WindowManager::createWindow(WindowType::editKeyboardShortcut, kWindowSize, WindowFlags::none, getEvents());

        window->setWidgets(_widgets);
        window->initScrollWidgets();

        const auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->windowTitlebarColour);
        window->setColour(WindowColour::secondary, skin->windowOptionsColour);

        return window;
    }

    static void editShortcut(uint32_t keyCode, [[maybe_unused]] uint32_t charCode)
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

        auto& cfg = Config::get();

        // Unbind any shortcuts that may be using the current keycode.
        for (auto& [id, shortcut] : cfg.shortcuts)
        {
            if (shortcut.keyCode == keyCode && shortcut.modifiers == Input::getKeyModifier())
            {
                shortcut.keyCode = 0xFFFFFFFF;
                shortcut.modifiers = KeyModifier::invalid;
            }
        }

        // Assign this keybinding to the shortcut we're currently rebinding.
        auto& shortcut = cfg.shortcuts.at(static_cast<Input::Shortcut>(_editingShortcutIndex));
        shortcut.keyCode = keyCode;
        shortcut.modifiers = Input::getKeyModifier();

        WindowManager::close(WindowType::editKeyboardShortcut);
        WindowManager::invalidate(WindowType::keyboardShortcuts);
        Config::write();
    }

    // 0x004BE8DF
    static void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        self.draw(drawingCtx);

        FormatArguments args{};
        args.push(ShortcutManager::getName(static_cast<Shortcut>(_editingShortcutIndex)));
        auto point = Ui::Point(self.x + 140, self.y + 32);
        tr.drawStringCentredWrapped(point, 272, Colour::black, StringIds::change_keyboard_shortcut_desc, args);
    }

    // 0x004BE821
    static void onMouseUp(Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case Widx::close:
                WindowManager::close(&self);
                return;
        }
    }

    static bool onKeyUp([[maybe_unused]] Window& self, const uint32_t keyCode, const uint32_t charCode)
    {
        editShortcut(keyCode, charCode);

        return true;
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .draw = draw,
        .keyUp = onKeyUp,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
