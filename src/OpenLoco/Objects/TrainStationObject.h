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
        uint8_t var_02;
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
    }
}
