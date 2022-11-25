#pragma once

#include "Core/Span.hpp"
#include "Map/Map.hpp"
#include "Object.h"
#include "Types.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace OpenLoco
{
    namespace Gfx
    {
        struct RenderTarget;
    }

    namespace TrainStationFlags
    {
        constexpr uint8_t recolourable = 1 << 0;
        constexpr uint8_t unk1 = 1 << 1; // Has no canopy??
    }

#pragma pack(push, 1)
    struct TrainStationObject
    {
        static constexpr auto kObjectType = ObjectType::trackStation;

        using CargoOffset = std::array<Map::Pos3, 2>;

        string_id name;
        uint8_t drawStyle; // 0x02
        uint8_t var_03;
        uint16_t trackPieces;    // 0x04
        int16_t buildCostFactor; // 0x06
        int16_t sellCostFactor;  // 0x08
        uint8_t costIndex;       // 0x0A
        uint8_t var_0B;
        uint8_t flags; // 0x0C
        uint8_t var_0D;
        uint32_t image; // 0x0E
        uint32_t var_12[4];
        uint8_t numCompatible; // 0x22
        uint8_t mods[7];
        uint16_t designedYear;             // 0x2A
        uint16_t obsoleteYear;             // 0x2C
        std::byte* cargoOffsetBytes[4][4]; // 0x2E
        uint32_t var_6E[16];

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
        std::vector<CargoOffset> getCargoOffsets(const uint8_t rotation, const uint8_t nibble) const;
    };
#pragma pack(pop)
    static_assert(sizeof(TrainStationObject) == 0xAE);

    namespace TrainStation::ImageIds
    {
        constexpr uint32_t preview_image = 0;
        constexpr uint32_t preview_image_windows = 1;
        // These are relative to var_12
        // var_12 is the imageIds per sequenceIndex (for start/middle/end of the platform)
        namespace Style0
        {
            constexpr uint32_t straightBackNE = 0;
            constexpr uint32_t straightFrontNE = 1;
            constexpr uint32_t straightCanopyNE = 2;
            constexpr uint32_t straightCanopyTranslucentNE = 3;
            constexpr uint32_t straightBackSE = 4;
            constexpr uint32_t straightFrontSE = 5;
            constexpr uint32_t straightCanopySE = 6;
            constexpr uint32_t straightCanopyTranslucentSE = 7;
            constexpr uint32_t diagonalNE0 = 8;
            constexpr uint32_t diagonalNE3 = 9;
            constexpr uint32_t diagonalNE1 = 10;
            constexpr uint32_t diagonalCanopyNE1 = 11;
            constexpr uint32_t diagonalCanopyTranslucentNE1 = 12;
            constexpr uint32_t diagonalSE1 = 13;
            constexpr uint32_t diagonalSE2 = 14;
            constexpr uint32_t diagonalSE3 = 15;
            constexpr uint32_t diagonalCanopyTranslucentSE3 = 16;
        }
    }
}
