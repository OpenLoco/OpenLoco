#pragma once

#include <OpenLoco/Core/Reflection.hpp>
#include <OpenLoco/S5/S5Entity.h>

namespace OpenLoco
{
    template<>
    struct Reflection<S5::EntityBase>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::EntityBase, baseType),
            REFL_FIELD(S5::EntityBase, type),
            REFL_FIELD(S5::EntityBase, nextQuadrantId),
            REFL_FIELD(S5::EntityBase, nextEntityId),
            REFL_FIELD(S5::EntityBase, llPreviousId),
            REFL_FIELD(S5::EntityBase, linkedListOffset),
            REFL_FIELD(S5::EntityBase, spriteHeightNegative),
            REFL_FIELD(S5::EntityBase, id),
            REFL_FIELD(S5::EntityBase, vehicleFlags),
            REFL_FIELD(S5::EntityBase, position),
            REFL_FIELD(S5::EntityBase, spriteWidth),
            REFL_FIELD(S5::EntityBase, spriteHeightPositive),
            REFL_FIELD(S5::EntityBase, spriteLeft),
            REFL_FIELD(S5::EntityBase, spriteTop),
            REFL_FIELD(S5::EntityBase, spriteRight),
            REFL_FIELD(S5::EntityBase, spriteBottom),
            REFL_FIELD(S5::EntityBase, spriteYaw),
            REFL_FIELD(S5::EntityBase, spritePitch),
            REFL_FIELD(S5::EntityBase, pad_20),
            REFL_FIELD(S5::EntityBase, owner),
            REFL_FIELD(S5::EntityBase, name)>;
    };

    template<>
    struct Reflection<S5::Entity>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::Entity, base),
            REFL_FIELD(S5::Entity, pad_24)>;
    };
}
