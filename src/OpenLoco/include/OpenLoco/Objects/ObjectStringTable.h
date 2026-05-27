#pragma once
#include "Types.hpp"
#include <span>

namespace OpenLoco
{
    struct LoadedObjectHandle;
}
namespace OpenLoco::ObjectManager
{
    struct StringTableResult
    {
        StringId str;
        uint32_t tableLength;
    };
    StringTableResult loadStringTable(std::span<const std::byte> data, const LoadedObjectHandle& handle, uint8_t index);
}
