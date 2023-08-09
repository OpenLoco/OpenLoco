#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct ChangeCompanyNameArgs
    {
        static constexpr auto command = GameCommand::changeCompanyName;

        ChangeCompanyNameArgs() = default;
        explicit ChangeCompanyNameArgs(const registers& regs)
            : companyId(CompanyId(regs.cx))
            , bufferIndex(regs.ax)
        {
            memcpy(buffer, &regs.edx, 4);
            memcpy(buffer + 4, &regs.ebp, 4);
            memcpy(buffer + 8, &regs.edi, 4);
        }

        CompanyId companyId;
        uint16_t bufferIndex;
        char buffer[37];

        explicit operator registers() const
        {
            registers regs;
            regs.cl = enumValue(companyId);
            regs.ax = bufferIndex; // [ 0, 1, 2]
            constexpr std::array<uint8_t, 3> iToOffset = { 24, 0, 12 };
            const auto offset = iToOffset[bufferIndex];

            std::memcpy(&regs.edx, buffer + offset, 4);
            std::memcpy(&regs.ebp, buffer + offset + 4, 4);
            std::memcpy(&regs.edi, buffer + offset + 8, 4);

            return regs;
        }
    };

    void changeCompanyName(registers& regs);
}
