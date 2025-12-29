#pragma once

#include "GameCommands/GameCommands.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <array>

namespace OpenLoco::GameCommands
{
    using QuadraColour = std::array<ColourScheme, 4>;

    constexpr uint8_t kBodyColour = 0;
    constexpr uint8_t kFrontBogieColour = 1;
    constexpr uint8_t kBackBogieColour = 2;

    enum class VehicleRepaintFlags : uint8_t
    {
        none = 0,
        bodyColour = (1U << 0),
        frontBogieColour = (1U << 1),
        backBogieColour = (1U << 2),
        applyToEntireCar = (1U << 3),
        applyToEntireTrain = (1U << 4),
        paintFromVehicleUi = bodyColour | frontBogieColour | backBogieColour | applyToEntireCar,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(VehicleRepaintFlags);

    struct VehicleRepaintArgs
    {
        static constexpr auto command = GameCommand::vehicleRepaint;

        VehicleRepaintArgs() = default;
        explicit VehicleRepaintArgs(const registers& regs)
            : head(EntityId(regs.ebp))
            , colours{ ColourScheme(regs.cx), ColourScheme(regs.ecx >> 16), ColourScheme(regs.dx), ColourScheme(regs.edx >> 16) }
            , paintFlags(VehicleRepaintFlags(regs.ax))
        {
        }

        bool hasRepaintFlags(VehicleRepaintFlags flags) const
        {
            return (paintFlags & flags) != VehicleRepaintFlags::none;
        }

        void setColours(ColourScheme colour)
        {
            if (hasRepaintFlags(VehicleRepaintFlags::bodyColour))
            {
                colours[kBodyColour] = colour;
            }
            if (hasRepaintFlags(VehicleRepaintFlags::frontBogieColour))
            {
                colours[kFrontBogieColour] = colour;
            }
            if (hasRepaintFlags(VehicleRepaintFlags::backBogieColour))
            {
                colours[kBackBogieColour] = colour;
            }
        }

        EntityId head;
        QuadraColour colours;
        VehicleRepaintFlags paintFlags;

        constexpr uint16_t convert(ColourScheme colour) const
        {
            return enumValue(colour.primary) | (enumValue(colour.secondary) << 8);
        }

        explicit operator registers() const
        {
            registers regs;
            regs.ebp = enumValue(head);
            regs.ecx = convert(colours[0]) | (convert(colours[1]) << 16);
            regs.edx = convert(colours[2]) | (convert(colours[3]) << 16);
            regs.ax = enumValue(paintFlags);
            return regs;
        }
    };

    void vehicleRepaint(registers& regs);
}
