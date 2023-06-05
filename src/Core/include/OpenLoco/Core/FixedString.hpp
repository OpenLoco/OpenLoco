#pragma once

#include <array>

namespace OpenLoco
{
    template<typename TCharType, size_t TCapacity>
    class FixedStringBase
    {
        using StorageType = std::array<TCharType, TCapacity>;

        StorageType _data;

    public:
        using iterator = typename StorageType::iterator;
        using const_iterator = typename StorageType::const_iterator;

        FixedStringBase() = default;
        FixedStringBase(const FixedStringBase&) = default;

        FixedStringBase(const TCharType* str)
        {
            assign(str);
        }

        FixedStringBase& operator=(const FixedStringBase&) = default;

        void assign(const TCharType* str)
        {
            auto it = _data.begin();
            while (*str && it != _data.end())
            {
                *it++ = *str++;
            }
            // Null terminate the string if we have space.
            if (it != _data.end())
            {
                *it = 0;
            }
        }
    };
}
