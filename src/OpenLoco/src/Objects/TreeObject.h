#pragma once

#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Span.hpp>
#include <array>

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

    enum class TreeObjectFlags : uint16_t
    {
        none = 0U,
        hasSnowVariation = 1U << 0,
        unk1 = 1U << 1,
        veryHighAltitude = 1U << 2,
        highAltitude = 1U << 3,
        requiresWater = 1U << 4,
        unk5 = 1U << 5,
        droughtResistant = 1U << 6,
        hasShadow = 1U << 7,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(TreeObjectFlags);

#pragma pack(push, 1)
    struct TreeObject
    {
        static constexpr auto kObjectType = ObjectType::tree;

        string_id name;                   // 0x00
        uint8_t var_02;                   // 0x02
        uint8_t height;                   // 0x03
        uint8_t var_04;                   // 0x04
        uint8_t var_05;                   // 0x05
        uint8_t numRotations;             // 0x06 (1,2,4)
        uint8_t growth;                   // 0x07 (number of tree size images)
        TreeObjectFlags flags;            // 0x08
        uint32_t sprites[6];              // 0x0A
        uint32_t snowSprites[6];          // 0x22
        uint16_t shadowImageOffset;       // 0x3A
        uint8_t var_3C;                   // 0x3C
        uint8_t seasonState;              // 0x3D (index for sprites, seasons + dying)
        uint8_t var_3E;                   // 0x3E
        uint8_t costIndex;                // 0x3F
        int16_t buildCostFactor;          // 0x40
        int16_t clearCostFactor;          // 0x42
        uint32_t colours;                 // 0x44
        int16_t rating;                   // 0x48
        uint16_t demolishRatingReduction; // 0x4A

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        uint8_t getTreeGrowthDisplayOffset() const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
        constexpr bool hasFlags(TreeObjectFlags flagsToTest) const
        {
            return (flags & flagsToTest) != TreeObjectFlags::none;
        }
    };
#pragma pack(pop)

    static_assert(sizeof(TreeObject) == 0x4C);
}
