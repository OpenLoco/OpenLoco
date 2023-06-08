#pragma once

#include "LabelFrame.h"
#include "Types.hpp"
#include "Vehicles/Orders.h"
#include <OpenLoco/Engine/World.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace OpenLoco::Vehicles
{
    struct VehicleHead;

    struct OrderRingView
    {
    public:
        struct Iterator
        {
        private:
            Order* _beginOrderTable;
            Order* _currentOrder;
            bool _hasLooped = false;

        public:
            Iterator(Order* beginOrderTable, Order* currentOrder)
                : _beginOrderTable(beginOrderTable)
                , _currentOrder(currentOrder)
            {
                // Prevent empty tables looping
                if (_currentOrder->getType() == OrderType::End)
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

            Iterator operator+(int32_t amount) const
            {
                Iterator res = *this;
                while (amount-- != 0)
                {
                    res++;
                }
                return res;
            }

            bool operator==(Iterator other) const
            {
                return _currentOrder == other._currentOrder && (_hasLooped || other._hasLooped);
            }

            bool operator!=(Iterator other) const
            {
                return !(*this == other);
            }

            Order& operator*()
            {
                return *_currentOrder;
            }

            const Order& operator*() const
            {
                return *_currentOrder;
            }

            Order* operator->()
            {
                return _currentOrder;
            }

            const Order* operator->() const
            {
                return _currentOrder;
            }

            // iterator traits
            using difference_type = std::ptrdiff_t;
            using value_type = Order;
            using pointer = const Order*;
            using reference = const Order&;
            using iterator_category = std::forward_iterator_tag;
        };

    private:
        uint32_t _beginTableOffset;
        uint32_t _currentOrderOffset;

    public:
        // currentOrderOffset is relative to beginTableOffset and is where the ring will begin and end
        OrderRingView(uint32_t beginTableOffset, uint32_t currentOrderOffset = 0)
            : _beginTableOffset(beginTableOffset)
            , _currentOrderOffset(currentOrderOffset)
        {
        }

        OrderRingView::Iterator begin() const;
        OrderRingView::Iterator end() const;

        Order* atIndex(const uint8_t index) const;
    };
}

namespace OpenLoco::Vehicles::OrderManager
{
    struct NumDisplayFrame
    {
        uint32_t orderOffset; // 0x0
        LabelFrame frame;     // 0x4
        uint8_t lineNumber;   // 0x24
    };

    Order* orders();
    uint32_t& numOrders();

    void zeroOrderTable();
    void freeOrders(VehicleHead* const head);

    void sub_470795(const uint32_t removeOrderTableOffset, const int16_t sizeOfRemovedOrderTable);
    std::pair<World::Pos3, std::string> generateOrderUiStringAndLoc(uint32_t orderOffset, uint8_t orderNum);
    void generateNumDisplayFrames(Vehicles::VehicleHead* head);
    const std::vector<NumDisplayFrame>& displayFrames();
    uint16_t reverseVehicleOrderTable(uint16_t tableOffset, uint16_t orderOfInterest);
    uint8_t swapAdjacentOrders(Order& a, Order& b);
}
