#pragma once

#include "Map/Track/TrackEnum.h"
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
        class DrawingContext;
    }
    enum class TownSize : uint8_t;

    enum class RoadObjectFlags : uint16_t
    {
        none = 0U,
        isOneWay = 1U << 0,
        unk_01 = 1U << 1,
        unk_02 = 1U << 2,
        unk_03 = 1U << 3, // Likely isTram
        unk_04 = 1U << 4,
        unk_05 = 1U << 5,
        isRoad = 1U << 6, // If not set this is tram track
        unk_07 = 1U << 7,
        unk_08 = 1U << 8,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(RoadObjectFlags);

#pragma pack(push, 1)
    struct RoadObject
    {
        static constexpr auto kObjectType = ObjectType::road;

        StringId name;
        World::Track::RoadTraitFlags roadPieces; // 0x02
        int16_t buildCostFactor;                 // 0x04
        int16_t sellCostFactor;                  // 0x06
        int16_t tunnelCostFactor;                // 0x08
        uint8_t costIndex;                       // 0x0A
        uint8_t tunnel;                          // 0x0B
        Speed16 maxSpeed;                        // 0x0C
        uint32_t image;                          // 0x0E
        RoadObjectFlags flags;                   // 0x12
        uint8_t numBridges;                      // 0x14
        uint8_t bridges[7];                      // 0x15
        uint8_t numStations;                     // 0x1C
        uint8_t stations[7];                     // 0x1D
        uint8_t paintStyle;                      // 0x24
        uint8_t numMods;                         // 0x25
        uint8_t mods[2];                         // 0x26
        uint8_t numCompatible;                   // 0x28
        uint8_t displayOffset;                   // 0x29
        uint16_t compatibleRoads;                // 0x2A
        uint16_t compatibleTracks;               // 0x2C
        TownSize targetTownSize;                 // 0x2E
        uint8_t pad_2F;

        void drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects* dependencies);
        void unload();

        constexpr bool hasFlags(RoadObjectFlags flagsToTest) const
        {
            return (flags & flagsToTest) != RoadObjectFlags::none;
        }

        constexpr bool hasTraitFlags(World::Track::RoadTraitFlags flagsToTest) const
        {
            return (roadPieces & flagsToTest) != World::Track::RoadTraitFlags::none;
        }
    };
#pragma pack(pop)

    static_assert(sizeof(RoadObject) == 0x30);

    namespace RoadObj::ImageIds
    {
        namespace Style0
        {
            constexpr uint32_t kStraight0NE = 34;
            constexpr uint32_t kStraight0SE = 35;
            constexpr uint32_t kRightCurveVerySmall0NE = 36;
            constexpr uint32_t kRightCurveVerySmall0SE = 37;
            constexpr uint32_t kRightCurveVerySmall0SW = 38;
            constexpr uint32_t kRightCurveVerySmall0NW = 39;
            constexpr uint32_t kJunctionLeft0NE = 40;
            constexpr uint32_t kJunctionLeft0SE = 41;
            constexpr uint32_t kJunctionLeft0SW = 42;
            constexpr uint32_t kJunctionLeft0NW = 43;
            constexpr uint32_t kJunctionCrossroads0NE = 44;
            constexpr uint32_t kRightCurveSmall0NE = 45;
            constexpr uint32_t kRightCurveSmall1NE = 46;
            constexpr uint32_t kRightCurveSmall2NE = 47;
            constexpr uint32_t kRightCurveSmall3NE = 48;
            constexpr uint32_t kRightCurveSmall0SE = 49;
            constexpr uint32_t kRightCurveSmall1SE = 50;
            constexpr uint32_t kRightCurveSmall2SE = 51;
            constexpr uint32_t kRightCurveSmall3SE = 52;
            constexpr uint32_t kRightCurveSmall0SW = 53;
            constexpr uint32_t kRightCurveSmall1SW = 54;
            constexpr uint32_t kRightCurveSmall2SW = 55;
            constexpr uint32_t kRightCurveSmall3SW = 56;
            constexpr uint32_t kRightCurveSmall0NW = 57;
            constexpr uint32_t kRightCurveSmall1NW = 58;
            constexpr uint32_t kRightCurveSmall2NW = 59;
            constexpr uint32_t kRightCurveSmall3NW = 60;
            constexpr uint32_t kStraightSlopeUp0NE = 61;
            constexpr uint32_t kStraightSlopeUp1NE = 62;
            constexpr uint32_t kStraightSlopeUp0SE = 63;
            constexpr uint32_t kStraightSlopeUp1SE = 64;
            constexpr uint32_t kStraightSlopeUp0SW = 65;
            constexpr uint32_t kStraightSlopeUp1SW = 66;
            constexpr uint32_t kStraightSlopeUp0NW = 67;
            constexpr uint32_t kStraightSlopeUp1NW = 68;
            constexpr uint32_t kStraightSteepSlopeUp0NE = 69;
            constexpr uint32_t kStraightSteepSlopeUp0SE = 70;
            constexpr uint32_t kStraightSteepSlopeUp0SW = 71;
            constexpr uint32_t kStraightSteepSlopeUp0NW = 72;
            constexpr uint32_t kTurnaround0NE = 73;
            constexpr uint32_t kTurnaround0SE = 74;
            constexpr uint32_t kTurnaround0SW = 75;
            constexpr uint32_t kTurnaround0NW = 76;
        }
        // Assumes rotational symmetry
        // k{TrackId}{sequenceIndex}{type}{direction}
        // type = Ballast, Sleeper, Rail
        namespace Style1
        {
            constexpr uint32_t kStraight0BallastNE = 34;
            constexpr uint32_t kStraight0BallastSE = 35;
            constexpr uint32_t kStraight0SleeperNE = 36;
            constexpr uint32_t kStraight0SleeperSE = 37;
            constexpr uint32_t kStraight0RailNE = 38;
            constexpr uint32_t kStraight0RailSE = 39;
            constexpr uint32_t kRightCurveSmall0BallastNE = 40;
            constexpr uint32_t kRightCurveSmall1BallastNE = 41;
            constexpr uint32_t kRightCurveSmall2BallastNE = 42;
            constexpr uint32_t kRightCurveSmall3BallastNE = 43;
            constexpr uint32_t kRightCurveSmall0BallastSE = 44;
            constexpr uint32_t kRightCurveSmall1BallastSE = 45;
            constexpr uint32_t kRightCurveSmall2BallastSE = 46;
            constexpr uint32_t kRightCurveSmall3BallastSE = 47;
            constexpr uint32_t kRightCurveSmall0BallastSW = 48;
            constexpr uint32_t kRightCurveSmall1BallastSW = 49;
            constexpr uint32_t kRightCurveSmall2BallastSW = 50;
            constexpr uint32_t kRightCurveSmall3BallastSW = 51;
            constexpr uint32_t kRightCurveSmall0BallastNW = 52;
            constexpr uint32_t kRightCurveSmall1BallastNW = 53;
            constexpr uint32_t kRightCurveSmall2BallastNW = 54;
            constexpr uint32_t kRightCurveSmall3BallastNW = 55;
            constexpr uint32_t kRightCurveSmall0SleeperNE = 56;
            constexpr uint32_t kRightCurveSmall1SleeperNE = 57;
            constexpr uint32_t kRightCurveSmall2SleeperNE = 58;
            constexpr uint32_t kRightCurveSmall3SleeperNE = 59;
            constexpr uint32_t kRightCurveSmall0SleeperSE = 60;
            constexpr uint32_t kRightCurveSmall1SleeperSE = 61;
            constexpr uint32_t kRightCurveSmall2SleeperSE = 62;
            constexpr uint32_t kRightCurveSmall3SleeperSE = 63;
            constexpr uint32_t kRightCurveSmall0SleeperSW = 64;
            constexpr uint32_t kRightCurveSmall1SleeperSW = 65;
            constexpr uint32_t kRightCurveSmall2SleeperSW = 66;
            constexpr uint32_t kRightCurveSmall3SleeperSW = 67;
            constexpr uint32_t kRightCurveSmall0SleeperNW = 68;
            constexpr uint32_t kRightCurveSmall1SleeperNW = 69;
            constexpr uint32_t kRightCurveSmall2SleeperNW = 70;
            constexpr uint32_t kRightCurveSmall3SleeperNW = 71;
            constexpr uint32_t kRightCurveSmall0RailNE = 72;
            constexpr uint32_t kRightCurveSmall1RailNE = 73;
            constexpr uint32_t kRightCurveSmall2RailNE = 74;
            constexpr uint32_t kRightCurveSmall3RailNE = 75;
            constexpr uint32_t kRightCurveSmall0RailSE = 76;
            constexpr uint32_t kRightCurveSmall1RailSE = 77;
            constexpr uint32_t kRightCurveSmall2RailSE = 78;
            constexpr uint32_t kRightCurveSmall3RailSE = 79;
            constexpr uint32_t kRightCurveSmall0RailSW = 80;
            constexpr uint32_t kRightCurveSmall1RailSW = 81;
            constexpr uint32_t kRightCurveSmall2RailSW = 82;
            constexpr uint32_t kRightCurveSmall3RailSW = 83;
            constexpr uint32_t kRightCurveSmall0RailNW = 84;
            constexpr uint32_t kRightCurveSmall1RailNW = 85;
            constexpr uint32_t kRightCurveSmall2RailNW = 86;
            constexpr uint32_t kRightCurveSmall3RailNW = 87;
            constexpr uint32_t kStraightSlopeUp0BallastNE = 88;
            constexpr uint32_t kStraightSlopeUp1BallastNE = 89;
            constexpr uint32_t kStraightSlopeUp0BallastSE = 90;
            constexpr uint32_t kStraightSlopeUp1BallastSE = 91;
            constexpr uint32_t kStraightSlopeUp0BallastSW = 92;
            constexpr uint32_t kStraightSlopeUp1BallastSW = 93;
            constexpr uint32_t kStraightSlopeUp0BallastNW = 94;
            constexpr uint32_t kStraightSlopeUp1BallastNW = 95;
            constexpr uint32_t kStraightSlopeUp0SleeperNE = 96;
            constexpr uint32_t kStraightSlopeUp1SleeperNE = 97;
            constexpr uint32_t kStraightSlopeUp0SleeperSE = 98;
            constexpr uint32_t kStraightSlopeUp1SleeperSE = 99;
            constexpr uint32_t kStraightSlopeUp0SleeperSW = 100;
            constexpr uint32_t kStraightSlopeUp1SleeperSW = 101;
            constexpr uint32_t kStraightSlopeUp0SleeperNW = 102;
            constexpr uint32_t kStraightSlopeUp1SleeperNW = 103;
            constexpr uint32_t kStraightSlopeUp0RailNE = 104;
            constexpr uint32_t kStraightSlopeUp1RailNE = 105;
            constexpr uint32_t kStraightSlopeUp0RailSE = 106;
            constexpr uint32_t kStraightSlopeUp1RailSE = 107;
            constexpr uint32_t kStraightSlopeUp0RailSW = 108;
            constexpr uint32_t kStraightSlopeUp1RailSW = 109;
            constexpr uint32_t kStraightSlopeUp0RailNW = 110;
            constexpr uint32_t kStraightSlopeUp1RailNW = 111;
            constexpr uint32_t kStraightSteepSlopeUp0BallastNE = 112;
            constexpr uint32_t kStraightSteepSlopeUp0BallastSE = 113;
            constexpr uint32_t kStraightSteepSlopeUp0BallastSW = 114;
            constexpr uint32_t kStraightSteepSlopeUp0BallastNW = 115;
            constexpr uint32_t kStraightSteepSlopeUp0SleeperNE = 116;
            constexpr uint32_t kStraightSteepSlopeUp0SleeperSE = 117;
            constexpr uint32_t kStraightSteepSlopeUp0SleeperSW = 118;
            constexpr uint32_t kStraightSteepSlopeUp0SleeperNW = 119;
            constexpr uint32_t kStraightSteepSlopeUp0RailNE = 120;
            constexpr uint32_t kStraightSteepSlopeUp0RailSE = 121;
            constexpr uint32_t kStraightSteepSlopeUp0RailSW = 122;
            constexpr uint32_t kStraightSteepSlopeUp0RailNW = 123;
            constexpr uint32_t kRightCurveVerySmall0BallastNE = 124;
            constexpr uint32_t kRightCurveVerySmall0BallastSE = 125;
            constexpr uint32_t kRightCurveVerySmall0BallastSW = 126;
            constexpr uint32_t kRightCurveVerySmall0BallastNW = 127;
            constexpr uint32_t kRightCurveVerySmall0SleeperNE = 128;
            constexpr uint32_t kRightCurveVerySmall0SleeperSE = 129;
            constexpr uint32_t kRightCurveVerySmall0SleeperSW = 130;
            constexpr uint32_t kRightCurveVerySmall0SleeperNW = 131;
            constexpr uint32_t kRightCurveVerySmall0RailNE = 132;
            constexpr uint32_t kRightCurveVerySmall0RailSE = 133;
            constexpr uint32_t kRightCurveVerySmall0RailSW = 134;
            constexpr uint32_t kRightCurveVerySmall0RailNW = 135;
            constexpr uint32_t kTurnaround0BallastNE = 136;
            constexpr uint32_t kTurnaround0BallastSE = 137;
            constexpr uint32_t kTurnaround0BallastSW = 138;
            constexpr uint32_t kTurnaround0BallastNW = 139;
            constexpr uint32_t kTurnaround0SleeperNE = 140;
            constexpr uint32_t kTurnaround0SleeperSE = 141;
            constexpr uint32_t kTurnaround0SleeperSW = 142;
            constexpr uint32_t kTurnaround0SleeperNW = 143;
            constexpr uint32_t kTurnaround0RailNE = 144;
            constexpr uint32_t kTurnaround0RailSE = 145;
            constexpr uint32_t kTurnaround0RailSW = 146;
            constexpr uint32_t kTurnaround0RailNW = 147;
        }
        namespace Style2
        {
            constexpr uint32_t kStraight0NE = 34;
            constexpr uint32_t kStraight0SE = 35;
            constexpr uint32_t kLeftCurveVerySmall0NW = 36;
            constexpr uint32_t kLeftCurveVerySmall0NE = 37;
            constexpr uint32_t kLeftCurveVerySmall0SE = 38;
            constexpr uint32_t kLeftCurveVerySmall0SW = 39;
            constexpr uint32_t kJunctionLeft0NE = 40;
            constexpr uint32_t kJunctionLeft0SE = 41;
            constexpr uint32_t kJunctionLeft0SW = 42;
            constexpr uint32_t kJunctionLeft0NW = 43;
            constexpr uint32_t kJunctionCrossroads0NE = 44;
            constexpr uint32_t kLeftCurveSmall3NW = 45;
            constexpr uint32_t kLeftCurveSmall1NW = 46;
            constexpr uint32_t kLeftCurveSmall2NW = 47;
            constexpr uint32_t kLeftCurveSmall0NW = 48;
            constexpr uint32_t kLeftCurveSmall3NE = 49;
            constexpr uint32_t kLeftCurveSmall1NE = 50;
            constexpr uint32_t kLeftCurveSmall2NE = 51;
            constexpr uint32_t kLeftCurveSmall0NE = 52;
            constexpr uint32_t kLeftCurveSmall3SE = 53;
            constexpr uint32_t kLeftCurveSmall1SE = 54;
            constexpr uint32_t kLeftCurveSmall2SE = 55;
            constexpr uint32_t kLeftCurveSmall0SE = 56;
            constexpr uint32_t kLeftCurveSmall3SW = 57;
            constexpr uint32_t kLeftCurveSmall1SW = 58;
            constexpr uint32_t kLeftCurveSmall2SW = 59;
            constexpr uint32_t kLeftCurveSmall0SW = 60;
            constexpr uint32_t kStraightSlopeUp0NE = 61;
            constexpr uint32_t kStraightSlopeUp1NE = 62;
            constexpr uint32_t kStraightSlopeUp0SE = 63;
            constexpr uint32_t kStraightSlopeUp1SE = 64;
            constexpr uint32_t kStraightSlopeUp0SW = 65;
            constexpr uint32_t kStraightSlopeUp1SW = 66;
            constexpr uint32_t kStraightSlopeUp0NW = 67;
            constexpr uint32_t kStraightSlopeUp1NW = 68;
            constexpr uint32_t kStraightSteepSlopeUp0NE = 69;
            constexpr uint32_t kStraightSteepSlopeUp0SE = 70;
            constexpr uint32_t kStraightSteepSlopeUp0SW = 71;
            constexpr uint32_t kStraightSteepSlopeUp0NW = 72;
            constexpr uint32_t kTurnaround0NE = 73;
            constexpr uint32_t kTurnaround0SE = 74;
            constexpr uint32_t kTurnaround0SW = 75;
            constexpr uint32_t kTurnaround0NW = 76;

            constexpr uint32_t kStraight0SW = 85;
            constexpr uint32_t kStraight0NW = 86;
            constexpr uint32_t kRightCurveVerySmall0NE = 87;
            constexpr uint32_t kRightCurveVerySmall0SE = 88;
            constexpr uint32_t kRightCurveVerySmall0SW = 89;
            constexpr uint32_t kRightCurveVerySmall0NW = 90;
            constexpr uint32_t kJunctionRight0NE = 91;
            constexpr uint32_t kJunctionRight0SE = 92;
            constexpr uint32_t kJunctionRight0SW = 93;
            constexpr uint32_t kJunctionRight0NW = 94;
            // Must duplicate kJunctionCrossroads0NE
            constexpr uint32_t kJunctionCrossroads0NE2 = 95;
            constexpr uint32_t kRightCurveSmall0NE = 96;
            constexpr uint32_t kRightCurveSmall1NE = 97;
            constexpr uint32_t kRightCurveSmall2NE = 98;
            constexpr uint32_t kRightCurveSmall3NE = 99;
            constexpr uint32_t kRightCurveSmall0SE = 100;
            constexpr uint32_t kRightCurveSmall1SE = 101;
            constexpr uint32_t kRightCurveSmall2SE = 102;
            constexpr uint32_t kRightCurveSmall3SE = 103;
            constexpr uint32_t kRightCurveSmall0SW = 104;
            constexpr uint32_t kRightCurveSmall1SW = 105;
            constexpr uint32_t kRightCurveSmall2SW = 106;
            constexpr uint32_t kRightCurveSmall3SW = 107;
            constexpr uint32_t kRightCurveSmall0NW = 108;
            constexpr uint32_t kRightCurveSmall1NW = 109;
            constexpr uint32_t kRightCurveSmall2NW = 110;
            constexpr uint32_t kRightCurveSmall3NW = 111;
            constexpr uint32_t kStraightSlopeDown1SW = 112;
            constexpr uint32_t kStraightSlopeDown0SW = 113;
            constexpr uint32_t kStraightSlopeDown1NW = 114;
            constexpr uint32_t kStraightSlopeDown0NW = 115;
            constexpr uint32_t kStraightSlopeDown1NE = 116;
            constexpr uint32_t kStraightSlopeDown0NE = 117;
            constexpr uint32_t kStraightSlopeDown1SE = 118;
            constexpr uint32_t kStraightSlopeDown0SE = 119;
            constexpr uint32_t kStraightSteepSlopeDown0SW = 120;
            constexpr uint32_t kStraightSteepSlopeDown0NW = 121;
            constexpr uint32_t kStraightSteepSlopeDown0NE = 122;
            constexpr uint32_t kStraightSteepSlopeDown0SE = 123;
        }
    }
}
