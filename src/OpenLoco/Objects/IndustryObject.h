#pragma once

#include "../Core/Span.hpp"
#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct RenderTarget;
    }

    namespace IndustryObjectFlags
    {
        constexpr uint32_t builtInClusters = 1 << 0;
        constexpr uint32_t builtOnHighGround = 1 << 1;
        constexpr uint32_t builtOnLowGround = 1 << 2;
        constexpr uint32_t builtOnSnow = 1 << 3;        // above summer snow line
        constexpr uint32_t builtBelowSnowLine = 1 << 4; // below winter snow line
        constexpr uint32_t builtOnFlatGround = 1 << 5;
        constexpr uint32_t builtNearWater = 1 << 6;
        constexpr uint32_t builtAwayFromWater = 1 << 7;
        constexpr uint32_t builtOnWater = 1 << 8;
        constexpr uint32_t builtNearTown = 1 << 9;
        constexpr uint32_t builtAwayFromTown = 1 << 10;
        constexpr uint32_t builtNearTrees = 1 << 11;
        constexpr uint32_t builtRequiresOpenSpace = 1 << 12;
        constexpr uint32_t oilfield = 1 << 13;
        constexpr uint32_t mines = 1 << 14;
        constexpr uint32_t canBeFoundedByPlayer = 1 << 16;
        constexpr uint32_t requiresAllCargo = 1 << 17;
        constexpr uint32_t unk18 = 1 << 18;
        constexpr uint32_t unk19 = 1 << 19;
        constexpr uint32_t hasShadows = 1 << 21;
        constexpr uint32_t unk23 = 1 << 23;
        constexpr uint32_t builtInDesert = 1 << 24;
        constexpr uint32_t builtNearDesert = 1 << 25;
        constexpr uint32_t unk27 = 1 << 27;
        constexpr uint32_t flag_28 = 1 << 28;
    }
#pragma pack(push, 1)
    struct IndustryObject
    {
        static constexpr auto kObjectType = ObjectType::industry;

        string_id name;               // 0x0
        string_id var_02;             // 0x2
        string_id nameClosingDown;    // 0x4
        string_id nameUpProduction;   // 0x6
        string_id nameDownProduction; // 0x8
        uint16_t nameSingular;        // 0x0A
        uint16_t namePlural;          // 0x0C
        uint32_t var_0E;              // 0x0E shadows image id base
        uint32_t var_12;              // 0x12 Base image id for building 0
        uint32_t var_16;
        uint32_t var_1A;
        uint8_t var_1E;
        uint8_t var_1F;
        uint8_t* var_20; // This is the height of a building image
        uint32_t var_24;
        uint32_t var_28[4];
        uint32_t var_38;
        uint8_t* var_3C[32]; // This is a pointer for buildings[32] image offsets 0xFF indicates end of image offsets
        uint8_t var_BC;
        uint8_t var_BD;
        uint32_t var_BE;
        uint32_t var_C2;
        uint32_t buildingSizeFlags; // 0xC6 flags indicating the building types size 1:large4x4, 0:small1x1
        uint16_t designedYear;      // 0xCA start year
        uint16_t obsoleteYear;      // 0xCC end year
        // Total industries of this type that can be created in a scenario
        // Note: this is not directly comparabile to total industries and vaires based
        // on scenario total industries cap settings. At low industries cap this value is ~3x the
        // amount of industries in a scenario.
        uint8_t totalOfTypeInScenario;  // 0xCE
        uint8_t costIndex;              // 0xCF
        int16_t costFactor;             // 0xD0
        int16_t clearCostFactor;        // 0xD2
        uint8_t scaffoldingSegmentType; // 0xD4
        Colour scaffoldingColour;       // 0xD5
        uint16_t var_D6;
        uint8_t pad_D8[0xDA - 0xD8];
        uint16_t var_DA;
        uint8_t pad_DC[0xDE - 0xDC];
        uint8_t producedCargoType[2]; // 0xDE (0xFF = null)
        uint8_t requiredCargoType[3]; // 0xE0 (0xFF = null)
        uint8_t pad_E3;
        uint32_t flags; // 0xE4
        uint8_t var_E8;
        uint8_t var_E9;
        uint8_t var_EA;
        uint8_t var_EB;
        uint8_t var_EC;       // Used by Livestock cow shed count??
        uint8_t wallTypes[4]; // 0xED There can be up to 4 different wall types for an industry
        // Selection of wall types isn't completely random from the 4 it is biased into 2 groups of 2
        uint8_t var_F1;
        uint8_t var_F2;
        uint8_t var_F3;

        bool requiresCargo() const;
        bool producesCargo() const;
        char* getProducedCargoString(const char* buffer) const;
        char* getRequiredCargoString(const char* buffer) const;
        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        void drawIndustry(Gfx::RenderTarget* clipped, int16_t x, int16_t y) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
    };
#pragma pack(pop)
    static_assert(sizeof(IndustryObject) == 0xF4);
}
