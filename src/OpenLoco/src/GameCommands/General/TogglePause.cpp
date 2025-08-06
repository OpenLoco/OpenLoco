#include "TogglePause.h"
#include "Audio/Audio.h"
#include "GameCommands/GameCommands.h"
#include "GameException.hpp"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "Ui/WindowType.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x00431E32
    static uint32_t togglePause(uint8_t flags)
    {
        if ((flags & Flags::apply) == 0)
        {
            return 0;
        }

        Ui::WindowManager::invalidate(Ui::WindowType::timeToolbar);

        if (SceneManager::getPauseFlags() & 1)
        {
            SceneManager::unsetPauseFlag(1 << 0);
        }
        else
        {
            SceneManager::setPauseFlag(1 << 0);
        }

        return 0;
    }

    void togglePause(registers& regs)
    {
        regs.ebx = togglePause(regs.bl);
    }
}
