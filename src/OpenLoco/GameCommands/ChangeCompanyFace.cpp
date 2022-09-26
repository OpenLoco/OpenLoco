#include "../CompanyManager.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/CompetitorObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui/WindowManager.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x00435506
    static uint32_t changeCompanyFace(uint8_t flags, CompanyId targetCompanyId, ObjectHeader& targetHeader)
    {
        GameCommands::setExpenditureType(ExpenditureType::Miscellaneous);

        // Find target competitor object among loaded objects
        auto foundCompetitor = ObjectManager::findObjectHandle(targetHeader);

        // See whether any other company is using this competitor already
        for (auto& company : CompanyManager::companies())
        {
            if (company.competitorId == foundCompetitor->id)
            {
                if (company.id() != targetCompanyId)
                {
                    GameCommands::setErrorText(StringIds::already_selected_for_another_company);
                    return GameCommands::FAILURE;
                }
                else
                {
                    // No change; no work required
                    return 0;
                }
            }
        }

        // Stop here if we're just querying
        if ((flags & GameCommands::Flags::apply) == 0)
            return 0;

        // Do we need to load the competitor, still?
        if (!foundCompetitor)
        {
            // Load the new competitor object
            if (!ObjectManager::load(targetHeader))
                return GameCommands::FAILURE;

            ObjectManager::reloadAll();
            Ui::WindowManager::close(Ui::WindowType::dropdown);

            foundCompetitor = ObjectManager::findObjectHandle(targetHeader);
            if (!foundCompetitor)
                return GameCommands::FAILURE;
        }

        // Unload the target company's current competitor
        auto* targetCompany = CompanyManager::get(targetCompanyId);
        auto headerToUnload = ObjectManager::getHeader({ ObjectType::competitor, targetCompany->competitorId });
        ObjectManager::unload(headerToUnload);
        ObjectManager::reloadAll();
        Ui::WindowManager::close(Ui::WindowType::dropdown);

        // Set the new competitor id
        targetCompany->competitorId = foundCompetitor->id;

        // Non-player companies should use the competitor object name
        if (!CompanyManager::isPlayerCompany(targetCompanyId))
        {
            auto* competitor = ObjectManager::get<CompetitorObject>(foundCompetitor->id);
            auto oldName = targetCompany->name;
            targetCompany->name = competitor->var_00;
            StringManager::emptyUserString(oldName);
        }

        Gfx::invalidateScreen();
        return 0;
    }

    void changeCompanyFace(registers& regs)
    {
        int32_t header[] = { regs.eax, regs.ecx, regs.edx, regs.edi };
        auto* targetHeader = reinterpret_cast<ObjectHeader*>(header);
        regs.ebx = changeCompanyFace(regs.bl, CompanyId(regs.bh), *targetHeader);
    }
}
