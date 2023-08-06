#include "GameCommands.h"

namespace OpenLoco::GameCommands
{

    struct ChangeLoanArgs
    {
        static constexpr auto command = GameCommand::changeLoan;
        ChangeLoanArgs() = default;
        explicit ChangeLoanArgs(const registers& regs)
            : newLoan(regs.edx)
        {
        }

        currency32_t newLoan;

        explicit operator registers() const
        {
            registers regs;
            regs.edx = newLoan;
            return regs;
        }
    };

    void changeLoan(registers& regs);

}
