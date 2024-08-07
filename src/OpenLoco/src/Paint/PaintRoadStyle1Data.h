#include "Objects/RoadObject.h"
#include "Paint.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <array>
#include <span>

namespace OpenLoco::Paint::Style1
{
    struct TrackPaintPiece
    {
    private:
        constexpr void rotateTunnelHeights()
        {
            tunnelHeights[1][0] = tunnelHeights[0][3];
            tunnelHeights[1][1] = tunnelHeights[0][0];
            tunnelHeights[1][2] = tunnelHeights[0][1];
            tunnelHeights[1][3] = tunnelHeights[0][2];

            tunnelHeights[2][0] = tunnelHeights[0][2];
            tunnelHeights[2][1] = tunnelHeights[0][3];
            tunnelHeights[2][2] = tunnelHeights[0][0];
            tunnelHeights[2][3] = tunnelHeights[0][1];

            tunnelHeights[3][0] = tunnelHeights[0][1];
            tunnelHeights[3][1] = tunnelHeights[0][2];
            tunnelHeights[3][2] = tunnelHeights[0][3];
            tunnelHeights[3][3] = tunnelHeights[0][0];
        }
        constexpr void rotateBridgeEdgesQuarters()
        {
            for (auto i = 1; i < 4; ++i)
            {
                bridgeEdges[i] = Numerics::rotl4bit(bridgeEdges[0], i);
            }
            for (auto i = 1; i < 4; ++i)
            {
                bridgeQuarters[i] = Numerics::rotl4bit(bridgeQuarters[0], i);
            }
        }
        constexpr void rotateSegements()
        {
            for (auto i = 1; i < 4; ++i)
            {
                segments[i] = rotlSegmentFlags(segments[0], i);
            }
        }

    public:
        constexpr TrackPaintPiece(
            const std::array<std::array<uint32_t, 3>, 4>& _imageIndexOffsets,
            const std::array<World::Pos3, 4>& _boundingBoxOffsets,
            const std::array<World::Pos3, 4>& _boundingBoxSizes,
            const std::array<uint8_t, 4>& _bridgeEdges,
            const std::array<uint8_t, 4>& _bridgeQuarters,
            const std::array<uint8_t, 4>& _bridgeType,
            const std::array<int16_t, 4>& _tunnelHeights,
            const std::array<SegmentFlags, 4>& _segments)
            : imageIndexOffsets(_imageIndexOffsets)
            , boundingBoxOffsets(_boundingBoxOffsets)
            , boundingBoxSizes(_boundingBoxSizes)
            , bridgeEdges(_bridgeEdges)
            , bridgeQuarters(_bridgeQuarters)
            , bridgeType(_bridgeType)
            , segments(_segments)
        {
            tunnelHeights = {};
            tunnelHeights[0] = _tunnelHeights;
            rotateTunnelHeights();
        }
        constexpr TrackPaintPiece(
            const std::array<std::array<uint32_t, 3>, 4>& _imageIndexOffsets,
            const std::array<World::Pos3, 4>& _boundingBoxOffsets,
            const std::array<World::Pos3, 4>& _boundingBoxSizes,
            uint8_t _bridgeEdges,
            uint8_t _bridgeQuarters,
            const std::array<uint8_t, 4>& _bridgeType,
            const std::array<int16_t, 4>& _tunnelHeights,
            SegmentFlags _segments)
            : imageIndexOffsets(_imageIndexOffsets)
            , boundingBoxOffsets(_boundingBoxOffsets)
            , boundingBoxSizes(_boundingBoxSizes)
            , bridgeEdges()
            , bridgeQuarters()
            , bridgeType(_bridgeType)
            , tunnelHeights()
            , segments()
        {
            tunnelHeights[0] = _tunnelHeights;
            bridgeEdges[0] = _bridgeEdges;
            bridgeQuarters[0] = _bridgeQuarters;
            segments[0] = _segments;
            rotateTunnelHeights();
            rotateBridgeEdgesQuarters();
            rotateSegements();
        }

        std::array<std::array<uint32_t, 3>, 4> imageIndexOffsets;
        std::array<World::Pos3, 4> boundingBoxOffsets;
        std::array<World::Pos3, 4> boundingBoxSizes;
        std::array<uint8_t, 4> bridgeEdges;
        std::array<uint8_t, 4> bridgeQuarters;
        std::array<uint8_t, 4> bridgeType;
        std::array<std::array<int16_t, 4>, 4> tunnelHeights;
        std::array<SegmentFlags, 4> segments;
    };

    constexpr int16_t kNoTunnel = -1;
    constexpr std::array<int16_t, 4> kNoTunnels = { kNoTunnel, kNoTunnel, kNoTunnel, kNoTunnel };
    constexpr std::array<uint8_t, 4> kFlatBridge = { 0, 0, 0, 0 };
    constexpr std::array<uint8_t, 4> kRotationTable1230 = { 1, 2, 3, 0 };
    constexpr std::array<uint8_t, 4> kRotationTable2301 = { 2, 3, 0, 1 };
    constexpr std::array<uint8_t, 4> kRotationTable3012 = { 3, 0, 1, 2 };

    consteval TrackPaintPiece rotateTrackPP(const TrackPaintPiece& reference, const std::array<uint8_t, 4>& rotationTable)
    {
        return TrackPaintPiece{
            std::array<std::array<uint32_t, 3>, 4>{
                reference.imageIndexOffsets[rotationTable[0]],
                reference.imageIndexOffsets[rotationTable[1]],
                reference.imageIndexOffsets[rotationTable[2]],
                reference.imageIndexOffsets[rotationTable[3]],
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
            std::array<uint8_t, 4>{
                reference.bridgeEdges[rotationTable[0]],
                reference.bridgeEdges[rotationTable[1]],
                reference.bridgeEdges[rotationTable[2]],
                reference.bridgeEdges[rotationTable[3]],
            },
            std::array<uint8_t, 4>{
                reference.bridgeQuarters[rotationTable[0]],
                reference.bridgeQuarters[rotationTable[1]],
                reference.bridgeQuarters[rotationTable[2]],
                reference.bridgeQuarters[rotationTable[3]],
            },
            std::array<uint8_t, 4>{
                reference.bridgeType[rotationTable[0]],
                reference.bridgeType[rotationTable[1]],
                reference.bridgeType[rotationTable[2]],
                reference.bridgeType[rotationTable[3]],
            },
            std::array<int16_t, 4>{
                reference.tunnelHeights[0][rotationTable[0]],
                reference.tunnelHeights[0][rotationTable[1]],
                reference.tunnelHeights[0][rotationTable[2]],
                reference.tunnelHeights[0][rotationTable[3]],
            },
            std::array<SegmentFlags, 4>{
                reference.segments[rotationTable[0]],
                reference.segments[rotationTable[1]],
                reference.segments[rotationTable[2]],
                reference.segments[rotationTable[3]],
            }
        };
    }

    // 0x0040D9AD, 0x0040DADE, 0x0040D9AD, 0x0040DADE
    constexpr TrackPaintPiece kStraight0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraight0BallastNE, RoadObj::ImageIds::Style1::kStraight0SleeperNE, RoadObj::ImageIds::Style1::kStraight0RailNE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraight0BallastSE, RoadObj::ImageIds::Style1::kStraight0SleeperSE, RoadObj::ImageIds::Style1::kStraight0RailSE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraight0BallastNE, RoadObj::ImageIds::Style1::kStraight0SleeperNE, RoadObj::ImageIds::Style1::kStraight0RailNE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraight0BallastSE, RoadObj::ImageIds::Style1::kStraight0SleeperSE, RoadObj::ImageIds::Style1::kStraight0RailSE },
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
        /* BridgeEdges */ 0b0101,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ std::array<int16_t, 4>{
            0,
            kNoTunnel,
            0,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x0y0 | SegmentFlags::x2y0 | SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1,
    };

    constexpr std::array<TrackPaintPiece, 1> kStraightTPP = {
        kStraight0,
    };

    // 0x0040DC0F, 0x0040DD46, 0x0040DE7D, 0x0040DFB2
    constexpr TrackPaintPiece kRightCurveVerySmall0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveVerySmall0BallastNE, RoadObj::ImageIds::Style1::kRightCurveVerySmall0SleeperNE, RoadObj::ImageIds::Style1::kRightCurveVerySmall0RailNE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveVerySmall0BallastSE, RoadObj::ImageIds::Style1::kRightCurveVerySmall0SleeperSE, RoadObj::ImageIds::Style1::kRightCurveVerySmall0RailSE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveVerySmall0BallastSW, RoadObj::ImageIds::Style1::kRightCurveVerySmall0SleeperSW, RoadObj::ImageIds::Style1::kRightCurveVerySmall0RailSW },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveVerySmall0BallastNW, RoadObj::ImageIds::Style1::kRightCurveVerySmall0SleeperNW, RoadObj::ImageIds::Style1::kRightCurveVerySmall0RailNW },
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
        /* BridgeEdges */ 0b0110,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            0,
            0,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    constexpr std::array<TrackPaintPiece, 1> kRightCurveVerySmallTPP = {
        kRightCurveVerySmall0,
    };

    constexpr TrackPaintPiece kLeftCurveVerySmall0 = rotateTrackPP(kRightCurveVerySmall0, kRotationTable1230);

    constexpr std::array<TrackPaintPiece, 1> kLeftCurveVerySmallTPP = {
        kLeftCurveVerySmall0,
    };

    // 0x0040E545, 0x0040E8E7, 0x0040EC8B, 0x0040F02D
    constexpr TrackPaintPiece kRightCurveSmall0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall0BallastNE, RoadObj::ImageIds::Style1::kRightCurveSmall0SleeperNE, RoadObj::ImageIds::Style1::kRightCurveSmall0RailNE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall0BallastSE, RoadObj::ImageIds::Style1::kRightCurveSmall0SleeperSE, RoadObj::ImageIds::Style1::kRightCurveSmall0RailSE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall0BallastSW, RoadObj::ImageIds::Style1::kRightCurveSmall0SleeperSW, RoadObj::ImageIds::Style1::kRightCurveSmall0RailSW },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall0BallastNW, RoadObj::ImageIds::Style1::kRightCurveSmall0SleeperNW, RoadObj::ImageIds::Style1::kRightCurveSmall0RailNW },
        },
        /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
            World::Pos3{ 2, 6, 0 },
            World::Pos3{ 6, 2, 0 },
            World::Pos3{ 2, 2, 0 },
            World::Pos3{ 0, 2, 0 },
        },
        /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
            World::Pos3{ 28, 20, 1 },
            World::Pos3{ 20, 28, 1 },
            World::Pos3{ 28, 28, 1 },
            World::Pos3{ 30, 28, 1 },
        },
        /* BridgeEdges */ 0b0111,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            kNoTunnel,
            0,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x2y0 | SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x0040E646, 0x0040E9E8, 0x0040ED8C, 0x0040F12E
    constexpr TrackPaintPiece kRightCurveSmall1 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall1BallastNE, RoadObj::ImageIds::Style1::kRightCurveSmall1SleeperNE, RoadObj::ImageIds::Style1::kRightCurveSmall1RailNE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall1BallastSE, RoadObj::ImageIds::Style1::kRightCurveSmall1SleeperSE, RoadObj::ImageIds::Style1::kRightCurveSmall1RailSE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall1BallastSW, RoadObj::ImageIds::Style1::kRightCurveSmall1SleeperSW, RoadObj::ImageIds::Style1::kRightCurveSmall1RailSW },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall1BallastNW, RoadObj::ImageIds::Style1::kRightCurveSmall1SleeperNW, RoadObj::ImageIds::Style1::kRightCurveSmall1RailNW },
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
        /* BridgeEdges */ 0b1001,
        /* BridgeQuarters */ 0b1000,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x0y0 | SegmentFlags::x1y0 | SegmentFlags::x0y1,
    };

    // 0x0040E715, 0x0040EAB9, 0x0040EE5D, 0x0040F1FF
    constexpr TrackPaintPiece kRightCurveSmall2 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall2BallastNE, RoadObj::ImageIds::Style1::kRightCurveSmall2SleeperNE, RoadObj::ImageIds::Style1::kRightCurveSmall2RailNE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall2BallastSE, RoadObj::ImageIds::Style1::kRightCurveSmall2SleeperSE, RoadObj::ImageIds::Style1::kRightCurveSmall2RailSE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall2BallastSW, RoadObj::ImageIds::Style1::kRightCurveSmall2SleeperSW, RoadObj::ImageIds::Style1::kRightCurveSmall2RailSW },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall2BallastNW, RoadObj::ImageIds::Style1::kRightCurveSmall2SleeperNW, RoadObj::ImageIds::Style1::kRightCurveSmall2RailNW },
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
        /* BridgeEdges */ 0b0110,
        /* BridgeQuarters */ 0b0010,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x0040E7E6, 0x0040EB8A, 0x0040EF2C, 0x0040F2D0
    constexpr TrackPaintPiece kRightCurveSmall3 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall3BallastNE, RoadObj::ImageIds::Style1::kRightCurveSmall3SleeperNE, RoadObj::ImageIds::Style1::kRightCurveSmall3RailNE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall3BallastSE, RoadObj::ImageIds::Style1::kRightCurveSmall3SleeperSE, RoadObj::ImageIds::Style1::kRightCurveSmall3RailSE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall3BallastSW, RoadObj::ImageIds::Style1::kRightCurveSmall3SleeperSW, RoadObj::ImageIds::Style1::kRightCurveSmall3RailSW },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kRightCurveSmall3BallastNW, RoadObj::ImageIds::Style1::kRightCurveSmall3SleeperNW, RoadObj::ImageIds::Style1::kRightCurveSmall3RailNW },
        },
        /* BoundingBoxOffsets */ std::array<World::Pos3, 4>{
            World::Pos3{ 6, 2, 0 },
            World::Pos3{ 2, 0, 0 },
            World::Pos3{ 2, 2, 0 },
            World::Pos3{ 2, 6, 0 },
        },
        /* BoundingBoxSizes */ std::array<World::Pos3, 4>{
            World::Pos3{ 20, 28, 1 },
            World::Pos3{ 28, 30, 1 },
            World::Pos3{ 28, 28, 1 },
            World::Pos3{ 28, 20, 1 },
        },
        /* BridgeEdges */ 0b1110,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            0,
            kNoTunnel,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x2y0 | SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    constexpr std::array<TrackPaintPiece, 4> kRightCurveSmallTPP = {
        kRightCurveSmall0,
        kRightCurveSmall1,
        kRightCurveSmall2,
        kRightCurveSmall3,
    };

    constexpr TrackPaintPiece kLeftCurveSmall0 = rotateTrackPP(kRightCurveSmall3, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmall1 = rotateTrackPP(kRightCurveSmall1, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmall2 = rotateTrackPP(kRightCurveSmall2, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmall3 = rotateTrackPP(kRightCurveSmall0, kRotationTable1230);

    constexpr std::array<TrackPaintPiece, 4> kLeftCurveSmallTPP = {
        kLeftCurveSmall0,
        kLeftCurveSmall1,
        kLeftCurveSmall2,
        kLeftCurveSmall3,
    };

    // 0x0040F409, 0x0040F61F, 0x0040F835, 0x0040FA4B
    constexpr TrackPaintPiece kStraightSlopeUp0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraightSlopeUp0BallastNE, RoadObj::ImageIds::Style1::kStraightSlopeUp0SleeperNE, RoadObj::ImageIds::Style1::kStraightSlopeUp0RailNE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraightSlopeUp0BallastSE, RoadObj::ImageIds::Style1::kStraightSlopeUp0SleeperSE, RoadObj::ImageIds::Style1::kStraightSlopeUp0RailSE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraightSlopeUp0BallastSW, RoadObj::ImageIds::Style1::kStraightSlopeUp0SleeperSW, RoadObj::ImageIds::Style1::kStraightSlopeUp0RailSW },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraightSlopeUp0BallastNW, RoadObj::ImageIds::Style1::kStraightSlopeUp0SleeperNW, RoadObj::ImageIds::Style1::kStraightSlopeUp0RailNW },
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
        /* BridgeEdges */ 0b0101,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ std::array<uint8_t, 4>{
            1,
            3,
            5,
            7,
        },
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            kNoTunnel,
            0,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x0y0 | SegmentFlags::x2y0 | SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1,
    };

    // 0x0040F512, 0x0040F728, 0x0040F93E, 0x0040FB54
    constexpr TrackPaintPiece kStraightSlopeUp1 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraightSlopeUp1BallastNE, RoadObj::ImageIds::Style1::kStraightSlopeUp1SleeperNE, RoadObj::ImageIds::Style1::kStraightSlopeUp1RailNE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraightSlopeUp1BallastSE, RoadObj::ImageIds::Style1::kStraightSlopeUp1SleeperSE, RoadObj::ImageIds::Style1::kStraightSlopeUp1RailSE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraightSlopeUp1BallastSW, RoadObj::ImageIds::Style1::kStraightSlopeUp1SleeperSW, RoadObj::ImageIds::Style1::kStraightSlopeUp1RailSW },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraightSlopeUp1BallastNW, RoadObj::ImageIds::Style1::kStraightSlopeUp1SleeperNW, RoadObj::ImageIds::Style1::kStraightSlopeUp1RailNW },
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
        /* BridgeEdges */ 0b0101,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ std::array<uint8_t, 4>{
            2,
            4,
            6,
            8,
        },
        /* TunnelHeights */ std::array<int16_t, 4>{
            16,
            kNoTunnel,
            kNoTunnel,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x0y0 | SegmentFlags::x2y0 | SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1,
    };

    constexpr std::array<TrackPaintPiece, 2> kStraightSlopeUpTPP = {
        kStraightSlopeUp0,
        kStraightSlopeUp1,
    };

    constexpr TrackPaintPiece kStraightSlopeDown0 = rotateTrackPP(kStraightSlopeUp1, kRotationTable2301);

    constexpr TrackPaintPiece kStraightSlopeDown1 = rotateTrackPP(kStraightSlopeUp0, kRotationTable2301);

    constexpr std::array<TrackPaintPiece, 2> kStraightSlopeDownTPP = {
        kStraightSlopeDown0,
        kStraightSlopeDown1,
    };

    // 0x0040FC61, 0x0040FD9E, 0x0040FEDB, 0x00410018
    constexpr TrackPaintPiece kStraightSteepSlopeUp0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraightSteepSlopeUp0BallastNE, RoadObj::ImageIds::Style1::kStraightSteepSlopeUp0SleeperNE, RoadObj::ImageIds::Style1::kStraightSteepSlopeUp0RailNE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraightSteepSlopeUp0BallastSE, RoadObj::ImageIds::Style1::kStraightSteepSlopeUp0SleeperSE, RoadObj::ImageIds::Style1::kStraightSteepSlopeUp0RailSE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraightSteepSlopeUp0BallastSW, RoadObj::ImageIds::Style1::kStraightSteepSlopeUp0SleeperSW, RoadObj::ImageIds::Style1::kStraightSteepSlopeUp0RailSW },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kStraightSteepSlopeUp0BallastNW, RoadObj::ImageIds::Style1::kStraightSteepSlopeUp0SleeperNW, RoadObj::ImageIds::Style1::kStraightSteepSlopeUp0RailNW },
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
        /* BridgeEdges */ 0b0101,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ std::array<uint8_t, 4>{
            9,
            10,
            11,
            12,
        },
        /* TunnelHeights */ std::array<int16_t, 4>{
            16,
            kNoTunnel,
            0,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x0y0 | SegmentFlags::x2y0 | SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1,
    };

    constexpr std::array<TrackPaintPiece, 1> kStraightSteepSlopeUpTPP = {
        kStraightSteepSlopeUp0,
    };

    constexpr TrackPaintPiece kStraightSteepSlopeDown0 = rotateTrackPP(kStraightSteepSlopeUp0, kRotationTable2301);

    constexpr std::array<TrackPaintPiece, 1> kStraightSteepSlopeDownTPP = {
        kStraightSteepSlopeDown0,
    };

    // 0x0040E0E9, 0x0040E1F3, 0x0040E2FB, 0x0040E403
    constexpr TrackPaintPiece kTurnaround0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kTurnaround0BallastNE, RoadObj::ImageIds::Style1::kTurnaround0SleeperNE, RoadObj::ImageIds::Style1::kTurnaround0RailNE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kTurnaround0BallastSE, RoadObj::ImageIds::Style1::kTurnaround0SleeperSE, RoadObj::ImageIds::Style1::kTurnaround0RailSE },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kTurnaround0BallastSW, RoadObj::ImageIds::Style1::kTurnaround0SleeperSW, RoadObj::ImageIds::Style1::kTurnaround0RailSW },
            std::array<uint32_t, 3>{ RoadObj::ImageIds::Style1::kTurnaround0BallastNW, RoadObj::ImageIds::Style1::kTurnaround0SleeperNW, RoadObj::ImageIds::Style1::kTurnaround0RailNW },
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
        /* BridgeEdges */ 0b0100,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            kNoTunnel,
            0,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x2y0 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x2y1,
    };

    constexpr std::array<TrackPaintPiece, 1> kTurnaroundTPP = {
        kTurnaround0,
    };

    constexpr std::array<std::span<const TrackPaintPiece>, 10> kTrackPaintParts = {
        kStraightTPP,
        kLeftCurveVerySmallTPP,
        kRightCurveVerySmallTPP,
        kLeftCurveSmallTPP,
        kRightCurveSmallTPP,
        kStraightSlopeUpTPP,
        kStraightSlopeDownTPP,
        kStraightSteepSlopeUpTPP,
        kStraightSteepSlopeDownTPP,
        kTurnaroundTPP,
    };
}
