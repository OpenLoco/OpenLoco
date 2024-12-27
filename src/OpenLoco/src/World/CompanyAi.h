#pragma once

#include "Economy/Currency.h"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/World.hpp>

namespace OpenLoco
{
    enum class AiThinkState : uint8_t
    {
        unk0,
        unk1,
        unk2,
        unk3,
        unk4,
        unk5,
        unk6,
        unk7,
        unk8,
        unk9,
        endCompany,
    };

    enum class AiThoughtType : uint8_t
    {
        unk0,
        unk1,
        unk2,
        unk3,
        unk4,
        unk5,
        unk6,
        unk7,
        unk8,
        unk9,
        unk10,
        unk11,
        unk12,
        unk13,
        unk14,
        unk15,
        unk16,
        unk17,
        unk18,
        unk19,

        null = 0xFF
    };
    constexpr auto kAiThoughtTypeCount = 20U;

    enum class AiThoughtStationFlags : uint8_t
    {
        none = 0U,

        aiAllocated = 1U << 0,
        operational = 1U << 1,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(AiThoughtStationFlags);

    constexpr auto kMaxAiThoughts = 60U;
    constexpr auto kAiThoughtIdNull = 0xFFU;

#pragma pack(push, 1)
    struct AiThought
    {
        struct Station
        {
            StationId id;                 // 0x0
            AiThoughtStationFlags var_02; // 0x2 flags?
            uint8_t rotation;             // 0x3
            World::Pos2 pos;              // 0x4
            uint8_t baseZ;                // 0x8
            uint8_t var_9;                // 0x9 aiStationIndex
            uint8_t var_A;                // 0xA aiStationIndex
            uint8_t var_B;                // 0xB
            uint8_t var_C;                // 0xC
            uint8_t pad_D[0xE - 0xD];

            constexpr bool hasFlags(AiThoughtStationFlags flags) const { return (var_02 & flags) != AiThoughtStationFlags::none; }
        };
        static_assert(sizeof(Station) == 0xE);
        AiThoughtType type; // 0x00 0x4A8
        uint8_t var_01;     // 0x4A9
        uint8_t pad_02;
        uint8_t numStations; // 0x03 0x4AB size of stations
        uint8_t var_04;      // 0x4AC station length
        uint8_t pad_05;
        Station stations[4]; // 0x06 0x4AE Will lists stations created that vehicles will route to
        uint8_t trackObjId;  // 0x3E 0x4E6 track or road (with high bit set)
        uint8_t pad_3F;
        uint8_t mods; // 0x40 0x4E8 track or road
        uint8_t pad_41;
        uint8_t cargoType;    // 0x42 0x4EA
        uint8_t var_43;       // 0x4EB
        uint8_t numVehicles;  // 0x44 0x4EC size of var_66
        uint8_t var_45;       // 0x4ED size of var_46
        uint16_t var_46[16];  // 0x4EF array of uint16_t object id
        EntityId vehicles[8]; // 0x66 0x50E see also numVehicles for current size
        currency32_t var_76;  // 0x51E
        uint8_t pad_7A[0x7C - 0x7A];
        currency32_t var_7C; // 0x524
        currency32_t var_80; // 0x528
        currency32_t var_84; // 0x52C
        uint8_t var_88;      // 0x530
        uint8_t var_89;      // 0x531 station obj type?
        uint8_t var_8A;      // 0x532
        uint8_t var_8B;      // 0x533
    };
#pragma pack(pop)
    static_assert(sizeof(AiThought) == 0x8C);

    void aiThink(CompanyId id);

    void setAiObservation(CompanyId id);
    void removeEntityFromThought(AiThought& thought, EntityId id);
}
