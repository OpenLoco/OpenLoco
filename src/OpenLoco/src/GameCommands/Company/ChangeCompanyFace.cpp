#include "ChangeCompanyFace.h"
#include "GameCommands/GameCommands.h"
#include "Localisation/StringIds.h"
#include "Objects/CompetitorObject.h"
#include "Objects/ObjectManager.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x00435506
    static uint32_t changeCompanyFace(uint8_t flags, CompanyId targetCompanyId, ObjectHeader& targetHeader)
    {
        GameCommands::setExpenditureType(ExpenditureType::Miscellaneous);
        GameCommands::setPosition({ static_cast<int16_t>(0x8000), 0, 0 });

        // Find target competitor object among loaded objects
        auto foundCompetitor = ObjectManager::findObjectHandle(targetHeader);
        if (foundCompetitor)
        {
            // See whether any other company is using this competitor already
            for (auto& company : CompanyManager::companies())
            {
                if (company.competitorId != foundCompetitor->id)
                    continue;

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

        // Any other company also using the same competitor?
        // (This shouldn't happen, but might've been hacked in)
        bool otherCompanyUsingOldCompetitor = false;
        for (auto& company : CompanyManager::companies())
        {
            if (company.id() == targetCompanyId)
                continue;

            if (company.competitorId == foundCompetitor->id)
            {
                otherCompanyUsingOldCompetitor = true;
                break;
            }
        }

        // Unload the target company's current competitor, if no other company is using it
        auto* targetCompany = CompanyManager::get(targetCompanyId);
        if (!otherCompanyUsingOldCompetitor)
        {
            auto& headerToUnload = ObjectManager::getHeader({ ObjectType::competitor, targetCompany->competitorId });
            ObjectManager::unload(headerToUnload);
            ObjectManager::reloadAll();
            Ui::WindowManager::close(Ui::WindowType::dropdown);
        }

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
        ChangeCompanyFaceArgs args(regs);
        regs.ebx = changeCompanyFace(regs.bl, args.companyId, args.objHeader);
    }
}
