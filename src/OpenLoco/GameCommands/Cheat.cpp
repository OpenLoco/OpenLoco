#include "Cheat.h"
#include "../CompanyManager.h"
#include "../Interop/Interop.hpp"
#include "../Types.hpp"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    namespace Cheats
    {
        static uint32_t clearLoan()
        {
            auto company = CompanyManager::getPlayerCompany();
            company->current_loan = 0;
            return 0;
        }

        static uint32_t switchCompany(CompanyId_t targetCompanyId)
        {
            auto ourId = CompanyManager::getControllingId();
            auto otherId = CompanyManager::getSecondaryPlayerId();

            // Already controlling the target company?
            if (targetCompanyId == ourId)
                return 0;

            // Is the other player controlling the target company? Swap companies.
            if (targetCompanyId == otherId)
            {
                CompanyManager::setSecondaryPlayerId(ourId);
                CompanyManager::setControllingId(otherId);
                return 0;
            }

            // Change control over to the other company.
            CompanyManager::setControllingId(targetCompanyId);
            return 0;
        }

        static uint32_t toggleBankruptcy(CompanyId_t targetCompanyId)
        {
            auto company = CompanyManager::get(targetCompanyId);
            company->challenge_flags ^= CompanyFlags::bankrupt;
            return 0;
        }

        static uint32_t toggleJail(CompanyId_t targetCompanyId)
        {
            auto company = CompanyManager::get(targetCompanyId);
            company->jail_status = 30;
            return 0;
        }
    }

    static uint32_t cheat(CheatCommand command, int32_t param1, int32_t param2, int32_t param3)
    {
        switch (command)
        {
            case CheatCommand::clearLoan:
                return Cheats::clearLoan();

            case CheatCommand::switchCompany:
                return Cheats::switchCompany(param1);

            case CheatCommand::toggleBankruptcy:
                return Cheats::toggleBankruptcy(param1);

            case CheatCommand::toggleJail:
                return Cheats::toggleJail(param1);

            default:
                break;
        }

        return 0;
    }

    void cheat(registers& regs)
    {
        regs.ebx = cheat(CheatCommand(regs.eax), regs.ebx, regs.ecx, regs.edx);
    }
}
