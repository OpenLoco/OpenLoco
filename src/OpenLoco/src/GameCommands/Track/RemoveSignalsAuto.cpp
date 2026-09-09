#include "GameCommands/Track/RemoveSignalsAuto.h"
#include "AutoSignals.h"
#include "GameCommands/Track/RemoveSignal.h"
#include "Map/TrackElement.h"

namespace OpenLoco::GameCommands
{
    static uint32_t removeSignal(const World::Pos3& pos, const uint16_t tad, const uint16_t sides, const uint8_t trackObjType, const uint8_t index, const Flags flags)
    {
        GameCommands::SignalRemovalArgs sargs{};
        sargs.pos = pos;
        sargs.rotation = tad & 0x3;
        sargs.trackId = (tad >> 3) & 0x3F;
        sargs.index = index;
        sargs.flags = sides;
        sargs.trackObjType = trackObjType;

        return GameCommands::doCommand(sargs, flags);
    }

    static uint32_t removeSignalsAuto(const SignalsRemovalAutoArgs& args, const Flags flags)
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
            [](const World::Pos3& pos, const uint16_t tad, const uint16_t sides, const uint8_t trackObjType, const uint8_t index, const Flags flags) {
                return removeSignal(pos, tad, sides, trackObjType, index, flags);
            });
    }

    void removeSignalsAuto(registers& regs, const Flags flags)
    {
        regs.ebx = removeSignalsAuto(SignalsRemovalAutoArgs(regs), flags);
    }
}
