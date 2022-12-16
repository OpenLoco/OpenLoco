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
    struct ScenarioTextObject
    {
        static constexpr auto kObjectType = ObjectType::scenarioText;

        string_id name;
        string_id details;
        uint8_t pad_04[0x6 - 0x4];

        // 0x0043EE17
        bool validate() const
        {
            return true;
        }
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(ScenarioTextObject) == 0x6);
}
