#pragma once
#include "../Types.hpp"
#include "Routing.h"
#include <iterator>
#include <optional>

namespace OpenLoco
{
    struct Company;
}

namespace OpenLoco::Vehicles
{
    struct VehicleHead;
    struct Car;
    struct TrackAndDirection;
}

namespace OpenLoco::VehicleManager
{
    void determineAvailableVehicles(Company& company);
    void deleteTrain(Vehicles::VehicleHead& head);
    void deleteCar(Vehicles::Car& car);
    void vehiclePickupWater(EntityId head, uint8_t flags);
    void vehiclePickupAir(EntityId head, uint8_t flags);
    void placeDownVehicle(Vehicles::VehicleHead* const head, const coord_t x, const coord_t y, const uint8_t baseZ, const Vehicles::TrackAndDirection& unk1, const uint16_t unk2);
}

namespace OpenLoco::Vehicles::OrderManager
{
    void freeOrders(VehicleHead* const head);
}

namespace OpenLoco::Vehicles::RoutingManager
{
    std::optional<RoutingHandle> getAndAllocateFreeRoutingHandle();
    void freeRoutingHandle(const RoutingHandle routing);
    uint16_t getRouting(const RoutingHandle routing);
    void freeRouting(const RoutingHandle routing);
    bool isEmptyRoutingSlotAvailable();

    struct RingView
    {
    private:
        struct Iterator
        {
            enum class Direction : bool
            {
                forward,
                reverse,
            };

        private:
            RoutingHandle _current;
            bool _hasLooped = false;
            bool _isEnd = false;
            Direction _direction = Direction::forward;

        public:
            Iterator(const RoutingHandle& begin, bool isEnd, Direction direction);

            Iterator& operator++();
            Iterator operator++(int)
            {
                Iterator res = *this;
                ++(*this);
                return res;
            }

            Iterator& operator--();
            Iterator operator--(int)
            {
                Iterator res = *this;
                --(*this);
                return res;
            }

            bool operator==(const Iterator& other) const;

            bool operator!=(const Iterator& other) const
            {
                return !(*this == other);
            }

            RoutingHandle& operator*()
            {
                return _current;
            }

            const RoutingHandle& operator*() const
            {
                return _current;
            }

            RoutingHandle& operator->()
            {
                return _current;
            }

            const RoutingHandle& operator->() const
            {
                return _current;
            }

            // iterator traits
            using difference_type = std::ptrdiff_t;
            using value_type = RoutingHandle;
            using pointer = const RoutingHandle*;
            using reference = const RoutingHandle&;
            using iterator_category = std::bidirectional_iterator_tag;
        };

        RoutingHandle _begin;

    public:
        // currentOrderOffset is relative to beginTableOffset and is where the ring will begin and end
        RingView(const RoutingHandle begin)
            : _begin(begin)
        {
        }

        RingView::Iterator begin() const { return Iterator(_begin, false, Iterator::Direction::forward); }
        RingView::Iterator end() const { return Iterator(_begin, true, Iterator::Direction::forward); }
        auto rbegin() const { return Iterator(_begin, false, Iterator::Direction::reverse); }
        auto rend() const { return Iterator(_begin, true, Iterator::Direction::reverse); }
    };
}
