#include "GameCommands/Vehicles/VehicleOrderInsert.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Objects/VehicleObject.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/Orders.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleHead.h"
#include "Vehicles/VehicleManager.h"
#include "World/StationManager.h"

using namespace OpenLoco::Vehicles;

namespace OpenLoco::GameCommands
{
    // 0x0047036E
    static uint32_t vehicleOrderInsert(const VehicleOrderInsertArgs& args, Flags flags)
    {
        auto* head = EntityManager::get<VehicleHead>(args.head);
        if (head == nullptr)
        {
            return GameCommands::kFailure;
        }

        GameCommands::setPosition(head->position);

        auto* order = reinterpret_cast<const Order*>(&args.rawOrder);

        // Ensure we can use any station that has been selected
        if (order->hasFlags(OrderFlags::HasStation))
        {
            auto* stationOrder = order->as<OrderStation>();
            if (stationOrder != nullptr)
            {
                auto* station = StationManager::get(stationOrder->getStation());
                if (station->owner != head->owner)
                {
                    setErrorText(StringIds::stationOwnedByAnotherCompany);
                    return kFailure;
                }
            }
        }

        if (args.orderOffset > head->sizeOfOrderTable)
        {
            return kFailure;
        }

        OrderStopAt* orderToConvert = nullptr;
        if (order->getType() == OrderType::StopAt && !(head->mode == TransportMode::water || head->mode == TransportMode::air))
        {
            // To get the previous order (to check if it was also the same stop order)
            // its a bit convoluted we need to iterate the order table keeping
            // a lagging previous iterator. Once the iterator either points at the
            // target order or its reached the end then we have the previous order.
            auto orderTable = OrderRingView(head->orderTableOffset, 0);
            auto iter = orderTable.begin();
            auto targetOrder = head->orderTableOffset + args.orderOffset;
            auto previous = iter;
            if (iter->getOffset() != targetOrder && iter != orderTable.end())
            {
                iter++;
                while (iter->getOffset() != targetOrder && iter != orderTable.end())
                {
                    iter++;
                    previous++;
                }
            }

            auto* existingOrder = previous->as<OrderStopAt>();
            auto* newOrder = order->as<OrderStopAt>();

            if (existingOrder != nullptr && newOrder != nullptr && existingOrder->getStation() == newOrder->getStation())
            {
                orderToConvert = existingOrder;
            }
        }

        // Certain order can't be used by ships or aircraft
        const bool willAddRouteThrough = order->is<OrderRouteThrough>() || orderToConvert != nullptr;

        if (willAddRouteThrough || order->is<OrderRouteWaypoint>())
        {
            for (auto* otherHead : VehicleManager::VehicleList())
            {
                if (otherHead->orderTableOffset != head->orderTableOffset)
                {
                    continue;
                }

                if (otherHead->mode == TransportMode::water && willAddRouteThrough)
                {
                    setErrorText(StringIds::orderTypeNotValidForShips);
                    return kFailure;
                }
                else if (otherHead->mode == TransportMode::air)
                {
                    setErrorText(StringIds::orderTypeNotValidForAircraft);
                    return kFailure;
                }
            }
        }

        // Verify we have room for more orders, both globally and for this particular vehicle
        if (!OrderManager::spaceLeftInGlobalOrderTableForOrder(order))
        {
            setErrorText(StringIds::no_space_for_more_vehicle_orders);
            return kFailure;
        }
        if (!OrderManager::spaceLeftInVehicleOrderTable(head))
        {
            setErrorText(StringIds::tooManyOrdersForThisVehicle);
            return kFailure;
        }

        if (!hasFlags(flags, GameCommands::Flags::apply))
        {
            return 0;
        }

        OrderManager::invalidateOrderPagesByOrderTableOffset(head->orderTableOffset);

        if (orderToConvert != nullptr)
        {
            orderToConvert->setType(OrderType::RouteThrough);
            return 0;
        }

        // All looks good -- insert a brand new order!
        OrderManager::insertOrder(head, args.orderOffset, order);

        return 0;
    }

    void vehicleOrderInsert(registers& regs, const Flags flags)
    {
        regs.ebx = vehicleOrderInsert(VehicleOrderInsertArgs(regs), flags);
    }
}
