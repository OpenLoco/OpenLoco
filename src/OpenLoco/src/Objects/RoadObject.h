#pragma once

#include "Object.h"
#include "Speed.hpp"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <span>

namespace OpenLoco
{
    namespace ObjectManager
    {
        struct DependentObjects;
    }
    enum class TownSize : uint8_t;

    namespace Gfx
    {
        struct RenderTarget;
    }
    enum class TownSize : uint8_t;

    enum class RoadObjectFlags : uint16_t
    {
        none = 0U,
        unk_00 = 1U << 0,
        unk_01 = 1U << 1,
        unk_02 = 1U << 2,
        unk_03 = 1U << 3, // Likely isTram
        unk_04 = 1U << 4,
        unk_05 = 1U << 5,
        isRoad = 1U << 6, // If not set this is tram track
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(RoadObjectFlags);

    enum class RoadObjectPieceFlags : uint16_t
    {
        none = 0U,
        oneWay = 1U << 0,
        track = 1U << 1,
        slope = 1U << 2,
        steepSlope = 1U << 3,
        intersection = 1U << 4,
        oneSided = 1U << 5,
        overtake = 1U << 6,
        streetLights = 1U << 8,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(RoadObjectPieceFlags);
#pragma pack(push, 1)
    struct RoadObject
    {
        static constexpr auto kObjectType = ObjectType::road;

        StringId name;
        RoadObjectPieceFlags roadPieces; // 0x02
        int16_t buildCostFactor;         // 0x04
        int16_t sellCostFactor;          // 0x06
        int16_t tunnelCostFactor;        // 0x08
        uint8_t costIndex;               // 0x0A
        uint8_t tunnel;                  // 0x0B
        Speed16 maxSpeed;                // 0x0C
        uint32_t image;                  // 0x0E
        RoadObjectFlags flags;           // 0x12
        uint8_t numBridges;              // 0x14
        uint8_t bridges[7];              // 0x15
        uint8_t numStations;             // 0x1C
        uint8_t stations[7];             // 0x1D
        uint8_t paintStyle;              // 0x24
        uint8_t numMods;                 // 0x25
        uint8_t mods[2];                 // 0x26
        uint8_t numCompatible;           // 0x28
        uint8_t pad_29;
        uint16_t compatibleRoads;  // 0x2A
        uint16_t compatibleTracks; // 0x2C
        TownSize targetTownSize;   // 0x2E
        uint8_t pad_2F;

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects* dependencies);
        void unload();

        constexpr bool hasFlags(RoadObjectFlags flagsToTest) const
        {
            return (flags & flagsToTest) != RoadObjectFlags::none;
        }

        constexpr bool hasPieceFlags(RoadObjectPieceFlags flagsToTest) const
        {
            return (roadPieces & flagsToTest) != RoadObjectPieceFlags::none;
        }
    };
#pragma pack(pop)

    static_assert(sizeof(RoadObject) == 0x30);
}
