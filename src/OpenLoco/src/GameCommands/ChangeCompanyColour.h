#pragma once

#include "GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct ChangeCompanyColourSchemeArgs
    {
        static constexpr auto command = GameCommand::changeCompanyColourScheme;

        ChangeCompanyColourSchemeArgs() = default;
        explicit ChangeCompanyColourSchemeArgs(const registers& regs)
            : companyId(CompanyId(regs.dl))
            , isPrimary()
            , value(regs.al)
            , colourType(regs.cl)
            , setColourMode(regs.dh)
        {
            if (!setColourMode)
            {
                isPrimary = regs.ah == 0;
            }
        }

        CompanyId companyId;
        bool isPrimary;
        uint8_t value;
        uint8_t colourType;
        bool setColourMode;

        explicit operator registers() const
        {
            registers regs;

            regs.cl = colourType;           // vehicle type or main
            regs.dh = setColourMode;        // [ 0, 1 ] -- 0 = set colour, 1 = toggle enabled/disabled;
            regs.dl = enumValue(companyId); // company id

            if (!setColourMode)
            {
                // cl is divided by 2 when used
                regs.ah = isPrimary ? 1 : 0; // [ 0, 1 ] -- primary or secondary palette
                regs.al = value;             // new colour
            }
            else if (setColourMode)
            {
                regs.al = value; // [ 0, 1 ] -- off or on
            }

            return regs;
        }
    };

    void changeCompanyColour(registers& regs);
}
