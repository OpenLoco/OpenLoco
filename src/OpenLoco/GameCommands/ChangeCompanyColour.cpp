#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../GameException.hpp"
#include "../Localisation/StringIds.h"
#include "../OpenLoco.h"
#include "../Ui/WindowManager.h"
#include "../Ui/WindowType.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x0043483D
    static uint32_t changeCompanyColour(uint8_t flags, uint8_t type, bool toggleMode, CompanyId companyId, bool isSecondary, uint8_t value)
    {
        GameCommands::setExpenditureType(ExpenditureType::Miscellaneous);
        GameCommands::setPosition({ static_cast<int16_t>(0x8000), 0, 0 });

        auto* company = CompanyManager::get(companyId);

        if (flags & Flags::apply)
        {
            // Toggling vehicle palette
            if (toggleMode)
            {
                if (value)
                    company->customVehicleColoursSet |= (1 << type);
                else
                    company->customVehicleColoursSet &= ~(1 << type);
            }
            // Setting a colour
            else
            {
                ColourScheme* colours;
                if (type == 0)
                    colours = &company->mainColours;
                else
                    colours = &company->vehicleColours[type - 1];

                if (!isSecondary)
                    colours->primary = static_cast<Colour>(value);
                else
                    colours->secondary = static_cast<Colour>(value);
            }

            company->updateVehicleColours();
            CompanyManager::updateColours();
            company->updateHeadquartersColour();
            Ui::WindowManager::invalidate(Ui::WindowType::company);
            return 0;
        }
        else
        {
            if (!sub_431E6A(companyId, nullptr))
                return GameCommands::FAILURE;

            if (toggleMode || type > 0 || isSecondary)
                return 0;

            // Check whether the requested colour is available
            uint32_t unavailableColours = CompanyManager::competingColourMask(companyId);
            if (unavailableColours & (1 << value))
            {
                setErrorText(StringIds::empty);
                return GameCommands::FAILURE;
            }
        }

        return 0;
    }

    void changeCompanyColour(registers& regs)
    {
        regs.ebx = changeCompanyColour(regs.bl, regs.cl, regs.dh, CompanyId(regs.dl), regs.ah, regs.al);
    }
}
