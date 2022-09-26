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
    struct CompetitorHeader : ObjectHeader
    {
        int32_t unk_10;

        bool isFlagsC0() const
        {
            return (flags & 0xC0) != 0;
        }

        bool operator==(const CompetitorHeader& rhs) const
        {
            if (isFlagsC0())
            {
                return (flags & ~0x00C0) == (rhs.flags & 0xFFFF) && getName() == rhs.getName();
            }
            else
            {
                // NB: unk_10 is ignored
                return std::memcmp(this, &rhs, sizeof(ObjectHeader)) == 0;
            }
        }
    };
    static_assert(sizeof(CompetitorHeader) == 0x14);

    loco_global<CompetitorHeader[32], 0x01129B18> _competitorHeaders;

    // 0x00435506
    static uint32_t changeCompanyFace(uint8_t flags, CompanyId targetCompanyId, CompetitorHeader& targetHeader)
    {
        GameCommands::setExpenditureType(ExpenditureType::Miscellaneous);

        // Find target competitor object among loaded objects
        auto foundCompetitorId = 0xFF;
        for (size_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::competitor); i++)
        {
            auto* competitorObj = ObjectManager::get<CompetitorObject>(i);
            if (competitorObj == nullptr)
                continue;

            auto& competitorHeader = _competitorHeaders[i];
            if (competitorHeader == targetHeader)
            {
                foundCompetitorId = i;
                break;
            }
        }

        // See whether any other company is using this competitor already
        for (auto& company : CompanyManager::companies())
        {
            if (company.competitorId == foundCompetitorId)
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
        if (foundCompetitorId == 0xFF)
        {
            // Load the new competitor object
            if (!ObjectManager::load(targetHeader))
                return GameCommands::FAILURE;

            ObjectManager::reloadAll();
            Ui::WindowManager::close(Ui::WindowType::dropdown);

            auto competitorHandle = ObjectManager::findObjectHandle(targetHeader);
            if (!competitorHandle)
                return GameCommands::FAILURE;

            foundCompetitorId = competitorHandle->id;
        }

        // Unload the target company's current competitor
        auto* targetCompany = CompanyManager::get(targetCompanyId);
        ObjectManager::unload(_competitorHeaders[targetCompany->competitorId]);
        ObjectManager::reloadAll();
        Ui::WindowManager::close(Ui::WindowType::dropdown);

        // Set the new competitor id
        targetCompany->competitorId = foundCompetitorId;

        // Non-player companies should use the competitor object name
        if (!CompanyManager::isPlayerCompany(targetCompanyId))
        {
            auto* competitor = ObjectManager::get<CompetitorObject>(foundCompetitorId);
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
        auto* targetHeader = reinterpret_cast<CompetitorHeader*>(header);
        regs.ebx = changeCompanyFace(regs.bl, CompanyId(regs.bh), *targetHeader);
    }
}
