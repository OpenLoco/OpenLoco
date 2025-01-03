#include "OrderManager.h"
#include "GameState.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Map/MapSelection.h"
#include "Map/RoadElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Objects/CargoObject.h"
#include "Objects/ObjectManager.h"
#include "S5/Limits.h"
#include "S5/S5.h"
#include "Ui/WindowManager.h"
#include "Vehicle.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/VehicleManager.h"
#include "World/StationManager.h"
#include <OpenLoco/Core/Exception.hpp>
#include <OpenLoco/Diagnostics/Logging.h>

#include <sstream>

using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Vehicles
{
    // 0x004FE070
    static constexpr std::array<uint8_t, 6> kOrderSizes = {
        sizeof(OrderEnd),
        sizeof(OrderStopAt),
        sizeof(OrderRouteThrough),
        sizeof(OrderRouteWaypoint),
        sizeof(OrderUnloadAll),
        sizeof(OrderWaitFor),
    };

    // 0x004B49F8
    void OrderStation::setFormatArguments(FormatArguments& args) const
    {
        auto station = StationManager::get(getStation());
        args.push(station->name);
        args.push(station->town);
    }

    // 0x004B4A31
    void OrderCargo::setFormatArguments(FormatArguments& args) const
    {
        auto cargoObj = ObjectManager::get<CargoObject>(getCargo());
        args.push(cargoObj->name);
        args.push(cargoObj->unitInlineSprite);
    }

    Order* OrderRingView::atIndex(const uint8_t index) const
    {
        auto size = std::distance(begin(), end());
        if (index >= size)
        {
            return nullptr;
        }
        auto chosenOrder = std::next(begin(), index);
        return &(*chosenOrder);
    }

    OrderRingView::Iterator& OrderRingView::Iterator::operator++()
    {
        if (enumValue(_currentOrder->getType()) >= std::size(kOrderSizes))
        {
            throw Exception::RuntimeError("Invalid order type!");
        }
        auto* newOrders = reinterpret_cast<uint8_t*>(_currentOrder) + kOrderSizes[static_cast<uint8_t>(_currentOrder->getType())];
        _currentOrder = reinterpret_cast<Order*>(newOrders);
        if (_currentOrder->getType() == OrderType::End)
        {
            _currentOrder = _beginOrderTable;
            _hasLooped = true;
        }
        return *this;
    }

    OrderRingView::Iterator OrderRingView::begin() const
    {
        auto* orderTable = OrderManager::orders();
        return Iterator(&orderTable[_beginTableOffset], &orderTable[_beginTableOffset + _currentOrderOffset]);
    }

    OrderRingView::Iterator OrderRingView::end() const
    {
        return begin();
    }
}

namespace OpenLoco::Vehicles::OrderManager
{
    // TODO: Make this a fixed vector of size 63 (kMaxNumOrderPerVehicle) no need for it to be dynamic
    std::vector<NumDisplayFrame> _displayFrames;
    const std::vector<NumDisplayFrame>& displayFrames() { return _displayFrames; }

    Order* orders() { return reinterpret_cast<Order*>(getGameState().orders); }
    uint32_t& orderTableLength() { return getGameState().orderTableLength; }

    void shiftOrdersLeft(const uint32_t offsetToShiftTowards, const int16_t sizeToShiftBy)
    {
        std::rotate(&orders()[offsetToShiftTowards], &orders()[offsetToShiftTowards + sizeToShiftBy], &orders()[orderTableLength()]);
    }

    void shiftOrdersRight(const uint32_t offsetToShiftFrom, const int16_t sizeToShiftBy)
    {
        std::rotate(&orders()[offsetToShiftFrom], &orders()[orderTableLength()], &orders()[orderTableLength() + sizeToShiftBy]);
    }

    // 0x00470795
    void reoffsetVehicleOrderTables(const uint32_t removeOrderTableOffset, const int16_t sizeOfRemovedOrderTable)
    {
        for (auto head : VehicleManager::VehicleList())
        {
            if (head->orderTableOffset >= removeOrderTableOffset)
            {
                head->orderTableOffset += sizeOfRemovedOrderTable;
            }
        }

        for (auto& frame : _displayFrames)
        {
            if (frame.orderOffset >= removeOrderTableOffset)
            {
                frame.orderOffset += sizeOfRemovedOrderTable;
            }
        }
    }

    bool spaceLeftInGlobalOrderTableForOrder(const Order* order)
    {
        return orderTableLength() + kOrderSizes[enumValue(order->getType())] <= Limits::kMaxOrders;
    }

    bool spaceLeftInVehicleOrderTable(VehicleHead* head)
    {
        auto ring = head->getCurrentOrders();
        size_t size = std::distance(ring.begin(), ring.end());

        return size < Limits::kMaxOrdersPerVehicle;
    }

    // 0x004704AB
    void insertOrder(VehicleHead* head, uint16_t orderOffset, const Order* order)
    {
        assert(order->getType() != OrderType::End && orderOffset < head->sizeOfOrderTable);
        auto insOrderLength = kOrderSizes[enumValue(order->getType())];

        // Shift current order to compensate for the new order.
        // For empty order tables, let current order point to the first order.
        if (head->sizeOfOrderTable > 1 && head->currentOrder >= orderOffset)
        {
            head->currentOrder += insOrderLength;
        }

        head->sizeOfOrderTable += insOrderLength;

        // Shift existing orders to make room for the new one
        shiftOrdersRight(head->orderTableOffset + orderOffset, insOrderLength);
        // Bookkeeping: change order table size
        orderTableLength() += insOrderLength;

        // Calculate destination offset and copy data
        auto rawOrder = order->getRaw();
        auto dest = reinterpret_cast<uint8_t*>(orders() + head->orderTableOffset + orderOffset);
        std::memcpy(dest, &rawOrder, insOrderLength);

        // Recalculate order offsets for other vehicles
        reoffsetVehicleOrderTables(head->orderTableOffset + 1, insOrderLength);
    }

    // 0x004705C0
    void deleteOrder(VehicleHead* head, uint16_t orderOffset)
    {
        assert(orderOffset < head->sizeOfOrderTable);
        // Find out what type the selected order is
        OrderRingView orders(head->orderTableOffset, orderOffset);
        auto& selectedOrder = *(orders.begin());

        auto removeOrderSize = kOrderSizes[enumValue(selectedOrder.getType())];
        head->sizeOfOrderTable -= removeOrderSize;

        // Are we removing an order that appears before the current order? Move back a bit
        if (head->currentOrder > orderOffset)
        {
            head->currentOrder -= removeOrderSize;
        }

        // Ensure we don't move beyond the order table size
        if (head->currentOrder + 1 >= head->sizeOfOrderTable)
        {
            head->currentOrder = 0;
        }

        // Move orders in the order table, effectively removing the order
        shiftOrdersLeft(head->orderTableOffset + orderOffset, removeOrderSize);

        // Bookkeeping: change order table size
        orderTableLength() -= removeOrderSize;

        // Compensate other vehicles to use new table offsets
        reoffsetVehicleOrderTables(head->orderTableOffset + orderOffset + 1, -removeOrderSize);
    }

    // 0x004702F7
    void zeroUnusedOrderTable()
    {
        // Zero all the unused entries after the end of the order table
        std::fill(std::begin(getGameState().orders) + orderTableLength(), std::end(getGameState().orders), 0);
    }

    // 0x004702EC
    void reset()
    {
        // No need to zero order table as it will get cleaned up on save
        orderTableLength() = 0;
    }

    // 0x00470334
    // Remove vehicle ?orders?
    void freeOrders(VehicleHead* const head)
    {
        // Copy the offset as it will get modified during sub_470795
        const auto offset = head->orderTableOffset;
        const auto size = head->sizeOfOrderTable;

        reoffsetVehicleOrderTables(offset, -size);

        // Shift orders table left to remove empty orders
        shiftOrdersLeft(offset, size);

        orderTableLength() -= head->sizeOfOrderTable;
    }

    // 0x00470312
    void allocateOrders(VehicleHead& head)
    {
        OrderEnd end{};
        constexpr auto insOrderLength = kOrderSizes[enumValue(OrderEnd::kType)];

        head.orderTableOffset = orderTableLength();
        // Bookkeeping: change order table size
        orderTableLength() += insOrderLength;

        head.currentOrder = 0;
        head.sizeOfOrderTable = insOrderLength;

        // Calculate destination offset and copy data
        auto rawOrder = end.getRaw();
        auto dest = reinterpret_cast<uint8_t*>(orders() + head.orderTableOffset);
        std::memcpy(dest, &rawOrder, insOrderLength);
    }

    // 0x00470B76
    std::pair<World::Pos3, std::string> generateOrderUiStringAndLoc(uint32_t orderOffset, uint8_t orderNum)
    {
        std::stringstream ss;
        ss << ControlCodes::inlineSpriteStr;

        auto imageId = Gfx::recolour(ImageIds::getNumberCircle(orderNum), Colour::white);
        ss.write(reinterpret_cast<const char*>(&imageId), 4);
        OrderRingView orderRing(orderOffset);
        auto order = orderRing.begin();
        World::Pos3 pos{};

        switch (order->getType())
        {
            case Vehicles::OrderType::StopAt:
            {
                auto* stopAt = order->as<Vehicles::OrderStopAt>();
                // 0x00470B7D
                auto* station = StationManager::get(stopAt->getStation());
                pos = World::Pos3{ station->x, station->y, station->z } + World::Pos3{ 0, 0, 30 };
                ss << ControlCodes::Colour::white;
                for (auto nextOrder = order + 1; nextOrder != order; ++nextOrder)
                {
                    if (!nextOrder->hasFlags(Vehicles::OrderFlags::HasCargo))
                    {
                        break;
                    }
                    auto* unload = nextOrder->as<Vehicles::OrderUnloadAll>();
                    auto* waitFor = nextOrder->as<Vehicles::OrderWaitFor>();
                    const CargoObject* cargoObj = nullptr;
                    if (unload != nullptr)
                    {
                        ss << " - ";
                        cargoObj = ObjectManager::get<CargoObject>(unload->getCargo());
                    }
                    else if (waitFor != nullptr)
                    {
                        ss << " + ";
                        cargoObj = ObjectManager::get<CargoObject>(waitFor->getCargo());
                    }
                    else
                    {
                        // Not possible
                        break;
                    }
                    ss << ControlCodes::inlineSpriteStr;
                    ss.write(reinterpret_cast<const char*>(&cargoObj->unitInlineSprite), 4);
                }
                break;
            }
            case Vehicles::OrderType::RouteThrough:
            {

                // 0x00470C25
                auto* station = StationManager::get(order->as<Vehicles::OrderRouteThrough>()->getStation());
                pos = World::Pos3{ station->x, station->y, station->z } + World::Pos3{ 0, 0, 30 };
                break;
            }
            case Vehicles::OrderType::RouteWaypoint:
                // 0x00470C6F
                pos = order->as<Vehicles::OrderRouteWaypoint>()->getWaypoint() + World::Pos3{ 16, 16, 8 };
                break;
            case Vehicles::OrderType::End:
            case Vehicles::OrderType::UnloadAll:
            case Vehicles::OrderType::WaitFor:
                return { {}, {} };
        }
        ss << std::endl;
        return { pos, ss.str() };
    }

    // 0x00470824
    void generateNumDisplayFrames(Vehicles::VehicleHead* head)
    {
        World::setMapSelectionFlags(World::MapSelectionFlags::unk_04);
        Gfx::invalidateScreen();
        _displayFrames.clear();
        auto orders = Vehicles::OrderRingView(head->orderTableOffset);
        uint8_t i = 0;
        for (auto& order : orders)
        {
            if (!order.hasFlags(Vehicles::OrderFlags::HasNumber))
            {
                continue;
            }
            NumDisplayFrame newFrame;
            newFrame.orderOffset = order.getOffset();

            auto lineNumber = 0;
            auto* stationOrder = order.as<Vehicles::OrderStation>();
            auto* waypointOrder = order.as<Vehicles::OrderRouteWaypoint>();
            if (stationOrder != nullptr)
            {
                lineNumber++; // station labels start on line 0 so add 1 for the number

                const auto stationId = stationOrder->getStation();
                for (auto innerOrder = orders.begin(); innerOrder->getOffset() != order.getOffset(); ++innerOrder)
                {
                    auto* innerStationOrder = innerOrder->as<Vehicles::OrderStation>();
                    if (innerStationOrder == nullptr)
                    {
                        continue;
                    }
                    if (stationId == innerStationOrder->getStation())
                    {
                        lineNumber++;
                    }
                }
            }
            else if (waypointOrder != nullptr)
            {
                const auto rawOrder = order.getRaw();
                for (auto innerOrder = orders.begin(); innerOrder->getOffset() != order.getOffset(); ++innerOrder)
                {
                    if (rawOrder == innerOrder->getRaw())
                    {
                        lineNumber++;
                    }
                }
            }

            // For some reason save only a byte when this could in theory be larger
            newFrame.lineNumber = static_cast<uint8_t>(lineNumber);
            _displayFrames.push_back(newFrame);
            i++;
        }

        i = 1;
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        auto tr = Gfx::TextRenderer(drawingCtx);
        for (auto& unk : _displayFrames)
        {
            auto order = Vehicles::OrderRingView(unk.orderOffset, 0).begin();
            if (!order->hasFlags(Vehicles::OrderFlags::HasNumber))
            {
                continue;
            }

            auto [loc, str] = generateOrderUiStringAndLoc(order->getOffset(), i);
            const auto pos = World::gameToScreen(loc, Ui::WindowManager::getCurrentRotation());
            auto stringWidth = tr.getStringWidth(str.c_str());
            for (auto zoom = 0; zoom < 4; ++zoom)
            {
                // The first line of the label will always be at the centre
                // of the station/waypoint. This works out where the subsequent
                // lines of the label will end up.
                auto width = (stringWidth + 3) << zoom;
                auto numberHeight = (unk.lineNumber * 10 /* lineHeight TODO make same as Windows::Vehicle.cpp lineHeight */) << zoom;
                auto firstLineHeight = 11 << zoom;
                auto midX = width / 2;
                auto midFirstLineY = firstLineHeight / 2;

                unk.frame.left[zoom] = (pos.x - midX) >> zoom;
                unk.frame.right[zoom] = (pos.x + midX) >> zoom;
                unk.frame.top[zoom] = (pos.y - midFirstLineY + numberHeight) >> zoom;
                unk.frame.bottom[zoom] = (pos.y + midFirstLineY + numberHeight) >> zoom;
            }
            i++;
        }
    }

    uint16_t reverseVehicleOrderTable(uint16_t tableOffset, uint16_t orderOfInterest)
    {
        // Currently the use of std:: algorithms is not feasible due to variable order lengths
        // TODO: simplify this after the changing the data structure for the order table

        // Retrieve list of raw orders
        std::vector<uint64_t> rawOrders{};
        Vehicles::OrderRingView orderTable(tableOffset);
        for (auto& order : orderTable)
        {
            rawOrders.push_back(order.getRaw());
        }

        // Nothing to do? Bail early
        if (rawOrders.size() == 0)
        {
            return orderOfInterest;
        }

        // Keep track of the type of the order of interest
        auto ooiType = orders()[tableOffset + orderOfInterest].getType();

        // Figure out where the order table starts in memory
        auto firstOrder = reinterpret_cast<uint8_t*>(orderTable.atIndex(0));
        auto dest = firstOrder;

        // Write reversed list over existing list
        for (auto it = rawOrders.rbegin(); it != rawOrders.rend(); ++it)
        {
            auto rawOrder = *it;
            auto orderType = rawOrder & 0x7;
            if (orderType >= std::size(kOrderSizes))
            {
                throw Exception::OutOfRange("Order type is greater than the order size table");
            }
            auto orderLength = kOrderSizes[orderType];
            std::memcpy(dest, &rawOrder, orderLength);
            dest += orderLength;
        }

        // Figure out the new position of the order of interest
        auto newOOIOffset = dest - orderOfInterest - kOrderSizes[enumValue(ooiType)];

        return newOOIOffset - firstOrder;
    }

    uint8_t swapAdjacentOrders(Order& a, Order& b)
    {
        // Currently the use of std:: algorithms is not feasible due to variable order lengths
        // TODO: simplify this after the changing the data structure for the order table

        const auto rawOrderA = a.getRaw();
        const auto rawOrderB = b.getRaw();
        const auto lengthOrderA = kOrderSizes[enumValue(a.getType())];
        const auto lengthOrderB = kOrderSizes[enumValue(b.getType())];

        // Ensure the two orders are indeed adjacent
        assert(&a + lengthOrderA == &b);

        // Copy B over A, and append B right after
        const auto dest = reinterpret_cast<uint8_t*>(&a);
        std::memcpy(dest, &rawOrderB, lengthOrderB);
        std::memcpy(dest + lengthOrderB, &rawOrderA, lengthOrderA);

        // Return the length with which to offset
        return lengthOrderB;
    }

    // 0x0047062B
    void removeOrdersForStation(const StationId stationId)
    {
        registers regs;
        regs.ebx = enumValue(stationId);
        call(0x0047062B, regs);
    }
}
