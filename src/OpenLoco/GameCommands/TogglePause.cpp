#include "../Audio/Audio.h"
#include "../GameException.hpp"
#include "../SceneManager.h"
#include "../Ui/WindowManager.h"
#include "../Ui/WindowType.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x00431E32
    uint32_t togglePause(uint8_t flags)
    {
        if ((flags & Flags::apply) == 0)
            return 0;

        Ui::WindowManager::invalidate(Ui::WindowType::timeToolbar);

        if (isPaused())
        {
            unsetPauseFlag(1 << 0);
            Audio::unpauseSound();
        }
        else
        {
            setPauseFlag(1 << 0);
            Audio::pauseSound();
            Ui::Windows::TimePanel::invalidateFrame();
        }

        return 0;
    }

    void togglePause(registers& regs)
    {
        regs.ebx = togglePause(regs.bl);
    }
}
