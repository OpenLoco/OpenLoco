#pragma once

#include "Map/Track/TrackEnum.h"
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
    struct RoadExtraObject
    {
        static constexpr auto kObjectType = ObjectType::roadExtra;

        StringId name;                           // 0x00
        World::Track::RoadPieceFlags roadPieces; // 0x02
        uint8_t paintStyle;                      // 0x04
        uint8_t costIndex;                       // 0x05
        int16_t buildCostFactor;                 // 0x06
        int16_t sellCostFactor;                  // 0x08
        uint32_t image;                          // 0x0A
        uint32_t var_0E;

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(RoadExtraObject) == 0x12);
}
