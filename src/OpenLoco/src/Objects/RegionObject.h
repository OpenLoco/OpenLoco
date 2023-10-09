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
    struct RegionObject
    {
        static constexpr auto kObjectType = ObjectType::region;

        StringId name;
        uint32_t image; // 0x02
        uint8_t pad_06[0x8 - 0x6];
        uint8_t var_08;
        uint8_t var_09[4];
        uint8_t pad_0D[0x12 - 0xD];

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        // 0x0043CB89
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects* dependencies);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(RegionObject) == 0x12);
}
