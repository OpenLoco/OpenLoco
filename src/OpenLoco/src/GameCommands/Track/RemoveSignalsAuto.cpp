#include "GameCommands/Track/RemoveSignalsAuto.h"
#include "AutoSignals.h"
#include "GameCommands/Track/RemoveSignal.h"
#include "Map/TrackElement.h"

namespace OpenLoco::GameCommands
{
    static uint32_t removeSignal(const World::Pos3& pos, const uint16_t tad, const uint16_t sides, const uint8_t trackObjType, const uint8_t flags)
    {
        GameCommands::SignalRemovalArgs sargs{};
        sargs.pos = pos;
        sargs.rotation = tad & 0x3;
        sargs.trackId = (tad >> 3) & 0x3F;
        sargs.index = 0;
        sargs.flags = sides;
        sargs.trackObjType = trackObjType;

        return GameCommands::doCommand(sargs, flags);
    }

    static uint32_t removeSignalsAuto(const SignalsRemovalAutoArgs& args, const uint8_t flags)
    {
        return autoSignalsWalk(
            args.pos,
            args.trackId,
            args.rotation,
            args.index,
            args.trackObjType,
            args.flags,
            args.step,
            flags,
            [](const World::TrackElement&) { return false; },
            [](const World::Pos3& pos, const uint16_t tad, const uint16_t sides, const uint8_t trackObjType, const uint8_t flags) {
                return removeSignal(pos, tad, sides, trackObjType, flags);
            });
    }

    void removeSignalsAuto(registers& regs, const uint8_t flags)
    {
        regs.ebx = removeSignalsAuto(SignalsRemovalAutoArgs(regs), flags);
    }
}
