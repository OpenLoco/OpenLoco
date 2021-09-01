#pragma once

#include <iterator>

namespace OpenLoco
{
    template<typename ValueType>
    class LocoFixedVector
    {
    private:
        ValueType* startAddress = nullptr;
        ValueType* endAddress = nullptr;

        class Iter
        {
        private:
            ValueType* arr;
            ValueType* endAdd;
            constexpr void findNonEmpty()
            {
                for (; arr != endAdd; ++arr)
                {
                    if (!arr->empty())
                    {
                        break;
                    }
                }
            }
        public:
            constexpr Iter(ValueType* _arr, ValueType* _endAdd)
                : arr(_arr)
                , endAdd(_endAdd)
            {
                // finds first valid entry
                findNonEmpty();
            }

            constexpr Iter& operator++()
            {
                ++arr;
                findNonEmpty();
                return *this;
            }

            constexpr Iter operator++(int)
            {
                Iter retval = *this;
                ++(*this);
                return retval;
            }

            constexpr bool operator==(Iter other) const
            {
                return arr == other.arr;
            }
            constexpr bool operator!=(Iter other) const
            {
                return !(*this == other);
            }

            constexpr ValueType& operator*() const
            {
                return *arr;
            }
            // iterator traits
            using difference_type = std::ptrdiff_t;
            using value_type = ValueType;
            using pointer = ValueType*;
            using reference = ValueType&;
            using iterator_category = std::forward_iterator_tag;
        };

    public:
        template<typename T>
        LocoFixedVector(T& _arr)
            : startAddress(reinterpret_cast<ValueType*>(T::address))
            , endAddress(reinterpret_cast<ValueType*>(T::endAddress))
        {
        }

        Iter begin() const
        {
            return Iter(startAddress, endAddress);
        }
        Iter end() const
        {
            return Iter(endAddress, endAddress);
        }
    };
}
