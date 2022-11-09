#pragma once

#include <iterator>

namespace OpenLoco
{
    template<typename ValueType, size_t Count>
    class FixedVector
    {
    private:
        ValueType* startAddress = nullptr;

        class Iter
        {
        private:
            ValueType* arr;
            size_t i = 0;

            constexpr void findNonEmpty()
            {
                for (; i < Count; ++i)
                {
                    if (!arr[i].empty())
                    {
                        break;
                    }
                }
            }

        public:
            constexpr Iter(ValueType* _arr, size_t _index)
                : arr(_arr)
                , i(_index)
            {
                // finds first valid entry
                findNonEmpty();
            }

            constexpr Iter& operator++()
            {
                ++i;
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
                return i == other.i;
            }
            constexpr bool operator!=(Iter other) const
            {
                return !(*this == other);
            }

            constexpr ValueType& operator*() const
            {
                return arr[i];
            }
            // iterator traits
            using difference_type = std::ptrdiff_t;
            using value_type = ValueType;
            using pointer = ValueType*;
            using reference = ValueType&;
            using iterator_category = std::forward_iterator_tag;
        };

    public:
        FixedVector(ValueType (&_arr)[Count])
            : startAddress(_arr)
        {
        }

        Iter begin() const
        {
            return Iter(startAddress, 0);
        }
        Iter end() const
        {
            return Iter(startAddress, Count);
        }

        [[nodiscard]] bool empty() const
        {
            return begin() == end();
        }

        [[nodiscard]] size_t size() const
        {
            return std::distance(begin(), end());
        }
    };
}
