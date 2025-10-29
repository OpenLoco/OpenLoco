#include <cstdint>

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct Animation
    {
        uint8_t pad_0[0x6];
    };
#pragma pack(pop)
    static_assert(sizeof(Animation) == 0x6);
}
