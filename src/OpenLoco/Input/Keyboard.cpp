#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../Console.h"
#include "../Entities/EntityManager.h"
#include "../GameCommands/GameCommands.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Intro.h"
#include "../Localisation/StringIds.h"
#include "../OpenLoco.h"
#include "../Tutorial.h"
#include "../Ui.h"
#include "../Ui/Screenshot.h"
#include "../Vehicles/Vehicle.h"
#include "ShortcutManager.h"
#include <SDL2/SDL.h>
#include <cstdint>
#include <functional>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;
using namespace OpenLoco::GameCommands;

namespace OpenLoco::Input
{

#pragma pack(push, 1)
    struct key
    {
        uint32_t keyCode;
        uint32_t charCode;
    };
#pragma pack(pop)

    static void normalKey();
    static void cheat();
    static void loc_4BECDE();
    static void loc_4BED04();
    static void loc_4BED79();

    static loco_global<int8_t, 0x00508F16> _screenshotCountdown;
    static loco_global<uint8_t, 0x00508F18> _keyModifier;
    static loco_global<Ui::WindowType, 0x005233B6> _modalWindowType;
    static loco_global<char[16], 0x0112C826> _commonFormatArgs;
    static std::string _cheatBuffer; // 0x0011364A5
    static loco_global<key[64], 0x0113E300> _keyQueue;
    static loco_global<uint32_t, 0x00525388> _keyQueueLastWrite;
    static loco_global<uint32_t, 0x00525384> _keyQueueReadIndex;
    static loco_global<uint32_t, 0x00525380> _keyQueueWriteIndex;
    static loco_global<uint8_t[256], 0x01140740> _keyboardState;
    static loco_global<uint8_t, 0x011364A4> _editingShortcutIndex;

    static const std::pair<std::string, std::function<void()>> kCheats[] = {
        { "DRIVER", loc_4BECDE },
        { "SHUNT", loc_4BED04 },
        { "FREECASH", loc_4BED79 }
    };

    bool hasKeyModifier(uint8_t modifier)
    {
        uint8_t keyModifier = _keyModifier;
        return (keyModifier & modifier) != 0;
    }

    static void loc_4BECDE()
    {
        setScreenFlag(ScreenFlags::driverCheatEnabled);

        Audio::playSound(Audio::SoundId::clickPress, Ui::width() / 2);
    }

    static void loc_4BED04()
    {
        if ((getScreenFlags() & ScreenFlags::driverCheatEnabled) == 0)
        {
            return;
            // Only works when DRIVER mode is active
        }

        for (auto i = (int32_t)WindowManager::count() - 1; i >= 0; i--)
        {
            auto w = WindowManager::get(i);

            if (w->type != WindowType::vehicle)
                continue;

            auto t = EntityManager::get<Vehicles::VehicleBase>(EntityId(w->number));
            if (t == nullptr)
                continue;

            if (t->owner != CompanyManager::getControllingId())
                continue;

            if (t->getTransportMode() != TransportMode::rail)
                continue;

            GameCommands::VehicleApplyShuntCheatArgs args;
            args.head = EntityId(w->number);
            GameCommands::doCommand(args, GameCommands::Flags::apply);
            Audio::playSound(Audio::SoundId::clickPress, Ui::width() / 2);

            return;
        }
    }

    static void loc_4BED79()
    {
        GameCommands::doCommand(GameCommands::ApplyFreeCashCheatArgs(), GameCommands::Flags::apply);

        Audio::playSound(Audio::SoundId::clickPress, Ui::width() / 2);
    }

    static void loc_4BEFEF()
    {
        switch (Tutorial::state())
        {
            case Tutorial::State::none:
                break;

            case Tutorial::State::playing:
            {
                const uint16_t next = Tutorial::nextInput();
                _keyModifier = next;
                if ((_keyModifier & KeyModifier::unknown) == 0)
                    return;

                Windows::ToolTip::closeAndReset();

                auto tutStringId = Tutorial::nextString();
                auto main = WindowManager::getMainWindow();
                auto cursor = getMouseLocation();

                Windows::ToolTip::update(main, 0, tutStringId, cursor.x, cursor.y);
                break;
            }

            case Tutorial::State::recording:
            {
                call(0x004BF005);
                break;
            }
        }
    }

    // 0x00406FBA
    void enqueueKey(uint32_t keycode)
    {
        uint32_t writeIndex = _keyQueueWriteIndex;
        auto nextWriteIndex = (writeIndex + 1) % std::size(_keyQueue);
        if (nextWriteIndex == _keyQueueReadIndex)
        {
            return;
        }
        _keyQueueLastWrite = writeIndex;
        _keyQueue[writeIndex] = { keycode, 0 };
        _keyQueueWriteIndex = nextWriteIndex;
    }

    void enqueueText(const char* text)
    {
        if (text != nullptr && text[0] != '\0')
        {
            auto index = _keyQueueLastWrite;
            _keyQueue[index].charCode = text[0];
        }
    }

    // 0x00407028
    static key* getNextKey()
    {
        uint32_t readIndex = _keyQueueReadIndex;
        if (readIndex == _keyQueueWriteIndex)
        {
            return nullptr;
        }
        auto* out = &_keyQueue[readIndex];
        readIndex++;
        // Wrap around at _keyQueue size
        readIndex %= std::size(_keyQueue);
        _keyQueueReadIndex = readIndex;
        return out;
    }

    static bool tryShortcut(Shortcut sc, uint32_t keyCode, uint8_t modifiers)
    {
        auto cfg = OpenLoco::Config::getNew();
        if (cfg.shortcuts[sc].keyCode == keyCode && cfg.shortcuts[sc].modifiers == modifiers)
        {
            ShortcutManager::execute(sc);
            return true;
        }

        return false;
    }

    // 0x004BEC5B
    void processKeyboardInput()
    {
        cheat();
        normalKey();
    }

    // 0x004BEC5B
    static void cheat()
    {
        // Used to handle INSERT cheat
        if ((_keyboardState[SDL_SCANCODE_INSERT] & 0x80) != 0 || (_keyboardState[SDL_SCANCODE_LALT] & 0x80) != 0 || (_keyboardState[SDL_SCANCODE_RALT] & 0x80) != 0)
        {
            if ((_keyModifier & KeyModifier::cheat) != 0)
            {
                return;
            }
            else
            {
                _keyModifier |= KeyModifier::cheat;
                _cheatBuffer.clear();
                return;
            }
        }

        if ((_keyModifier & KeyModifier::cheat) == 0)
            return;

        _keyModifier = _keyModifier & (~KeyModifier::cheat);

        if (isTitleMode())
            return;

        for (const auto& cheat : kCheats)
        {
            if (strcmp(_cheatBuffer.c_str(), cheat.first.c_str()) == 0)
            {
                cheat.second();
                break;
            }
        }
    }

    static void editShortcut(key* k)
    {
        if (k->keyCode == SDLK_UP)
            return;
        if (k->keyCode == SDLK_DOWN)
            return;
        if (k->keyCode == SDLK_LEFT)
            return;
        if (k->keyCode == SDLK_RIGHT)
            return;
        if (k->keyCode == SDLK_NUMLOCKCLEAR)
            return;
        if (k->keyCode == SDLK_LGUI)
            return;
        if (k->keyCode == SDLK_RGUI)
            return;

        auto& cfg = Config::getNew();

        // Unbind any shortcuts that may be using the current keycode.
        for (size_t i = 0; i < ShortcutManager::kCount; i++)
        {
            if (cfg.shortcuts[i].keyCode == k->keyCode && cfg.shortcuts[i].modifiers == _keyModifier)
            {
                cfg.shortcuts[i].keyCode = 0xFFFFFFFF;
                cfg.shortcuts[i].modifiers = 0xFF;
            }
        }

        // Assign this keybinding to the shortcut we're currently rebinding.
        cfg.shortcuts[_editingShortcutIndex].keyCode = k->keyCode;
        cfg.shortcuts[_editingShortcutIndex].modifiers = _keyModifier;

        WindowManager::close(WindowType::editKeyboardShortcut);
        WindowManager::invalidate(WindowType::keyboardShortcuts);
        Config::write();
    }

    // 0x004BEDA0
    static void normalKey()
    {
        while (true)
        {
            auto* nextKey = getNextKey();
            if (nextKey == nullptr)
            {
                loc_4BEFEF();
                break;
            }

            if (nextKey->keyCode == SDLK_LSHIFT || nextKey->keyCode == SDLK_RSHIFT)
                continue;

            if (nextKey->keyCode == SDLK_LCTRL || nextKey->keyCode == SDLK_RCTRL)
                continue;

            if ((_keyModifier & KeyModifier::cheat) != 0)
            {
                if (nextKey->charCode >= 'a' && nextKey->charCode <= 'z')
                {
                    nextKey->charCode = toupper(nextKey->charCode);
                }

                if (nextKey->charCode >= 'A' && nextKey->charCode <= 'Z')
                {
                    _cheatBuffer += nextKey->charCode;
                }

                continue;
            }

            auto ti = WindowManager::find(WindowType::textInput);
            if (ti != nullptr)
            {
                if (tryShortcut(Shortcut::screenshot, nextKey->keyCode, _keyModifier))
                    continue;

                Ui::Windows::TextInput::handleInput(nextKey->charCode, nextKey->keyCode);
                continue;
            }

            if (*_modalWindowType == WindowType::fileBrowserPrompt)
            {
                ti = WindowManager::find(WindowType::fileBrowserPrompt);
                if (ti != nullptr)
                {
                    if (tryShortcut(Shortcut::screenshot, nextKey->keyCode, _keyModifier))
                        continue;

                    Ui::Windows::PromptBrowse::handleInput(nextKey->charCode, nextKey->keyCode);
                    continue;
                }
            }

            if (*_modalWindowType == WindowType::confirmationPrompt)
            {
                ti = WindowManager::find(WindowType::confirmationPrompt);
                if (ti != nullptr)
                {
                    Ui::Windows::PromptOkCancel::handleInput(nextKey->charCode, nextKey->keyCode);
                    continue;
                }
            }

            ti = WindowManager::find(WindowType::editKeyboardShortcut);
            if (ti != nullptr)
            {
                editShortcut(nextKey);
                continue;
            }

            if (Tutorial::state() == Tutorial::State::playing)
            {
                Tutorial::stop();
                continue;
            }

            if (!isTitleMode())
            {
                for (size_t i = 0; i < ShortcutManager::kCount; i++)
                {
                    if (tryShortcut((Shortcut)i, nextKey->keyCode, _keyModifier))
                        break;
                }
                continue;
            }

            if (Intro::state() == Intro::State::state_9)
            {
                Intro::state(Intro::State::end);
                continue;
            }

            if (Intro::state() != Intro::State::none)
            {
                Intro::state(Intro::State::state_8);
            }

            if (tryShortcut(Shortcut::sendMessage, nextKey->keyCode, _keyModifier))
                continue;

            if (tryShortcut(Shortcut::screenshot, nextKey->keyCode, _keyModifier))
                continue;
        }
    }

    static void edgeScroll()
    {
        if (Tutorial::state() != Tutorial::State::none)
            return;

        if (Config::get().edgeScrolling == 0)
            return;

        if (Input::state() != State::normal && Input::state() != State::dropdownActive)
            return;

        if (hasKeyModifier(KeyModifier::shift) || hasKeyModifier(KeyModifier::control))
            return;

        Ui::Point delta = { 0, 0 };
        auto cursor = getMouseLocation();

        if (cursor.x == 0)
            delta.x -= 12;

        if (cursor.x == Ui::width() - 1)
            delta.x += 12;

        if (cursor.y == 0)
            delta.y -= 12;

        if (cursor.y == Ui::height() - 1)
            delta.y += 12;

        if (delta.x == 0 && delta.y == 0)
            return;

        auto main = WindowManager::getMainWindow();
        if ((main->flags & WindowFlags::viewportNoScrolling) != 0)
            return;

        if (OpenLoco::isTitleMode())
            return;

        auto viewport = main->viewports[0];
        if (viewport == nullptr)
            return;

        delta.x *= 1 << viewport->zoom;
        delta.y *= 1 << viewport->zoom;
        main->viewportConfigurations[0].saved_view_x += delta.x;
        main->viewportConfigurations[0].saved_view_y += delta.y;
        Input::setFlag(Flags::viewportScrolling);
    }

    static void keyScroll()
    {
        if (Tutorial::state() != Tutorial::State::none)
            return;

        if (*_modalWindowType != WindowType::undefined)
            return;

        if (WindowManager::find(WindowType::textInput) != nullptr)
            return;

        Ui::Point delta = { 0, 0 };

        if (_keyboardState[SDL_SCANCODE_LEFT] & 0x80)
            delta.x -= 8;

        if (_keyboardState[SDL_SCANCODE_UP] & 0x80)
            delta.y -= 8;

        if (_keyboardState[SDL_SCANCODE_DOWN] & 0x80)
            delta.y += 8;

        if (_keyboardState[SDL_SCANCODE_RIGHT] & 0x80)
            delta.x += 8;

        if (delta.x == 0 && delta.y == 0)
            return;

        auto main = WindowManager::getMainWindow();
        if ((main->flags & WindowFlags::viewportNoScrolling) != 0)
            return;

        if (OpenLoco::isTitleMode())
            return;

        auto viewport = main->viewports[0];
        if (viewport == nullptr)
            return;

        delta.x *= 1 << viewport->zoom;
        delta.y *= 1 << viewport->zoom;
        main->viewportConfigurations[0].saved_view_x += delta.x;
        main->viewportConfigurations[0].saved_view_y += delta.y;
        Input::setFlag(Flags::viewportScrolling);
    }

    // 0x004BE92A
    void handleKeyboard()
    {
        if (_screenshotCountdown != 0)
        {
            _screenshotCountdown--;
            if (_screenshotCountdown == 0)
            {
                try
                {
                    std::string fileName = saveScreenshot();
                    *((const char**)(&_commonFormatArgs[0])) = fileName.c_str();
                    Windows::showError(StringIds::screenshot_saved_as, StringIds::null, false);
                }
                catch (const std::exception&)
                {
                    Windows::showError(StringIds::screenshot_failed);
                }
            }
        }

        edgeScroll();

        _keyModifier = _keyModifier & ~(KeyModifier::shift | KeyModifier::control | KeyModifier::unknown);

        if (addr<0x005251CC, uint8_t>() != 1)
        {
            return;
        }

        if (_keyboardState[SDL_SCANCODE_LSHIFT] & 0x80)
            _keyModifier |= KeyModifier::shift;

        if (_keyboardState[SDL_SCANCODE_RSHIFT] & 0x80)
            _keyModifier |= KeyModifier::shift;

        if (_keyboardState[SDL_SCANCODE_LCTRL] & 0x80)
            _keyModifier |= KeyModifier::control;

        if (_keyboardState[SDL_SCANCODE_RCTRL] & 0x80)
            _keyModifier |= KeyModifier::control;

        keyScroll();
    }
}
