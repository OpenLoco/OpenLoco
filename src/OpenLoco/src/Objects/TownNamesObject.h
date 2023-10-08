#pragma once

#include "Object.h"
#include "Types.hpp"
#include <span>

namespace OpenLoco
{
    namespace ObjectManager
    {
        struct DependentObjects;
    }
#pragma pack(push, 1)
    struct TownNamesObject
    {
        struct Unk
        {
            uint8_t count;
            uint8_t fill;
            uint16_t offset;
        };
        static constexpr auto kObjectType = ObjectType::townNames;
        static constexpr auto kMinNumNameCombinations = 80;

        StringId name; // 0x00
        Unk unks[6];   // 0x02

        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)
    static_assert(sizeof(TownNamesObject) == 0x1A);
}
