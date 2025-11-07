#include <OpenLoco/Engine/World.hpp>
#include <cstdint>

namespace OpenLoco::World
{
    struct Wave;
}

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct Wave
    {
        World::Pos2 loc; // 0x00
        uint16_t frame;  // 0x04
    };
    static_assert(sizeof(Wave) == 0x6);
#pragma pack(pop)

    S5::Wave exportWave(const OpenLoco::World::Wave& src);
}
