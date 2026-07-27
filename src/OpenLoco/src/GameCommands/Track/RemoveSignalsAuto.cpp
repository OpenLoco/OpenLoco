#include "GameCommands/Track/RemoveSignalsAuto.h"

namespace OpenLoco::GameCommands
{
    static uint32_t removeSignalsAuto(const SignalsRemovalAutoArgs&, const uint8_t)
    {
        return 0;
    }

    void removeSignalsAuto(registers& regs, const uint8_t flags)
    {
        regs.ebx = removeSignalsAuto(SignalsRemovalAutoArgs(regs), flags);
    }
}
