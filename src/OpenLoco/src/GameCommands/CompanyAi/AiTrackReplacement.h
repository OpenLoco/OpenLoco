#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct AiTrackReplacementArgs
    {
        static constexpr auto command = GameCommand::aiTrackReplacement;

        AiTrackReplacementArgs() = default;
        explicit AiTrackReplacementArgs(const registers& regs)
            : pos(regs.ax, regs.cx, regs.di)
            , rotation(regs.bh & 0x3)
            , trackId(regs.dl)
            , sequenceIndex(regs.dh)
            , trackObjectId(regs.bp)
        {
        }

        World::Pos3 pos;
        uint8_t rotation;
        uint8_t trackId;
        uint8_t sequenceIndex;
        uint8_t trackObjectId;

        explicit operator registers() const
        {
            registers regs;
            regs.ax = pos.x;
            regs.cx = pos.y;
            regs.di = pos.z;
            regs.bh = rotation;
            regs.dl = trackId;
            regs.dh = sequenceIndex;
            regs.bp = trackObjectId;
            return regs;
        }
    };
}
