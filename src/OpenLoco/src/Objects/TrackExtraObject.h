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
    struct TrackExtraObject
    {
        static constexpr auto kObjectType = ObjectType::trackExtra;

        string_id name;
        uint16_t trackPieces;    // 0x02
        uint8_t paintStyle;      // 0x04
        uint8_t costIndex;       // 0x05
        int16_t buildCostFactor; // 0x06
        int16_t sellCostFactor;  // 0x08
        uint32_t image;          // 0x0A
        uint32_t var_0E;

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(TrackExtraObject) == 0x12);
}
