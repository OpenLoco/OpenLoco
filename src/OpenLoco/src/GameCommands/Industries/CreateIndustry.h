#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct IndustryPlacementArgs
    {
        static constexpr auto command = GameCommand::createIndustry;

        IndustryPlacementArgs() = default;
        explicit IndustryPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx)
            , type(regs.dl & 0x7F)
            , buildImmediately(regs.dl & 0x80)
            , srand0(regs.ebp)
            , srand1(regs.edi)
        {
        }

        World::Pos2 pos;
        uint8_t type;
        bool buildImmediately = false; // No scaffolding required (editor mode)
        uint32_t srand0;
        uint32_t srand1;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.dl = type | (buildImmediately ? 0x80 : 0);
            regs.ebp = srand0;
            regs.edi = srand1;
            regs.esi = enumValue(command); // Vanilla bug? Investigate when doing createIndustry
            return regs;
        }
    };

    void createIndustry(registers& regs);
}
