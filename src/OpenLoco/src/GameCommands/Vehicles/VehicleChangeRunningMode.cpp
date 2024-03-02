#include "VehicleChangeRunningMode.h"
#include "Economy/Expenditures.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Speed.hpp"
#include "Types.hpp"
#include "Ui/WindowManager.h"
#include "VehicleSell.h"
#include "Vehicles/Vehicle.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Literals;

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

    static void invalidateWindow(EntityId headId)
    {
        Ui::WindowManager::invalidate(Ui::WindowType::vehicle, static_cast<Ui::WindowNumber_t>(headId));
    }

    // 0x004B6AEE
    static uint32_t changeLocalExpressMode(const Vehicles::Vehicle& train, const uint8_t flags)
    {
        if (!(flags & Flags::apply))
        {
            return 0;
        }

        train.veh1->var_48 ^= Vehicles::Flags48::expressMode;
        invalidateWindow(train.head->head);
        return 0;
    }

    // 0x004B6A08
    static uint32_t startStopVehicle(const Vehicles::Vehicle& train, bool startVehicle, const uint8_t flags)
    {
        // Starting this vehicle -- can we?
        if (startVehicle && !sub_4B6B0C(train.head))
        {
            return FAILURE;
        }

        // Stopping this vehicle, but vehicle is already stopped?
        if (!startVehicle && train.head->hasVehicleFlags(VehicleFlags::commandStop))
        {
            return 0;
        }

        // Starting this vehicle, but vehicle is already travelling?
        if (startVehicle && !train.head->hasVehicleFlags(VehicleFlags::commandStop))
        {
            return 0;
        }

        if (!(flags & Flags::apply))
        {
            return 0;
        }

        train.head->vehicleFlags ^= VehicleFlags::commandStop;
        if (train.head->hasVehicleFlags(VehicleFlags::commandStop))
        {
            train.head->vehicleFlags &= ~VehicleFlags::manualControl;
        }

        if (!(train.head->hasVehicleFlags(VehicleFlags::unk_2) || !CompanyManager::isPlayerCompany(train.head->owner)))
        {
            auto madeProfit = train.veh2->profit[0] | train.veh2->profit[1] | train.veh2->profit[2] | train.veh2->profit[3];
            if (madeProfit != 0)
            {
                companyEmotionEvent(getUpdatingCompanyId(), Emotion::happy);
            }
        }

        invalidateWindow(train.head->head);
        return 0;
    }

    // 0x004B6AAF
    static uint32_t toggleManualDriving(const Vehicles::Vehicle& train, const uint8_t flags)
    {
        // Can we change driving modes?
        if (!sub_4B6B0C(train.head))
        {
            return FAILURE;
        }

        if (!(flags & Flags::apply))
        {
            return 0;
        }

        train.head->vehicleFlags ^= VehicleFlags::manualControl;
        train.head->manualPower = -40;

        if ((train.head->vehicleFlags & VehicleFlags::manualControl) != VehicleFlags::none)
        {
            train.head->vehicleFlags &= ~VehicleFlags::commandStop;
        }
        else
        {
            train.head->vehicleFlags |= VehicleFlags::commandStop;
        }

        if (train.head->status == Vehicles::Status::approaching)
        {
            train.head->stationId = StationId::null;
            train.head->status = Vehicles::Status::travelling;
        }

        invalidateWindow(train.head->head);

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
            if ((train.head->status == Vehicles::Status::stuck || train.head->status == Vehicles::Status::crashed) && CompanyManager::isPlayerCompany(train.head->owner))
            {
                if (flags & Flags::apply)
                {
                    // 0x004B69C7
                    train.head->sub_4AD778();
                    train.head->status = Vehicles::Status::stopped;
                    train.veh2->currentSpeed = 0_mph;

                    GameCommands::VehicleSellArgs sargs{};
                    sargs.car = args.head;
                    GameCommands::doCommand(sargs, GameCommands::Flags::apply);
                }
                return 0;
            }

            // Switching to manual driving?
            if (args.mode == VehicleChangeRunningModeArgs::Mode::driveManually)
                return toggleManualDriving(train, flags);

            bool startVehicle = args.mode == VehicleChangeRunningModeArgs::Mode::startVehicle;
            return startStopVehicle(train, startVehicle, flags);
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
