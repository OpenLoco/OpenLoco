#include "GameCommands/Company/UpdateOwnerStatus.h"
#include "GameCommands/GameCommands.h"
#include "World/CompanyManager.h"

namespace OpenLoco::GameCommands
{
    // 0x004383CA
    static uint32_t updateOwnerStatus(const uint8_t flags, const OwnerStatus& ownerStatus)
    {
        if (flags & Flags::apply)
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

    void updateOwnerStatus(registers& regs, const uint8_t flags)
    {
        UpdateOwnerStatusArgs args(regs);
        regs.ebx = updateOwnerStatus(flags, args.ownerStatus);
    }
}
