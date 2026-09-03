#pragma once

#include <OpenLoco/Core/Reflection.hpp>
#include <OpenLoco/Engine/World.hpp>

namespace OpenLoco
{
    template<>
    struct Reflection<World::Pos2>
    {
        using Fields = FieldList<
            REFL_FIELD(World::Pos2, x),
            REFL_FIELD(World::Pos2, y)>;
    };

    template<>
    struct Reflection<World::Pos3>
    {
        using Fields = FieldList<
            REFL_FIELD(World::Pos3, x),
            REFL_FIELD(World::Pos3, y),
            REFL_FIELD(World::Pos3, z)>;
    };
}
