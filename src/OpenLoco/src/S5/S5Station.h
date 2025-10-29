#include <cstdint>

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct Station
    {
        uint8_t pad_000[0x3D2];
    };
    static_assert(sizeof(Station) == 0x3D2);
#pragma pack(pop)
}
