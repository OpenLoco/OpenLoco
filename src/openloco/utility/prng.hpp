#pragma once

#include "numeric.hpp"
#include <cstdint>

namespace openloco::utility
{
#pragma pack(push, 1)
    struct prng
    {
    private:
        uint32_t _srand_0;
        uint32_t _srand_1;

    public:
        uint32_t srand_0() { return _srand_0; }
        uint32_t srand_1() { return _srand_1; }

        uint32_t rand_next()
        {
            auto srand0 = _srand_0;
            auto srand1 = _srand_1;
            _srand_0 = ror<uint32_t>(srand1 ^ 0x1234567F, 7);
            _srand_1 = ror<uint32_t>(srand0, 3);
            return _srand_1;
        }

        int32_t rand_next(int32_t high)
        {
            return rand_next(0, high);
        }

        int32_t rand_next(int32_t low, int32_t high)
        {
            int32_t positive = rand_next() & 0x7FFFFFFF;
            return low + (positive % ((high + 1) - low));
        }

        bool rand_bool()
        {
            return (rand_next() & 1) != 0;
        }
    };
#pragma pack(pop)
}
