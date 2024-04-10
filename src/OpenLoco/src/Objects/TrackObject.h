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

#pragma pack(push, 1)
    struct TrackObject
    {
        static constexpr auto kObjectType = ObjectType::track;

        StringId name;
        World::Track::TrackTraitFlags trackPieces;        // 0x02
        World::Track::TrackTraitFlags stationTrackPieces; // 0x04
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

        constexpr bool hasTraitFlags(World::Track::TrackTraitFlags flagsToTest) const
        {
            return (trackPieces & flagsToTest) != World::Track::TrackTraitFlags::none;
        }
    };
#pragma pack(pop)

    static_assert(sizeof(TrackObject) == 0x36);

    namespace TrackObj::ImageIds
    {
        constexpr uint32_t kUiPreviewImage0 = 0;
        constexpr uint32_t kUiPreviewImage1 = 1;
        constexpr uint32_t kUiPreviewImage2 = 2;
        constexpr uint32_t kUiPreviewImage3 = 3;
        constexpr uint32_t kUiPreviewImage4 = 4;
        constexpr uint32_t kUiPreviewImage5 = 5;
        constexpr uint32_t kUiPreviewImage6 = 6;
        constexpr uint32_t kUiPreviewImage7 = 7;
        constexpr uint32_t kUiPreviewImage8 = 8;
        constexpr uint32_t kUiPreviewImage9 = 9;
        constexpr uint32_t kUiPreviewImage10 = 10;
        constexpr uint32_t kUiPreviewImage11 = 11;
        constexpr uint32_t kUiPreviewImage12 = 12;
        constexpr uint32_t kUiPreviewImage13 = 13;
        constexpr uint32_t kUiPreviewImage14 = 14;
        constexpr uint32_t kUiPreviewImage15 = 15;
        constexpr uint32_t kUiPickupFromTrack = 16;
        constexpr uint32_t kUiPlaceOnTrack = 17;

        // Assumes rotational symmetry
        // k{TrackId}{sequenceIndex}[type]{direction}
        // type = Ballast, Sleeper, Rail
        // type only used for mergeable track ids
        namespace Style0
        {
            constexpr uint32_t kStraight0BallastNE = 18;
            constexpr uint32_t kStraight0BallastSE = 19;
            constexpr uint32_t kStraight0SleeperNE = 20;
            constexpr uint32_t kStraight0SleeperSE = 21;
            constexpr uint32_t kStraight0RailNE = 22;
            constexpr uint32_t kStraight0RailSE = 23;
            constexpr uint32_t kRightCurveSmall0BallastNE = 24;
            constexpr uint32_t kRightCurveSmall1BallastNE = 25;
            constexpr uint32_t kRightCurveSmall2BallastNE = 26;
            constexpr uint32_t kRightCurveSmall3BallastNE = 27;
            constexpr uint32_t kRightCurveSmall0BallastSE = 28;
            constexpr uint32_t kRightCurveSmall1BallastSE = 29;
            constexpr uint32_t kRightCurveSmall2BallastSE = 30;
            constexpr uint32_t kRightCurveSmall3BallastSE = 31;
            constexpr uint32_t kRightCurveSmall0BallastSW = 32;
            constexpr uint32_t kRightCurveSmall1BallastSW = 33;
            constexpr uint32_t kRightCurveSmall2BallastSW = 34;
            constexpr uint32_t kRightCurveSmall3BallastSW = 35;
            constexpr uint32_t kRightCurveSmall0BallastNW = 36;
            constexpr uint32_t kRightCurveSmall1BallastNW = 37;
            constexpr uint32_t kRightCurveSmall2BallastNW = 38;
            constexpr uint32_t kRightCurveSmall3BallastNW = 39;
            constexpr uint32_t kRightCurveSmall0SleeperNE = 40;
            constexpr uint32_t kRightCurveSmall1SleeperNE = 41;
            constexpr uint32_t kRightCurveSmall2SleeperNE = 42;
            constexpr uint32_t kRightCurveSmall3SleeperNE = 43;
            constexpr uint32_t kRightCurveSmall0SleeperSE = 44;
            constexpr uint32_t kRightCurveSmall1SleeperSE = 45;
            constexpr uint32_t kRightCurveSmall2SleeperSE = 46;
            constexpr uint32_t kRightCurveSmall3SleeperSE = 47;
            constexpr uint32_t kRightCurveSmall0SleeperSW = 48;
            constexpr uint32_t kRightCurveSmall1SleeperSW = 49;
            constexpr uint32_t kRightCurveSmall2SleeperSW = 50;
            constexpr uint32_t kRightCurveSmall3SleeperSW = 51;
            constexpr uint32_t kRightCurveSmall0SleeperNW = 52;
            constexpr uint32_t kRightCurveSmall1SleeperNW = 53;
            constexpr uint32_t kRightCurveSmall2SleeperNW = 54;
            constexpr uint32_t kRightCurveSmall3SleeperNW = 55;
            constexpr uint32_t kRightCurveSmall0RailNE = 56;
            constexpr uint32_t kRightCurveSmall1RailNE = 57;
            constexpr uint32_t kRightCurveSmall2RailNE = 58;
            constexpr uint32_t kRightCurveSmall3RailNE = 59;
            constexpr uint32_t kRightCurveSmall0RailSE = 60;
            constexpr uint32_t kRightCurveSmall1RailSE = 61;
            constexpr uint32_t kRightCurveSmall2RailSE = 62;
            constexpr uint32_t kRightCurveSmall3RailSE = 63;
            constexpr uint32_t kRightCurveSmall0RailSW = 64;
            constexpr uint32_t kRightCurveSmall1RailSW = 65;
            constexpr uint32_t kRightCurveSmall2RailSW = 66;
            constexpr uint32_t kRightCurveSmall3RailSW = 67;
            constexpr uint32_t kRightCurveSmall0RailNW = 68;
            constexpr uint32_t kRightCurveSmall1RailNW = 69;
            constexpr uint32_t kRightCurveSmall2RailNW = 70;
            constexpr uint32_t kRightCurveSmall3RailNW = 71;
            constexpr uint32_t kRightCurveSmallSlopeUp0NE = 72;
            constexpr uint32_t kRightCurveSmallSlopeUp1NE = 73;
            constexpr uint32_t kRightCurveSmallSlopeUp2NE = 74;
            constexpr uint32_t kRightCurveSmallSlopeUp3NE = 75;
            constexpr uint32_t kRightCurveSmallSlopeUp0SE = 76;
            constexpr uint32_t kRightCurveSmallSlopeUp1SE = 77;
            constexpr uint32_t kRightCurveSmallSlopeUp2SE = 78;
            constexpr uint32_t kRightCurveSmallSlopeUp3SE = 79;
            constexpr uint32_t kRightCurveSmallSlopeUp0SW = 80;
            constexpr uint32_t kRightCurveSmallSlopeUp1SW = 81;
            constexpr uint32_t kRightCurveSmallSlopeUp2SW = 82;
            constexpr uint32_t kRightCurveSmallSlopeUp3SW = 83;
            constexpr uint32_t kRightCurveSmallSlopeUp0NW = 84;
            constexpr uint32_t kRightCurveSmallSlopeUp1NW = 85;
            constexpr uint32_t kRightCurveSmallSlopeUp2NW = 86;
            constexpr uint32_t kRightCurveSmallSlopeUp3NW = 87;
            constexpr uint32_t kRightCurveSmallSlopeDown0NE = 88;
            constexpr uint32_t kRightCurveSmallSlopeDown1NE = 89;
            constexpr uint32_t kRightCurveSmallSlopeDown2NE = 90;
            constexpr uint32_t kRightCurveSmallSlopeDown3NE = 91;
            constexpr uint32_t kRightCurveSmallSlopeDown0SE = 92;
            constexpr uint32_t kRightCurveSmallSlopeDown1SE = 93;
            constexpr uint32_t kRightCurveSmallSlopeDown2SE = 94;
            constexpr uint32_t kRightCurveSmallSlopeDown3SE = 95;
            constexpr uint32_t kRightCurveSmallSlopeDown0SW = 96;
            constexpr uint32_t kRightCurveSmallSlopeDown1SW = 97;
            constexpr uint32_t kRightCurveSmallSlopeDown2SW = 98;
            constexpr uint32_t kRightCurveSmallSlopeDown3SW = 99;
            constexpr uint32_t kRightCurveSmallSlopeDown0NW = 100;
            constexpr uint32_t kRightCurveSmallSlopeDown1NW = 101;
            constexpr uint32_t kRightCurveSmallSlopeDown2NW = 102;
            constexpr uint32_t kRightCurveSmallSlopeDown3NW = 103;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp0NE = 104;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp1NE = 105;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp2NE = 106;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp3NE = 107;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp0SE = 108;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp1SE = 109;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp2SE = 110;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp3SE = 111;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp0SW = 112;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp1SW = 113;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp2SW = 114;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp3SW = 115;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp0NW = 116;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp1NW = 117;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp2NW = 118;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp3NW = 119;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown0NE = 120;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown1NE = 121;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown2NE = 122;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown3NE = 123;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown0SE = 124;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown1SE = 125;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown2SE = 126;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown3SE = 127;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown0SW = 128;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown1SW = 129;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown2SW = 130;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown3SW = 131;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown0NW = 132;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown1NW = 133;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown2NW = 134;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown3NW = 135;
            constexpr uint32_t kRightCurve0BallastNE = 136;
            constexpr uint32_t kRightCurve1BallastNE = 137;
            constexpr uint32_t kRightCurve2BallastNE = 138;
            constexpr uint32_t kRightCurve3BallastNE = 139;
            constexpr uint32_t kRightCurve4BallastNE = 140;
            constexpr uint32_t kRightCurve0BallastSE = 141;
            constexpr uint32_t kRightCurve1BallastSE = 142;
            constexpr uint32_t kRightCurve2BallastSE = 143;
            constexpr uint32_t kRightCurve3BallastSE = 144;
            constexpr uint32_t kRightCurve4BallastSE = 145;
            constexpr uint32_t kRightCurve0BallastSW = 146;
            constexpr uint32_t kRightCurve1BallastSW = 147;
            constexpr uint32_t kRightCurve2BallastSW = 148;
            constexpr uint32_t kRightCurve3BallastSW = 149;
            constexpr uint32_t kRightCurve4BallastSW = 150;
            constexpr uint32_t kRightCurve0BallastNW = 151;
            constexpr uint32_t kRightCurve1BallastNW = 152;
            constexpr uint32_t kRightCurve2BallastNW = 153;
            constexpr uint32_t kRightCurve3BallastNW = 154;
            constexpr uint32_t kRightCurve4BallastNW = 155;
            constexpr uint32_t kRightCurve0SleeperNE = 156;
            constexpr uint32_t kRightCurve1SleeperNE = 157;
            constexpr uint32_t kRightCurve2SleeperNE = 158;
            constexpr uint32_t kRightCurve3SleeperNE = 159;
            constexpr uint32_t kRightCurve4SleeperNE = 160;
            constexpr uint32_t kRightCurve0SleeperSE = 161;
            constexpr uint32_t kRightCurve1SleeperSE = 162;
            constexpr uint32_t kRightCurve2SleeperSE = 163;
            constexpr uint32_t kRightCurve3SleeperSE = 164;
            constexpr uint32_t kRightCurve4SleeperSE = 165;
            constexpr uint32_t kRightCurve0SleeperSW = 166;
            constexpr uint32_t kRightCurve1SleeperSW = 167;
            constexpr uint32_t kRightCurve2SleeperSW = 168;
            constexpr uint32_t kRightCurve3SleeperSW = 169;
            constexpr uint32_t kRightCurve4SleeperSW = 170;
            constexpr uint32_t kRightCurve0SleeperNW = 171;
            constexpr uint32_t kRightCurve1SleeperNW = 172;
            constexpr uint32_t kRightCurve2SleeperNW = 173;
            constexpr uint32_t kRightCurve3SleeperNW = 174;
            constexpr uint32_t kRightCurve4SleeperNW = 175;
            constexpr uint32_t kRightCurve0RailNE = 176;
            constexpr uint32_t kRightCurve1RailNE = 177;
            constexpr uint32_t kRightCurve2RailNE = 178;
            constexpr uint32_t kRightCurve3RailNE = 179;
            constexpr uint32_t kRightCurve4RailNE = 180;
            constexpr uint32_t kRightCurve0RailSE = 181;
            constexpr uint32_t kRightCurve1RailSE = 182;
            constexpr uint32_t kRightCurve2RailSE = 183;
            constexpr uint32_t kRightCurve3RailSE = 184;
            constexpr uint32_t kRightCurve4RailSE = 185;
            constexpr uint32_t kRightCurve0RailSW = 186;
            constexpr uint32_t kRightCurve1RailSW = 187;
            constexpr uint32_t kRightCurve2RailSW = 188;
            constexpr uint32_t kRightCurve3RailSW = 189;
            constexpr uint32_t kRightCurve4RailSW = 190;
            constexpr uint32_t kRightCurve0RailNW = 191;
            constexpr uint32_t kRightCurve1RailNW = 192;
            constexpr uint32_t kRightCurve2RailNW = 193;
            constexpr uint32_t kRightCurve3RailNW = 194;
            constexpr uint32_t kRightCurve4RailNW = 195;
            constexpr uint32_t kStraightSlopeUp0NE = 196;
            constexpr uint32_t kStraightSlopeUp1NE = 197;
            constexpr uint32_t kStraightSlopeUp0SE = 198;
            constexpr uint32_t kStraightSlopeUp1SE = 199;
            constexpr uint32_t kStraightSlopeUp0SW = 200;
            constexpr uint32_t kStraightSlopeUp1SW = 201;
            constexpr uint32_t kStraightSlopeUp0NW = 202;
            constexpr uint32_t kStraightSlopeUp1NW = 203;
            constexpr uint32_t kStraightSteepSlopeUp0NE = 204;
            constexpr uint32_t kStraightSteepSlopeUp0SE = 205;
            constexpr uint32_t kStraightSteepSlopeUp0SW = 206;
            constexpr uint32_t kStraightSteepSlopeUp0NW = 207;
            constexpr uint32_t kRightCurveLarge0BallastNE = 208;
            constexpr uint32_t kRightCurveLarge1BallastNE = 209;
            constexpr uint32_t kRightCurveLarge2BallastNE = 210;
            constexpr uint32_t kRightCurveLarge3BallastNE = 211;
            constexpr uint32_t kRightCurveLarge4BallastNE = 212;
            constexpr uint32_t kRightCurveLarge0BallastSE = 213;
            constexpr uint32_t kRightCurveLarge1BallastSE = 214;
            constexpr uint32_t kRightCurveLarge2BallastSE = 215;
            constexpr uint32_t kRightCurveLarge3BallastSE = 216;
            constexpr uint32_t kRightCurveLarge4BallastSE = 217;
            constexpr uint32_t kRightCurveLarge0BallastSW = 218;
            constexpr uint32_t kRightCurveLarge1BallastSW = 219;
            constexpr uint32_t kRightCurveLarge2BallastSW = 220;
            constexpr uint32_t kRightCurveLarge3BallastSW = 221;
            constexpr uint32_t kRightCurveLarge4BallastSW = 222;
            constexpr uint32_t kRightCurveLarge0BallastNW = 223;
            constexpr uint32_t kRightCurveLarge1BallastNW = 224;
            constexpr uint32_t kRightCurveLarge2BallastNW = 225;
            constexpr uint32_t kRightCurveLarge3BallastNW = 226;
            constexpr uint32_t kRightCurveLarge4BallastNW = 227;
            constexpr uint32_t kLeftCurveLarge0BallastNE = 228;
            constexpr uint32_t kLeftCurveLarge1BallastNE = 229;
            constexpr uint32_t kLeftCurveLarge2BallastNE = 230;
            constexpr uint32_t kLeftCurveLarge3BallastNE = 231;
            constexpr uint32_t kLeftCurveLarge4BallastNE = 232;
            constexpr uint32_t kLeftCurveLarge0BallastSE = 233;
            constexpr uint32_t kLeftCurveLarge1BallastSE = 234;
            constexpr uint32_t kLeftCurveLarge2BallastSE = 235;
            constexpr uint32_t kLeftCurveLarge3BallastSE = 236;
            constexpr uint32_t kLeftCurveLarge4BallastSE = 237;
            constexpr uint32_t kLeftCurveLarge0BallastSW = 238;
            constexpr uint32_t kLeftCurveLarge1BallastSW = 239;
            constexpr uint32_t kLeftCurveLarge2BallastSW = 240;
            constexpr uint32_t kLeftCurveLarge3BallastSW = 241;
            constexpr uint32_t kLeftCurveLarge4BallastSW = 242;
            constexpr uint32_t kLeftCurveLarge0BallastNW = 243;
            constexpr uint32_t kLeftCurveLarge1BallastNW = 244;
            constexpr uint32_t kLeftCurveLarge2BallastNW = 245;
            constexpr uint32_t kLeftCurveLarge3BallastNW = 246;
            constexpr uint32_t kLeftCurveLarge4BallastNW = 247;
            constexpr uint32_t kRightCurveLarge0SleeperNE = 248;
            constexpr uint32_t kRightCurveLarge1SleeperNE = 249;
            constexpr uint32_t kRightCurveLarge2SleeperNE = 250;
            constexpr uint32_t kRightCurveLarge3SleeperNE = 251;
            constexpr uint32_t kRightCurveLarge4SleeperNE = 252;
            constexpr uint32_t kRightCurveLarge0SleeperSE = 253;
            constexpr uint32_t kRightCurveLarge1SleeperSE = 254;
            constexpr uint32_t kRightCurveLarge2SleeperSE = 255;
            constexpr uint32_t kRightCurveLarge3SleeperSE = 256;
            constexpr uint32_t kRightCurveLarge4SleeperSE = 257;
            constexpr uint32_t kRightCurveLarge0SleeperSW = 258;
            constexpr uint32_t kRightCurveLarge1SleeperSW = 259;
            constexpr uint32_t kRightCurveLarge2SleeperSW = 260;
            constexpr uint32_t kRightCurveLarge3SleeperSW = 261;
            constexpr uint32_t kRightCurveLarge4SleeperSW = 262;
            constexpr uint32_t kRightCurveLarge0SleeperNW = 263;
            constexpr uint32_t kRightCurveLarge1SleeperNW = 264;
            constexpr uint32_t kRightCurveLarge2SleeperNW = 265;
            constexpr uint32_t kRightCurveLarge3SleeperNW = 266;
            constexpr uint32_t kRightCurveLarge4SleeperNW = 267;
            constexpr uint32_t kLeftCurveLarge0SleeperNE = 268;
            constexpr uint32_t kLeftCurveLarge1SleeperNE = 269;
            constexpr uint32_t kLeftCurveLarge2SleeperNE = 270;
            constexpr uint32_t kLeftCurveLarge3SleeperNE = 271;
            constexpr uint32_t kLeftCurveLarge4SleeperNE = 272;
            constexpr uint32_t kLeftCurveLarge0SleeperSE = 273;
            constexpr uint32_t kLeftCurveLarge1SleeperSE = 274;
            constexpr uint32_t kLeftCurveLarge2SleeperSE = 275;
            constexpr uint32_t kLeftCurveLarge3SleeperSE = 276;
            constexpr uint32_t kLeftCurveLarge4SleeperSE = 277;
            constexpr uint32_t kLeftCurveLarge0SleeperSW = 278;
            constexpr uint32_t kLeftCurveLarge1SleeperSW = 279;
            constexpr uint32_t kLeftCurveLarge2SleeperSW = 280;
            constexpr uint32_t kLeftCurveLarge3SleeperSW = 281;
            constexpr uint32_t kLeftCurveLarge4SleeperSW = 282;
            constexpr uint32_t kLeftCurveLarge0SleeperNW = 283;
            constexpr uint32_t kLeftCurveLarge1SleeperNW = 284;
            constexpr uint32_t kLeftCurveLarge2SleeperNW = 285;
            constexpr uint32_t kLeftCurveLarge3SleeperNW = 286;
            constexpr uint32_t kLeftCurveLarge4SleeperNW = 287;
            constexpr uint32_t kRightCurveLarge0RailNE = 288;
            constexpr uint32_t kRightCurveLarge1RailNE = 289;
            constexpr uint32_t kRightCurveLarge2RailNE = 290;
            constexpr uint32_t kRightCurveLarge3RailNE = 291;
            constexpr uint32_t kRightCurveLarge4RailNE = 292;
            constexpr uint32_t kRightCurveLarge0RailSE = 293;
            constexpr uint32_t kRightCurveLarge1RailSE = 294;
            constexpr uint32_t kRightCurveLarge2RailSE = 295;
            constexpr uint32_t kRightCurveLarge3RailSE = 296;
            constexpr uint32_t kRightCurveLarge4RailSE = 297;
            constexpr uint32_t kRightCurveLarge0RailSW = 298;
            constexpr uint32_t kRightCurveLarge1RailSW = 299;
            constexpr uint32_t kRightCurveLarge2RailSW = 300;
            constexpr uint32_t kRightCurveLarge3RailSW = 301;
            constexpr uint32_t kRightCurveLarge4RailSW = 302;
            constexpr uint32_t kRightCurveLarge0RailNW = 303;
            constexpr uint32_t kRightCurveLarge1RailNW = 304;
            constexpr uint32_t kRightCurveLarge2RailNW = 305;
            constexpr uint32_t kRightCurveLarge3RailNW = 306;
            constexpr uint32_t kRightCurveLarge4RailNW = 307;
            constexpr uint32_t kLeftCurveLarge0RailNE = 308;
            constexpr uint32_t kLeftCurveLarge1RailNE = 309;
            constexpr uint32_t kLeftCurveLarge2RailNE = 310;
            constexpr uint32_t kLeftCurveLarge3RailNE = 311;
            constexpr uint32_t kLeftCurveLarge4RailNE = 312;
            constexpr uint32_t kLeftCurveLarge0RailSE = 313;
            constexpr uint32_t kLeftCurveLarge1RailSE = 314;
            constexpr uint32_t kLeftCurveLarge2RailSE = 315;
            constexpr uint32_t kLeftCurveLarge3RailSE = 316;
            constexpr uint32_t kLeftCurveLarge4RailSE = 317;
            constexpr uint32_t kLeftCurveLarge0RailSW = 318;
            constexpr uint32_t kLeftCurveLarge1RailSW = 319;
            constexpr uint32_t kLeftCurveLarge2RailSW = 320;
            constexpr uint32_t kLeftCurveLarge3RailSW = 321;
            constexpr uint32_t kLeftCurveLarge4RailSW = 322;
            constexpr uint32_t kLeftCurveLarge0RailNW = 323;
            constexpr uint32_t kLeftCurveLarge1RailNW = 324;
            constexpr uint32_t kLeftCurveLarge2RailNW = 325;
            constexpr uint32_t kLeftCurveLarge3RailNW = 326;
            constexpr uint32_t kLeftCurveLarge4RailNW = 327;
            constexpr uint32_t kDiagonal0BallastE = 328;
            constexpr uint32_t kDiagonal2BallastE = 329;
            constexpr uint32_t kDiagonal1BallastE = 330;
            constexpr uint32_t kDiagonal3BallastE = 331;
            constexpr uint32_t kDiagonal0BallastS = 332;
            constexpr uint32_t kDiagonal2BallastS = 333;
            constexpr uint32_t kDiagonal1BallastS = 334;
            constexpr uint32_t kDiagonal3BallastS = 335;
            constexpr uint32_t kDiagonal0SleeperE = 336;
            constexpr uint32_t kDiagonal2SleeperE = 337;
            constexpr uint32_t kDiagonal1SleeperE = 338;
            constexpr uint32_t kDiagonal3SleeperE = 339;
            constexpr uint32_t kDiagonal0SleeperS = 340;
            constexpr uint32_t kDiagonal2SleeperS = 341;
            constexpr uint32_t kDiagonal1SleeperS = 342;
            constexpr uint32_t kDiagonal3SleeperS = 343;
            constexpr uint32_t kDiagonal0RailE = 344;
            constexpr uint32_t kDiagonal2RailE = 345;
            constexpr uint32_t kDiagonal1RailE = 346;
            constexpr uint32_t kDiagonal3RailE = 347;
            constexpr uint32_t kDiagonal0RailS = 348;
            constexpr uint32_t kDiagonal2RailS = 349;
            constexpr uint32_t kDiagonal1RailS = 350;
            constexpr uint32_t kDiagonal3RailS = 351;
            constexpr uint32_t kSBendLeft0BallastNE = 352;
            constexpr uint32_t kSBendLeft1BallastNE = 353;
            constexpr uint32_t kSBendLeft2BallastNE = 354;
            constexpr uint32_t kSBendLeft3BallastNE = 355;
            constexpr uint32_t kSBendLeft0BallastSE = 356;
            constexpr uint32_t kSBendLeft1BallastSE = 357;
            constexpr uint32_t kSBendLeft2BallastSE = 358;
            constexpr uint32_t kSBendLeft3BallastSE = 359;
            constexpr uint32_t kSBendRight0BallastNE = 360;
            constexpr uint32_t kSBendRight1BallastNE = 361;
            constexpr uint32_t kSBendRight2BallastNE = 362;
            constexpr uint32_t kSBendRight3BallastNE = 363;
            constexpr uint32_t kSBendRight0BallastSE = 364;
            constexpr uint32_t kSBendRight1BallastSE = 365;
            constexpr uint32_t kSBendRight2BallastSE = 366;
            constexpr uint32_t kSBendRight3BallastSE = 367;
            constexpr uint32_t kSBendLeft0SleeperNE = 368;
            constexpr uint32_t kSBendLeft1SleeperNE = 369;
            constexpr uint32_t kSBendLeft2SleeperNE = 370;
            constexpr uint32_t kSBendLeft3SleeperNE = 371;
            constexpr uint32_t kSBendLeft0SleeperSE = 372;
            constexpr uint32_t kSBendLeft1SleeperSE = 373;
            constexpr uint32_t kSBendLeft2SleeperSE = 374;
            constexpr uint32_t kSBendLeft3SleeperSE = 375;
            constexpr uint32_t kSBendRight0SleeperNE = 376;
            constexpr uint32_t kSBendRight1SleeperNE = 377;
            constexpr uint32_t kSBendRight2SleeperNE = 378;
            constexpr uint32_t kSBendRight3SleeperNE = 379;
            constexpr uint32_t kSBendRight0SleeperSE = 380;
            constexpr uint32_t kSBendRight1SleeperSE = 381;
            constexpr uint32_t kSBendRight2SleeperSE = 382;
            constexpr uint32_t kSBendRight3SleeperSE = 383;
            constexpr uint32_t kSBendLeft0RailNE = 384;
            constexpr uint32_t kSBendLeft1RailNE = 385;
            constexpr uint32_t kSBendLeft2RailNE = 386;
            constexpr uint32_t kSBendLeft3RailNE = 387;
            constexpr uint32_t kSBendLeft0RailSE = 388;
            constexpr uint32_t kSBendLeft1RailSE = 389;
            constexpr uint32_t kSBendLeft2RailSE = 390;
            constexpr uint32_t kSBendLeft3RailSE = 391;
            constexpr uint32_t kSBendRight0RailNE = 392;
            constexpr uint32_t kSBendRight1RailNE = 393;
            constexpr uint32_t kSBendRight2RailNE = 394;
            constexpr uint32_t kSBendRight3RailNE = 395;
            constexpr uint32_t kSBendRight0RailSE = 396;
            constexpr uint32_t kSBendRight1RailSE = 397;
            constexpr uint32_t kSBendRight2RailSE = 398;
            constexpr uint32_t kSBendRight3RailSE = 399;
            constexpr uint32_t kRightCurveVerySmall0BallastNE = 400;
            constexpr uint32_t kRightCurveVerySmall0BallastSE = 401;
            constexpr uint32_t kRightCurveVerySmall0BallastSW = 402;
            constexpr uint32_t kRightCurveVerySmall0BallastNW = 403;
            constexpr uint32_t kRightCurveVerySmall0SleeperNE = 404;
            constexpr uint32_t kRightCurveVerySmall0SleeperSE = 405;
            constexpr uint32_t kRightCurveVerySmall0SleeperSW = 406;
            constexpr uint32_t kRightCurveVerySmall0SleeperNW = 407;
            constexpr uint32_t kRightCurveVerySmall0RailNE = 408;
            constexpr uint32_t kRightCurveVerySmall0RailSE = 409;
            constexpr uint32_t kRightCurveVerySmall0RailSW = 410;
            constexpr uint32_t kRightCurveVerySmall0RailNW = 411;
        }
    }
}
