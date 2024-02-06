#include "Objects/TrackExtraObject.h"
#include "Paint.h"
#include <array>
#include <optional>
#include <span>

namespace OpenLoco::Paint
{

    namespace Style0
    {
        struct TrackPaintAdditionPiece
        {
            std::array<uint32_t, 4> imageIds;
            std::array<World::Pos3, 4> boundingBoxOffsets;
            std::array<World::Pos3, 4> boundingBoxSizes;
            bool isIsMergable;
        };
        constexpr std::array<uint8_t, 4> kRotationTable1230 = { 1, 2, 3, 0 };
        constexpr std::array<uint8_t, 4> kRotationTable2301 = { 2, 3, 0, 1 };
        constexpr std::array<uint8_t, 4> kRotationTable3012 = { 3, 0, 1, 2 };

        consteval TrackPaintAdditionPiece rotateTrackPPA(const TrackPaintAdditionPiece& reference, const std::array<uint8_t, 4>& rotationTable)
        {
            return TrackPaintAdditionPiece{
                std::array<uint32_t, 4>{
                    reference.imageIds[rotationTable[0]],
                    reference.imageIds[rotationTable[1]],
                    reference.imageIds[rotationTable[2]],
                    reference.imageIds[rotationTable[3]],
                },
                std::array<World::Pos3, 4>{
                    reference.boundingBoxOffsets[rotationTable[0]],
                    reference.boundingBoxOffsets[rotationTable[1]],
                    reference.boundingBoxOffsets[rotationTable[2]],
                    reference.boundingBoxOffsets[rotationTable[3]],
                },
                std::array<World::Pos3, 4>{
                    reference.boundingBoxSizes[rotationTable[0]],
                    reference.boundingBoxSizes[rotationTable[1]],
                    reference.boundingBoxSizes[rotationTable[2]],
                    reference.boundingBoxSizes[rotationTable[3]],
                },
                reference.isIsMergable
            };
        }

        using namespace OpenLoco::TrackExtraObj::ImageIds::Style0;

        // 0x0041DDCD, 0x0041DE0C, 0x0041DE4B, 0x0041DE8A
        constexpr TrackPaintAdditionPiece kStraightAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraight0NE,
                kStraight0SE,
                kStraight0SW,
                kStraight0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 5, 0 },
                World::Pos3{ 5, 2, 0 },
                World::Pos3{ 2, 5, 0 },
                World::Pos3{ 5, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 22, 1 },
                World::Pos3{ 22, 28, 1 },
                World::Pos3{ 28, 22, 1 },
                World::Pos3{ 22, 28, 1 },
            },
            /* Mergable */ true,
        };

        constexpr std::array<TrackPaintAdditionPiece, 1> kStraightTPPA = {
            kStraightAddition0,
        };

        // 0x00420ED5, 0x00420FDD, 0x004210E5, 0x004211ED
        constexpr TrackPaintAdditionPiece kDiagonalAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kDiagonal0NE,
                kDiagonal0SE,
                kDiagonal0SW,
                kDiagonal0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
            },
            /* Mergable */ true,
        };

        // 0x00420F17, 0x0042101F, 0x00421127, 0x0042122F
        constexpr TrackPaintAdditionPiece kDiagonalAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kDiagonal1NE,
                kDiagonal1SE,
                kDiagonal1SW,
                kDiagonal1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 16, 0 },
                World::Pos3{ 16, 16, 0 },
                World::Pos3{ 16, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ true,
        };

        // 0x00420F59, 0x00421061, 0x00421169, 0x00421271
        constexpr TrackPaintAdditionPiece kDiagonalAddition2 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kDiagonal2NE,
                kDiagonal2SE,
                kDiagonal2SW,
                kDiagonal2NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 16, 16, 0 },
                World::Pos3{ 16, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 16, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ true,
        };

        // 0x00420F9B, 0x004210A3, 0x004211AB, 0x004212B3
        constexpr TrackPaintAdditionPiece kDiagonalAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kDiagonal3NE,
                kDiagonal3SE,
                kDiagonal3SW,
                kDiagonal3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
            },
            /* Mergable */ true,
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kDiagonalTPPA = {
            kDiagonalAddition0,
            kDiagonalAddition1,
            kDiagonalAddition2,
            kDiagonalAddition3,
        };

        // 0x0041E3F1, 0x0041E433, 0x0041E475, 0x0041E4B7
        constexpr TrackPaintAdditionPiece kRightCurveVerySmallAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveVerySmall0NE,
                kRightCurveVerySmall0SE,
                kRightCurveVerySmall0SW,
                kRightCurveVerySmall0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
            },
            /* Mergable */ true,
        };

        constexpr std::array<TrackPaintAdditionPiece, 1> kRightCurveVerySmallTPPA = {
            kRightCurveVerySmallAddition0,
        };

        constexpr TrackPaintAdditionPiece kLeftCurveVerySmallAddition0 = rotateTrackPPA(kRightCurveVerySmallAddition0, kRotationTable1230);

        constexpr std::array<TrackPaintAdditionPiece, 1> kLeftCurveVerySmallTPPA = {
            kLeftCurveVerySmallAddition0,
        };

        // 0x0041E849, 0x0041E945, 0x0041EA41, 0x0041EB3D
        constexpr TrackPaintAdditionPiece kRightCurveSmallAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmall0NE,
                kRightCurveSmall0SE,
                kRightCurveSmall0SW,
                kRightCurveSmall0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 0 },
                World::Pos3{ 6, 2, 0 },
                World::Pos3{ 2, 6, 0 },
                World::Pos3{ 6, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ true,
        };

        // 0x0041E888, 0x0041E984, 0x0041EA80, 0x0041EB7C
        constexpr TrackPaintAdditionPiece kRightCurveSmallAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmall1NE,
                kRightCurveSmall1SE,
                kRightCurveSmall1SW,
                kRightCurveSmall1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 16, 0 },
                World::Pos3{ 16, 16, 0 },
                World::Pos3{ 16, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ true,
        };

        // 0x0041E8C7, 0x0041E9C3, 0x0041EABF, 0x0041EBBB
        constexpr TrackPaintAdditionPiece kRightCurveSmallAddition2 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmall2NE,
                kRightCurveSmall2SE,
                kRightCurveSmall2SW,
                kRightCurveSmall2NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 16, 16, 0 },
                World::Pos3{ 16, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 16, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ true,
        };

        // 0x0041E906, 0x0041EA02, 0x0041EAFE, 0x0041EBFA
        constexpr TrackPaintAdditionPiece kRightCurveSmallAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmall3NE,
                kRightCurveSmall3SE,
                kRightCurveSmall3SW,
                kRightCurveSmall3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 6, 2, 0 },
                World::Pos3{ 2, 6, 0 },
                World::Pos3{ 6, 2, 0 },
                World::Pos3{ 2, 6, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
            },
            /* Mergable */ true,
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kRightCurveSmallTPPA = {
            kRightCurveSmallAddition0,
            kRightCurveSmallAddition1,
            kRightCurveSmallAddition2,
            kRightCurveSmallAddition3,
        };

        constexpr TrackPaintAdditionPiece kLeftCurveSmallAddition0 = rotateTrackPPA(kRightCurveSmallAddition3, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallAddition1 = rotateTrackPPA(kRightCurveSmallAddition1, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallAddition2 = rotateTrackPPA(kRightCurveSmallAddition2, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallAddition3 = rotateTrackPPA(kRightCurveSmallAddition0, kRotationTable1230);

        constexpr std::array<TrackPaintAdditionPiece, 4> kLeftCurveSmallTPPA = {
            kLeftCurveSmallAddition0,
            kLeftCurveSmallAddition1,
            kLeftCurveSmallAddition2,
            kLeftCurveSmallAddition3,
        };

        // 0x0041FF0D, 0x00420048, 0x00420183, 0x004202BE
        constexpr TrackPaintAdditionPiece kRightCurveAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurve0NE,
                kRightCurve0SE,
                kRightCurve0SW,
                kRightCurve0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 0 },
                World::Pos3{ 6, 2, 0 },
                World::Pos3{ 2, 6, 0 },
                World::Pos3{ 6, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ true,
        };

        // 0x0041FF4C, 0x00420087, 0x004201C2, 0x004202FD
        constexpr TrackPaintAdditionPiece kRightCurveAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurve1NE,
                kRightCurve1SE,
                kRightCurve1SW,
                kRightCurve1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 16, 0 },
                World::Pos3{ 16, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 14, 1 },
                World::Pos3{ 14, 28, 1 },
                World::Pos3{ 28, 14, 1 },
                World::Pos3{ 14, 28, 1 },
            },
            /* Mergable */ true,
        };

        // 0x0041FF8B, 0x004200C6, 0x00420201, 0x0042033C
        constexpr TrackPaintAdditionPiece kRightCurveAddition2 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurve2NE,
                kRightCurve2SE,
                kRightCurve2SW,
                kRightCurve2NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 16, 0 },
                World::Pos3{ 16, 16, 0 },
                World::Pos3{ 16, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ true,
        };

        // 0x0041FFCA, 0x00420105, 0x00420240, 0x0042037B
        constexpr TrackPaintAdditionPiece kRightCurveAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurve3NE,
                kRightCurve3SE,
                kRightCurve3SW,
                kRightCurve3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 16, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 16, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 28, 1 },
                World::Pos3{ 28, 14, 1 },
                World::Pos3{ 14, 28, 1 },
                World::Pos3{ 28, 14, 1 },
            },
            /* Mergable */ true,
        };

        // 0x00420009, 0x00420144, 0x0042027F, 0x004203BA
        constexpr TrackPaintAdditionPiece kRightCurveAddition4 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurve4NE,
                kRightCurve4SE,
                kRightCurve4SW,
                kRightCurve4NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 6, 2, 0 },
                World::Pos3{ 2, 6, 0 },
                World::Pos3{ 6, 2, 0 },
                World::Pos3{ 2, 6, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
            },
            /* Mergable */ true,
        };

        constexpr std::array<TrackPaintAdditionPiece, 5> kRightCurveTPPA = {
            kRightCurveAddition0,
            kRightCurveAddition1,
            kRightCurveAddition2,
            kRightCurveAddition3,
            kRightCurveAddition4,
        };

        constexpr TrackPaintAdditionPiece kLeftCurveAddition0 = rotateTrackPPA(kRightCurveAddition4, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveAddition1 = rotateTrackPPA(kRightCurveAddition3, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveAddition2 = rotateTrackPPA(kRightCurveAddition2, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveAddition3 = rotateTrackPPA(kRightCurveAddition1, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveAddition4 = rotateTrackPPA(kRightCurveAddition0, kRotationTable1230);

        constexpr std::array<TrackPaintAdditionPiece, 5> kLeftCurveTPPA = {
            kLeftCurveAddition0,
            kLeftCurveAddition1,
            kLeftCurveAddition2,
            kLeftCurveAddition3,
            kLeftCurveAddition4,
        };

        // 0x00420991, 0x00420ADB, 0x00420C25, 0x00420D6F
        constexpr TrackPaintAdditionPiece kRightCurveLargeAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveLarge0NE,
                kRightCurveLarge0SE,
                kRightCurveLarge0SW,
                kRightCurveLarge0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 3, 0 },
                World::Pos3{ 3, 2, 0 },
                World::Pos3{ 2, 3, 0 },
                World::Pos3{ 3, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
            },
            /* Mergable */ true,
        };

        // 0x004209D3, 0x00420B1D, 0x00420C67, 0x00420DB1
        constexpr TrackPaintAdditionPiece kRightCurveLargeAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveLarge1NE,
                kRightCurveLarge1SE,
                kRightCurveLarge1SW,
                kRightCurveLarge1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 16, 0 },
                World::Pos3{ 16, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 14, 1 },
                World::Pos3{ 14, 28, 1 },
                World::Pos3{ 28, 14, 1 },
                World::Pos3{ 14, 28, 1 },
            },
            /* Mergable */ true,
        };

        // 0x00420A15, 0x00420B5F, 0x00420CA9, 0x00420DF3
        constexpr TrackPaintAdditionPiece kRightCurveLargeAddition2 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveLarge2NE,
                kRightCurveLarge2SE,
                kRightCurveLarge2SW,
                kRightCurveLarge2NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 16, 0 },
                World::Pos3{ 16, 16, 0 },
                World::Pos3{ 16, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ true,
        };

        // 0x00420A57, 0x00420BA1, 0x00420CEB, 0x00420E35
        constexpr TrackPaintAdditionPiece kRightCurveLargeAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveLarge3NE,
                kRightCurveLarge3SE,
                kRightCurveLarge3SW,
                kRightCurveLarge3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 16, 16, 0 },
                World::Pos3{ 16, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 16, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ true,
        };

        // 0x00420A99, 0x00420BE3, 0x00420D2D, 0x00420E77
        constexpr TrackPaintAdditionPiece kRightCurveLargeAddition4 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveLarge4NE,
                kRightCurveLarge4SE,
                kRightCurveLarge4SW,
                kRightCurveLarge4NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
            },
            /* Mergable */ true,
        };

        constexpr std::array<TrackPaintAdditionPiece, 5> kRightCurveLargeTPPA = {
            kRightCurveLargeAddition0,
            kRightCurveLargeAddition1,
            kRightCurveLargeAddition2,
            kRightCurveLargeAddition3,
            kRightCurveLargeAddition4,
        };

        // 0x00420469, 0x004205B3, 0x004206FD, 0x00420847
        constexpr TrackPaintAdditionPiece kLeftCurveLargeAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kLeftCurveLarge0NE,
                kLeftCurveLarge0SE,
                kLeftCurveLarge0SW,
                kLeftCurveLarge0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 3, 0 },
                World::Pos3{ 3, 2, 0 },
                World::Pos3{ 2, 3, 0 },
                World::Pos3{ 3, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
            },
            /* Mergable */ true,
        };

        // 0x004204AB, 0x004205F5, 0x0042073F, 0x00420889
        constexpr TrackPaintAdditionPiece kLeftCurveLargeAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kLeftCurveLarge1NE,
                kLeftCurveLarge1SE,
                kLeftCurveLarge1SW,
                kLeftCurveLarge1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 16, 0 },
                World::Pos3{ 16, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 14, 1 },
                World::Pos3{ 14, 28, 1 },
                World::Pos3{ 28, 14, 1 },
                World::Pos3{ 14, 28, 1 },
            },
            /* Mergable */ true,
        };

        // 0x004204ED, 0x00420637, 0x00420781, 0x004208CB
        constexpr TrackPaintAdditionPiece kLeftCurveLargeAddition2 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kLeftCurveLarge2NE,
                kLeftCurveLarge2SE,
                kLeftCurveLarge2SW,
                kLeftCurveLarge2NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 16, 0 },
                World::Pos3{ 16, 16, 0 },
                World::Pos3{ 16, 2, 0 },
                World::Pos3{ 2, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ true,
        };

        // 0x0042052F, 0x00420679, 0x004207C3, 0x0042090D
        constexpr TrackPaintAdditionPiece kLeftCurveLargeAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kLeftCurveLarge3NE,
                kLeftCurveLarge3SE,
                kLeftCurveLarge3SW,
                kLeftCurveLarge3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 16, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 16, 0 },
                World::Pos3{ 16, 16, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ true,
        };

        // 0x00420571, 0x004206BB, 0x00420805, 0x0042094F
        constexpr TrackPaintAdditionPiece kLeftCurveLargeAddition4 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kLeftCurveLarge4NE,
                kLeftCurveLarge4SE,
                kLeftCurveLarge4SW,
                kLeftCurveLarge4NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
            },
            /* Mergable */ true,
        };

        constexpr std::array<TrackPaintAdditionPiece, 5> kLeftCurveLargeTPPA = {
            kLeftCurveLargeAddition0,
            kLeftCurveLargeAddition1,
            kLeftCurveLargeAddition2,
            kLeftCurveLargeAddition3,
            kLeftCurveLargeAddition4,
        };

        constexpr TrackPaintAdditionPiece kDiagonalRightCurveLargeAddition0 = rotateTrackPPA(kLeftCurveLargeAddition4, kRotationTable3012);

        constexpr TrackPaintAdditionPiece kDiagonalRightCurveLargeAddition1 = rotateTrackPPA(kLeftCurveLargeAddition2, kRotationTable3012);

        constexpr TrackPaintAdditionPiece kDiagonalRightCurveLargeAddition2 = rotateTrackPPA(kLeftCurveLargeAddition3, kRotationTable3012);

        constexpr TrackPaintAdditionPiece kDiagonalRightCurveLargeAddition3 = rotateTrackPPA(kLeftCurveLargeAddition1, kRotationTable3012);

        constexpr TrackPaintAdditionPiece kDiagonalRightCurveLargeAddition4 = rotateTrackPPA(kLeftCurveLargeAddition0, kRotationTable3012);

        constexpr std::array<TrackPaintAdditionPiece, 5> kDiagonalRightCurveLargeTPPA = {
            kDiagonalRightCurveLargeAddition0,
            kDiagonalRightCurveLargeAddition1,
            kDiagonalRightCurveLargeAddition2,
            kDiagonalRightCurveLargeAddition3,
            kDiagonalRightCurveLargeAddition4,
        };

        constexpr TrackPaintAdditionPiece kDiagonalLeftCurveLargeAddition0 = rotateTrackPPA(kRightCurveLargeAddition4, kRotationTable2301);

        constexpr TrackPaintAdditionPiece kDiagonalLeftCurveLargeAddition1 = rotateTrackPPA(kRightCurveLargeAddition2, kRotationTable2301);

        constexpr TrackPaintAdditionPiece kDiagonalLeftCurveLargeAddition2 = rotateTrackPPA(kRightCurveLargeAddition3, kRotationTable2301);

        constexpr TrackPaintAdditionPiece kDiagonalLeftCurveLargeAddition3 = rotateTrackPPA(kRightCurveLargeAddition1, kRotationTable2301);

        constexpr TrackPaintAdditionPiece kDiagonalLeftCurveLargeAddition4 = rotateTrackPPA(kRightCurveLargeAddition0, kRotationTable2301);

        constexpr std::array<TrackPaintAdditionPiece, 5> kDiagonalLeftCurveLargeTPPA = {
            kDiagonalLeftCurveLargeAddition0,
            kDiagonalLeftCurveLargeAddition1,
            kDiagonalLeftCurveLargeAddition2,
            kDiagonalLeftCurveLargeAddition3,
            kDiagonalLeftCurveLargeAddition4,
        };

        // 0x004216AD, 0x004217A9, 0x00421962, 0x00421A5E
        constexpr TrackPaintAdditionPiece kSBendLeftAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kSBendLeft0NE,
                kSBendLeft0SE,
                kSBendLeft0SW,
                kSBendLeft0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
            },
            /* Mergable */ true,
        };

        // 0x004216EC, 0x004217E8, 0x00421923, 0x00421A1F
        constexpr TrackPaintAdditionPiece kSBendLeftAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kSBendLeft1NE,
                kSBendLeft1SE,
                kSBendLeft1SW,
                kSBendLeft1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 6, 0 },
                World::Pos3{ 6, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 24, 1 },
                World::Pos3{ 24, 28, 1 },
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
            },
            /* Mergable */ true,
        };

        // 0x0042172B, 0x00421827, 0x004218E4, 0x004219E0
        constexpr TrackPaintAdditionPiece kSBendLeftAddition2 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kSBendLeft2NE,
                kSBendLeft2SE,
                kSBendLeft2SW,
                kSBendLeft2NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 0 },
                World::Pos3{ 6, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
                World::Pos3{ 28, 24, 1 },
                World::Pos3{ 24, 28, 1 },
            },
            /* Mergable */ true,
        };

        // 0x0042176A, 0x00421866, 0x004218A5, 0x004219A1
        constexpr TrackPaintAdditionPiece kSBendLeftAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kSBendLeft3NE,
                kSBendLeft3SE,
                kSBendLeft3SW,
                kSBendLeft3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
            },
            /* Mergable */ true,
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kSBendLeftTPPA = {
            kSBendLeftAddition0,
            kSBendLeftAddition1,
            kSBendLeftAddition2,
            kSBendLeftAddition3,
        };

        // 0x00421A9D, 0x00421B99, 0x00421D52, 0x00421E4E
        constexpr TrackPaintAdditionPiece kSBendRightAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kSBendRight0NE,
                kSBendRight0SE,
                kSBendRight0SW,
                kSBendRight0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
            },
            /* Mergable */ true,
        };

        // 0x00421ADC, 0x00421BD8, 0x00421D13, 0x00421E0F
        constexpr TrackPaintAdditionPiece kSBendRightAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kSBendRight1NE,
                kSBendRight1SE,
                kSBendRight1SW,
                kSBendRight1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 0 },
                World::Pos3{ 6, 2, 0 },
                World::Pos3{ 2, 0, 0 },
                World::Pos3{ 2, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 24, 28, 1 },
            },
            /* Mergable */ true,
        };

        // 0x00421B1B, 0x00421C17, 0x00421CD4, 0x00421DD0
        constexpr TrackPaintAdditionPiece kSBendRightAddition2 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kSBendRight2NE,
                kSBendRight2SE,
                kSBendRight2SW,
                kSBendRight2NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 0, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 6, 0 },
                World::Pos3{ 6, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 24, 28, 1 },
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
            },
            /* Mergable */ true,
        };

        // 0x00421B5A, 0x00421C56, 0x00421C95, 0x00421D91
        constexpr TrackPaintAdditionPiece kSBendRightAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kSBendRight3NE,
                kSBendRight3SE,
                kSBendRight3SW,
                kSBendRight3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
                World::Pos3{ 2, 2, 0 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
            },
            /* Mergable */ true,
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kSBendRightTPPA = {
            kSBendRightAddition0,
            kSBendRightAddition1,
            kSBendRightAddition2,
            kSBendRightAddition3,
        };

        // 0x0042132D, 0x004213B9, 0x00421445, 0x004214D1
        constexpr TrackPaintAdditionPiece kStraightSlopeUpAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraightSlopeUp0NE,
                kStraightSlopeUp0SE,
                kStraightSlopeUp0SW,
                kStraightSlopeUp0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ false,
        };

        // 0x00421373, 0x004213FF, 0x0042148B, 0x00421517
        constexpr TrackPaintAdditionPiece kStraightSlopeUpAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraightSlopeUp1NE,
                kStraightSlopeUp1SE,
                kStraightSlopeUp1SW,
                kStraightSlopeUp1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ false,
        };

        constexpr std::array<TrackPaintAdditionPiece, 2> kStraightSlopeUpTPPA = {
            kStraightSlopeUpAddition0,
            kStraightSlopeUpAddition1,
        };

        constexpr TrackPaintAdditionPiece kStraightSlopeDownAddition0 = rotateTrackPPA(kStraightSlopeUpAddition1, kRotationTable2301);

        constexpr TrackPaintAdditionPiece kStraightSlopeDownAddition1 = rotateTrackPPA(kStraightSlopeUpAddition0, kRotationTable2301);

        constexpr std::array<TrackPaintAdditionPiece, 2> kStraightSlopeDownTPPA = {
            kStraightSlopeDownAddition0,
            kStraightSlopeDownAddition1,
        };

        // 0x0042155D, 0x004215A3, 0x004215E9, 0x0042162F
        constexpr TrackPaintAdditionPiece kStraightSteepSlopeUpAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraightSteepSlopeUp0NE,
                kStraightSteepSlopeUp0SE,
                kStraightSteepSlopeUp0SW,
                kStraightSteepSlopeUp0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ false,
        };

        constexpr std::array<TrackPaintAdditionPiece, 1> kStraightSteepSlopeUpTPPA = {
            kStraightSteepSlopeUpAddition0,
        };

        constexpr TrackPaintAdditionPiece kStraightSteepSlopeDownAddition0 = rotateTrackPPA(kStraightSteepSlopeUpAddition0, kRotationTable2301);

        constexpr std::array<TrackPaintAdditionPiece, 1> kStraightSteepSlopeDownTPPA = {
            kStraightSteepSlopeDownAddition0,
        };

        // 0x0041ECA9, 0x0041EDC1, 0x0041EED9, 0x0041EFF1
        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeUpAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSlopeUp0NE,
                kRightCurveSmallSlopeUp0SE,
                kRightCurveSmallSlopeUp0SW,
                kRightCurveSmallSlopeUp0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ false,
        };

        // 0x0041ECEF, 0x0041EE07, 0x0041EF1F, 0x0041F037
        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeUpAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSlopeUp1NE,
                kRightCurveSmallSlopeUp1SE,
                kRightCurveSmallSlopeUp1SW,
                kRightCurveSmallSlopeUp1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 2 },
                World::Pos3{ 2, 16, 2 },
                World::Pos3{ 16, 16, 2 },
                World::Pos3{ 16, 2, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ false,
        };

        // 0x0041ED35, 0x0041EE4D, 0x0041EF65, 0x0041F07D
        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeUpAddition2 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSlopeUp2NE,
                kRightCurveSmallSlopeUp2SE,
                kRightCurveSmallSlopeUp2SW,
                kRightCurveSmallSlopeUp2NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 16, 16, 2 },
                World::Pos3{ 16, 2, 2 },
                World::Pos3{ 2, 2, 2 },
                World::Pos3{ 2, 16, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ false,
        };

        // 0x0041ED7B, 0x0041EE93, 0x0041EFAB, 0x0041F0C3
        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeUpAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSlopeUp3NE,
                kRightCurveSmallSlopeUp3SE,
                kRightCurveSmallSlopeUp3SW,
                kRightCurveSmallSlopeUp3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
            },
            /* Mergable */ false,
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kRightCurveSmallSlopeUpTPPA = {
            kRightCurveSmallSlopeUpAddition0,
            kRightCurveSmallSlopeUpAddition1,
            kRightCurveSmallSlopeUpAddition2,
            kRightCurveSmallSlopeUpAddition3,
        };

        // 0x0041F109, 0x0041F221, 0x0041F339, 0x0041F451
        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeDownAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSlopeDown0NE,
                kRightCurveSmallSlopeDown0SE,
                kRightCurveSmallSlopeDown0SW,
                kRightCurveSmallSlopeDown0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ false,
        };

        // 0x0041F14F, 0x0041F267, 0x0041F37F, 0x0041F497
        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeDownAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSlopeDown1NE,
                kRightCurveSmallSlopeDown1SE,
                kRightCurveSmallSlopeDown1SW,
                kRightCurveSmallSlopeDown1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 2 },
                World::Pos3{ 2, 16, 2 },
                World::Pos3{ 16, 16, 2 },
                World::Pos3{ 16, 2, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ false,
        };

        // 0x0041F195, 0x0041F2AD, 0x0041F3C5, 0x0041F4DD
        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeDownAddition2 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSlopeDown2NE,
                kRightCurveSmallSlopeDown2SE,
                kRightCurveSmallSlopeDown2SW,
                kRightCurveSmallSlopeDown2NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 16, 16, 2 },
                World::Pos3{ 16, 2, 2 },
                World::Pos3{ 2, 2, 2 },
                World::Pos3{ 2, 16, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ false,
        };

        // 0x0041F1DB, 0x0041F2F3, 0x0041F40B, 0x0041F523
        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeDownAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSlopeDown3NE,
                kRightCurveSmallSlopeDown3SE,
                kRightCurveSmallSlopeDown3SW,
                kRightCurveSmallSlopeDown3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
            },
            /* Mergable */ false,
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kRightCurveSmallSlopeDownTPPA = {
            kRightCurveSmallSlopeDownAddition0,
            kRightCurveSmallSlopeDownAddition1,
            kRightCurveSmallSlopeDownAddition2,
            kRightCurveSmallSlopeDownAddition3,
        };

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeUpAddition0 = rotateTrackPPA(kRightCurveSmallSlopeDownAddition3, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeUpAddition1 = rotateTrackPPA(kRightCurveSmallSlopeDownAddition1, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeUpAddition2 = rotateTrackPPA(kRightCurveSmallSlopeDownAddition2, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeUpAddition3 = rotateTrackPPA(kRightCurveSmallSlopeDownAddition0, kRotationTable1230);

        constexpr std::array<TrackPaintAdditionPiece, 4> kLeftCurveSmallSlopeUpTPPA = {
            kLeftCurveSmallSlopeUpAddition0,
            kLeftCurveSmallSlopeUpAddition1,
            kLeftCurveSmallSlopeUpAddition2,
            kLeftCurveSmallSlopeUpAddition3,
        };

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeDownAddition0 = rotateTrackPPA(kRightCurveSmallSlopeUpAddition3, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeDownAddition1 = rotateTrackPPA(kRightCurveSmallSlopeUpAddition1, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeDownAddition2 = rotateTrackPPA(kRightCurveSmallSlopeUpAddition2, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeDownAddition3 = rotateTrackPPA(kRightCurveSmallSlopeUpAddition0, kRotationTable1230);

        constexpr std::array<TrackPaintAdditionPiece, 4> kLeftCurveSmallSlopeDownTPPA = {
            kLeftCurveSmallSlopeDownAddition0,
            kLeftCurveSmallSlopeDownAddition1,
            kLeftCurveSmallSlopeDownAddition2,
            kLeftCurveSmallSlopeDownAddition3,
        };

        // 0x0041F5D9, 0x0041F6F1, 0x0041F809, 0x0041F921
        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeUpAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSteepSlopeUp0NE,
                kRightCurveSmallSteepSlopeUp0SE,
                kRightCurveSmallSteepSlopeUp0SW,
                kRightCurveSmallSteepSlopeUp0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ false,
        };

        // 0x0041F61F, 0x0041F737, 0x0041F84F, 0x0041F96A
        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeUpAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSteepSlopeUp1NE,
                kRightCurveSmallSteepSlopeUp1SE,
                kRightCurveSmallSteepSlopeUp1SW,
                kRightCurveSmallSteepSlopeUp1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 2 },
                World::Pos3{ 2, 16, 2 },
                World::Pos3{ 16, 16, 2 },
                World::Pos3{ 16, 2, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ false,
        };

        // 0x0041F665, 0x0041F77D, 0x0041F895, 0x0041F9B3
        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeUpAddition2 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSteepSlopeUp2NE,
                kRightCurveSmallSteepSlopeUp2SE,
                kRightCurveSmallSteepSlopeUp2SW,
                kRightCurveSmallSteepSlopeUp2NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 16, 16, 2 },
                World::Pos3{ 16, 2, 2 },
                World::Pos3{ 2, 2, 2 },
                World::Pos3{ 2, 16, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ false,
        };

        // 0x0041F6AB, 0x0041F7C3, 0x0041F8DB, 0x0041F9FC
        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeUpAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSteepSlopeUp3NE,
                kRightCurveSmallSteepSlopeUp3SE,
                kRightCurveSmallSteepSlopeUp3SW,
                kRightCurveSmallSteepSlopeUp3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
            },
            /* Mergable */ false,
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kRightCurveSmallSteepSlopeUpTPPA = {
            kRightCurveSmallSteepSlopeUpAddition0,
            kRightCurveSmallSteepSlopeUpAddition1,
            kRightCurveSmallSteepSlopeUpAddition2,
            kRightCurveSmallSteepSlopeUpAddition3,
        };

        // 0x0041FA45, 0x0041FB69, 0x0041FC8D, 0x0041FDB1
        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeDownAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSteepSlopeDown0NE,
                kRightCurveSmallSteepSlopeDown0SE,
                kRightCurveSmallSteepSlopeDown0SW,
                kRightCurveSmallSteepSlopeDown0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ false,
        };

        // 0x0041FA8E, 0x0041FBB2, 0x0041FCD6, 0x0041FDFA
        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeDownAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSteepSlopeDown1NE,
                kRightCurveSmallSteepSlopeDown1SE,
                kRightCurveSmallSteepSlopeDown1SW,
                kRightCurveSmallSteepSlopeDown1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 2 },
                World::Pos3{ 2, 16, 2 },
                World::Pos3{ 16, 16, 2 },
                World::Pos3{ 16, 2, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ false,
        };

        // 0x0041FAD7, 0x0041FBFB, 0x0041FD1F, 0x0041FE43
        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeDownAddition2 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSteepSlopeDown2NE,
                kRightCurveSmallSteepSlopeDown2SE,
                kRightCurveSmallSteepSlopeDown2SW,
                kRightCurveSmallSteepSlopeDown2NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 16, 16, 2 },
                World::Pos3{ 16, 2, 2 },
                World::Pos3{ 2, 2, 2 },
                World::Pos3{ 2, 16, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ false,
        };

        // 0x0041FB20, 0x0041FC44, 0x0041FD68, 0x0041FE8C
        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeDownAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSteepSlopeDown3NE,
                kRightCurveSmallSteepSlopeDown3SE,
                kRightCurveSmallSteepSlopeDown3SW,
                kRightCurveSmallSteepSlopeDown3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
                World::Pos3{ 6, 2, 2 },
                World::Pos3{ 2, 6, 2 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
            },
            /* Mergable */ false,
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kRightCurveSmallSteepSlopeDownTPPA = {
            kRightCurveSmallSteepSlopeDownAddition0,
            kRightCurveSmallSteepSlopeDownAddition1,
            kRightCurveSmallSteepSlopeDownAddition2,
            kRightCurveSmallSteepSlopeDownAddition3,
        };

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeUpAddition0 = rotateTrackPPA(kRightCurveSmallSteepSlopeDownAddition3, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeUpAddition1 = rotateTrackPPA(kRightCurveSmallSteepSlopeDownAddition1, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeUpAddition2 = rotateTrackPPA(kRightCurveSmallSteepSlopeDownAddition2, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeUpAddition3 = rotateTrackPPA(kRightCurveSmallSteepSlopeDownAddition0, kRotationTable1230);

        constexpr std::array<TrackPaintAdditionPiece, 4> kLeftCurveSmallSteepSlopeUpTPPA = {
            kLeftCurveSmallSteepSlopeUpAddition0,
            kLeftCurveSmallSteepSlopeUpAddition1,
            kLeftCurveSmallSteepSlopeUpAddition2,
            kLeftCurveSmallSteepSlopeUpAddition3,
        };

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeDownAddition0 = rotateTrackPPA(kRightCurveSmallSteepSlopeUpAddition3, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeDownAddition1 = rotateTrackPPA(kRightCurveSmallSteepSlopeUpAddition1, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeDownAddition2 = rotateTrackPPA(kRightCurveSmallSteepSlopeUpAddition2, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeDownAddition3 = rotateTrackPPA(kRightCurveSmallSteepSlopeUpAddition0, kRotationTable1230);

        constexpr std::array<TrackPaintAdditionPiece, 4> kLeftCurveSmallSteepSlopeDownTPPA = {
            kLeftCurveSmallSteepSlopeDownAddition0,
            kLeftCurveSmallSteepSlopeDownAddition1,
            kLeftCurveSmallSteepSlopeDownAddition2,
            kLeftCurveSmallSteepSlopeDownAddition3,
        };

        constexpr std::array<std::span<const TrackPaintAdditionPiece>, 26> kTrackPaintAdditionParts = {
            kStraightTPPA,
            kDiagonalTPPA,
            kLeftCurveVerySmallTPPA,
            kRightCurveVerySmallTPPA,
            kLeftCurveSmallTPPA,
            kRightCurveSmallTPPA,
            kLeftCurveTPPA,
            kRightCurveTPPA,
            kLeftCurveLargeTPPA,
            kRightCurveLargeTPPA,
            kDiagonalLeftCurveLargeTPPA,
            kDiagonalRightCurveLargeTPPA,
            kSBendLeftTPPA,
            kSBendRightTPPA,
            kStraightSlopeUpTPPA,
            kStraightSlopeDownTPPA,
            kStraightSteepSlopeUpTPPA,
            kStraightSteepSlopeDownTPPA,
            kLeftCurveSmallSlopeUpTPPA,
            kRightCurveSmallSlopeUpTPPA,
            kLeftCurveSmallSlopeDownTPPA,
            kRightCurveSmallSlopeDownTPPA,
            kLeftCurveSmallSteepSlopeUpTPPA,
            kRightCurveSmallSteepSlopeUpTPPA,
            kLeftCurveSmallSteepSlopeDownTPPA,
            kRightCurveSmallSteepSlopeDownTPPA,
        };
    }
    namespace Style1
    {
        struct TrackAdditionSupport
        {
            std::array<std::array<uint32_t, 2>, 4> imageIds;
            int16_t height;
            std::array<uint8_t, 4> frequencies;   // Make array
            std::array<SegmentFlags, 4> segments; // Make array

            constexpr TrackAdditionSupport(
                const std::array<std::array<uint32_t, 2>, 4>& _imageIds,
                const int16_t _height,
                const std::array<uint8_t, 4>& _frequencies,
                const std::array<SegmentFlags, 4>& _segments)
                : imageIds(_imageIds)
                , height(_height)
                , frequencies(_frequencies)
                , segments(_segments)
            {
            }

            constexpr TrackAdditionSupport(
                const std::array<std::array<uint32_t, 2>, 4>& _imageIds,
                const int16_t _height,
                const uint8_t _frequency,
                const SegmentFlags _segment)
                : imageIds(_imageIds)
                , height(_height)
                , frequencies()
                , segments()
            {
                frequencies[0] = _frequency;
                frequencies[1] = rotl4bit(frequencies[0], 2);
                frequencies[2] = frequencies[0];
                frequencies[3] = frequencies[1];

                segments[0] = _segment;
                for (auto i = 1U; i < 4; ++i)
                {
                    segments[i] = rotlSegmentFlags(segments[0], i);
                }
            }
        };

        struct TrackPaintAdditionPiece
        {
            std::array<uint32_t, 4> imageIds;
            std::array<World::Pos3, 4> boundingBoxOffsets;
            std::array<World::Pos3, 4> boundingBoxSizes;
            bool isIsMergable;
            std::optional<TrackAdditionSupport> supports;
        };
        constexpr std::array<uint8_t, 4> kRotationTable1230 = { 1, 2, 3, 0 };
        constexpr std::array<uint8_t, 4> kRotationTable2301 = { 2, 3, 0, 1 };
        constexpr std::array<uint8_t, 4> kRotationTable3012 = { 3, 0, 1, 2 };

        constexpr TrackPaintAdditionPiece kNullTrackPaintAdditionPiece = {};
        constexpr auto kNoSupports = std::nullopt;

        consteval std::optional<TrackAdditionSupport> rotateTrackPPASupport(const std::optional<TrackAdditionSupport>& reference, const std::array<uint8_t, 4>& rotationTable)
        {
            if (!reference.has_value())
            {
                return std::nullopt;
            }
            return TrackAdditionSupport{
                std::array<std::array<uint32_t, 2>, 4>{
                    reference->imageIds[rotationTable[0]],
                    reference->imageIds[rotationTable[1]],
                    reference->imageIds[rotationTable[2]],
                    reference->imageIds[rotationTable[3]],
                },
                reference->height,
                std::array<uint8_t, 4>{
                    reference->frequencies[rotationTable[0]],
                    reference->frequencies[rotationTable[1]],
                    reference->frequencies[rotationTable[2]],
                    reference->frequencies[rotationTable[3]],
                },
                std::array<SegmentFlags, 4>{
                    reference->segments[rotationTable[0]],
                    reference->segments[rotationTable[1]],
                    reference->segments[rotationTable[2]],
                    reference->segments[rotationTable[3]],
                }
            };
        }

        consteval TrackPaintAdditionPiece rotateTrackPPA(const TrackPaintAdditionPiece& reference, const std::array<uint8_t, 4>& rotationTable)
        {
            return TrackPaintAdditionPiece{
                std::array<uint32_t, 4>{
                    reference.imageIds[rotationTable[0]],
                    reference.imageIds[rotationTable[1]],
                    reference.imageIds[rotationTable[2]],
                    reference.imageIds[rotationTable[3]],
                },
                std::array<World::Pos3, 4>{
                    reference.boundingBoxOffsets[rotationTable[0]],
                    reference.boundingBoxOffsets[rotationTable[1]],
                    reference.boundingBoxOffsets[rotationTable[2]],
                    reference.boundingBoxOffsets[rotationTable[3]],
                },
                std::array<World::Pos3, 4>{
                    reference.boundingBoxSizes[rotationTable[0]],
                    reference.boundingBoxSizes[rotationTable[1]],
                    reference.boundingBoxSizes[rotationTable[2]],
                    reference.boundingBoxSizes[rotationTable[3]],
                },
                reference.isIsMergable,
                rotateTrackPPASupport(reference.supports, rotationTable)
            };
        }

        using namespace OpenLoco::TrackExtraObj::ImageIds::Style1;

        // 0x00421E91, 0x00421F0D, 0x00421F89, 0x00422005
        constexpr TrackPaintAdditionPiece kStraightAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraight0NE,
                kStraight0SE,
                kStraight0NE,
                kStraight0SE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 5, 26 },
                World::Pos3{ 5, 2, 26 },
                World::Pos3{ 2, 5, 26 },
                World::Pos3{ 5, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 22, 1 },
                World::Pos3{ 22, 28, 1 },
                World::Pos3{ 28, 22, 1 },
                World::Pos3{ 22, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                },
                /* SupportHeight */ 0,
                /* Frequency */ 2,
                /* Segment */ SegmentFlags::x1y0,
            },
        };

        constexpr std::array<TrackPaintAdditionPiece, 1> kStraightTPPA = {
            kStraightAddition0,
        };

        // 0x00425229, 0x004252B9, 0x00425349, 0x004253D9
        constexpr TrackPaintAdditionPiece kDiagonalAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kDiagonal0NE,
                kDiagonal0SE,
                kDiagonal0SW,
                kDiagonal0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ kNoSupports,
        };

        constexpr TrackPaintAdditionPiece kDiagonalAddition1 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kDiagonalAddition2 = kNullTrackPaintAdditionPiece;

        // 0x00425272, 0x00425302, 0x00425392, 0x00425422
        constexpr TrackPaintAdditionPiece kDiagonalAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kDiagonal0SW,
                kDiagonal0NW,
                kDiagonal0NE,
                kDiagonal0SE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ kNoSupports,
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kDiagonalTPPA = {
            kDiagonalAddition0,
            kDiagonalAddition1,
            kDiagonalAddition2,
            kDiagonalAddition3,
        };

        // 0x004226E1, 0x00422728, 0x0042276F, 0x004227B6
        constexpr TrackPaintAdditionPiece kRightCurveVerySmallAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveVerySmall0NE,
                kRightCurveVerySmall0SE,
                kRightCurveVerySmall0SW,
                kRightCurveVerySmall0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ kNoSupports,
        };

        constexpr std::array<TrackPaintAdditionPiece, 1> kRightCurveVerySmallTPPA = {
            kRightCurveVerySmallAddition0,
        };

        constexpr TrackPaintAdditionPiece kLeftCurveVerySmallAddition0 = rotateTrackPPA(kRightCurveVerySmallAddition0, kRotationTable1230);

        constexpr std::array<TrackPaintAdditionPiece, 1> kLeftCurveVerySmallTPPA = {
            kLeftCurveVerySmallAddition0,
        };

        // 0x00422B89, 0x00422C83, 0x00422D7D, 0x00422E77
        constexpr TrackPaintAdditionPiece kRightCurveSmallAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmall0NE,
                kRightCurveSmall0SE,
                kRightCurveSmall0SW,
                kRightCurveSmall0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 26 },
                World::Pos3{ 6, 2, 26 },
                World::Pos3{ 2, 6, 26 },
                World::Pos3{ 6, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                },
                /* SupportHeight */ 0,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x1y0,
            },
        };

        constexpr TrackPaintAdditionPiece kRightCurveSmallAddition1 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kRightCurveSmallAddition2 = kNullTrackPaintAdditionPiece;

        // 0x00422C07, 0x00422D01, 0x00422DFB, 0x00422EF5
        constexpr TrackPaintAdditionPiece kRightCurveSmallAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmall3NE,
                kRightCurveSmall3SE,
                kRightCurveSmall3SW,
                kRightCurveSmall3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 6, 2, 26 },
                World::Pos3{ 2, 6, 26 },
                World::Pos3{ 6, 2, 26 },
                World::Pos3{ 2, 6, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
            },
            /* Mergable */ true,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                },
                /* SupportHeight */ 0,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x0y1,
            },
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kRightCurveSmallTPPA = {
            kRightCurveSmallAddition0,
            kRightCurveSmallAddition1,
            kRightCurveSmallAddition2,
            kRightCurveSmallAddition3,
        };

        constexpr TrackPaintAdditionPiece kLeftCurveSmallAddition0 = rotateTrackPPA(kRightCurveSmallAddition3, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallAddition1 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kLeftCurveSmallAddition2 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kLeftCurveSmallAddition3 = rotateTrackPPA(kRightCurveSmallAddition0, kRotationTable1230);

        constexpr std::array<TrackPaintAdditionPiece, 4> kLeftCurveSmallTPPA = {
            kLeftCurveSmallAddition0,
            kLeftCurveSmallAddition1,
            kLeftCurveSmallAddition2,
            kLeftCurveSmallAddition3,
        };

        // 0x00424209, 0x004243D6, 0x004245A3, 0x00424770
        constexpr TrackPaintAdditionPiece kRightCurveAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraight0NE,
                kStraight0SE,
                kStraight0NE,
                kStraight0SE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 26 },
                World::Pos3{ 6, 2, 26 },
                World::Pos3{ 2, 6, 26 },
                World::Pos3{ 6, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ kNoSupports,
        };

        // 0x00424250, 0x0042441D, 0x004245EA, 0x004247B7
        constexpr TrackPaintAdditionPiece kRightCurveAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurve1NE,
                kRightCurve1SE,
                kRightCurve1SW,
                kRightCurve1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 16, 26 },
                World::Pos3{ 16, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 14, 1 },
                World::Pos3{ 14, 28, 1 },
                World::Pos3{ 28, 14, 1 },
                World::Pos3{ 14, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportRightCurve1NE, kSupportConnectorRightCurve1NE },
                    std::array<uint32_t, 2>{ kSupportRightCurve1SE, kSupportConnectorRightCurve1SE },
                    std::array<uint32_t, 2>{ kSupportRightCurve1SW, kSupportConnectorRightCurve1SW },
                    std::array<uint32_t, 2>{ kSupportRightCurve1NW, kSupportConnectorRightCurve1NW },
                },
                /* SupportHeight */ 0,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x2y0,
            },
        };

        // 0x004242CC, 0x00424499, 0x00424666, 0x00424833
        constexpr TrackPaintAdditionPiece kRightCurveAddition2 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurve1SW,
                kRightCurve1NW,
                kRightCurve1NE,
                kRightCurve1SE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 16, 26 },
                World::Pos3{ 16, 16, 26 },
                World::Pos3{ 16, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
                World::Pos3{ 14, 14, 1 },
            },
            /* Mergable */ true,
            /* Supports */ kNoSupports,
        };

        // 0x00424313, 0x004244E0, 0x004246AD, 0x0042487A
        constexpr TrackPaintAdditionPiece kRightCurveAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurve1NE,
                kRightCurve1SE,
                kRightCurve1SW,
                kRightCurve1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 16, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 16, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 14, 28, 1 },
                World::Pos3{ 28, 14, 1 },
                World::Pos3{ 14, 28, 1 },
                World::Pos3{ 28, 14, 1 },
            },
            /* Mergable */ true,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportRightCurve3NE, kSupportConnectorRightCurve3NE },
                    std::array<uint32_t, 2>{ kSupportRightCurve3SE, kSupportConnectorRightCurve3SE },
                    std::array<uint32_t, 2>{ kSupportRightCurve3SW, kSupportConnectorRightCurve3SW },
                    std::array<uint32_t, 2>{ kSupportRightCurve3NW, kSupportConnectorRightCurve3NW },
                },
                /* SupportHeight */ 0,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x0y2,
            },
        };

        // 0x0042438F, 0x0042455C, 0x00424729, 0x004248F6
        constexpr TrackPaintAdditionPiece kRightCurveAddition4 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraight0SE,
                kStraight0NE,
                kStraight0SE,
                kStraight0NE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 6, 2, 26 },
                World::Pos3{ 2, 6, 26 },
                World::Pos3{ 6, 2, 26 },
                World::Pos3{ 2, 6, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
            },
            /* Mergable */ true,
            /* Supports */ kNoSupports,
        };

        constexpr std::array<TrackPaintAdditionPiece, 5> kRightCurveTPPA = {
            kRightCurveAddition0,
            kRightCurveAddition1,
            kRightCurveAddition2,
            kRightCurveAddition3,
            kRightCurveAddition4,
        };

        constexpr TrackPaintAdditionPiece kLeftCurveAddition0 = rotateTrackPPA(kRightCurveAddition4, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveAddition1 = rotateTrackPPA(kRightCurveAddition3, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveAddition2 = rotateTrackPPA(kRightCurveAddition2, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveAddition3 = rotateTrackPPA(kRightCurveAddition1, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveAddition4 = rotateTrackPPA(kRightCurveAddition0, kRotationTable1230);

        constexpr std::array<TrackPaintAdditionPiece, 5> kLeftCurveTPPA = {
            kLeftCurveAddition0,
            kLeftCurveAddition1,
            kLeftCurveAddition2,
            kLeftCurveAddition3,
            kLeftCurveAddition4,
        };

        // 0x00424DDD, 0x00424EE9, 0x00424FF5, 0x00425101
        constexpr TrackPaintAdditionPiece kRightCurveLargeAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraight0NE,
                kStraight0SE,
                kStraight0NE,
                kStraight0SE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 3, 26 },
                World::Pos3{ 3, 2, 26 },
                World::Pos3{ 2, 3, 26 },
                World::Pos3{ 3, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ kNoSupports,
        };

        // 0x00424E24, 0x00424F30, 0x0042503C, 0x00425148
        constexpr TrackPaintAdditionPiece kRightCurveLargeAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmall0NE,
                kRightCurveSmall0SE,
                kRightCurveSmall0SW,
                kRightCurveSmall0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 16, 26 },
                World::Pos3{ 16, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 14, 1 },
                World::Pos3{ 14, 28, 1 },
                World::Pos3{ 28, 14, 1 },
                World::Pos3{ 14, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                },
                /* SupportHeight */ 0,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x1y0,
            },
        };

        constexpr TrackPaintAdditionPiece kRightCurveLargeAddition2 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kRightCurveLargeAddition3 = kNullTrackPaintAdditionPiece;

        // 0x00424EA2, 0x00424FAE, 0x004250BA, 0x004251C6
        constexpr TrackPaintAdditionPiece kRightCurveLargeAddition4 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kDiagonal0SW,
                kDiagonal0NW,
                kDiagonal0NE,
                kDiagonal0SE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ kNoSupports,
        };

        constexpr std::array<TrackPaintAdditionPiece, 5> kRightCurveLargeTPPA = {
            kRightCurveLargeAddition0,
            kRightCurveLargeAddition1,
            kRightCurveLargeAddition2,
            kRightCurveLargeAddition3,
            kRightCurveLargeAddition4,
        };

        // 0x004249AD, 0x00424AB9, 0x00424BC5, 0x00424CD1
        constexpr TrackPaintAdditionPiece kLeftCurveLargeAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraight0NE,
                kStraight0SE,
                kStraight0NE,
                kStraight0SE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 3, 26 },
                World::Pos3{ 3, 2, 26 },
                World::Pos3{ 2, 3, 26 },
                World::Pos3{ 3, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ kNoSupports,
        };

        // 0x004249F4, 0x00424B00, 0x00424C0C, 0x00424D18
        constexpr TrackPaintAdditionPiece kLeftCurveLargeAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmall3SE,
                kRightCurveSmall3SW,
                kRightCurveSmall3NW,
                kRightCurveSmall3NE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 16, 26 },
                World::Pos3{ 16, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 14, 1 },
                World::Pos3{ 14, 28, 1 },
                World::Pos3{ 28, 14, 1 },
                World::Pos3{ 14, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                },
                /* SupportHeight */ 0,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x1y2,
            },
        };

        constexpr TrackPaintAdditionPiece kLeftCurveLargeAddition2 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kLeftCurveLargeAddition3 = kNullTrackPaintAdditionPiece;

        // 0x00424A72, 0x00424B7E, 0x00424C8A, 0x00424D96
        constexpr TrackPaintAdditionPiece kLeftCurveLargeAddition4 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kDiagonal0SE,
                kDiagonal0SW,
                kDiagonal0NW,
                kDiagonal0NE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
                World::Pos3{ 28, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ kNoSupports,
        };

        constexpr std::array<TrackPaintAdditionPiece, 5> kLeftCurveLargeTPPA = {
            kLeftCurveLargeAddition0,
            kLeftCurveLargeAddition1,
            kLeftCurveLargeAddition2,
            kLeftCurveLargeAddition3,
            kLeftCurveLargeAddition4,
        };

        constexpr TrackPaintAdditionPiece kDiagonalRightCurveLargeAddition0 = rotateTrackPPA(kLeftCurveLargeAddition4, kRotationTable3012);

        constexpr TrackPaintAdditionPiece kDiagonalRightCurveLargeAddition1 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kDiagonalRightCurveLargeAddition2 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kDiagonalRightCurveLargeAddition3 = rotateTrackPPA(kLeftCurveLargeAddition1, kRotationTable3012);

        constexpr TrackPaintAdditionPiece kDiagonalRightCurveLargeAddition4 = rotateTrackPPA(kLeftCurveLargeAddition0, kRotationTable3012);

        constexpr std::array<TrackPaintAdditionPiece, 5> kDiagonalRightCurveLargeTPPA = {
            kDiagonalRightCurveLargeAddition0,
            kDiagonalRightCurveLargeAddition1,
            kDiagonalRightCurveLargeAddition2,
            kDiagonalRightCurveLargeAddition3,
            kDiagonalRightCurveLargeAddition4,
        };

        constexpr TrackPaintAdditionPiece kDiagonalLeftCurveLargeAddition0 = rotateTrackPPA(kRightCurveLargeAddition4, kRotationTable2301);

        constexpr TrackPaintAdditionPiece kDiagonalLeftCurveLargeAddition1 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kDiagonalLeftCurveLargeAddition2 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kDiagonalLeftCurveLargeAddition3 = rotateTrackPPA(kRightCurveLargeAddition1, kRotationTable2301);

        constexpr TrackPaintAdditionPiece kDiagonalLeftCurveLargeAddition4 = rotateTrackPPA(kRightCurveLargeAddition0, kRotationTable2301);

        constexpr std::array<TrackPaintAdditionPiece, 5> kDiagonalLeftCurveLargeTPPA = {
            kDiagonalLeftCurveLargeAddition0,
            kDiagonalLeftCurveLargeAddition1,
            kDiagonalLeftCurveLargeAddition2,
            kDiagonalLeftCurveLargeAddition3,
            kDiagonalLeftCurveLargeAddition4,
        };

        // 0x00425B5D, 0x00425CE3, 0x00425FA8, 0x0042612E
        constexpr TrackPaintAdditionPiece kSBendLeftAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraight0NE,
                kStraight0SE,
                kStraight0NE,
                kStraight0SE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ kNoSupports,
        };

        // 0x00425BA4, 0x00425D2A, 0x00425F2C, 0x004260B2
        constexpr TrackPaintAdditionPiece kSBendLeftAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurve1SE,
                kRightCurve1SW,
                kRightCurve1NW,
                kRightCurve1NE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 6, 26 },
                World::Pos3{ 6, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 24, 1 },
                World::Pos3{ 24, 28, 1 },
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportRightCurve3SE, kSupportConnectorRightCurve3SE },
                    std::array<uint32_t, 2>{ kSupportRightCurve3SW, kSupportConnectorRightCurve3SW },
                    std::array<uint32_t, 2>{ kSupportRightCurve3NW, kSupportConnectorRightCurve3NW },
                    std::array<uint32_t, 2>{ kSupportRightCurve3NE, kSupportConnectorRightCurve3NE },
                },
                /* SupportHeight */ 0,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x2y2,
            },
        };

        // 0x00425C20, 0x00425DA6, 0x00425EB0, 0x00426036
        constexpr TrackPaintAdditionPiece kSBendLeftAddition2 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurve1NW,
                kRightCurve1NE,
                kRightCurve1SE,
                kRightCurve1SW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 26 },
                World::Pos3{ 6, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
                World::Pos3{ 28, 24, 1 },
                World::Pos3{ 24, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportRightCurve3NW, kSupportConnectorRightCurve3NW },
                    std::array<uint32_t, 2>{ kSupportRightCurve3NE, kSupportConnectorRightCurve3NE },
                    std::array<uint32_t, 2>{ kSupportRightCurve3SE, kSupportConnectorRightCurve3SE },
                    std::array<uint32_t, 2>{ kSupportRightCurve3SW, kSupportConnectorRightCurve3SW },
                },
                /* SupportHeight */ 0,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x0y0,
            },
        };

        // 0x00425C9C, 0x00425E22, 0x00425E69, 0x00425FEF
        constexpr TrackPaintAdditionPiece kSBendLeftAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraight0NE,
                kStraight0SE,
                kStraight0NE,
                kStraight0SE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ kNoSupports,
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kSBendLeftTPPA = {
            kSBendLeftAddition0,
            kSBendLeftAddition1,
            kSBendLeftAddition2,
            kSBendLeftAddition3,
        };

        // 0x00426175, 0x004262FB, 0x004265C0, 0x00426746
        constexpr TrackPaintAdditionPiece kSBendRightAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraight0NE,
                kStraight0SE,
                kStraight0NE,
                kStraight0SE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ kNoSupports,
        };

        // 0x004261BC, 0x00426342, 0x00426544, 0x004266CA
        constexpr TrackPaintAdditionPiece kSBendRightAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurve1NE,
                kRightCurve1SE,
                kRightCurve1SW,
                kRightCurve1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 26 },
                World::Pos3{ 6, 2, 26 },
                World::Pos3{ 2, 0, 26 },
                World::Pos3{ 2, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 24, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportRightCurve1NE, kSupportConnectorRightCurve1NE },
                    std::array<uint32_t, 2>{ kSupportRightCurve1SE, kSupportConnectorRightCurve1SE },
                    std::array<uint32_t, 2>{ kSupportRightCurve1SW, kSupportConnectorRightCurve1SW },
                    std::array<uint32_t, 2>{ kSupportRightCurve1NW, kSupportConnectorRightCurve1NW },
                },
                /* SupportHeight */ 0,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x2y0,
            },
        };

        // 0x00426238, 0x004263BE, 0x004264C8, 0x0042664E
        constexpr TrackPaintAdditionPiece kSBendRightAddition2 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurve1SW,
                kRightCurve1NW,
                kRightCurve1NE,
                kRightCurve1SE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 0, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 6, 26 },
                World::Pos3{ 6, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 24, 28, 1 },
                World::Pos3{ 28, 26, 1 },
                World::Pos3{ 26, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportRightCurve1SW, kSupportConnectorRightCurve1SW },
                    std::array<uint32_t, 2>{ kSupportRightCurve1NW, kSupportConnectorRightCurve1NW },
                    std::array<uint32_t, 2>{ kSupportRightCurve1NE, kSupportConnectorRightCurve1NE },
                    std::array<uint32_t, 2>{ kSupportRightCurve1SE, kSupportConnectorRightCurve1SE },
                },
                /* SupportHeight */ 0,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x0y2,
            },
        };

        // 0x004262B4, 0x0042643A, 0x00426481, 0x00426607
        constexpr TrackPaintAdditionPiece kSBendRightAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraight0NE,
                kStraight0SE,
                kStraight0NE,
                kStraight0SE,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
                World::Pos3{ 2, 2, 26 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
                World::Pos3{ 28, 27, 1 },
                World::Pos3{ 27, 28, 1 },
            },
            /* Mergable */ true,
            /* Supports */ kNoSupports,
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kSBendRightTPPA = {
            kSBendRightAddition0,
            kSBendRightAddition1,
            kSBendRightAddition2,
            kSBendRightAddition3,
        };

        // 0x004254A1, 0x004255B7, 0x004256CD, 0x004257E3
        constexpr TrackPaintAdditionPiece kStraightSlopeUpAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraightSlopeUp0NE,
                kStraightSlopeUp0SE,
                kStraightSlopeUp0SW,
                kStraightSlopeUp0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ false,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                },
                /* SupportHeight */ 4,
                /* Frequency */ 1,
                /* Segment */ SegmentFlags::x1y0,
            },
        };

        // 0x0042552C, 0x00425642, 0x00425758, 0x0042586E
        constexpr TrackPaintAdditionPiece kStraightSlopeUpAddition1 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraightSlopeUp1NE,
                kStraightSlopeUp1SE,
                kStraightSlopeUp1SW,
                kStraightSlopeUp1NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ false,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                },
                /* SupportHeight */ 12,
                /* Frequency */ 1,
                /* Segment */ SegmentFlags::x1y0,
            },
        };

        constexpr std::array<TrackPaintAdditionPiece, 2> kStraightSlopeUpTPPA = {
            kStraightSlopeUpAddition0,
            kStraightSlopeUpAddition1,
        };

        constexpr TrackPaintAdditionPiece kStraightSlopeDownAddition0 = rotateTrackPPA(kStraightSlopeUpAddition1, kRotationTable2301);

        constexpr TrackPaintAdditionPiece kStraightSlopeDownAddition1 = rotateTrackPPA(kStraightSlopeUpAddition0, kRotationTable2301);

        constexpr std::array<TrackPaintAdditionPiece, 2> kStraightSlopeDownTPPA = {
            kStraightSlopeDownAddition0,
            kStraightSlopeDownAddition1,
        };

        // 0x004258F9, 0x00425984, 0x00425A0F, 0x00425A9A
        constexpr TrackPaintAdditionPiece kStraightSteepSlopeUpAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kStraightSteepSlopeUp0NE,
                kStraightSteepSlopeUp0SE,
                kStraightSteepSlopeUp0SW,
                kStraightSteepSlopeUp0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ false,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                },
                /* SupportHeight */ 8,
                /* Frequency */ 2,
                /* Segment */ SegmentFlags::x1y0,
            },
        };

        constexpr std::array<TrackPaintAdditionPiece, 1> kStraightSteepSlopeUpTPPA = {
            kStraightSteepSlopeUpAddition0,
        };

        constexpr TrackPaintAdditionPiece kStraightSteepSlopeDownAddition0 = rotateTrackPPA(kStraightSteepSlopeUpAddition0, kRotationTable2301);

        constexpr std::array<TrackPaintAdditionPiece, 1> kStraightSteepSlopeDownTPPA = {
            kStraightSteepSlopeDownAddition0,
        };

        // 0x00422FE1, 0x004230F9, 0x00423211, 0x00423329
        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeUpAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSlopeUp0NE,
                kRightCurveSmallSlopeUp0SE,
                kRightCurveSmallSlopeUp0SW,
                kRightCurveSmallSlopeUp0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ false,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                },
                /* SupportHeight */ 4,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x1y0,
            },
        };

        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeUpAddition1 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeUpAddition2 = kNullTrackPaintAdditionPiece;

        // 0x0042306E, 0x00423186, 0x0042329E, 0x004233B6
        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeUpAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSlopeUp3NE,
                kRightCurveSmallSlopeUp3SE,
                kRightCurveSmallSlopeUp3SW,
                kRightCurveSmallSlopeUp3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
            },
            /* Mergable */ false,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                },
                /* SupportHeight */ 12,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x0y1,
            },
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kRightCurveSmallSlopeUpTPPA = {
            kRightCurveSmallSlopeUpAddition0,
            kRightCurveSmallSlopeUpAddition1,
            kRightCurveSmallSlopeUpAddition2,
            kRightCurveSmallSlopeUpAddition3,
        };

        // 0x00423441, 0x00423559, 0x00423671, 0x00423789
        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeDownAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSlopeDown0NE,
                kRightCurveSmallSlopeDown0SE,
                kRightCurveSmallSlopeDown0SW,
                kRightCurveSmallSlopeDown0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ false,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                },
                /* SupportHeight */ 12,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x1y0,
            },
        };

        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeDownAddition1 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeDownAddition2 = kNullTrackPaintAdditionPiece;

        // 0x004234CE, 0x004235E6, 0x004236FE, 0x00423816
        constexpr TrackPaintAdditionPiece kRightCurveSmallSlopeDownAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSlopeDown3NE,
                kRightCurveSmallSlopeDown3SE,
                kRightCurveSmallSlopeDown3SW,
                kRightCurveSmallSlopeDown3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
            },
            /* Mergable */ false,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                },
                /* SupportHeight */ 4,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x0y1,
            },
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kRightCurveSmallSlopeDownTPPA = {
            kRightCurveSmallSlopeDownAddition0,
            kRightCurveSmallSlopeDownAddition1,
            kRightCurveSmallSlopeDownAddition2,
            kRightCurveSmallSlopeDownAddition3,
        };

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeUpAddition0 = rotateTrackPPA(kRightCurveSmallSlopeDownAddition3, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeUpAddition1 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeUpAddition2 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeUpAddition3 = rotateTrackPPA(kRightCurveSmallSlopeDownAddition0, kRotationTable1230);

        constexpr std::array<TrackPaintAdditionPiece, 4> kLeftCurveSmallSlopeUpTPPA = {
            kLeftCurveSmallSlopeUpAddition0,
            kLeftCurveSmallSlopeUpAddition1,
            kLeftCurveSmallSlopeUpAddition2,
            kLeftCurveSmallSlopeUpAddition3,
        };

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeDownAddition0 = rotateTrackPPA(kRightCurveSmallSlopeUpAddition3, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeDownAddition1 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeDownAddition2 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSlopeDownAddition3 = rotateTrackPPA(kRightCurveSmallSlopeUpAddition0, kRotationTable1230);

        constexpr std::array<TrackPaintAdditionPiece, 4> kLeftCurveSmallSlopeDownTPPA = {
            kLeftCurveSmallSlopeDownAddition0,
            kLeftCurveSmallSlopeDownAddition1,
            kLeftCurveSmallSlopeDownAddition2,
            kLeftCurveSmallSlopeDownAddition3,
        };

        // 0x00423911, 0x00423A29, 0x00423B41, 0x00423C59
        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeUpAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSteepSlopeUp0NE,
                kRightCurveSmallSteepSlopeUp0SE,
                kRightCurveSmallSteepSlopeUp0SW,
                kRightCurveSmallSteepSlopeUp0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ false,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                },
                /* SupportHeight */ 8,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x1y0,
            },
        };

        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeUpAddition1 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeUpAddition2 = kNullTrackPaintAdditionPiece;

        // 0x0042399E, 0x00423AB6, 0x00423BCE, 0x00423CE6
        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeUpAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSteepSlopeUp3NE,
                kRightCurveSmallSteepSlopeUp3SE,
                kRightCurveSmallSteepSlopeUp3SW,
                kRightCurveSmallSteepSlopeUp3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
            },
            /* Mergable */ false,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                },
                /* SupportHeight */ 8,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x0y1,
            },
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kRightCurveSmallSteepSlopeUpTPPA = {
            kRightCurveSmallSteepSlopeUpAddition0,
            kRightCurveSmallSteepSlopeUpAddition1,
            kRightCurveSmallSteepSlopeUpAddition2,
            kRightCurveSmallSteepSlopeUpAddition3,
        };

        // 0x00423D71, 0x00423E89, 0x00423FA1, 0x004240B9
        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeDownAddition0 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSteepSlopeDown0NE,
                kRightCurveSmallSteepSlopeDown0SE,
                kRightCurveSmallSteepSlopeDown0SW,
                kRightCurveSmallSteepSlopeDown0NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
            },
            /* Mergable */ false,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                },
                /* SupportHeight */ 8,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x1y0,
            },
        };

        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeDownAddition1 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeDownAddition2 = kNullTrackPaintAdditionPiece;

        // 0x00423DFE, 0x00423F16, 0x0042402E, 0x00424146
        constexpr TrackPaintAdditionPiece kRightCurveSmallSteepSlopeDownAddition3 = {
            /* ImageIds */ std::array<uint32_t, 4>{
                kRightCurveSmallSteepSlopeDown3NE,
                kRightCurveSmallSteepSlopeDown3SE,
                kRightCurveSmallSteepSlopeDown3SW,
                kRightCurveSmallSteepSlopeDown3NW,
            },
            /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
                World::Pos3{ 6, 2, 34 },
                World::Pos3{ 2, 6, 34 },
            },
            /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
                World::Pos3{ 20, 28, 1 },
                World::Pos3{ 28, 20, 1 },
            },
            /* Mergable */ false,
            /* Supports */ TrackAdditionSupport{
                /* ImageIds */ std::array<std::array<uint32_t, 2>, 4>{
                    std::array<uint32_t, 2>{ kSupportStraight0SE, kSupportConnectorStraight0SE },
                    std::array<uint32_t, 2>{ kSupportStraight0SW, kSupportConnectorStraight0SW },
                    std::array<uint32_t, 2>{ kSupportStraight0NW, kSupportConnectorStraight0NW },
                    std::array<uint32_t, 2>{ kSupportStraight0NE, kSupportConnectorStraight0NE },
                },
                /* SupportHeight */ 8,
                /* Frequency */ 0,
                /* Segment */ SegmentFlags::x0y1,
            },
        };

        constexpr std::array<TrackPaintAdditionPiece, 4> kRightCurveSmallSteepSlopeDownTPPA = {
            kRightCurveSmallSteepSlopeDownAddition0,
            kRightCurveSmallSteepSlopeDownAddition1,
            kRightCurveSmallSteepSlopeDownAddition2,
            kRightCurveSmallSteepSlopeDownAddition3,
        };

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeUpAddition0 = rotateTrackPPA(kRightCurveSmallSteepSlopeDownAddition3, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeUpAddition1 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeUpAddition2 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeUpAddition3 = rotateTrackPPA(kRightCurveSmallSteepSlopeDownAddition0, kRotationTable1230);

        constexpr std::array<TrackPaintAdditionPiece, 4> kLeftCurveSmallSteepSlopeUpTPPA = {
            kLeftCurveSmallSteepSlopeUpAddition0,
            kLeftCurveSmallSteepSlopeUpAddition1,
            kLeftCurveSmallSteepSlopeUpAddition2,
            kLeftCurveSmallSteepSlopeUpAddition3,
        };

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeDownAddition0 = rotateTrackPPA(kRightCurveSmallSteepSlopeUpAddition3, kRotationTable1230);

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeDownAddition1 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeDownAddition2 = kNullTrackPaintAdditionPiece;

        constexpr TrackPaintAdditionPiece kLeftCurveSmallSteepSlopeDownAddition3 = rotateTrackPPA(kRightCurveSmallSteepSlopeUpAddition0, kRotationTable1230);

        constexpr std::array<TrackPaintAdditionPiece, 4> kLeftCurveSmallSteepSlopeDownTPPA = {
            kLeftCurveSmallSteepSlopeDownAddition0,
            kLeftCurveSmallSteepSlopeDownAddition1,
            kLeftCurveSmallSteepSlopeDownAddition2,
            kLeftCurveSmallSteepSlopeDownAddition3,
        };

        constexpr std::array<std::span<const TrackPaintAdditionPiece>, 26> kTrackPaintAdditionParts = {
            kStraightTPPA,
            kDiagonalTPPA,
            kLeftCurveVerySmallTPPA,
            kRightCurveVerySmallTPPA,
            kLeftCurveSmallTPPA,
            kRightCurveSmallTPPA,
            kLeftCurveTPPA,
            kRightCurveTPPA,
            kLeftCurveLargeTPPA,
            kRightCurveLargeTPPA,
            kDiagonalLeftCurveLargeTPPA,
            kDiagonalRightCurveLargeTPPA,
            kSBendLeftTPPA,
            kSBendRightTPPA,
            kStraightSlopeUpTPPA,
            kStraightSlopeDownTPPA,
            kStraightSteepSlopeUpTPPA,
            kStraightSteepSlopeDownTPPA,
            kLeftCurveSmallSlopeUpTPPA,
            kRightCurveSmallSlopeUpTPPA,
            kLeftCurveSmallSlopeDownTPPA,
            kRightCurveSmallSlopeDownTPPA,
            kLeftCurveSmallSteepSlopeUpTPPA,
            kRightCurveSmallSteepSlopeUpTPPA,
            kLeftCurveSmallSteepSlopeDownTPPA,
            kRightCurveSmallSteepSlopeDownTPPA,
        };
    }
}
