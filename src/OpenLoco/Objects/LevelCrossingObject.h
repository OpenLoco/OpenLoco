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
    struct LevelCrossingObject
    {
        static constexpr auto kObjectType = ObjectType::levelCrossing;

        string_id name;
        int16_t costFactor;     // 0x02
        int16_t sellCostFactor; // 0x04
        uint8_t costIndex;      // 0x06
        uint8_t animationSpeed; // 0x07
        uint8_t closingFrames;  // 0x08
        uint8_t closedFrames;   // 0x09
        uint8_t pad_0A[0x0C - 0x0A];
        uint16_t designedYear; // 0x0C
        uint32_t image;        // 0x0E

        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::Context& context, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
    };
#pragma pack(pop)
    static_assert(sizeof(LevelCrossingObject) == 0x12);
}
