#pragma once

#include "Core/Span.hpp"
#include "Types.hpp"
#include "Object.h"
#include <utility>

namespace OpenLoco
{
    namespace SteamObjectFlags
    {
        constexpr uint16_t applyWind = (1 << 0);
        constexpr uint16_t disperseOnCollision = (1 << 1);
        constexpr uint16_t unk2 = (1 << 2);
        constexpr uint16_t unk3 = (1 << 3);
    }
#pragma pack(push, 1)
    struct SteamObject
    {
        struct ImageAndHeight
        {
            uint8_t imageOffset;
            uint8_t height;
        };
        static constexpr auto kObjectType = ObjectType::steam;

        string_id name; // 0x00 probably not confirmed
        uint8_t pad_02[0x4 - 0x2];
        uint8_t numStationaryTicks; // 0x04 while stationary can be affected by wind
        uint8_t var_05;
        uint8_t var_06;
        uint8_t var_07;
        uint16_t flags; // 0x08
        uint32_t var_0A;
        uint32_t baseImageId;           // 0x0E
        uint16_t totalNumFramesType0;   // 0x12
        uint16_t totalNumFramesType1;   // 0x14
        ImageAndHeight* frameInfoType0; // 0x16
        ImageAndHeight* frameInfoType1; // 0x1A
        uint8_t soundEffect;            // 0x1E probably not confirmed
        uint8_t var_1F[9];              // size tbc

        // 0x00440DDE
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();

        std::pair<uint16_t, ImageAndHeight*> getFramesInfo(bool isType1) const
        {
            if (!isType1)
            {
                return { totalNumFramesType0, frameInfoType0 };
            }
            else
            {
                return { totalNumFramesType1, frameInfoType1 };
            }
        }
    };
#pragma pack(pop)
    static_assert(sizeof(SteamObject) == 0x28);
}
