#pragma once

#include "GameCommands/GameCommands.h"
#include <OpenLoco/Core/EnumFlags.hpp>

struct ColourScheme;
using Colour = uint8_t;

namespace OpenLoco::GameCommands
{
    using QuadraColour = std::array<ColourScheme, 4>;

    constexpr uint8_t kBodyColour = 0;
    constexpr uint8_t kFrontBogieColour = 1;
    constexpr uint8_t kBackBogieColour = 2;

    namespace VehicleRepaintFlags
    {
        constexpr uint8_t bodyColour = (1U << 0);
        constexpr uint8_t frontBogieColour = (1U << 1);
        constexpr uint8_t backBogieColour = (1U << 2);
        constexpr uint8_t applyToEntireCar = (1U << 3);
        constexpr uint8_t applyToEntireTrain = (1U << 4);
    };

    struct VehicleRepaintArgs
    {
        static constexpr auto command = GameCommand::vehiclePaint;

        VehicleRepaintArgs() = default;
        explicit VehicleRepaintArgs(const registers& regs)
            : head(EntityId(regs.di))
            , colours{ regs.cx, regs.ecx >> 8, regs.dx, regs.edx >> 8 }
            , paintFlags(regs.bl)
        {
        }

        EntityId head;
        QuadraColour colours;
        uint8_t paintFlags;

        explicit operator registers() const
        {
            registers regs;
            regs.ebp = enumValue(head);
            regs.ecx = enumValue(colours[0]) | (enumValue(colours[1]) << 8);
            regs.edx = enumValue(colours[2]) | (enumValue(colours[3]) << 8);
            regs.ax = enumValue(paintFlags);
            return regs;
        }
    };

    void vehicleRepaint(registers& regs);
}
