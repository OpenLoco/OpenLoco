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
    struct SnowObject
    {
        static constexpr auto kObjectType = ObjectType::snow;
        static constexpr auto kImageOffsetEighthZoom = 0;
        static constexpr auto kImageOffsetQuarterZoom = 19;
        static constexpr auto kImageOffsetHalfZoom = 57;
        static constexpr auto kImageOffsetFullZoom = 95;

        string_id name;
        uint32_t image; // 0x02

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        // 0x00469A6B
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(SnowObject) == 0x6);
}
