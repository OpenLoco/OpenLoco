#pragma once

#include "Map/Map.hpp"
#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/Span.hpp>

namespace OpenLoco
{
    namespace ObjectManager
    {
        struct DependentObjects;
    }
    namespace Gfx
    {
        struct RenderTarget;
    }

#pragma pack(push, 1)
    struct DockObject
    {
        static constexpr auto kObjectType = ObjectType::dock;

        string_id name;
        int16_t buildCostFactor; // 0x02
        int16_t sellCostFactor;  // 0x04
        uint8_t costIndex;       // 0x06
        uint8_t var_07;
        uint32_t image; // 0x08
        uint32_t var_0C;
        uint8_t pad_10[0x12 - 0x10];
        uint8_t numAux01;    // 0x12
        uint8_t numAux02Ent; // 0x13
        uint8_t pad_14[0x1C - 0x14];
        uint32_t var_1C[1];     // odd that this is size 1 but that is how its used
        uint16_t designedYear;  // 0x20
        uint16_t obsoleteYear;  // 0x22
        Map::Pos2 boatPosition; // 0x24

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(DockObject) == 0x28);
}
