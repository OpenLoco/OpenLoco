#pragma once
#include "Routing.h"
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
        private:
            uint16_t* _beginRing;
            uint16_t* _current;
            bool _hasLooped = false;

        public:
            Iterator(uint16_t* begin, uint16_t* current)
                : _beginRing(begin)
                , _current(current)
            {
                // Prevent empty tables looping
                if (*current == -1)
                {
                    _hasLooped = true;
                }
            }

            Iterator& operator++();

            Iterator operator++(int)
            {
                Iterator res = *this;
                ++(*this);
                return res;
            }

            bool operator==(Iterator other) const
            {
                return _current == other._current && (_hasLooped || other._hasLooped);
            }

            bool operator!=(Iterator other) const
            {
                return !(*this == other);
            }

            uint16_t& operator*()
            {
                return *_current;
            }

            const uint16_t& operator*() const
            {
                return *_current;
            }

            uint16_t* operator->()
            {
                return _current;
            }

            const uint16_t* operator->() const
            {
                return _current;
            }

            // iterator traits
            using difference_type = std::ptrdiff_t;
            using value_type = uint16_t;
            using pointer = const uint16_t*;
            using reference = const uint16_t&;
            using iterator_category = std::forward_iterator_tag;
        };

        RoutingHandle _begin;

    public:
        // currentOrderOffset is relative to beginTableOffset and is where the ring will begin and end
        RingView(const RoutingHandle begin)
            : _begin(begin)
        {
        }

        RingView::Iterator begin() const;
        RingView::Iterator end() const;

        uint16_t* atIndex(const uint8_t index) const;
    };
}
