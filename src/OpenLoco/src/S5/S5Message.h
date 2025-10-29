#include <cstdint>

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct Message
    {
        uint8_t pad_0[0xD4];
    };
    static_assert(sizeof(Message) == 0xD4);
#pragma pack(pop)
}
