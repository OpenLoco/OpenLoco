#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct HeadquarterRemovalArgs
    {
        static constexpr auto command = GameCommand::removeCompanyHeadquarters;

        HeadquarterRemovalArgs() = default;
        explicit HeadquarterRemovalArgs(const World::Pos3& place)
            : pos(place)
        {
        }
        explicit HeadquarterRemovalArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
        {
        }

        World::Pos3 pos;
        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            return regs;
        }
    };

    void removeCompanyHeadquarters(registers& regs);
}
