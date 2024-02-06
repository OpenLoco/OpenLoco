#pragma once

#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/World.hpp>
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
        hasGlass = 1U << 1,
        onlyOnLevelLand = 1U << 2,
        unk3 = 1U << 3,
        twoSided = 1U << 4, // twoSided and hasGlass are mutually exclusive (okay they technically aren't but we wont render them correctly)
        unk5 = 1U << 5,
        hasSecondaryColour = 1U << 6,
        hasTertiaryColour = 1U << 7, // Note: Not usable in Locomotion as only 2 remaps work
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(WallObjectFlags);

#pragma pack(push, 1)
    struct WallObject
    {
        static constexpr auto kObjectType = ObjectType::wall;

        StringId name;
        uint32_t sprite;       // 0x02
        uint8_t var_06;        // 0x06 tool cursor type not used in Locomotion
        WallObjectFlags flags; // 0x07
        World::SmallZ height;  // 0x08
        uint8_t var_09;        // 0x09 flags2 None of these are used in Locomotion 0x10 used to be animation

        // 0x004C4AF0
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
    static_assert(sizeof(WallObject) == 0xA);

    namespace WallObj::ImageIds
    {
        constexpr uint32_t kFlatSE = 0;
        constexpr uint32_t kFlatNE = 1;
        constexpr uint32_t kSlopedSE = 2;
        constexpr uint32_t kSlopedNE = 3;
        constexpr uint32_t kSlopedNW = 4;
        constexpr uint32_t kSlopedSW = 5;
        // If two sided the following are for the back side of the wall
        constexpr uint32_t kGlassFlatSE = 6;
        constexpr uint32_t kGlassFlatNE = 7;
        constexpr uint32_t kGlassSlopedSE = 8;
        constexpr uint32_t kGlassSlopedNE = 9;
        constexpr uint32_t kGlassSlopedNW = 10;
        constexpr uint32_t kGlassSlopedSW = 11;
    }
}
