#pragma once

#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Location.hpp>

namespace OpenLoco::World
{
    struct Wave
    {
        World::Pos2 loc; // 0x00
        uint16_t frame;  // 0x04
        bool empty() const
        {
            return loc.x == Location::null;
        }
    };
}
