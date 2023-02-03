#pragma once

#include "Object.h"
#include "Speed.hpp"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Span.hpp>

namespace OpenLoco
{
    namespace ObjectManager
    {
        struct DependentObjects;
    }
    namespace Gfx
    {
        struct RenderTarget;
    }

    enum class Flags12 : uint16_t // Made this a uint16_t to maintain the size of the RoadObject struct
    {
        none = 0U,
        unk_01 = 1U << 1,
        unk_02 = 1U << 2,
        unk_03 = 1U << 3, // Likely isTram
        unk_04 = 1U << 4,
        unk_05 = 1U << 5,
        isRoad = 1U << 6, // If not set this is tram track
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(Flags12);

    enum class RoadPieceFlags : uint16_t
    {
        none = 0U,
        oneWay = 1U << 0,
        track = 1U << 1,
        slope = 1U << 2,
        steepSlope = 1U << 3,
        intersection = 1U << 2, // This is never referenced anywhere and in the same spot as slope
        oneSided = 1U << 5,
        overtake = 1U << 6,
        streetLights = 1U << 8, // This is never referenced anywhere and why is it offset by 8 instead of 7?
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(RoadPieceFlags);
#pragma pack(push, 1)
    struct RoadObject
    {
        static constexpr auto kObjectType = ObjectType::road;

        string_id name;
        RoadPieceFlags roadPieces; // 0x02
        int16_t buildCostFactor;   // 0x04
        int16_t sellCostFactor;    // 0x06
        int16_t tunnelCostFactor;  // 0x08
        uint8_t costIndex;         // 0x0A
        uint8_t var_0B;
        Speed16 maxSpeed;      // 0x0C
        uint32_t image;        // 0x0E
        Flags12 flags;         // 0x12
        uint8_t numBridges;    // 0x14
        uint8_t bridges[7];    // 0x15
        uint8_t numStations;   // 0x1C
        uint8_t stations[7];   // 0x1D
        uint8_t paintStyle;    // 0x24
        uint8_t numMods;       // 0x25
        uint8_t mods[2];       // 0x26
        uint8_t numCompatible; // 0x28
        uint8_t pad_29[0x30 - 0x29];

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects* dependencies);
        void unload();

        constexpr bool hasFlags(Flags12 flagsToTest) const
        {
            return (flags & flagsToTest) != Flags12::none;
        }

        constexpr bool hasFlags(RoadPieceFlags flagsToTest) const
        {
            return (roadPieces & flagsToTest) != RoadPieceFlags::none;
        }
    };
#pragma pack(pop)

    static_assert(sizeof(RoadObject) == 0x30);
}
