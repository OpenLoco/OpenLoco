#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct UpdateOwnerStatusArgs
    {
        static constexpr auto command = GameCommand::updateOwnerStatus;
        UpdateOwnerStatusArgs() = default;
        explicit UpdateOwnerStatusArgs(const registers& regs)
            : ownerStatus(regs.ax, regs.cx)
        {
        }

        OwnerStatus ownerStatus;

        explicit operator registers() const
        {
            registers regs;
            int16_t res[2];
            ownerStatus.getData(res);
            regs.ax = res[0];
            regs.cx = res[1];
            return regs;
        }
    };

    void updateOwnerStatus(registers& regs);
}
