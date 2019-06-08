#include "numeric.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <cstddef>
#include <cstdint>
#include <limits.h>
#include <type_traits>

namespace openloco::utility
{
    // Finds the first bit set in a 32-bits numeral and returns its index, or -1 if no bit is set.
    int32_t bitscanforward(uint32_t source)
    {
#if defined(_MSC_VER) && (_MSC_VER >= 1400) // Visual Studio 2005
        DWORD i;
        uint8_t success = _BitScanForward(&i, source);
        return success != 0 ? i : -1;
#elif defined(__GNUC__)
        int32_t success = __builtin_ffs(source);
        return success - 1;
#else
#pragma message "Falling back to iterative bitscan forward, consider using intrinsics"
        for (int32_t i = 0; i < 32; i++)
            if (source & (1u << i))
                return i;

        return -1;
#endif
    }
}
