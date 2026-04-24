#pragma once

#include <cstdint>

namespace OpenLoco::Audio
{
    enum class AudioHandle : uint32_t
    {
        null = ~0u
    };

    enum class BufferId : uint32_t
    {
        null = ~0u
    };
}
