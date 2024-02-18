#pragma once

#include "Object.h"
#include "Types.hpp"
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

#pragma pack(push, 1)
    struct TrackExtraObject
    {
        static constexpr auto kObjectType = ObjectType::trackExtra;

        StringId name;
        uint16_t trackPieces;    // 0x02
        uint8_t paintStyle;      // 0x04
        uint8_t costIndex;       // 0x05
        int16_t buildCostFactor; // 0x06
        int16_t sellCostFactor;  // 0x08
        uint32_t image;          // 0x0A
        uint32_t var_0E;

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(TrackExtraObject) == 0x12);

    namespace TrackExtraObj::ImageIds
    {
        // Note: Style imageIds are relative to 0x0A so you need to +8 to get its
        // real id relative to object at rest
        namespace Style0
        {
            constexpr uint32_t kStraight0NE = 0;
            constexpr uint32_t kStraight0SE = 1;
            constexpr uint32_t kStraight0SW = 2;
            constexpr uint32_t kStraight0NW = 3;
            constexpr uint32_t kRightCurveSmall0NE = 4;
            constexpr uint32_t kRightCurveSmall1NE = 5;
            constexpr uint32_t kRightCurveSmall2NE = 6;
            constexpr uint32_t kRightCurveSmall3NE = 7;
            constexpr uint32_t kRightCurveSmall0SE = 8;
            constexpr uint32_t kRightCurveSmall1SE = 9;
            constexpr uint32_t kRightCurveSmall2SE = 10;
            constexpr uint32_t kRightCurveSmall3SE = 11;
            constexpr uint32_t kRightCurveSmall0SW = 12;
            constexpr uint32_t kRightCurveSmall1SW = 13;
            constexpr uint32_t kRightCurveSmall2SW = 14;
            constexpr uint32_t kRightCurveSmall3SW = 15;
            constexpr uint32_t kRightCurveSmall0NW = 16;
            constexpr uint32_t kRightCurveSmall1NW = 17;
            constexpr uint32_t kRightCurveSmall2NW = 18;
            constexpr uint32_t kRightCurveSmall3NW = 19;
            constexpr uint32_t kRightCurve0NE = 20;
            constexpr uint32_t kRightCurve1NE = 21;
            constexpr uint32_t kRightCurve2NE = 22;
            constexpr uint32_t kRightCurve3NE = 23;
            constexpr uint32_t kRightCurve4NE = 24;
            constexpr uint32_t kRightCurve0SE = 25;
            constexpr uint32_t kRightCurve1SE = 26;
            constexpr uint32_t kRightCurve2SE = 27;
            constexpr uint32_t kRightCurve3SE = 28;
            constexpr uint32_t kRightCurve4SE = 29;
            constexpr uint32_t kRightCurve0SW = 30;
            constexpr uint32_t kRightCurve1SW = 31;
            constexpr uint32_t kRightCurve2SW = 32;
            constexpr uint32_t kRightCurve3SW = 33;
            constexpr uint32_t kRightCurve4SW = 34;
            constexpr uint32_t kRightCurve0NW = 35;
            constexpr uint32_t kRightCurve1NW = 36;
            constexpr uint32_t kRightCurve2NW = 37;
            constexpr uint32_t kRightCurve3NW = 38;
            constexpr uint32_t kRightCurve4NW = 39;
            constexpr uint32_t kSBendLeft0NE = 40;
            constexpr uint32_t kSBendLeft1NE = 41;
            constexpr uint32_t kSBendLeft2NE = 42;
            constexpr uint32_t kSBendLeft3NE = 43;
            constexpr uint32_t kSBendLeft0SE = 44;
            constexpr uint32_t kSBendLeft1SE = 45;
            constexpr uint32_t kSBendLeft2SE = 46;
            constexpr uint32_t kSBendLeft3SE = 47;
            constexpr uint32_t kSBendLeft3SW = 48;
            constexpr uint32_t kSBendLeft2SW = 49;
            constexpr uint32_t kSBendLeft1SW = 50;
            constexpr uint32_t kSBendLeft0SW = 51;
            constexpr uint32_t kSBendLeft3NW = 52;
            constexpr uint32_t kSBendLeft2NW = 53;
            constexpr uint32_t kSBendLeft1NW = 54;
            constexpr uint32_t kSBendLeft0NW = 55;
            constexpr uint32_t kSBendRight0NE = 56;
            constexpr uint32_t kSBendRight1NE = 57;
            constexpr uint32_t kSBendRight2NE = 58;
            constexpr uint32_t kSBendRight3NE = 59;
            constexpr uint32_t kSBendRight0SE = 60;
            constexpr uint32_t kSBendRight1SE = 61;
            constexpr uint32_t kSBendRight2SE = 62;
            constexpr uint32_t kSBendRight3SE = 63;
            constexpr uint32_t kSBendRight3SW = 64;
            constexpr uint32_t kSBendRight2SW = 65;
            constexpr uint32_t kSBendRight1SW = 66;
            constexpr uint32_t kSBendRight0SW = 67;
            constexpr uint32_t kSBendRight3NW = 68;
            constexpr uint32_t kSBendRight2NW = 69;
            constexpr uint32_t kSBendRight1NW = 70;
            constexpr uint32_t kSBendRight0NW = 71;
            constexpr uint32_t kStraightSlopeUp0NE = 72;
            constexpr uint32_t kStraightSlopeUp1NE = 73;
            constexpr uint32_t kStraightSlopeUp0SE = 74;
            constexpr uint32_t kStraightSlopeUp1SE = 75;
            constexpr uint32_t kStraightSlopeUp0SW = 76;
            constexpr uint32_t kStraightSlopeUp1SW = 77;
            constexpr uint32_t kStraightSlopeUp0NW = 78;
            constexpr uint32_t kStraightSlopeUp1NW = 79;
            constexpr uint32_t kStraightSteepSlopeUp0NE = 80;
            constexpr uint32_t kStraightSteepSlopeUp0SE = 81;
            constexpr uint32_t kStraightSteepSlopeUp0SW = 82;
            constexpr uint32_t kStraightSteepSlopeUp0NW = 83;
            constexpr uint32_t kRightCurveSmallSlopeUp0NE = 84;
            constexpr uint32_t kRightCurveSmallSlopeUp1NE = 85;
            constexpr uint32_t kRightCurveSmallSlopeUp2NE = 86;
            constexpr uint32_t kRightCurveSmallSlopeUp3NE = 87;
            constexpr uint32_t kRightCurveSmallSlopeUp0SE = 88;
            constexpr uint32_t kRightCurveSmallSlopeUp1SE = 89;
            constexpr uint32_t kRightCurveSmallSlopeUp2SE = 90;
            constexpr uint32_t kRightCurveSmallSlopeUp3SE = 91;
            constexpr uint32_t kRightCurveSmallSlopeUp0SW = 92;
            constexpr uint32_t kRightCurveSmallSlopeUp1SW = 93;
            constexpr uint32_t kRightCurveSmallSlopeUp2SW = 94;
            constexpr uint32_t kRightCurveSmallSlopeUp3SW = 95;
            constexpr uint32_t kRightCurveSmallSlopeUp0NW = 96;
            constexpr uint32_t kRightCurveSmallSlopeUp1NW = 97;
            constexpr uint32_t kRightCurveSmallSlopeUp2NW = 98;
            constexpr uint32_t kRightCurveSmallSlopeUp3NW = 99;
            constexpr uint32_t kRightCurveSmallSlopeDown0NE = 100;
            constexpr uint32_t kRightCurveSmallSlopeDown1NE = 101;
            constexpr uint32_t kRightCurveSmallSlopeDown2NE = 102;
            constexpr uint32_t kRightCurveSmallSlopeDown3NE = 103;
            constexpr uint32_t kRightCurveSmallSlopeDown0SE = 104;
            constexpr uint32_t kRightCurveSmallSlopeDown1SE = 105;
            constexpr uint32_t kRightCurveSmallSlopeDown2SE = 106;
            constexpr uint32_t kRightCurveSmallSlopeDown3SE = 107;
            constexpr uint32_t kRightCurveSmallSlopeDown0SW = 108;
            constexpr uint32_t kRightCurveSmallSlopeDown1SW = 109;
            constexpr uint32_t kRightCurveSmallSlopeDown2SW = 110;
            constexpr uint32_t kRightCurveSmallSlopeDown3SW = 111;
            constexpr uint32_t kRightCurveSmallSlopeDown0NW = 112;
            constexpr uint32_t kRightCurveSmallSlopeDown1NW = 113;
            constexpr uint32_t kRightCurveSmallSlopeDown2NW = 114;
            constexpr uint32_t kRightCurveSmallSlopeDown3NW = 115;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp0NE = 116;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp1NE = 117;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp2NE = 118;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp3NE = 119;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp0SE = 120;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp1SE = 121;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp2SE = 122;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp3SE = 123;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp0SW = 124;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp1SW = 125;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp2SW = 126;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp3SW = 127;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp0NW = 128;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp1NW = 129;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp2NW = 130;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp3NW = 131;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown0NE = 132;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown1NE = 133;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown2NE = 134;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown3NE = 135;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown0SE = 136;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown1SE = 137;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown2SE = 138;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown3SE = 139;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown0SW = 140;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown1SW = 141;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown2SW = 142;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown3SW = 143;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown0NW = 144;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown1NW = 145;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown2NW = 146;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown3NW = 147;
            constexpr uint32_t kRightCurveLarge0NE = 148;
            constexpr uint32_t kRightCurveLarge1NE = 149;
            constexpr uint32_t kRightCurveLarge2NE = 150;
            constexpr uint32_t kRightCurveLarge3NE = 151;
            constexpr uint32_t kRightCurveLarge4NE = 152;
            constexpr uint32_t kRightCurveLarge0SE = 153;
            constexpr uint32_t kRightCurveLarge1SE = 154;
            constexpr uint32_t kRightCurveLarge2SE = 155;
            constexpr uint32_t kRightCurveLarge3SE = 156;
            constexpr uint32_t kRightCurveLarge4SE = 157;
            constexpr uint32_t kRightCurveLarge0SW = 158;
            constexpr uint32_t kRightCurveLarge1SW = 159;
            constexpr uint32_t kRightCurveLarge2SW = 160;
            constexpr uint32_t kRightCurveLarge3SW = 161;
            constexpr uint32_t kRightCurveLarge4SW = 162;
            constexpr uint32_t kRightCurveLarge0NW = 163;
            constexpr uint32_t kRightCurveLarge1NW = 164;
            constexpr uint32_t kRightCurveLarge2NW = 165;
            constexpr uint32_t kRightCurveLarge3NW = 166;
            constexpr uint32_t kRightCurveLarge4NW = 167;
            constexpr uint32_t kLeftCurveLarge0NE = 168;
            constexpr uint32_t kLeftCurveLarge1NE = 169;
            constexpr uint32_t kLeftCurveLarge2NE = 170;
            constexpr uint32_t kLeftCurveLarge3NE = 171;
            constexpr uint32_t kLeftCurveLarge4NE = 172;
            constexpr uint32_t kLeftCurveLarge0SE = 173;
            constexpr uint32_t kLeftCurveLarge1SE = 174;
            constexpr uint32_t kLeftCurveLarge2SE = 175;
            constexpr uint32_t kLeftCurveLarge3SE = 176;
            constexpr uint32_t kLeftCurveLarge4SE = 177;
            constexpr uint32_t kLeftCurveLarge0SW = 178;
            constexpr uint32_t kLeftCurveLarge1SW = 179;
            constexpr uint32_t kLeftCurveLarge2SW = 180;
            constexpr uint32_t kLeftCurveLarge3SW = 181;
            constexpr uint32_t kLeftCurveLarge4SW = 182;
            constexpr uint32_t kLeftCurveLarge0NW = 183;
            constexpr uint32_t kLeftCurveLarge1NW = 184;
            constexpr uint32_t kLeftCurveLarge2NW = 185;
            constexpr uint32_t kLeftCurveLarge3NW = 186;
            constexpr uint32_t kLeftCurveLarge4NW = 187;
            constexpr uint32_t kDiagonal0NE = 188;
            constexpr uint32_t kDiagonal2NE = 189;
            constexpr uint32_t kDiagonal1NE = 190;
            constexpr uint32_t kDiagonal3NE = 191;
            constexpr uint32_t kDiagonal0SE = 192;
            constexpr uint32_t kDiagonal2SE = 193;
            constexpr uint32_t kDiagonal1SE = 194;
            constexpr uint32_t kDiagonal3SE = 195;
            constexpr uint32_t kDiagonal0SW = 196;
            constexpr uint32_t kDiagonal2SW = 197;
            constexpr uint32_t kDiagonal1SW = 198;
            constexpr uint32_t kDiagonal3SW = 199;
            constexpr uint32_t kDiagonal0NW = 200;
            constexpr uint32_t kDiagonal2NW = 201;
            constexpr uint32_t kDiagonal1NW = 202;
            constexpr uint32_t kDiagonal3NW = 203;
            constexpr uint32_t kRightCurveVerySmall0NE = 204;
            constexpr uint32_t kRightCurveVerySmall0SE = 205;
            constexpr uint32_t kRightCurveVerySmall0SW = 206;
            constexpr uint32_t kRightCurveVerySmall0NW = 207;
        }

        namespace Style1
        {
            constexpr uint32_t kStraight0NE = 0;
            constexpr uint32_t kStraight0SE = 1;
            constexpr uint32_t kRightCurveSmall0NE = 6;
            constexpr uint32_t kRightCurveSmall3NE = 7;
            constexpr uint32_t kRightCurveSmall0SE = 8;
            constexpr uint32_t kRightCurveSmall3SE = 9;
            constexpr uint32_t kRightCurveSmall0SW = 10;
            constexpr uint32_t kRightCurveSmall3SW = 11;
            constexpr uint32_t kRightCurveSmall0NW = 12;
            constexpr uint32_t kRightCurveSmall3NW = 13;
            constexpr uint32_t kRightCurve1NE = 14;
            constexpr uint32_t kRightCurve1SW = 15;
            constexpr uint32_t kRightCurve1SE = 16;
            constexpr uint32_t kRightCurve1NW = 17;
            constexpr uint32_t kRightCurveVerySmall0NE = 18;
            constexpr uint32_t kRightCurveVerySmall0SE = 19;
            constexpr uint32_t kRightCurveVerySmall0SW = 20;
            constexpr uint32_t kRightCurveVerySmall0NW = 21;
            constexpr uint32_t kDiagonal0NW = 22;
            constexpr uint32_t kDiagonal0NE = 23;
            constexpr uint32_t kDiagonal0SE = 24;
            constexpr uint32_t kDiagonal0SW = 25;
            constexpr uint32_t kStraightSlopeUp0NE = 50;
            constexpr uint32_t kStraightSlopeUp0SE = 51;
            constexpr uint32_t kStraightSlopeUp0SW = 52;
            constexpr uint32_t kStraightSlopeUp0NW = 53;
            constexpr uint32_t kStraightSlopeUp1NE = 54;
            constexpr uint32_t kStraightSlopeUp1SE = 55;
            constexpr uint32_t kStraightSlopeUp1SW = 56;
            constexpr uint32_t kStraightSlopeUp1NW = 57;
            constexpr uint32_t kStraightSteepSlopeUp0NE = 58;
            constexpr uint32_t kStraightSteepSlopeUp0SE = 59;
            constexpr uint32_t kStraightSteepSlopeUp0SW = 60;
            constexpr uint32_t kStraightSteepSlopeUp0NW = 61;
            constexpr uint32_t kRightCurveSmallSlopeUp0NE = 62;
            constexpr uint32_t kRightCurveSmallSlopeUp3NE = 63;
            constexpr uint32_t kRightCurveSmallSlopeUp0SE = 64;
            constexpr uint32_t kRightCurveSmallSlopeUp3SE = 65;
            constexpr uint32_t kRightCurveSmallSlopeUp0SW = 66;
            constexpr uint32_t kRightCurveSmallSlopeUp3SW = 67;
            constexpr uint32_t kRightCurveSmallSlopeUp0NW = 68;
            constexpr uint32_t kRightCurveSmallSlopeUp3NW = 69;
            constexpr uint32_t kRightCurveSmallSlopeDown0NE = 70;
            constexpr uint32_t kRightCurveSmallSlopeDown3NE = 71;
            constexpr uint32_t kRightCurveSmallSlopeDown0SE = 72;
            constexpr uint32_t kRightCurveSmallSlopeDown3SE = 73;
            constexpr uint32_t kRightCurveSmallSlopeDown0SW = 74;
            constexpr uint32_t kRightCurveSmallSlopeDown3SW = 75;
            constexpr uint32_t kRightCurveSmallSlopeDown0NW = 76;
            constexpr uint32_t kRightCurveSmallSlopeDown3NW = 77;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp0NE = 78;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp3NE = 79;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp0SE = 80;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp3SE = 81;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp0SW = 82;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp3SW = 83;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp0NW = 84;
            constexpr uint32_t kRightCurveSmallSteepSlopeUp3NW = 85;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown0NE = 86;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown3NE = 87;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown0SE = 88;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown3SE = 89;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown0SW = 90;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown3SW = 91;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown0NW = 92;
            constexpr uint32_t kRightCurveSmallSteepSlopeDown3NW = 93;
            constexpr uint32_t kSupportStraight0SE = 94;
            constexpr uint32_t kSupportConnectorStraight0SE = 95;
            constexpr uint32_t kSupportStraight0SW = 96;
            constexpr uint32_t kSupportConnectorStraight0SW = 97;
            constexpr uint32_t kSupportStraight0NW = 98;
            constexpr uint32_t kSupportConnectorStraight0NW = 99;
            constexpr uint32_t kSupportStraight0NE = 100;
            constexpr uint32_t kSupportConnectorStraight0NE = 101;
            constexpr uint32_t kSupportRightCurve1NE = 102;
            constexpr uint32_t kSupportConnectorRightCurve1NE = 103;
            constexpr uint32_t kSupportRightCurve3NE = 104;
            constexpr uint32_t kSupportConnectorRightCurve3NE = 105;
            constexpr uint32_t kSupportRightCurve1SE = 106;
            constexpr uint32_t kSupportConnectorRightCurve1SE = 107;
            constexpr uint32_t kSupportRightCurve3SE = 108;
            constexpr uint32_t kSupportConnectorRightCurve3SE = 109;
            constexpr uint32_t kSupportRightCurve1SW = 110;
            constexpr uint32_t kSupportConnectorRightCurve1SW = 111;
            constexpr uint32_t kSupportRightCurve3SW = 112;
            constexpr uint32_t kSupportConnectorRightCurve3SW = 113;
            constexpr uint32_t kSupportRightCurve1NW = 114;
            constexpr uint32_t kSupportConnectorRightCurve1NW = 115;
            constexpr uint32_t kSupportRightCurve3NW = 116;
            constexpr uint32_t kSupportConnectorRightCurve3NW = 117;
        }
    }
}
