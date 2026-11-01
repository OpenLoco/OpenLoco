#include "GameCommands/Vehicles/VehicleOrderShare.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "S5/Limits.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/Orders.h"
#include "Vehicles/VehicleHead.h"
#include "Vehicles/VehicleManager.h"
#include <utility>

namespace OpenLoco::GameCommands
{
    static bool isOrderValidForVehicle(const Vehicles::Order& order, const Vehicles::VehicleHead& head)
    {
        if (order.is<Vehicles::OrderRouteThrough>()
            && (head.mode == OpenLoco::TransportMode::water || head.mode == OpenLoco::TransportMode::air))
        {
            setErrorText(head.mode == OpenLoco::TransportMode::water ? StringIds::orderTypeNotValidForShips : StringIds::orderTypeNotValidForAircraft);
            return false;
        }

        if (order.is<Vehicles::OrderRouteWaypoint>() && head.mode == OpenLoco::TransportMode::air)
        {
            setErrorText(StringIds::orderTypeNotValidForAircraft);
            return false;
        }

        return true;
    }

    static bool areOrdersValidForVehicle(const Vehicles::VehicleHead& sourceHead, const Vehicles::VehicleHead& targetHead)
    {
        for (const auto& order : Vehicles::OrderRingView(sourceHead.orderTableOffset))
        {
            if (!isOrderValidForVehicle(order, targetHead))
            {
                return false;
            }
        }

        return true;
    }

    static uint32_t vehicleOrderShare(const VehicleOrderShareArgs& args, Flags flags)
    {
        auto* head = EntityManager::get<Vehicles::VehicleHead>(args.head);
        if (head == nullptr)
        {
            return kFailure;
        }

        setPosition(head->position);
        if (!checkCompanyCompatibility(head->owner))
        {
            return kFailure;
        }

        if (args.sourceHead == EntityId::null)
        {
            if (!Vehicles::OrderManager::isOrderTableShared(head))
            {
                return 0;
            }

            if (Vehicles::OrderManager::orderTableLength() + head->sizeOfOrderTable > S5::Limits::kMaxOrders)
            {
                setErrorText(StringIds::no_space_for_more_vehicle_orders);
                return kFailure;
            }

            if (!hasFlags(flags, Flags::apply))
            {
                return 0;
            }

            if (!Vehicles::OrderManager::detachOrders(head))
            {
                setErrorText(StringIds::no_space_for_more_vehicle_orders);
                return kFailure;
            }

            return 0;
        }

        auto* sourceHead = EntityManager::get<Vehicles::VehicleHead>(args.sourceHead);
        if (sourceHead == nullptr || sourceHead->owner != head->owner || sourceHead->vehicleType != head->vehicleType)
        {
            return kFailure;
        }

        if (sourceHead->sizeOfOrderTable == 1 && head->sizeOfOrderTable > 1)
        {
            std::swap(head, sourceHead);
        }

        if (head == sourceHead || head->orderTableOffset == sourceHead->orderTableOffset)
        {
            return 0;
        }

        if (!areOrdersValidForVehicle(*sourceHead, *head))
        {
            return kFailure;
        }

        for (auto* sharedHead : VehicleManager::VehicleList())
        {
            if (sharedHead->orderTableOffset == sourceHead->orderTableOffset && !areOrdersValidForVehicle(*sourceHead, *sharedHead))
            {
                return kFailure;
            }
        }

        if (!hasFlags(flags, Flags::apply))
        {
            return 0;
        }

        Vehicles::OrderManager::linkOrders(head, sourceHead);
        return 0;
    }

    void vehicleOrderShare(registers& regs, const Flags flags)
    {
        regs.ebx = vehicleOrderShare(VehicleOrderShareArgs(regs), flags);
    }
}
