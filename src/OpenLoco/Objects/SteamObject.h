#pragma once

#include "../Core/Span.hpp"
#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
#pragma pack(push, 1)
    struct SteamObject
    {
        static constexpr auto kObjectType = ObjectType::steam;

        string_id name; // 0x00 probably not confirmed
        uint8_t pad_02[0x5 - 0x2];
        uint8_t var_05;
        uint8_t var_06;
        uint8_t var_07;
        uint16_t var_08;
        uint32_t var_0A;
        uint32_t baseImageId; // 0x0E
        uint16_t var_12;
        uint16_t var_14;
        uint8_t* var_16;
        uint8_t* var_1A;
        uint8_t sound_effect; // 0x1E probably not confirmed
        uint8_t var_1F[9];    // size tbc

        // 0x00440DDE
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
    };
#pragma pack(pop)
    static_assert(sizeof(SteamObject) == 0x28);
}
