#pragma once

#include "../Localisation/StringManager.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct drawpixelinfo_t;
    }

#pragma pack(push, 1)
    struct land_object
    {
        string_id name;
        uint8_t cost_index; // 0x02
        uint8_t var_03;
        uint8_t pad_04[0x8 - 0x04];
        uint8_t cost_factor; // 0x08
        uint8_t pad_09[0x0A - 0x09];
        uint32_t image; // 0x0A
        uint8_t var_0E;
        uint8_t pad_0F[0x16 - 0x0F];
        uint32_t var_16;

        void drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)

    namespace Land::ImageIds
    {
        constexpr uint32_t landscape_generator_tile_icon = 1;
        constexpr uint32_t toolbar_terraform_land = 3;
    }
}
