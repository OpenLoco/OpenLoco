#include "GameCommands.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    struct HeadquarterPlacementArgs
    {
        static constexpr auto command = GameCommand::buildCompanyHeadquarters;

        HeadquarterPlacementArgs() = default;
        explicit HeadquarterPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , type(regs.dl)
            , buildImmediately(regs.bh & 0x80)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t type;
        bool buildImmediately = false; // No scaffolding required (editor mode)
        explicit operator registers() const
        {

            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            regs.dx = type;
            regs.bh = rotation | (buildImmediately ? 0x80 : 0);
            return regs;
        }
    };

    void buildCompanyHeadquarters(registers& regs);
}
