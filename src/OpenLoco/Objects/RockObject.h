#pragma once

#include "../Core/Span.hpp"
#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
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
        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
    static_assert(sizeof(RockObject) == 0x6);
}
