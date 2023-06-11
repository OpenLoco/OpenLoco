#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "Objects/VehicleObject.h"
#include "Ui/WindowManager.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/Orders.h"
#include "Vehicles/Vehicle.h"
#include "World/StationManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Vehicles;

namespace OpenLoco::GameCommands
{
    // 0x0047036E
    static uint32_t vehicleOrderInsert(const VehicleOrderInsertArgs& args, uint8_t flags)
    {
        auto* head = EntityManager::get<VehicleHead>(args.head);
        if (head == nullptr)
        {
            return GameCommands::FAILURE;
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
                    return FAILURE;
                }
            }
        }

        // Waypoint orders can't be used by ships or aircraft
        if (order->is<OrderRouteThrough>() || order->is<OrderRouteWaypoint>())
        {
            if (head->mode == TransportMode::water)
            {
                setErrorText(StringIds::orderTypeNotValidForShips);
                return FAILURE;
            }
            else if (head->mode == TransportMode::air)
            {
                setErrorText(StringIds::orderTypeNotValidForAircraft);
                return FAILURE;
            }
        }

        // Verify we have room for more orders, both globally and for this particular vehicle
        if (!OrderManager::spaceLeftInGlobalOrderTableForOrder(order))
        {
            setErrorText(StringIds::no_space_for_more_vehicle_orders);
            return FAILURE;
        }
        if (!OrderManager::spaceLeftInVehicleOrderTable(head))
        {
            setErrorText(StringIds::tooManyOrdersForThisVehicle);
            return FAILURE;
        }

        if (!(flags & GameCommands::Flags::apply))
        {
            return 0;
        }

        Ui::WindowManager::sub_4B93A5(enumValue(head->id));

        // If we're inserting the same stop order once more, change its type to route through
        if (order->getType() == OrderType::StopAt && !(head->mode == TransportMode::water || head->mode == TransportMode::air))
        {
            // 0x004704AB
            auto orderTable = OrderRingView(head->orderTableOffset, args.orderOffset);
            auto* existingOrder = (*orderTable.begin()).as<OrderStopAt>();
            auto* newOrder = order->as<OrderStopAt>();

            if (existingOrder != nullptr && newOrder != nullptr && existingOrder->getStation() == newOrder->getStation())
            {
                // 0x00470499
                existingOrder->setType(OrderType::RouteThrough);
                return 0;
            }
        }

        // All looks good -- insert a brand new order!
        OrderManager::insertOrder(head, args.orderOffset, order);

        return 0;
    }

    void vehicleOrderInsert(registers& regs)
    {
        regs.ebx = vehicleOrderInsert(VehicleOrderInsertArgs(regs), regs.bl);
    }
}
