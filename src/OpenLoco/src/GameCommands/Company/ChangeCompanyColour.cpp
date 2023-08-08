#include "ChangeCompanyColour.h"
#include "Audio/Audio.h"
#include "GameCommands/GameCommands.h"
#include "GameException.hpp"
#include "Localisation/StringIds.h"
#include "OpenLoco.h"
#include "Ui/WindowManager.h"
#include "Ui/WindowType.h"
#include "World/CompanyManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x0043483D
    static uint32_t changeCompanyColour(const ChangeCompanyColourSchemeArgs& args, uint8_t flags)
    {
        GameCommands::setExpenditureType(ExpenditureType::Miscellaneous);
        GameCommands::setPosition({ static_cast<int16_t>(0x8000), 0, 0 });

        auto* company = CompanyManager::get(args.companyId);

        if (flags & Flags::apply)
        {
            // Toggling vehicle palette
            if (args.setColourMode)
            {
                if (args.value)
                    company->customVehicleColoursSet |= (1 << args.colourType);
                else
                    company->customVehicleColoursSet &= ~(1 << args.colourType);
            }
            // Setting a colour
            else
            {
                ColourScheme* colours;
                if (args.colourType == 0)
                    colours = &company->mainColours;
                else
                    colours = &company->vehicleColours[args.colourType - 1];

                if (args.isPrimary)
                    colours->primary = static_cast<Colour>(args.value);
                else
                    colours->secondary = static_cast<Colour>(args.value);
            }

            company->updateVehicleColours();
            CompanyManager::updateColours();
            company->updateHeadquartersColour();
            Ui::WindowManager::invalidate(Ui::WindowType::company);
            return 0;
        }
        else
        {
            if (!sub_431E6A(args.companyId, nullptr))
                return GameCommands::FAILURE;

            if (args.setColourMode || args.colourType > 0 || !args.isPrimary)
                return 0;

            // Check whether the requested colour is available
            uint32_t unavailableColours = CompanyManager::competingColourMask(args.companyId);
            if (unavailableColours & (1 << args.value))
            {
                setErrorText(StringIds::empty);
                return GameCommands::FAILURE;
            }
        }

        return 0;
    }

    void changeCompanyColour(registers& regs)
    {
        regs.ebx = changeCompanyColour(ChangeCompanyColourSchemeArgs(regs), regs.bl);
    }
}
