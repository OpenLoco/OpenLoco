#pragma once

#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Span.hpp>
#include <utility>

namespace OpenLoco
{
    namespace ObjectManager
    {
        struct DependentObjects;
    }
    enum class SteamObjectFlags : uint16_t
    {
        none = 0U,
        applyWind = 1U << 0,
        disperseOnCollision = 1U << 1,
        unk2 = 1U << 2,
        unk3 = 1U << 3,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(SteamObjectFlags);

#pragma pack(push, 1)
    struct SteamObject
    {
        struct ImageAndHeight
        {
            uint8_t imageOffset;
            uint8_t height;
        };
        static constexpr auto kObjectType = ObjectType::steam;

        string_id name;               // 0x00
        uint16_t numImages;           // 0x02
        uint8_t numStationaryTicks;   // 0x04 while stationary can be affected by wind
        uint8_t spriteWidth;          // 0x05
        uint8_t spriteHeightNegative; // 0x06
        uint8_t spriteHeightPositive; // 0x07
        SteamObjectFlags flags;       // 0x08
        uint32_t var_0A;
        uint32_t baseImageId;                 // 0x0E
        uint16_t totalNumFramesType0;         // 0x12
        uint16_t totalNumFramesType1;         // 0x14
        const ImageAndHeight* frameInfoType0; // 0x16
        const ImageAndHeight* frameInfoType1; // 0x1A
        uint8_t numSoundEffects;              // 0x1E
        SoundObjectId_t soundEffects[9];      // size tbc

        // 0x00440DDE
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects* dependencies);
        void unload();

        constexpr bool hasFlags(SteamObjectFlags flagsToTest) const
        {
            return (flags & flagsToTest) != SteamObjectFlags::none;
        }

        std::pair<uint16_t, const ImageAndHeight*> getFramesInfo(bool isType1) const
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
