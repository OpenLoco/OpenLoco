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

    enum class WallObjectFlags : uint8_t
    {
        none = 0U,
        hasPrimaryColour = 1U << 0,
        unk1 = 1U << 1,
        onlyOnLevelLand = 1U << 2,
        unk3 = 1U << 3,
        unk4 = 1U << 4,
        unk5 = 1U << 5,
        hasSecondaryColour = 1U << 6,
        hasTertiaryColour = 1U << 7,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(WallObjectFlags);

#pragma pack(push, 1)
    struct WallObject
    {
        static constexpr auto kObjectType = ObjectType::wall;

        StringId name;
        uint32_t sprite; // 0x02
        uint8_t var_06;
        WallObjectFlags flags; // 0x07
        uint8_t height;
        uint8_t var_09;

        // 0x004C4AF0
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
    static_assert(sizeof(WallObject) == 0xA);
}
