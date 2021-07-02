#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../GameException.hpp"
#include "../OpenLoco.h"
#include "../Ui/WindowManager.h"
#include "../Ui/WindowType.h"
#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x0043483D
    static uint32_t changeCompanyColour(uint8_t flags, uint8_t type, bool toggleMode, CompanyId_t companyId, bool isSecondary, uint8_t value)
    {
        GameCommands::setExpenditureType(ExpenditureType::Miscellaneous);
        GameCommands::setPosition({ static_cast<int16_t>(0x8000), 0, 0 });

        auto* company = CompanyManager::get(companyId);

        if ((flags & Flags::apply) == 1)
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
                    colours->primary = value;
                else
                    colours->secondary = value;
            }

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
            uint32_t availableColours = 0x7FFFFFFFF & ~CompanyManager::competingColourMask(companyId);
            if (!(availableColours & value))
            {
                setErrorText(0);
                return GameCommands::FAILURE;
            }
        }

        return 0;
    }

    void changeCompanyColour(registers& regs)
    {
        regs.ebx = changeCompanyColour(regs.bl, regs.cl, regs.dh, regs.dl, regs.ah, regs.al);
    }
}
