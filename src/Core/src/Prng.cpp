#include "Prng.h"
#include "Numerics.hpp"

namespace OpenLoco::Core
{
    uint32_t Prng::randNext()
    {
        auto srand0 = _srand_0;
        auto srand1 = _srand_1;
        _srand_0 += Numerics::ror<uint32_t>(srand1 ^ 0x1234567F, 7);
        _srand_1 = Numerics::ror<uint32_t>(srand0, 3);
        return _srand_1;
    }
}
