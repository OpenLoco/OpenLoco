#pragma once
#include "Types.hpp"
#include <OpenLoco/Core/Span.hpp>

namespace OpenLoco
{
    struct LoadedObjectHandle;
}
namespace OpenLoco::ObjectManager
{
    struct StringTableResult
    {
        string_id str;
        uint32_t tableLength;
    };
    StringTableResult loadStringTable(stdx::span<const std::byte> data, const LoadedObjectHandle& handle, uint8_t index);
}
