#include "GameCommands.h"

namespace OpenLoco::GameCommands
{

    struct ChangeLandMaterialArgs
    {
        static constexpr auto command = GameCommand::changeLandMaterial;
        ChangeLandMaterialArgs() = default;
        explicit ChangeLandMaterialArgs(const registers& regs)
            : pointA(regs.ax, regs.cx)
            , pointB(regs.di, regs.bp)
            , landType(regs.dl)
        {
        }

        World::Pos2 pointA;
        World::Pos2 pointB;
        uint8_t landType;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pointA.x;
            regs.cx = pointA.y;
            regs.di = pointB.x;
            regs.bp = pointB.y;
            regs.dl = landType;
            return regs;
        }
    };

    void changeLandMaterial(registers& regs);

}
