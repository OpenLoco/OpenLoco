#pragma once
#include "Routing.h"
#include <iterator>
#include <optional>

namespace OpenLoco
{
    struct Company;
}

namespace OpenLoco::VehicleManager
{
    void determineAvailableVehicles(Company& company);
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
