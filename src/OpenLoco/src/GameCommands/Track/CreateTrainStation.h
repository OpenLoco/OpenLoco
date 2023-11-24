#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct TrackStationPlacementArgs
    {
        static constexpr auto command = GameCommand::createTrainStation;

        TrackStationPlacementArgs() = default;
        explicit TrackStationPlacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dl & 0xF)
            , index(regs.dh & 0x3)
            , trackObjectId(regs.bp)
            , type(regs.edi >> 16)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t index;
        uint8_t trackObjectId;
        uint8_t type;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.edi = pos.z | (type << 16);
            regs.bh = rotation;
            regs.dl = trackId;
            regs.dh = index;
            regs.bp = trackObjectId;
            return regs;
        }
    };

    void createTrainStation(registers& regs);
}
