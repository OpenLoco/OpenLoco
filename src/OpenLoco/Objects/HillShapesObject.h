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
    struct HillShapesObject
    {
        static constexpr auto kObjectType = ObjectType::hillShapes;

        string_id name;
        uint8_t hillHeightMapCount;     // 0x02
        uint8_t mountainHeightMapCount; // 0x03
        uint32_t image;                 // 0x04
        uint32_t var_08;                // 0x08
        uint8_t pad_0C[0x0E - 0x0C];

        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
        // 0x00463BB3
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(HillShapesObject) == 0xE);
}
