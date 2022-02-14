#include "../CompanyManager.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    static loco_global<CompanyId, 0x009C68EB> _updatingCompanyId;

    // 0x004383CA
    static uint32_t updateOwnerStatus(const uint8_t flags, OwnerStatus ownerStatus)
    {
        if (flags & Flags::apply)
        {
            auto companyId = CompanyManager::getUpdatingCompanyId();
            auto company = CompanyManager::get(companyId);
            if (company == nullptr)
            {
                return FAILURE;
            }
            company->ownerStatus = ownerStatus;
        }
        return 0;
    }

    void updateOwnerStatus(registers& regs)
    {
        UpdateOwnerStatusArgs args(regs);
        regs.ebx = updateOwnerStatus(regs.bl, args.ownerStatus);
    }
}
