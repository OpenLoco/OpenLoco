#include "../Audio/Audio.h"
#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "../Tutorial.h"
#include "../Ui/WindowManager.h"
#include "../Ui/WindowType.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    static loco_global<uint16_t, 0x0050A002> _savePromptType;

    // 0x0043BFF8
    static uint32_t loadGame()
    {
        registers regs;
        call(0x0043BFF8);
        return regs.ebx;
    }

    // 0x0043C182
    static uint32_t quitGame()
    {
        registers regs;
        call(0x0043C182);
        return regs.ebx;
    }

    // 0x0043C0FD
    static uint32_t returnToTitle()
    {
        registers regs;
        call(0x0043C0FD);
        return regs.ebx;
    }

    // 0x0043C427
    void confirmSaveGame()
    {
        // registers regs;
        call(0x0043C427);
    }

    // 0x0043BFCB
    static uint32_t loadSaveQuit(const uint8_t flags, uint8_t dl, uint8_t di)
    {
        if ((flags & GameCommandFlag::apply) == 0)
            return 0;

        if (dl == 1)
        {
            Ui::WindowManager::close(Ui::WindowType::saveGamePrompt);
            return 0;
        }

        if (dl == 0)
        {
            _savePromptType = di;
            Ui::Windows::TextInput::cancel();
            Ui::Windows::PromptSaveWindow::open(_savePromptType);

            if (!isTitleMode())
            {
                // 0x0043C369
                if (Tutorial::state() == Tutorial::tutorial_state::playing)
                {
                    Tutorial::stop();
                }
                else if (!isNetworked() || _savePromptType != 2)
                {
                    if (getScreenAge() >= 0xF00)
                    {
                        auto window = Ui::WindowManager::bringToFront(Ui::WindowType::saveGamePrompt);
                        Audio::playSound(Audio::sound_id::open_window, window->x + (window->width / 2));
                        return 0;
                    }
                }
            }
        }

        // 0x0043BFE3
        switch (_savePromptType)
        {
            case 0:
                return loadGame();

            case 1:
                return returnToTitle();

            case 2:
                return quitGame();
        }

        return 0;
    }

    void loadSaveQuit(registers& regs)
    {
        loadSaveQuit(regs.bl, regs.dl, regs.di);
    }
}
