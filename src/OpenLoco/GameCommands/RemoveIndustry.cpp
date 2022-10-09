#include "../Economy/Economy.h"
#include "../Economy/Expenditures.h"
#include "../Industry.h"
#include "../IndustryManager.h"
#include "../Localisation/StringManager.h"
#include "../Map/TileManager.h"
#include "../MessageManager.h"
#include "../Objects/IndustryObject.h"
#include "../Objects/ObjectManager.h"
#include "../S5/S5.h"
#include "../StationManager.h"
#include "../Ui/WindowManager.h"
#include "GameCommands.h"

namespace OpenLoco::GameCommands
{
    // 0x0045579F
    static void removeIndustryElement(const Map::Pos3& pos) {}

    // 0x00455A5C
    static void revokeAllSurfaceClaims() {}

    // 0x00455943
    static uint32_t removeIndustry(IndustryId id, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Miscellaneous);
        auto* industry = IndustryManager::get(id);
        if (industry == nullptr)
        {
            return FAILURE;
        }
        auto* indObj = industry->getObject();
        if (indObj == nullptr)
        {
            return FAILURE;
        }

        const auto height = Map::TileManager::getHeight(Map::Pos2{ industry->x, industry->y } + Map::Pos2{ 16, 16 });
        setPosition({ industry->x, industry->y, height.landHeight });
        if (flags & Flags::apply)
        {
            for (auto i = industry->numTiles; i != 0; --i)
            {
                auto tile = industry->tiles[0];
                tile.z &= ~(1 << 15);
                removeIndustryElement(tile);
            }

            S5::getOptions().madeAnyChanges = 1;
            Ui::WindowManager::close(Ui::WindowType::industry, enumValue(id));
            StringManager::emptyUserString(industry->name);
            industry->name = StringIds::null;
            Ui::Windows::IndustryList::removeIndustry(id);
            revokeAllSurfaceClaims();

            for (auto& station : StationManager::stations())
            {
                for (auto& stat : station.cargoStats)
                {
                    if (stat.industryId == id)
                    {
                        stat.industryId = IndustryId::null;
                    }
                }
            }

            MessageManager::removeAllSubjectRefs(enumValue(id), MessageItemArgumentType::industry);
        }

        return Economy::getInflationAdjustedCost(indObj->clearCostFactor, indObj->costIndex, 3);
    }

    void removeIndustry(registers& regs)
    {
        IndustryRemovalArgs args(regs);
        regs.ebx = removeIndustry(args.industryId, regs.bl);
    }
}
