#pragma once
#include <OpenLoco/Core/EnumFlags.hpp>

namespace OpenLoco
{
    enum class GameStateFlags : uint32_t
    {
        none = 0U,
        tileManagerLoaded = (1U << 0),
        unk2 = (1U << 1),
        preferredOwnerName = (1U << 2),
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(GameStateFlags);
}