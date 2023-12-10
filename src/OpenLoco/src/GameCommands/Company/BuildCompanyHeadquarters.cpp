#include "BuildCompanyHeadquarters.h"
#include "Audio/Audio.h"
#include "Economy/Expenditures.h"
#include "GameCommands/Company/RemoveCompanyHeadquarters.h"
#include "GameCommands/GameCommands.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"

namespace OpenLoco::GameCommands
{
    // 0x0042ECFC
    static uint32_t buildCompanyHeadquarters(HeadquarterPlacementArgs args, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Construction);
        setPosition(World::Pos3(args.pos.x + World::kTileSize / 2, args.pos.y + World::kTileSize / 2, args.pos.z));

        currency32_t totalCost = 0;
        auto targetCompanyId = getUpdatingCompanyId();
        auto* company = CompanyManager::get(targetCompanyId);

        if (company->headquartersX != -1 && !(flags & Flags::ghost))
        {
            HeadquarterRemovalArgs rmArgs{};
            rmArgs.pos = World::Pos3(company->headquartersX, company->headquartersY, company->headquartersZ * World::kSmallZStep);

            auto rmCost = doCommand(rmArgs, flags);
            if (rmCost != FAILURE)
            {
                totalCost += rmCost;
            }
            else
            {
                return FAILURE;
            }
        }

        BuildingPlacementArgs buildArgs{};
        buildArgs.pos = args.pos;
        buildArgs.rotation = args.rotation;
        buildArgs.type = args.type;
        buildArgs.variation = company->getHeadquarterPerformanceVariation();
        buildArgs.colour = CompanyManager::getCompanyColour(targetCompanyId);
        buildArgs.buildImmediately = args.buildImmediately;

        auto buildCost = doCommand(buildArgs, flags);
        if (buildCost != FAILURE)
        {
            totalCost += buildCost;
        }
        else
        {
            return FAILURE;
        }

        if ((flags & Flags::apply) && !(flags & Flags::ghost))
        {
            company->headquartersX = args.pos.x;
            company->headquartersY = args.pos.y;
            company->headquartersZ = args.pos.z / World::kSmallZStep;
            Ui::WindowManager::invalidate(Ui::WindowType::company, Ui::WindowNumber_t(targetCompanyId));
        }

        if ((flags & Flags::apply) && !(flags & (Flags::aiAllocated | Flags::ghost)))
        {
            Audio::playSound(Audio::SoundId::construct, args.pos);
        }

        return totalCost;
    }

    void buildCompanyHeadquarters(registers& regs)
    {
        HeadquarterPlacementArgs args(regs);
        regs.ebx = buildCompanyHeadquarters(args, regs.bl);
    }
}
