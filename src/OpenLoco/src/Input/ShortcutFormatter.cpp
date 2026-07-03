#include "Input/ShortcutFormatter.h"
#include "Config.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include <OpenLoco/Utility/LookupTable.hpp>
#include <SDL3/SDL_keyboard.h>
#include <cstring>

namespace OpenLoco::Input::ShortcutFormatter
{
    static void copyString(char* buffer, const size_t bufferLength, const char* value)
    {
        if (bufferLength == 0)
        {
            return;
        }

        std::strncpy(buffer, value, bufferLength - 1);
        buffer[bufferLength - 1] = '\0';
    }

    static void getBindingString(const uint32_t keyCode, char* buffer, const size_t bufferLength)
    {
        static constexpr auto keysToString = Utility::buildLookupTable<uint32_t, StringId>({
            { SDLK_BACKSPACE, StringIds::keyboard_backspace },
            { SDLK_TAB, StringIds::keyboard_tab },
            { SDLK_RETURN, StringIds::keyboard_return },
            { SDLK_PAUSE, StringIds::keyboard_pause },
            { SDLK_CAPSLOCK, StringIds::keyboard_caps },
            { SDLK_ESCAPE, StringIds::keyboard_escape },
            { SDLK_SPACE, StringIds::keyboard_spacebar },
            { SDLK_PAGEUP, StringIds::keyboard_pageup },
            { SDLK_PAGEDOWN, StringIds::keyboard_pagedown },
            { SDLK_END, StringIds::keyboard_end },
            { SDLK_HOME, StringIds::keyboard_home },
            { SDLK_LEFT, StringIds::keyboard_left },
            { SDLK_UP, StringIds::keyboard_up },
            { SDLK_RIGHT, StringIds::keyboard_right },
            { SDLK_DOWN, StringIds::keyboard_down },
            { SDLK_INSERT, StringIds::keyboard_insert },
            { SDLK_DELETE, StringIds::keyboard_delete },
            { SDLK_KP_1, StringIds::keyboard_numpad_1 },
            { SDLK_KP_2, StringIds::keyboard_numpad_2 },
            { SDLK_KP_3, StringIds::keyboard_numpad_3 },
            { SDLK_KP_4, StringIds::keyboard_numpad_4 },
            { SDLK_KP_5, StringIds::keyboard_numpad_5 },
            { SDLK_KP_6, StringIds::keyboard_numpad_6 },
            { SDLK_KP_7, StringIds::keyboard_numpad_7 },
            { SDLK_KP_8, StringIds::keyboard_numpad_8 },
            { SDLK_KP_9, StringIds::keyboard_numpad_9 },
            { SDLK_KP_0, StringIds::keyboard_numpad_0 },
            { SDLK_KP_DIVIDE, StringIds::keyboard_numpad_divide },
            { SDLK_KP_ENTER, StringIds::keyboard_numpad_enter },
            { SDLK_KP_MINUS, StringIds::keyboard_numpad_minus },
            { SDLK_KP_MULTIPLY, StringIds::keyboard_numpad_multiply },
            { SDLK_KP_PERIOD, StringIds::keyboard_numpad_period },
            { SDLK_KP_PLUS, StringIds::keyboard_numpad_plus },
            { SDLK_NUMLOCKCLEAR, StringIds::keyboard_numlock },
            { SDLK_SCROLLLOCK, StringIds::keyboard_scroll },
            { SDLK_MENU, StringIds::keyboard_menu },
        });

        auto match = keysToString.find(keyCode);
        if (match != keysToString.end())
        {
            StringManager::formatString(buffer, bufferLength, match->second);
            return;
        }

        copyString(buffer, bufferLength, SDL_GetKeyName(static_cast<SDL_Keycode>(keyCode)));
    }

    Binding getBinding(const Config::KeyboardShortcut& shortcut, char* buffer, const size_t bufferLength)
    {
        if (shortcut.keyCode == 0xFFFFFFFF || shortcut.modifiers == KeyModifier::invalid)
        {
            return Binding{
                StringIds::empty,
                StringIds::empty,
                "",
                false,
            };
        }

        auto modifierStringId = StringIds::empty;
        if ((shortcut.modifiers & KeyModifier::shift) == KeyModifier::shift)
        {
            modifierStringId = StringIds::keyboard_shortcut_modifier_shift;
        }
        else if ((shortcut.modifiers & KeyModifier::control) == KeyModifier::control)
        {
            modifierStringId = StringIds::keyboard_shortcut_modifier_ctrl;
        }

        getBindingString(shortcut.keyCode, buffer, bufferLength);
        return Binding{
            modifierStringId,
            StringIds::stringptr,
            buffer,
            true,
        };
    }

    Binding getBinding(const Shortcut shortcut, char* buffer, const size_t bufferLength)
    {
        const auto& shortcuts = Config::get().shortcuts;
        auto match = shortcuts.find(shortcut);
        if (match == shortcuts.end())
        {
            return Binding{
                StringIds::empty,
                StringIds::empty,
                "",
                false,
            };
        }

        return getBinding(match->second, buffer, bufferLength);
    }
}
