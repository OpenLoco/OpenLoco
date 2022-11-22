#pragma once

#include "Core/Span.hpp"
#include "Object.h"
#include "Types.hpp"

namespace OpenLoco
{
    namespace Gfx
    {
        struct RenderTarget;
    }

#pragma pack(push, 1)
    struct RockObject
    {
        static constexpr auto kObjectType = ObjectType::rock;

        string_id name;
        uint32_t image; // 0x02

        // 0x004699FC
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
    static_assert(sizeof(RockObject) == 0x6);
}
