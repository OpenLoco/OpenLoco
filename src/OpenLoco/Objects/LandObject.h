#pragma once

#include "../Core/Span.hpp"
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
        uint8_t var_04;
        uint8_t flags; //0x05
        uint8_t var_06;
        uint8_t var_07;
        int8_t cost_factor; // 0x08
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
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
    };
    static_assert(sizeof(LandObject) == 0x1E);
#pragma pack(pop)

    namespace Land::ImageIds
    {
        constexpr uint32_t landscape_generator_tile_icon = 1;
        constexpr uint32_t toolbar_terraform_land = 3;
    }
}
