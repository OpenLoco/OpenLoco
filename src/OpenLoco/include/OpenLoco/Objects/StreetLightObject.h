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
        class DrawingContext;
    }

#pragma pack(push, 1)
    struct StreetLightObject
    {
        static constexpr auto kObjectType = ObjectType::streetLight;

        StringId name;
        uint16_t designedYear[3]; // 0x02
        uint32_t image;           // 0x08

        void drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const;
        // 0x00477F5F
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(StreetLightObject) == 0xC);

    namespace Streetlight::ImageIds
    {
        constexpr uint32_t kStyle0NE = 0;
        constexpr uint32_t kStyle0SE = 1;
        constexpr uint32_t kStyle0SW = 2;
        constexpr uint32_t kStyle0NW = 3;
        constexpr uint32_t kStyle1NE = 4;
        constexpr uint32_t kStyle1SE = 5;
        constexpr uint32_t kStyle1SW = 6;
        constexpr uint32_t kStyle1NW = 7;
        constexpr uint32_t kStyle2NE = 8;
        constexpr uint32_t kStyle2SE = 9;
        constexpr uint32_t kStyle2SW = 10;
        constexpr uint32_t kStyle2NW = 11;
    }
}
