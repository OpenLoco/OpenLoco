#include "GameCommands/Track/CreateSignalsAuto.h"
#include "AutoSignals.h"
#include "GameCommands/Track/CreateSignal.h"
#include "Map/TrackElement.h"

namespace OpenLoco::GameCommands
{
    static uint32_t placeSignal(const World::Pos3& pos, const uint16_t tad, const uint16_t sides, const uint8_t trackObjType, const uint8_t signalType, const uint8_t index, const uint8_t flags)
    {
        GameCommands::SignalPlacementArgs sargs{};
        sargs.pos = pos;
        sargs.rotation = tad & 0x3;
        sargs.trackId = (tad >> 3) & 0x3F;
        sargs.index = index;
        sargs.sides = sides;
        sargs.trackObjType = trackObjType;
        sargs.type = signalType;

        return GameCommands::doCommand(sargs, flags);
    }

    static uint32_t createSignalsAuto(const SignalsPlacementAutoArgs& args, const uint8_t flags)
    {
        return autoSignalsWalk(
            args.pos,
            args.trackId,
            args.rotation,
            args.index,
            args.trackObjType,
            args.sides,
            args.step,
            flags,
            [](const World::TrackElement& elTrack) { return elTrack.hasSignal(); },
            [signalType = args.type](const World::Pos3& pos, const uint16_t tad, const uint16_t sides, const uint8_t trackObjType, const uint8_t index, const uint8_t flags) {
                return placeSignal(pos, tad, sides, trackObjType, signalType, index, flags);
            });
    }

    void createSignalsAuto(registers& regs, const uint8_t flags)
    {
        regs.ebx = createSignalsAuto(SignalsPlacementAutoArgs(regs), flags);
    }
}
