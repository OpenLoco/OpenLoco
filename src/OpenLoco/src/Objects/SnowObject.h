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
    namespace Gfx
    {
        struct RenderTarget;
    }

#pragma pack(push, 1)
    struct SnowObject
    {
        static constexpr auto kObjectType = ObjectType::snow;

        string_id name;
        uint32_t image; // 0x02

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        // 0x00469A6B
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)
    static_assert(sizeof(SnowObject) == 0x6);

    namespace SnowLine::ImageIds
    {
        constexpr uint32_t surfaceEighthZoom = 0;
        constexpr uint32_t outlineEighthZoom = 10;
        constexpr uint32_t surfaceQuarterZoom = 19;
        constexpr uint32_t outlineQuarterZoom = 38;
        constexpr uint32_t surfaceHalfZoom = 57;
        constexpr uint32_t outlineHalfZoom = 76;
        constexpr uint32_t surfaceFullZoom = 95;
        constexpr uint32_t outlineFullZoom = 114;
    }
}
