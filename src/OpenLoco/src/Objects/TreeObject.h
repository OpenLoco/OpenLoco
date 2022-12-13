#pragma once

#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/Span.hpp>
#include <array>

namespace OpenLoco
{
    namespace Gfx
    {
        struct RenderTarget;
    }

    namespace TreeObjectFlags
    {
        constexpr uint16_t hasSnowVariation = (1 << 0);
        constexpr uint16_t unk1 = (1 << 1);
        constexpr uint16_t veryHighAltitude = (1 << 2);
        constexpr uint16_t highAltitude = (1 << 3);
        constexpr uint16_t requiresWater = (1 << 4);
        constexpr uint16_t unk5 = (1 << 5);
        constexpr uint16_t droughtResistant = (1 << 6);
        constexpr uint16_t hasShadow = (1 << 7);
    }

#pragma pack(push, 1)
    struct TreeObject
    {
        static constexpr auto kObjectType = ObjectType::tree;

        string_id name;
        uint8_t var_02;
        uint8_t height; // 0x03
        uint8_t var_04;
        uint8_t var_05;
        uint8_t numRotations;       // 0x06 (1,2,4)
        uint8_t growth;             // 0x07 (number of tree size images)
        uint16_t flags;             // 0x08
        uint32_t sprites[2][6];     // 0x0A
        uint16_t shadowImageOffset; // 0x3A
        uint8_t var_3C;
        uint8_t seasonState; // 0x3D (index for sprites, seasons + dying)
        uint8_t var_3E;
        uint8_t costIndex;       // 0x3F
        int16_t buildCostFactor; // 0x40
        int16_t clearCostFactor; // 0x42
        uint32_t colours;        // 0x44
        int16_t rating;          // 0x48
        uint16_t var_4A;

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        uint8_t getTreeGrowthDisplayOffset() const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(TreeObject) == 0x4C);
}
