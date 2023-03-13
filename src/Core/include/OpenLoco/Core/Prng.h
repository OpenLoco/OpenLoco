#pragma once

#include <cassert>
#include <cstdint>

namespace OpenLoco::Core
{
#pragma pack(push, 1)
    struct Prng
    {
    private:
        uint32_t _srand_0{};
        uint32_t _srand_1{};

    public:
        uint32_t srand_0() const { return _srand_0; }
        uint32_t srand_1() const { return _srand_1; }

        Prng()
        {
        }

        Prng(uint32_t s0, uint32_t s1)
            : _srand_0(s0)
            , _srand_1(s1)
        {
        }

        uint32_t randNext();

        int32_t randNext(int32_t high)
        {
            high &= 0x7FFFFFFF; // no negatives allowed
            return randNext(0, high);
        }

        // NOTE: Callers to ensure low is less than high
        int32_t randNext(int32_t low, int32_t high)
        {
            assert(low <= high);
            int32_t positive = randNext() & 0x7FFFFFFF;
            return low + (positive % ((high + 1) - low));
        }

        bool randBool()
        {
            return (randNext() & 1) != 0;
        }
    };
#pragma pack(pop)
}
