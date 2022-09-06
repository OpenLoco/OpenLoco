#pragma once

#include "../Core/Span.hpp"
#include "../Graphics/Colour.h"
#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct RenderTarget;
    }

    namespace BuildingObjectFlags
    {
        constexpr uint32_t largeTle = 1 << 0; // 2x2 tile
        constexpr uint32_t miscBuilding = 1 << 1;
        constexpr uint32_t undestructible = 1 << 2;
        constexpr uint32_t isHeadquarters = 1 << 3;
    }

#pragma pack(push, 1)
    struct BuildingObject
    {
        static constexpr auto kObjectType = ObjectType::building;

        string_id name;               // 0x0
        uint32_t image;               // 0x2
        uint8_t var_06;               // 0x6
        uint8_t numVariations;        // 0x7
        uint8_t* varationHeights;     // 0x8
        uint32_t var_0C;              // 0xC
        uint8_t* variationsArr10[32]; // 0x10
        uint32_t colours;             // 0x90
        uint16_t designedYear;        // 0x94
        uint16_t obsoleteYear;        // 0x96
        uint8_t flags;                // 0x98
        uint8_t clearCostIndex;       // 0x99
        uint16_t clearCostFactor;     // 0x9A
        uint8_t pad_9C[0xA0 - 0x9C];
        uint8_t producedQuantity[2];  // 0xA0
        uint8_t producedCargoType[2]; // 0xA2
        uint8_t var_A4[2];
        uint8_t var_A6[2];
        uint8_t var_A8[2];
        int16_t demolishRatingReduction;
        uint8_t var_AC;
        uint8_t var_AD;
        uint32_t var_AE[4];

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        void drawBuilding(Gfx::RenderTarget* clipped, uint8_t buildingRotation, int16_t x, int16_t y, Colour colour) const;
        void drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
    };
#pragma pack(pop)
    static_assert(sizeof(BuildingObject) == 0xBE);
}
