#pragma once

#include "Numeric.hpp"
#include <cstdint>

namespace OpenLoco::Utility
{
#pragma pack(push, 1)
    struct prng
    {
    private:
        uint32_t _srand_0{};
        uint32_t _srand_1{};

    public:
        uint32_t srand_0() { return _srand_0; }
        uint32_t srand_1() { return _srand_1; }

        prng()
        {
        }

        prng(uint32_t s0, uint32_t s1)
            : _srand_0(s0)
            , _srand_1(s1)
        {
        }

        uint32_t randNext()
        {
            auto srand0 = _srand_0;
            auto srand1 = _srand_1;
            _srand_0 += ror<uint32_t>(srand1 ^ 0x1234567F, 7);
            _srand_1 = ror<uint32_t>(srand0, 3);
            return _srand_1;
        }

        int32_t randNext(int32_t high)
        {
            return randNext(0, high);
        }

        int32_t randNext(int32_t low, int32_t high)
        {
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
