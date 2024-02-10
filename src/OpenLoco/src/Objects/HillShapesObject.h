#pragma once

#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
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

    enum class HillShapeFlags : uint16_t
    {
        none = 0U,
        isHeightMap = 1 << 0,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(HillShapeFlags);

#pragma pack(push, 1)
    struct HillShapesObject
    {
        static constexpr auto kObjectType = ObjectType::hillShapes;

        StringId name;
        uint8_t hillHeightMapCount;     // 0x02
        uint8_t mountainHeightMapCount; // 0x03
        uint32_t image;                 // 0x04
        uint32_t imageHills;            // 0x08
        HillShapeFlags flags;           // 0x0C

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        // 0x00463BB3
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(HillShapesObject) == 0xE);
}
