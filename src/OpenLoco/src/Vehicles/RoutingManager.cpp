#include "RoutingManager.h"
#include "GameState.h"
#include <algorithm>

namespace OpenLoco::Vehicles::RoutingManager
{
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

    void resetRoutings(const RoutingHandle handle)
    {
        auto& vehRoutingArr = routings()[handle.getVehicleRef()];
        std::fill(std::begin(vehRoutingArr), std::end(vehRoutingArr), kAllocatedButFreeRouting);
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
            std::fill(std::begin(vehRoutingArr), std::end(vehRoutingArr), kAllocatedButFreeRouting);
            return { RoutingHandle(*vehicleRef, 0) };
        }
        return std::nullopt;
    }

    uint16_t getRouting(const RoutingHandle handle)
    {
        return routings()[handle.getVehicleRef()][handle.getIndex()];
    }

    void setRouting(const RoutingHandle handle, uint16_t routing)
    {
        routings()[handle.getVehicleRef()][handle.getIndex()] = routing;
    }

    void freeRouting(const RoutingHandle handle)
    {
        setRouting(handle, kAllocatedButFreeRouting);
    }

    // 0x004B1E77
    void freeRoutingHandle(const RoutingHandle handle)
    {
        auto& vehRoutingArr = routings()[handle.getVehicleRef()];
        std::fill(std::begin(vehRoutingArr), std::end(vehRoutingArr), kRoutingNull);
    }

    // 0x004A8810
    void resetRoutingTable()
    {
        std::fill_n(&routings()[0][0], Limits::kMaxVehicles * Limits::kMaxRoutingsPerVehicle, kRoutingNull);
    }

    RingView::Iterator::Iterator(const RoutingHandle& begin, bool isEnd, Direction direction)
        : _current(begin)
        , _isEnd(isEnd)
        , _direction(direction)
    {
        if (routings()[_current.getVehicleRef()][_current.getIndex()] == kAllocatedButFreeRouting)
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
        // If this is an end iterator then its value is implied to be kAllocatedButFreeRouting
        if (_isEnd)
        {
            return routings()[other._current.getVehicleRef()][other._current.getIndex()] == kAllocatedButFreeRouting;
        }
        if (other._isEnd)
        {
            return routings()[_current.getVehicleRef()][_current.getIndex()] == kAllocatedButFreeRouting;
        }
        return false;
    }
}
