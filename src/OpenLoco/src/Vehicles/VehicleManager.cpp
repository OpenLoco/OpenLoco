#include "VehicleManager.h"
#include "Company.h"
#include "CompanyManager.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Interop/Interop.hpp"
#include "MessageManager.h"
#include "Objects/CargoObject.h"
#include "Orders.h"
#include "StationManager.h"
#include "Ui/WindowManager.h"
#include "Vehicle.h"
#include <sstream>

using namespace OpenLoco::Interop;

namespace OpenLoco::VehicleManager
{
    // 0x004C3A0C
    void determineAvailableVehicles(Company& company)
    {
        registers regs;
        regs.esi = X86Pointer(&company);
        call(0x004C3A0C, regs);
    }

    // 0x004279CC
    void vehiclePickupWater(EntityId head, uint8_t flags)
    {
        registers regs;
        regs.di = enumValue(head);
        regs.bl = flags;
        call(0x004279CC, regs);
    }

    // 0x00426B29
    void vehiclePickupAir(EntityId head, uint8_t flags)
    {
        registers regs;
        regs.di = enumValue(head);
        regs.bl = flags;
        call(0x00426B29, regs);
    }

    // 0x004B05E4
    void placeDownVehicle(Vehicles::VehicleHead* const head, const coord_t x, const coord_t y, const uint8_t baseZ, const Vehicles::TrackAndDirection& unk1, const uint16_t unk2)
    {
        registers regs{};
        regs.esi = X86Pointer(head);
        regs.ax = x;
        regs.cx = y;
        regs.bx = unk2;
        regs.dl = baseZ;
        regs.ebp = unk1.track._data;
        call(0x004B05E4, regs);
    }

    // 0x004AEFB5
    void deleteCar(Vehicles::Car& car)
    {
        registers regs;
        regs.esi = X86Pointer(car.front);
        call(0x004AEFB5, regs);
    }

    // 0x004AF06E
    void deleteTrain(Vehicles::VehicleHead& head)
    {
        Ui::WindowManager::close(Ui::WindowType::vehicle, enumValue(head.id));
        auto* vehListWnd = Ui::WindowManager::find(Ui::WindowType::vehicleList, enumValue(head.owner));
        if (vehListWnd != nullptr)
        {
            vehListWnd->invalidate();
            Ui::Windows::VehicleList::removeTrainFromList(*vehListWnd, head.id);
        }
        // Change to vanilla, update the build window to a valid train
        auto* vehBuildWnd = Ui::WindowManager::find(Ui::WindowType::buildVehicle, enumValue(head.owner));
        if (vehBuildWnd != nullptr)
        {
            vehBuildWnd->invalidate();
            Ui::Windows::BuildVehicle::sub_4B92A5(vehBuildWnd);
        }

        // 0x004AF0A3
        switch (head.mode)
        {
            case TransportMode::road:
            case TransportMode::rail:
                head.liftUpVehicle();
                break;
            case TransportMode::air:
                vehiclePickupAir(head.id, GameCommands::Flags::apply);
                break;
            case TransportMode::water:
                vehiclePickupWater(head.id, GameCommands::Flags::apply);
                break;
        }

        Vehicles::Vehicle train(head);
        auto* nextVeh = train.veh2->nextVehicleComponent();
        while (nextVeh != nullptr && !nextVeh->isVehicleTail())
        {
            Vehicles::Car car(nextVeh);
            deleteCar(car);
            nextVeh = train.veh2->nextVehicleComponent();
        }

        Audio::stopVehicleNoise(head.id);
        Vehicles::RoutingManager::freeRoutingHandle(head.routingHandle);
        Vehicles::OrderManager::freeOrders(&head);
        MessageManager::removeAllSubjectRefs(enumValue(head.id), MessageItemArgumentType::vehicle);
        const auto companyId = head.owner;
        EntityManager::freeEntity(train.tail);
        EntityManager::freeEntity(train.veh2);
        EntityManager::freeEntity(train.veh1);
        EntityManager::freeEntity(train.head);
        CompanyManager::get(companyId)->recalculateTransportCounts();
    }
}

namespace OpenLoco::Vehicles::RoutingManager
{
    constexpr uint16_t kAllocatedButFreeRoutingStation = -2; // Indicates that this array is allocated to a vehicle but no station has been set.
    constexpr uint16_t kRoutingNull = -1;                    // Indicates that this array is allocated to a vehicle but no station has been set.

    static auto& routings() { return getGameState().routings; }

    static std::optional<uint16_t> findFreeRoutingVehicleRef()
    {
        const auto& routingArr = routings();
        const auto res = std::find_if(std::begin(routingArr), std::end(routingArr), [](const auto& route) { return route[0] == kRoutingNull; });
        if (res == std::end(routingArr))
        {
            return std::nullopt;
        }
        return std::distance(std::begin(routingArr), res);
    }

    bool isEmptyRoutingSlotAvailable()
    {
        return findFreeRoutingVehicleRef().has_value();
    }

    // 0x004B1E00
    std::optional<RoutingHandle> getAndAllocateFreeRoutingHandle()
    {
        auto vehicleRef = findFreeRoutingVehicleRef();
        if (vehicleRef.has_value())
        {
            auto& vehRoutingArr = routings()[*vehicleRef];
            std::fill(std::begin(vehRoutingArr), std::end(vehRoutingArr), kAllocatedButFreeRoutingStation);
            return { RoutingHandle(*vehicleRef, 0) };
        }
        return std::nullopt;
    }

    uint16_t getRouting(const RoutingHandle routing)
    {
        return routings()[routing.getVehicleRef()][routing.getIndex()];
    }

    void freeRouting(const RoutingHandle routing)
    {
        routings()[routing.getVehicleRef()][routing.getIndex()] = kAllocatedButFreeRoutingStation;
    }

    // 0x004B1E77
    void freeRoutingHandle(const RoutingHandle routing)
    {
        auto& vehRoutingArr = routings()[routing.getVehicleRef()];
        std::fill(std::begin(vehRoutingArr), std::end(vehRoutingArr), kRoutingNull);
    }

    RingView::Iterator::Iterator(const RoutingHandle& begin, bool isEnd, Direction direction)
        : _current(begin)
        , _isEnd(isEnd)
        , _direction(direction)
    {
        if (routings()[_current.getVehicleRef()][_current.getIndex()] == kAllocatedButFreeRoutingStation)
        {
            _hasLooped = true;
        }
    }

    RingView::Iterator& RingView::Iterator::operator++()
    {
        if (_direction == Direction::reverse)
        {
            return --*this;
        }
        _current.setIndex((_current.getIndex() + 1) & 0x3F);

        if (_current.getIndex() == 0)
        {
            _hasLooped = true;
        }
        return *this;
    }

    RingView::Iterator& RingView::Iterator::operator--()
    {
        _current.setIndex((_current.getIndex() - 1) & 0x3F);

        if (_current.getIndex() == 0x3F)
        {
            _hasLooped = true;
        }
        return *this;
    }

    bool RingView::Iterator::operator==(const RingView::Iterator& other) const
    {
        if ((_hasLooped || other._hasLooped) && _current == other._current)
        {
            return true;
        }
        // If this is an end iterator then its value is implied to be kAllocatedButFreeRoutingStation
        if (_isEnd)
        {
            return routings()[other._current.getVehicleRef()][other._current.getIndex()] == kAllocatedButFreeRoutingStation;
        }
        if (other._isEnd)
        {
            return routings()[_current.getVehicleRef()][_current.getIndex()] == kAllocatedButFreeRoutingStation;
        }
        return false;
    }
}

namespace OpenLoco::Vehicles::OrderManager
{
    // TODO: Make this a fixed vector of size 63 (kMaxNumOrderPerVehicle) no need for it to be dynamic
    std::vector<NumDisplayFrame> _displayFrames;
    const std::vector<NumDisplayFrame>& displayFrames() { return _displayFrames; }

    static auto& orders() { return getGameState().orders; }
    static auto& numOrders() { return getGameState().numOrders; }

    static void sub_470795(const uint32_t removeOrderTableOffset, const int16_t sizeOfRemovedOrderTable)
    {
        for (auto head : EntityManager::VehicleList())
        {
            if (head->orderTableOffset >= removeOrderTableOffset)
            {
                head->orderTableOffset += sizeOfRemovedOrderTable;
            }
        }
    }

    // 0x00470334
    // Remove vehicle ?orders?
    void freeOrders(VehicleHead* const head)
    {
        // Copy the offset as it will get modified during sub_470795
        const auto offset = head->orderTableOffset;
        const auto size = head->sizeOfOrderTable;

        sub_470795(offset, -size);

        // Fold orders table left to remove empty orders
        std::rotate(&orders()[offset], &orders()[offset + size], &orders()[numOrders()]);

        numOrders() -= head->sizeOfOrderTable;
    }

    // 0x00470B76
    std::pair<Map::Pos3, std::string> generateOrderUiStringAndLoc(uint32_t orderOffset, uint8_t orderNum)
    {
        std::stringstream ss;
        ss << ControlCodes::inlineSpriteStr;

        auto imageId = Gfx::recolour(ImageIds::getNumberCircle(orderNum), Colour::white);
        ss.write(reinterpret_cast<const char*>(&imageId), 4);
        OrderRingView orderRing(orderOffset);
        auto order = orderRing.begin();
        Map::Pos3 pos{};

        switch (order->getType())
        {
            case Vehicles::OrderType::StopAt:
            {
                auto* stopAt = order->as<Vehicles::OrderStopAt>();
                // 0x00470B7D
                auto* station = StationManager::get(stopAt->getStation());
                pos = Map::Pos3{ station->x, station->y, station->z } + Map::Pos3{ 0, 0, 30 };
                ss << ControlCodes::Colour::white;
                for (auto nextOrder = order + 1; nextOrder != order; ++nextOrder)
                {
                    if (!nextOrder->hasFlag(Vehicles::OrderFlags::HasCargo))
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
                pos = Map::Pos3{ station->x, station->y, station->z } + Map::Pos3{ 0, 0, 30 };
                break;
            }
            case Vehicles::OrderType::RouteWaypoint:
                // 0x00470C6F
                pos = order->as<Vehicles::OrderRouteWaypoint>()->getWaypoint() + Map::Pos3{ 16, 16, 8 };
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
        Input::setMapSelectionFlags(Input::MapSelectionFlags::unk_04);
        Gfx::invalidateScreen();
        _displayFrames.clear();
        auto orders = Vehicles::OrderRingView(head->orderTableOffset);
        uint8_t i = 0;
        for (auto& order : orders)
        {
            if (!order.hasFlag(Vehicles::OrderFlags::HasNumber))
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
        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        for (auto& unk : _displayFrames)
        {
            auto order = Vehicles::OrderRingView(unk.orderOffset, 0).begin();
            if (!order->hasFlag(Vehicles::OrderFlags::HasNumber))
            {
                continue;
            }

            auto [loc, str] = generateOrderUiStringAndLoc(order->getOffset(), i);
            const auto pos = Map::gameToScreen(loc, Ui::WindowManager::getCurrentRotation());
            auto stringWidth = drawingCtx.getStringWidth(str.c_str());
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
}
