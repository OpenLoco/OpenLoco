#include "../audio/audio.h"
#include "../companymgr.h"
#include "../config.h"
#include "../console.h"
#include "../game_commands.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../openloco.h"
#include "../things/thingmgr.h"
#include "../tutorial.h"
#include "../ui.h"
#include "../win32.h"
#include "ShortcutManager.h"
#include <cstdint>
#include <functional>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>
#include <windows.h>
#endif

#define DIK_INSERT 0xD2

using namespace openloco::interop;
using namespace openloco::ui;

namespace openloco::input
{

#pragma pack(push, 1)
    struct key
    {
        uint32_t keyCode;
        uint32_t charCode;
    };
#pragma pack(pop)

    static void normal_key();
    static void cheat();
    static void loc_4BECDE();
    static void loc_4BED04();
    static void loc_4BED79();

    static loco_global<uint8_t, 0x00508F18> _keyModifier;
    static loco_global<uint8_t, 0x00508F14> _screenFlags;
    static loco_global<ui::WindowType, 0x005233B6> _modalWindowType;
    static std::string _cheatBuffer; // 0x0011364A5
    static loco_global<uint8_t[256], 0x01140740> _keyboardState;
    static loco_global<uint8_t, 0x011364A4> _11364A4;

    static std::pair<std::string, std::function<void()>> cheats[] = {
        { "DRIVER", loc_4BECDE },
        { "SHUNT", loc_4BED04 },
        { "FREECASH", loc_4BED79 }
    };

    bool has_key_modifier(uint8_t modifier)
    {
        uint8_t keyModifier = _keyModifier;
        return (keyModifier & modifier) != 0;
    }

    static void loc_4BECDE()
    {
        _screenFlags |= screen_flags::unknown_6;

        audio::play_sound(audio::sound_id::click_press, ui::width() / 2);
    }

    static void loc_4BED04()
    {
        if ((_screenFlags & screen_flags::unknown_6) == 0)
        {
            return;
            // Only works when DRIVER mode is active
        }

        for (auto i = (int32_t)WindowManager::count() - 1; i >= 0; i--)
        {
            auto w = WindowManager::get(i);

            if (w->type != WindowType::vehicle)
                continue;

            auto t = thingmgr::get<openloco::vehicle>(w->number);
            if (t->owner != companymgr::get_controlling_id())
                continue;

            if (t->var_42 != 0)
                continue;

            registers regs;
            regs.cx = w->number;
            regs.bl = 1;
            game_commands::do_command(77, regs);
            audio::play_sound(audio::sound_id::click_press, ui::width() / 2);

            return;
        }
    }

    static void loc_4BED79()
    {
        registers regs;
        regs.bl = 1;
        game_commands::do_command(78, regs);

        audio::play_sound(audio::sound_id::click_press, ui::width() / 2);
    }

    static key* get_next_key()
    {
        registers regs;
        call(0x00407028, regs);
        return (key*)regs.eax;
    }

    static bool try_shortcut(Shortcut sc, uint32_t keyCode, uint8_t modifiers)
    {
        auto cfg = openloco::config::get();
        if (cfg.keyboard_shortcuts[sc].var_0 == keyCode && cfg.keyboard_shortcuts[sc].var_1 == modifiers)
        {
            ShortcutManager::execute(sc);
            return true;
        }

        return false;
    }

    /** 0x004BEC5B */
    void process_keyboard_input()
    {
        cheat();
        normal_key();
    }

    static void cheat()
    {
        // Used to handle INSERT cheat
        if ((_keyboardState[DIK_INSERT] & 0x80) != 0)
        {
            if ((_keyModifier & key_modifier::cheat) != 0)
            {
                return;
            }
            else
            {
                _keyModifier |= key_modifier::cheat;
                _cheatBuffer.clear();
                return;
            }
        }

        if ((_keyModifier & key_modifier::cheat) == 0)
            return;

        _keyModifier = _keyModifier & (~key_modifier::cheat);

        if (is_title_mode())
            return;

        for (auto cheat : cheats)
        {
            if (strcmp(_cheatBuffer.c_str(), cheat.first.c_str()) == 0)
            {
                cheat.second();
                break;
            }
        }
    }

    static void edit_shortcut(key* k)
    {
        if (k->keyCode == VK_UP)
            return;
        if (k->keyCode == VK_DOWN)
            return;
        if (k->keyCode == VK_LEFT)
            return;
        if (k->keyCode == VK_RIGHT)
            return;
        if (k->keyCode == VK_NUMLOCK)
            return;
        if (k->keyCode == VK_LWIN)
            return;
        if (k->keyCode == VK_RWIN)
            return;

        auto& cfg = config::get();
        for (int i = 0; i < 35; i++)
        {
            if (cfg.keyboard_shortcuts[i].var_0 == k->keyCode && cfg.keyboard_shortcuts[i].var_1 == _keyModifier)
            {
                cfg.keyboard_shortcuts[i].var_0 = 0xFF;
                cfg.keyboard_shortcuts[i].var_1 = 0xFF;
            }
        }

        cfg.keyboard_shortcuts[_11364A4].var_0 = k->keyCode;
        cfg.keyboard_shortcuts[_11364A4].var_1 = _keyModifier;

        WindowManager::close(WindowType::editKeyboardShortcut);
        WindowManager::invalidate(WindowType::keyboardShortcuts);
        config::write();
    }

    static void normal_key()
    {
        while (true)
        {
            auto eax = get_next_key();
            if (eax == nullptr)
                break;

            if (eax->keyCode >= 255)
                continue;

            if (eax->keyCode == 0x10) // VK_SHIFT
                continue;

            if (eax->keyCode == 0x11) // VK_CONTROL
                continue;

            if ((_keyModifier & key_modifier::cheat) != 0)
            {
                if (eax->charCode >= 'a' && eax->charCode <= 'z')
                {
                    eax->charCode = toupper(eax->charCode);
                }

                if (eax->charCode >= 'A' && eax->charCode <= 'Z')
                {
                    _cheatBuffer += eax->charCode;
                }

                continue;
            }

            auto ti = WindowManager::find(WindowType::textInput);
            if (ti != nullptr)
            {
                if (try_shortcut(Shortcut::screenshot, eax->keyCode, _keyModifier))
                    continue;

                ui::textinput::sub_4CE910(eax->charCode, eax->keyCode);
                continue;
            }

            if (_modalWindowType == WindowType::fileBrowserPrompt)
            {
                ti = WindowManager::find(WindowType::fileBrowserPrompt);
                if (ti != nullptr)
                {
                    if (try_shortcut(Shortcut::screenshot, eax->keyCode, _keyModifier))
                        continue;

                    registers regs;
                    regs.eax = eax->charCode;
                    regs.ebx = eax->keyCode;
                    call(0x0044685C, regs);
                    continue;
                }
            }

            if (_modalWindowType == WindowType::confirmationPrompt)
            {
                ti = WindowManager::find(WindowType::confirmationPrompt);
                if (ti != nullptr)
                {
                    registers regs;
                    regs.eax = eax->charCode;
                    regs.ebx = eax->keyCode;
                    call(0x0044685C, regs);
                }
            }

            ti = WindowManager::find(WindowType::editKeyboardShortcut);
            if (ti != nullptr)
            {
                edit_shortcut(eax);
                continue;
            }

            if (tutorial::state() == tutorial::tutorial_state::playing)
            {
                tutorial::stop();
                continue;
            }

            if (!is_title_mode())
            {
                for (int i = 0; i < 35; i++)
                {
                    if (try_shortcut((Shortcut)i, eax->keyCode, _keyModifier))
                        break;
                }
                continue;
            }

            if (intro::state() == (intro::intro_state)9)
            {
                intro::state(intro::intro_state::end);
                continue;
            }

            if (intro::state() != intro::intro_state::none)
            {
                intro::state((intro::intro_state)8);
            }

            if (try_shortcut(Shortcut::sendMessage, eax->keyCode, _keyModifier))
                continue;

            if (try_shortcut(Shortcut::screenshot, eax->keyCode, _keyModifier))
                continue;
        }
    }
}
