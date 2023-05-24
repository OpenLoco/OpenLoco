#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands.h"
#include "Types.hpp"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::GameCommands
{
    // 0x004B6B0C
    static bool sub_4B6B0C(Vehicles::VehicleHead* head)
    {
        // Looks like a variant of Vehicles::VehicleHead::canBeModified?
        registers regs;
        regs.esi = X86Pointer(head);
        call(0x004B6B0C, regs);
        return regs.eax != 0;
    }

    // 0x004B6AEE
    static uint32_t changeLocalExpressMode(const Vehicles::Vehicle& train, const uint8_t flags)
    {
        if (!(flags & Flags::apply))
        {
            return 0;
        }

        train.veh1->var_48 ^= Vehicles::Flags48::expressMode;
        Ui::WindowManager::invalidate(Ui::WindowType::vehicle, static_cast<Ui::WindowNumber_t>(train.head->id));
        return 0;
    }

    static uint32_t startVehicle([[maybe_unused]] const Vehicles::Vehicle& train, const uint8_t flags)
    {
        if (train.head->status != Vehicles::Status::stopped)
        {
            return 0;
        }

        if (!(flags & Flags::apply))
        {
            return 0;
        }

        // TODO: finish -- combine with stopVehicle?
        return 0;
    }

    static uint32_t stopVehicle(const Vehicles::Vehicle& train, [[maybe_unused]] const uint8_t flags)
    {
        if (!sub_4B6B0C(train.head))
        {
            return FAILURE;
        }

        if (train.head->status == Vehicles::Status::stopped)
        {
            return 0;
        }

        // TODO: finish -- combine with startVehicle?
        return 0;
    }

    // 0x004B6AAF
    static uint32_t driveManually(const Vehicles::Vehicle& train, const uint8_t flags)
    {
        if (!(flags & Flags::apply))
        {
            return 0;
        }

        train.head->vehicleFlags ^= VehicleFlags::manualControl;
        train.head->vehicleFlags |= VehicleFlags::commandStop;
        train.head->var_6E = -40;

        if (train.head->status == Vehicles::Status::approaching)
        {
            train.head->stationId = StationId::null;
            train.head->status = Vehicles::Status::travelling;
        }

        Ui::WindowManager::invalidate(Ui::WindowType::vehicle, static_cast<Ui::WindowNumber_t>(train.head->id));

        return 0;
    }

    // 0x004B694B
    static uint32_t vehicleChangeRunningMode(const VehicleChangeRunningModeArgs& args, const uint8_t flags)
    {
        setExpenditureType(ExpenditureType::TrainRunningCosts);

        try
        {
            Vehicles::Vehicle train(args.head);
            setPosition(train.veh2->position);

            // If we're only changing local/express, we can take a shortcut.
            if (args.mode == VehicleChangeRunningModeArgs::Mode::toggleLocalExpress)
            {
                return changeLocalExpressMode(train, flags);
            }

            // If a vehicle is stuck or crashed, immediately sell the vehicle instead.
            if ((train.head->status == Vehicles::Status::stuck || train.head->status == Vehicles::Status::crashed) &&
                CompanyManager::isPlayerCompany(train.head->owner))
            {
                if (flags & Flags::apply)
                {
                    GameCommands::VehicleSellArgs sargs{};
                    sargs.car = args.head;
                    GameCommands::doCommand(sargs, GameCommands::Flags::apply);
                }
                return 0;
            }

            switch (args.mode)
            {
                case VehicleChangeRunningModeArgs::Mode::stopVehicle:
                    return stopVehicle(train, flags);
                case VehicleChangeRunningModeArgs::Mode::startVehicle:
                    return startVehicle(train, flags);
                case VehicleChangeRunningModeArgs::Mode::driveManually:
                    return driveManually(train, flags);
                default:
                    return FAILURE;
            }
        }
        catch (std::runtime_error&)
        {
            return FAILURE;
        }
    }

    void vehicleChangeRunningMode(registers& regs)
    {
        regs.ebx = vehicleChangeRunningMode(VehicleChangeRunningModeArgs(regs), regs.bl);
    }
}
