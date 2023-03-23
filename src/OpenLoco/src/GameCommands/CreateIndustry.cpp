#include "GameCommands.h"
#include "Localisation/StringIds.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "World/IndustryManager.h"

namespace OpenLoco::GameCommands
{
    static bool sub_454C91(uint8_t type, const World::Pos2& pos)
    {
        const auto* indObj = ObjectManager::get<IndustryObject>(type);

        // TODO: MAKE THIS A FUNCTION
        // 0x00E0C38C
        uint32_t producedCargoTypes = 0;
        for (auto& cargoType : indObj->producedCargoType)
        {
            if (cargoType != 0xFFU)
            {
                producedCargoTypes |= (1U << cargoType);
            }
        }

        // 0x00E0C390
        uint32_t requiredCargoTypes = 0;
        for (auto& cargoType : indObj->requiredCargoType)
        {
            if (cargoType != 0xFFU)
            {
                requiredCargoTypes |= (1U << cargoType);
            }
        }

        for (const auto& ind : IndustryManager::industries())
        {
            const auto distance = Math::Vector::manhattanDistance(World::Pos2{ ind.x, ind.y }, pos);

            const auto* indObj2 = ind.getObject();
            uint32_t producedCargoTypes2 = 0;
            for (auto& cargoType : indObj->producedCargoType)
            {
                if (cargoType != 0xFFU)
                {
                    producedCargoTypes2 |= (1U << cargoType);
                }
            }

            uint32_t requiredCargoTypes2 = 0;
            for (auto& cargoType : indObj->requiredCargoType)
            {
                if (cargoType != 0xFFU)
                {
                    requiredCargoTypes2 |= (1U << cargoType);
                }
            }

            const auto hasAtLeast1SameCargo = ((producedCargoTypes & producedCargoTypes2) != 0)
                || ((requiredCargoTypes & requiredCargoTypes2) != 0);

            const auto tooClose = 32 * (hasAtLeast1SameCargo ? 24 : 9);

            if (distance < tooClose)
            {
                GameCommands::setErrorText(StringIds::too_close_to_another_industry);
                return false;
            }
        }

        // 0x00454DBE
        // Find free industry slot
        return true;
    }
}
