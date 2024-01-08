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
    namespace Gfx
    {
        struct RenderTarget;
    }

    enum class TrackObjectFlags : uint16_t
    {
        none = 0U,
        unk_00 = 1U << 0,
        unk_01 = 1U << 1,
        unk_02 = 1U << 2,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(TrackObjectFlags);

    enum class TrackObjectPieceFlags : uint16_t
    {
        none = 0U,
        diagonal = 1U << 0,
        largeCurve = 1U << 1,
        normalCurve = 1U << 2,
        smallCurve = 1U << 3,
        verySmallCurve = 1U << 4,
        slope = 1U << 5,
        steepSlope = 1U << 6,
        oneSided = 1U << 7,
        slopedCurve = 1U << 8,
        sBend = 1U << 9,
        junction = 1U << 10,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(TrackObjectPieceFlags);

#pragma pack(push, 1)
    struct TrackObject
    {
        static constexpr auto kObjectType = ObjectType::track;

        StringId name;
        TrackObjectPieceFlags trackPieces; // 0x02
        uint16_t stationTrackPieces;       // 0x04
        uint8_t var_06;
        uint8_t numCompatible;     // 0x07
        uint8_t numMods;           // 0x08
        uint8_t numSignals;        // 0x09
        uint8_t mods[4];           // 0x0A
        uint16_t signals;          // 0x0E bitset
        uint16_t compatibleTracks; // 0x10 bitset
        uint16_t compatibleRoads;  // 0x12 bitset
        int16_t buildCostFactor;   // 0x14
        int16_t sellCostFactor;    // 0x16
        int16_t tunnelCostFactor;  // 0x18
        uint8_t costIndex;         // 0x1A
        uint8_t tunnel;            // 0x1B
        Speed16 curveSpeed;        // 0x1C
        uint32_t image;            // 0x1E
        TrackObjectFlags flags;    // 0x22
        uint8_t numBridges;        // 0x24
        uint8_t bridges[7];        // 0x25
        uint8_t numStations;       // 0x2C
        uint8_t stations[7];       // 0x2D
        uint8_t displayOffset;     // 0x34
        uint8_t pad_35;

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects* dependencies);
        void unload();

        constexpr bool hasFlags(TrackObjectFlags flagsToTest) const
        {
            return (flags & flagsToTest) != TrackObjectFlags::none;
        }

        constexpr bool hasPieceFlags(TrackObjectPieceFlags flagsToTest) const
        {
            return (trackPieces & flagsToTest) != TrackObjectPieceFlags::none;
        }
    };
#pragma pack(pop)

    static_assert(sizeof(TrackObject) == 0x36);

    namespace TrackObj::ImageIds
    {
        // Assumes rotational symmetry
        namespace Style0
        {
            constexpr uint32_t straightBallastNE = 18;
            constexpr uint32_t straightBallastSW = 19;
            constexpr uint32_t straightSleeperNE = 20;
            constexpr uint32_t straightSleeperSW = 21;
            constexpr uint32_t straightRailNE = 22;
            constexpr uint32_t straightRailSW = 23;

            constexpr uint32_t diagonal0BallastE = 328;
            constexpr uint32_t diagonal2BallastE = 329;
            constexpr uint32_t diagonal1BallastE = 330;
            constexpr uint32_t diagonal3BallastE = 331;
            constexpr uint32_t diagonal0BallastS = 332;
            constexpr uint32_t diagonal2BallastS = 333;
            constexpr uint32_t diagonal1BallastS = 334;
            constexpr uint32_t diagonal3BallastS = 335;
            constexpr uint32_t diagonal0SleeperE = 336;
            constexpr uint32_t diagonal2SleeperE = 337;
            constexpr uint32_t diagonal1SleeperE = 338;
            constexpr uint32_t diagonal3SleeperE = 339;
            constexpr uint32_t diagonal0SleeperS = 340;
            constexpr uint32_t diagonal2SleeperS = 341;
            constexpr uint32_t diagonal1SleeperS = 342;
            constexpr uint32_t diagonal3SleeperS = 343;
            constexpr uint32_t diagonal0RailE = 344;
            constexpr uint32_t diagonal2RailE = 345;
            constexpr uint32_t diagonal1RailE = 346;
            constexpr uint32_t diagonal3RailE = 347;
            constexpr uint32_t diagonal0RailS = 348;
            constexpr uint32_t diagonal2RailS = 349;
            constexpr uint32_t diagonal1RailS = 350;
            constexpr uint32_t diagonal3RailS = 351;
        }
    }
}
