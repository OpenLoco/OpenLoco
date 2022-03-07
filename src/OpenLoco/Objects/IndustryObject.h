#pragma once

#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
    }

    namespace IndustryObjectFlags
    {
        constexpr uint32_t built_on_water = 1 << 8;
        constexpr uint32_t oilfield = 1 << 13;
        constexpr uint32_t mines = 1 << 14;
        constexpr uint32_t can_be_founded_by_user = 1 << 16;
        constexpr uint32_t requires_all_cargo = 1 << 17;
        constexpr uint32_t flag_28 = 1 << 28;
    }
#pragma pack(push, 1)
    struct IndustryObject
    {
        static constexpr auto kObjectType = ObjectType::industry;

        string_id name;
        uint8_t pad_02[0x04 - 0x02];
        string_id nameClosingDown;    // 0x4
        string_id nameUpProduction;   // 0x6
        string_id nameDownProduction; // 0x8
        uint16_t nameSingular;        // 0x0A
        uint16_t namePlural;          // 0x0C
        uint8_t pad_0E[0xC6 - 0x0E];
        uint32_t var_C6;
        uint16_t designedYear; // 0xCA start year
        uint16_t obsoleteYear; // 0xCC end year
        uint8_t var_CE;
        uint8_t cost_index;  // 0xCF
        int16_t cost_factor; // 0xD0
        uint8_t pad_D2[0xDE - 0xD2];
        uint8_t produced_cargo_type[2]; // 0xDE (0xFF = null)
        uint8_t required_cargo_type[3]; // 0xE0 (0xFF = null)
        uint8_t pad_E3;
        uint32_t flags; // 0xE4
        uint8_t pad_E8[0xEA - 0xE8];
        uint8_t var_EA;
        uint8_t var_EB;
        uint8_t pad_EC;
        uint8_t var_ED;
        uint8_t var_EE;
        uint8_t var_EF;
        uint8_t var_F0;

        bool requiresCargo() const;
        bool producesCargo() const;
        char* getProducedCargoString(const char* buffer) const;
        char* getRequiredCargoString(const char* buffer) const;
        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
        void drawIndustry(Gfx::Context* clipped, int16_t x, int16_t y) const;
    };
    static_assert(sizeof(IndustryObject) == 0xF1);
#pragma pack(pop)
}
