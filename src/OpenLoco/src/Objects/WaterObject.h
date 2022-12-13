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
    namespace Gfx
    {
        struct RenderTarget;
    }

#pragma pack(push, 1)
    struct WaterObject
    {
        static constexpr auto kObjectType = ObjectType::water;

        string_id name;
        uint8_t costIndex; // 0x02
        uint8_t var_03;
        int8_t costFactor; // 0x04
        uint8_t var_05;
        uint32_t image; // 0x06
        uint32_t var_0A;

        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
    static_assert(sizeof(WaterObject) == 0xE);

    namespace Water::ImageIds
    {
        static constexpr uint32_t kToolbarTerraformWater = 42;
    }
}
