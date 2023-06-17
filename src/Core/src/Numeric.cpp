#include "Numeric.hpp"

#include <cstddef>
#include <cstdint>
#include <limits.h>
#include <type_traits>
#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace OpenLoco::Numerics
{
    // Finds the first bit set in a 32-bits numeral and returns its index, or -1 if no bit is set.
    int32_t bitScanForward(uint32_t source)
    {
#if defined(_MSC_VER) && (_MSC_VER >= 1400) // Visual Studio 2005
        unsigned long i;
        uint8_t success = _BitScanForward(&i, source);
        return success != 0 ? i : -1;
#elif defined(__GNUC__)
        auto result = __builtin_ffs(source);
        return result - 1;
#else
#pragma message "Falling back to iterative bitscan forward, consider using intrinsics"
        if (source != 0)
        {
            for (int32_t i = 0; i < 32; i++)
            {
                if (source & (1u << i))
                {
                    return i;
                }
            }
        }
        return -1;
#endif
    }

    int32_t bitScanReverse(uint32_t source)
    {
#if defined(_MSC_VER) && (_MSC_VER >= 1400) // Visual Studio 2005
        unsigned long i;
        uint8_t success = _BitScanReverse(&i, source);
        return success != 0 ? i : -1;
#elif defined(__GNUC__)
        auto result = source == 0 ? -1 : __builtin_clz(source) ^ 31;
        return result;
#else
#pragma message "Falling back to iterative bitscan reverse, consider using intrinsics"
        if (source != 0)
        {
            for (int32_t i = 31; i > -1; i--)
            {
                if (source & (1u << i))
                {
                    return i;
                }
            }
        }
        return -1;
#endif
    }
}
