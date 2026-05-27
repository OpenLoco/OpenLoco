#pragma once

#include <OpenLoco/Engine/World.hpp>

namespace OpenLoco::World
{
    struct Animation
    {
        uint8_t baseZ;
        uint8_t type;
        World::Pos2 pos;
    };
}
