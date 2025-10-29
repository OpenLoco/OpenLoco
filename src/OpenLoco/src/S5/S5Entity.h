#include <cstdint>

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct Entity
    {
        uint8_t pad_00[0x80];
    };
    static_assert(sizeof(Entity) == 0x80);
#pragma pack(pop)
}
