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
    struct LevelCrossingObject
    {
        static constexpr auto kObjectType = ObjectType::levelCrossing;

        StringId name;
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
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
    };
#pragma pack(pop)
    static_assert(sizeof(LevelCrossingObject) == 0x12);
}
