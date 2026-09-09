#include "GameCommands/Company/UpdateOwnerStatus.h"
#include "GameCommands/GameCommands.h"
#include "World/CompanyManager.h"

namespace OpenLoco::GameCommands
{
    // 0x004383CA
    static uint32_t updateOwnerStatus(const Flags flags, const OwnerStatus& ownerStatus)
    {
        if (hasFlags(flags, Flags::apply))
        {
            auto companyId = GameCommands::getUpdatingCompanyId();
            auto* company = CompanyManager::get(companyId);
            if (company == nullptr)
            {
                return kFailure;
            }
            company->ownerStatus = ownerStatus;
        }
        return 0;
    }

    void updateOwnerStatus(registers& regs, const Flags flags)
    {
        UpdateOwnerStatusArgs args(regs);
        regs.ebx = updateOwnerStatus(flags, args.ownerStatus);
    }
}
