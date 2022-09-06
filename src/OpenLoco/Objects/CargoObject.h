#pragma once

#include "../Core/Span.hpp"
#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
    namespace CargoObjectFlags
    {
        constexpr uint8_t unk0 = (1 << 0);
        constexpr uint8_t refit = (1 << 1);
        constexpr uint8_t unk2 = (1 << 2);
    }

#pragma pack(push, 1)
    struct CargoObject
    {
        static constexpr auto kObjectType = ObjectType::cargo;

        string_id name; // 0x0
        uint16_t var_2;
        uint16_t var_4;
        string_id unitsAndCargoName; // 0x6
        string_id unitNameSingular;  // 0x8
        string_id unitNamePlural;    // 0xA
        uint32_t unitInlineSprite;   // 0xC
        std::uint8_t pad_10[0x12 - 0x10];
        uint8_t flags;                 // 0x12
        uint8_t numPlatformVariations; // 0x13
        uint8_t var_14;
        uint8_t premiumDays;       // 0x15
        uint8_t maxNonPremiumDays; // 0x16
        uint16_t nonPremiumRate;   // 0x17
        uint16_t penaltyRate;      // 0x19
        uint16_t paymentFactor;    // 0x1B
        uint8_t paymentIndex;      // 0x1D
        uint8_t unitSize;          // 0x1E

        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(CargoObject) == 0x1F);
    namespace Cargo::ImageIds
    {
        constexpr uint32_t inlineSprite = 0;
        // There are numPlatformVariations sprites after this one
        constexpr uint32_t stationPlatformBegin = 1;
    }
}
