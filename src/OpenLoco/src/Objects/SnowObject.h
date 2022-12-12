#pragma once

#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/Span.hpp>

namespace OpenLoco
{
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
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(SnowObject) == 0x6);
}
