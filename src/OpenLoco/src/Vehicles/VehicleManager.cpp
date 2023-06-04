#include "VehicleManager.h"
#include "Entities/EntityManager.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "MessageManager.h"
#include "OrderManager.h"
#include "Orders.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "Vehicle.h"
#include "World/Company.h"
#include "World/CompanyManager.h"

#include <OpenLoco/Interop/Interop.hpp>
#include <sstream>

using namespace OpenLoco::Interop;

namespace OpenLoco::VehicleManager
{
    // 0x004A8826
    void update()
    {
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && !isEditorMode())
        {
            for (auto* v : VehicleList())
            {
                v->updateVehicle();
            }
        }
    }

    // 0x004C3C54
    void updateMonthly()
    {
        for (auto v : VehicleList())
        {
            v->updateMonthly();
        }
    }

    // 0x004B94CF
    void updateDaily()
    {
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && !isEditorMode())
        {
            for (auto* v : VehicleList())
            {
                v->updateDaily();
            }
            Ui::WindowManager::invalidate(Ui::WindowType::vehicleList);
        }
    }

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
