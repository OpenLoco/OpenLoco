#pragma once

#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/Span.hpp>

namespace OpenLoco
{
    namespace ObjectManager
    {
        struct DependentObjects;
    }
#pragma pack(push, 1)
    struct ClimateObject
    {
        static constexpr auto kObjectType = ObjectType::climate;

        string_id name;          // 0x00
        uint8_t firstSeason;     // 0x02
        uint8_t seasonLength[4]; // 0x03
        uint8_t winterSnowLine;  // 0x07
        uint8_t summerSnowLine;  // 0x08
        uint8_t pad_09;

        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(ClimateObject) == 0xA);
}
