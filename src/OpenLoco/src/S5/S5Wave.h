#include <cstdint>

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct Wave
    {
        uint8_t pad_0[0x6];
    };
    static_assert(sizeof(Wave) == 0x6);
#pragma pack(pop)
}
