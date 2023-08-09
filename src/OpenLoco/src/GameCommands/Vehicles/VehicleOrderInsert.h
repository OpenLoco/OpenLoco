#pragma once

#include "GameCommands/GameCommands.h"

namespace OpenLoco::GameCommands
{
    struct VehicleOrderInsertArgs
    {
        static constexpr auto command = GameCommand::vehicleOrderInsert;

        VehicleOrderInsertArgs() = default;
        explicit VehicleOrderInsertArgs(const registers& regs)
            : head(EntityId(regs.di))
            , orderOffset(regs.dx)
            , rawOrder((uint64_t(regs.cx) << 32ULL) | regs.eax)
        {
        }

        EntityId head;
        uint32_t orderOffset;
        uint64_t rawOrder;

        explicit operator registers() const
        {
            registers regs;
            regs.di = enumValue(head);
            regs.dx = orderOffset;
            regs.eax = rawOrder & 0xFFFFFFFF;
            regs.cx = rawOrder >> 32;
            return regs;
        }
    };

    void vehicleOrderInsert(registers& regs);
}
