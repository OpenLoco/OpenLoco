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
    struct StreetLightObject
    {
        static constexpr auto kObjectType = ObjectType::streetLight;

        string_id name;
        uint16_t designedYear[3]; // 0x02
        uint32_t image;           // 0x08

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        // 0x00477F5F
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(StreetLightObject) == 0xC);
}
