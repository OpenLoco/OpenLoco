#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct ChangeCompanyOwnerNameArgs
    {
        static constexpr auto command = GameCommand::changeCompanyOwnerName;

        ChangeCompanyOwnerNameArgs() = default;
        explicit ChangeCompanyOwnerNameArgs(const registers& regs)
            : companyId(CompanyId(regs.cx))
            , bufferIndex(regs.ax)
        {
            memcpy(newName, &regs.edx, 4);
            memcpy(newName + 4, &regs.ebp, 4);
            memcpy(newName + 8, &regs.edi, 4);
        }

        CompanyId companyId;
        uint16_t bufferIndex;
        char newName[37];

        explicit operator registers() const
        {
            registers regs;
            regs.cl = enumValue(companyId);
            regs.ax = bufferIndex; // [ 0, 1, 2]
            constexpr std::array<uint8_t, 3> iToOffset = { 24, 0, 12 };
            const auto offset = iToOffset[bufferIndex];

            std::memcpy(&regs.edx, newName + offset, 4);
            std::memcpy(&regs.ebp, newName + offset + 4, 4);
            std::memcpy(&regs.edi, newName + offset + 8, 4);

            return regs;
        }
    };

    void changeCompanyOwnerName(registers& regs);
}
