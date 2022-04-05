#pragma once

#include "../Map/Map.hpp"
#include "../Types.hpp"
#include "Object.h"
#include <cstddef>
#include <vector>

namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
    }

    namespace TrainStationFlags
    {
        constexpr uint8_t recolourable = 1 << 0;
    }

#pragma pack(push, 1)
    struct TrainStationObject
    {
        static constexpr auto kObjectType = ObjectType::trackStation;

        struct CargoOffset
        {
            Map::Pos3 offsets[2];
        };

        string_id name;
        uint8_t drawStyle; // 0x02
        uint8_t var_03;
        uint16_t track_pieces;      // 0x04
        uint16_t build_cost_factor; // 0x06
        uint16_t sell_cost_factor;  // 0x08
        uint8_t cost_index;         // 0x0A
        uint8_t var_0B;
        uint8_t flags; // 0x0C
        uint8_t var_0D;
        uint32_t image; // 0x0E
        uint32_t var_12[4];
        uint8_t num_compatible; // 0x22
        uint8_t mods[7];
        uint16_t designed_year;            // 0x2A
        uint16_t obsolete_year;            // 0x2C
        std::byte* cargoOffsetBytes[4][4]; // 0x2E
        uint8_t pad_6E[0xAC - 0x6E];

        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::Context& context, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
        std::vector<CargoOffset> getCargoOffsets(const uint8_t rotation, const uint8_t nibble) const;
    };
#pragma pack(pop)

    namespace TrainStation::ImageIds
    {
        constexpr uint32_t preview_image = 0;
        constexpr uint32_t preview_image_windows = 1;
        // These are relative to to var_12
        // var_12 is the imageIds per sequenceIndex (for start/middle/end of the platform)
        constexpr uint32_t style0StraightBackNE = 0;
        constexpr uint32_t style0StraightFrontNE = 1;
        constexpr uint32_t style0StraightCanopyNE = 2;
        constexpr uint32_t style0StraightCanopyTranslucentNE = 3;
        constexpr uint32_t style0StraightBackSE = 4;
        constexpr uint32_t style0StraightFrontSE = 5;
        constexpr uint32_t style0StraightCanopySE = 6;
        constexpr uint32_t style0StraightCanopyTranslucentSE = 7;
        constexpr uint32_t style0DiagonalNE0 = 8;
        constexpr uint32_t style0DiagonalNE3 = 9;
        constexpr uint32_t style0DiagonalNE1 = 10;
        constexpr uint32_t style0DiagonalCanopyNE1 = 11;
        constexpr uint32_t style0DiagonalCanopyTranslucentNE1 = 12;
        constexpr uint32_t style0DiagonalSE1 = 13;
        constexpr uint32_t style0DiagonalSE2 = 14;
        constexpr uint32_t style0DiagonalSE3 = 15;
        constexpr uint32_t style0DiagonalCanopyTranslucentSE3 = 16;
    }
}
