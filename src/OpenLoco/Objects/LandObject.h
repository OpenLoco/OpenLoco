#pragma once

#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
    }

    namespace LandObjectFlags
    {
        constexpr uint8_t unk0 = (1 << 0);
        constexpr uint8_t unk1 = (1 << 1);
        constexpr uint8_t isDesert = (1 << 2);
        constexpr uint8_t noTrees = (1 << 3);
    }

#pragma pack(push, 1)
    struct LandObject
    {
        static constexpr auto kObjectType = ObjectType::land;

        string_id name;
        uint8_t cost_index; // 0x02
        uint8_t var_03;
        uint8_t pad_04;
        uint8_t flags; //0x05
        uint8_t pad_06[0x8 - 0x6];
        uint8_t cost_factor; // 0x08
        uint8_t pad_09[0x0A - 0x09];
        uint32_t image; // 0x0A
        uint8_t var_0E;
        uint8_t pad_0F[0x16 - 0x0F];
        uint32_t var_16;
        uint8_t var_1B;
        uint8_t var_1C;

        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)

    namespace Land::ImageIds
    {
        constexpr uint32_t landscape_generator_tile_icon = 1;
        constexpr uint32_t toolbar_terraform_land = 3;
    }
}
