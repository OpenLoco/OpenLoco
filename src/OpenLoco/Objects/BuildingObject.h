#pragma once

#include "../Graphics/Colour.h"
#include "../Localisation/StringManager.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
    }

    namespace BuildingObjectFlags
    {
        constexpr uint32_t large_tile = 1 << 0; // 2x2 tile
        constexpr uint32_t misc_building = 1 << 1;
        constexpr uint32_t undestructible = 1 << 2;
        constexpr uint32_t is_headquarters = 1 << 3;
    }

#pragma pack(push, 1)
    struct BuildingObject
    {
        string_id name;
        uint8_t pad_02[0x07 - 0x02];
        uint8_t numVariations; //0x7
        uint8_t pad_08[0x90 - 0x08];
        uint32_t colours;      // 0x90
        uint16_t designedYear; // 0x94
        uint16_t obsoleteYear; // 0x96
        uint8_t flags;         // 0x98
        uint8_t pad_99[0xA0 - 0x99];
        uint8_t var_A0[2];
        uint8_t producedCargoType[2];
        uint8_t var_A4[2];
        uint8_t var_A6[2];
        uint8_t var_A8[2];

        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
        void drawBuilding(Gfx::Context* clipped, uint8_t buildingRotation, int16_t x, int16_t y, Colour_t colour) const;
        void drawDescription(Gfx::Context& context, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
    };
#pragma pack(pop)
    static_assert(sizeof(BuildingObject) == 0xAA);
}
