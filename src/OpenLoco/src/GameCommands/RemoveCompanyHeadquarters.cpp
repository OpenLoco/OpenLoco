#include "Audio/Audio.h"
#include "Economy/Expenditures.h"
#include "GameCommands.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"

namespace OpenLoco::GameCommands
{
    // 0x0042EEAF
    static uint32_t removeCompanyHeadquarters(World::Pos3 pos, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(pos);

        currency32_t totalCost = 0;
        auto targetCompanyId = CompanyId::null;

        if (!(flags & Flags::ghost))
        {
            auto baseZ = pos.z / World::kSmallZStep;
            for (auto& company : CompanyManager::companies())
            {
                if (company.headquartersX != pos.x)
                    continue;

                if (company.headquartersY != pos.y)
                    continue;

                if (company.headquartersZ != baseZ)
                    continue;

                if (!sub_431E6A(company.id(), nullptr))
                    return GameCommands::FAILURE;

                targetCompanyId = company.id();
                break;
            }
        }

        BuildingRemovalArgs args{};
        args.pos = pos;
        if (auto cost = GameCommands::doCommand(args, flags); cost != FAILURE)
        {
            totalCost += cost;
        }
        else
        {
            return FAILURE;
        }

        if ((flags & Flags::apply) && !(flags & Flags::ghost))
        {
            auto* company = CompanyManager::get(targetCompanyId);
            company->headquartersX = -1;
            Ui::WindowManager::invalidate(Ui::WindowType::company, Ui::WindowNumber_t(targetCompanyId));
        }

        if ((flags & Flags::apply) && !(flags & (Flags::flag_4 | Flags::ghost)))
        {
            Audio::playSound(Audio::SoundId::demolish, pos);
        }

        return totalCost;
    }

    void removeCompanyHeadquarters(registers& regs)
    {
        HeadquarterRemovalArgs args(regs);
        regs.ebx = removeCompanyHeadquarters(args.pos, regs.bl);
    }
}
