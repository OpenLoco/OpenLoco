#include "VehicleSpeedControl.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Types.hpp"
#include "Ui/WindowManager.h"
#include "VehicleSell.h"
#include "Vehicles/Vehicle.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x004BAB63
    static uint32_t vehicleSpeedControl(const VehicleSpeedControlArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::TrainRunningCosts);

        if (!(flags & Flags::apply))
        {
            return 0;
        }

        auto* head = EntityManager::get<Vehicles::VehicleHead>(args.head);
        if (head == nullptr)
        {
            return FAILURE;
        }

        head->var_6E = args.speed;

        if (head->hasVehicleFlags(VehicleFlags::commandStop))
        {
            if (head->status == Vehicles::Status::stuck || head->status == Vehicles::Status::crashed)
            {
                // The game would initially call vehicleChangeRunningMode directly,
                // which would in turn sell the vehicle. Our implementation changes this
                // to sell a crashed vehicle directly.
                GameCommands::VehicleSellArgs sargs{};
                sargs.car = args.head;
                GameCommands::doCommand(sargs, GameCommands::Flags::apply);
            }
        }

        Ui::WindowManager::invalidate(Ui::WindowType::vehicle, static_cast<Ui::WindowNumber_t>(args.head));
        return 0;
    }

    void vehicleSpeedControl(registers& regs)
    {
        regs.ebx = vehicleSpeedControl(VehicleSpeedControlArgs(regs), regs.bl);
    }
}
