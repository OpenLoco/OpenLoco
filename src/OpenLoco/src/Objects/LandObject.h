#pragma once

#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Span.hpp>

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

    enum class LandObjectFlags : uint8_t
    {
        none = 0U,
        unk0 = 1U << 0,
        unk1 = 1U << 1,
        isDesert = 1U << 2,
        noTrees = 1U << 3,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(LandObjectFlags);

#pragma pack(push, 1)
    struct LandObject
    {
        static constexpr auto kObjectType = ObjectType::land;

        string_id name;
        uint8_t costIndex; // 0x02
        uint8_t var_03;
        uint8_t var_04;
        LandObjectFlags flags; // 0x05
        uint8_t var_06;
        uint8_t var_07;
        int8_t costFactor; // 0x08
        uint8_t pad_09;
        uint32_t image; // 0x0A
        uint32_t var_0E;
        uint32_t var_12;
        uint32_t var_16;
        uint8_t pad_1A;
        uint8_t numVariations;       // 0x1B
        uint8_t variationLikelihood; // 0x1C
        uint8_t pad_1D;

        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects* dependencies);
        void unload();
        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;

        constexpr bool hasFlags(LandObjectFlags flagsToTest) const
        {
            return (flags & flagsToTest) != LandObjectFlags::none;
        }
    };
    static_assert(sizeof(LandObject) == 0x1E);
#pragma pack(pop)

    namespace Land::ImageIds
    {
        constexpr uint32_t landscape_generator_tile_icon = 1;
        constexpr uint32_t toolbar_terraform_land = 3;
    }
}
