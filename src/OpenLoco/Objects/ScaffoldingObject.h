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
    struct ScaffoldingObject
    {
        static constexpr auto kObjectType = ObjectType::scaffolding;

        string_id name;
        uint32_t image; // 0x02
        uint8_t pad_06[0x12 - 0x06];

        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
        // 0x0042DF0B
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(ScaffoldingObject) == 0x12);
}
