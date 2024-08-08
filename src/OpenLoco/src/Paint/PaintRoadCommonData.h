#pragma once
#include "Paint.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <array>
#include <span>

namespace OpenLoco::Paint
{
    struct RoadPaintCommonPiece
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
        constexpr RoadPaintCommonPiece(
            const std::array<World::Pos3, 4>& _boundingBoxOffsets,
            const std::array<World::Pos3, 4>& _boundingBoxSizes,
            const std::array<uint8_t, 4>& _bridgeEdges,
            const std::array<uint8_t, 4>& _bridgeQuarters,
            const std::array<uint8_t, 4>& _bridgeType,
            const std::array<int16_t, 4>& _tunnelHeights,
            const std::array<SegmentFlags, 4>& _segments)
            : boundingBoxOffsets(_boundingBoxOffsets)
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
        constexpr RoadPaintCommonPiece(
            const std::array<World::Pos3, 4>& _boundingBoxOffsets,
            const std::array<World::Pos3, 4>& _boundingBoxSizes,
            uint8_t _bridgeEdges,
            uint8_t _bridgeQuarters,
            const std::array<uint8_t, 4>& _bridgeType,
            const std::array<int16_t, 4>& _tunnelHeights,
            SegmentFlags _segments)
            : boundingBoxOffsets(_boundingBoxOffsets)
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

    consteval RoadPaintCommonPiece rotateRoadPCP(const RoadPaintCommonPiece& reference, const std::array<uint8_t, 4>& rotationTable)
    {
        return RoadPaintCommonPiece{
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

    constexpr RoadPaintCommonPiece kStraight0 = {
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

    constexpr std::array<RoadPaintCommonPiece, 1> kStraightRPCP = {
        kStraight0,
    };

    constexpr RoadPaintCommonPiece kRightCurveVerySmall0 = {
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

    constexpr std::array<RoadPaintCommonPiece, 1> kRightCurveVerySmallRPCP = {
        kRightCurveVerySmall0,
    };

    constexpr RoadPaintCommonPiece kLeftCurveVerySmall0 = rotateRoadPCP(kRightCurveVerySmall0, kRotationTable1230);

    constexpr std::array<RoadPaintCommonPiece, 1> kLeftCurveVerySmallRPCP = {
        kLeftCurveVerySmall0,
    };

    constexpr RoadPaintCommonPiece kRightCurveSmall0 = {
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

    constexpr RoadPaintCommonPiece kRightCurveSmall1 = {
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

    constexpr RoadPaintCommonPiece kRightCurveSmall2 = {
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

    constexpr RoadPaintCommonPiece kRightCurveSmall3 = {
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

    constexpr std::array<RoadPaintCommonPiece, 4> kRightCurveSmallRPCP = {
        kRightCurveSmall0,
        kRightCurveSmall1,
        kRightCurveSmall2,
        kRightCurveSmall3,
    };

    constexpr RoadPaintCommonPiece kLeftCurveSmall0 = rotateRoadPCP(kRightCurveSmall3, kRotationTable1230);

    constexpr RoadPaintCommonPiece kLeftCurveSmall1 = rotateRoadPCP(kRightCurveSmall1, kRotationTable1230);

    constexpr RoadPaintCommonPiece kLeftCurveSmall2 = rotateRoadPCP(kRightCurveSmall2, kRotationTable1230);

    constexpr RoadPaintCommonPiece kLeftCurveSmall3 = rotateRoadPCP(kRightCurveSmall0, kRotationTable1230);

    constexpr std::array<RoadPaintCommonPiece, 4> kLeftCurveSmallRPCP = {
        kLeftCurveSmall0,
        kLeftCurveSmall1,
        kLeftCurveSmall2,
        kLeftCurveSmall3,
    };

    constexpr RoadPaintCommonPiece kStraightSlopeUp0 = {
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

    constexpr RoadPaintCommonPiece kStraightSlopeUp1 = {
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

    constexpr std::array<RoadPaintCommonPiece, 2> kStraightSlopeUpRPCP = {
        kStraightSlopeUp0,
        kStraightSlopeUp1,
    };

    constexpr RoadPaintCommonPiece kStraightSlopeDown0 = rotateRoadPCP(kStraightSlopeUp1, kRotationTable2301);

    constexpr RoadPaintCommonPiece kStraightSlopeDown1 = rotateRoadPCP(kStraightSlopeUp0, kRotationTable2301);

    constexpr std::array<RoadPaintCommonPiece, 2> kStraightSlopeDownRPCP = {
        kStraightSlopeDown0,
        kStraightSlopeDown1,
    };

    constexpr RoadPaintCommonPiece kStraightSteepSlopeUp0 = {
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

    constexpr std::array<RoadPaintCommonPiece, 1> kStraightSteepSlopeUpRPCP = {
        kStraightSteepSlopeUp0,
    };

    constexpr RoadPaintCommonPiece kStraightSteepSlopeDown0 = rotateRoadPCP(kStraightSteepSlopeUp0, kRotationTable2301);

    constexpr std::array<RoadPaintCommonPiece, 1> kStraightSteepSlopeDownRPCP = {
        kStraightSteepSlopeDown0,
    };

    constexpr RoadPaintCommonPiece kTurnaround0 = {
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

    constexpr std::array<RoadPaintCommonPiece, 1> kTurnaroundRPCP = {
        kTurnaround0,
    };

    constexpr std::array<std::span<const RoadPaintCommonPiece>, 10> kRoadPaintCommonParts = {
        kStraightRPCP,
        kLeftCurveVerySmallRPCP,
        kRightCurveVerySmallRPCP,
        kLeftCurveSmallRPCP,
        kRightCurveSmallRPCP,
        kStraightSlopeUpRPCP,
        kStraightSlopeDownRPCP,
        kStraightSteepSlopeUpRPCP,
        kStraightSteepSlopeDownRPCP,
        kTurnaroundRPCP,
    };

    struct RoadPaintMergeablePiece
    {
    private:
        constexpr void rotateTunnelHeights()
        {
            streetlightHeights[1][0] = streetlightHeights[0][3];
            streetlightHeights[1][1] = streetlightHeights[0][0];
            streetlightHeights[1][2] = streetlightHeights[0][1];
            streetlightHeights[1][3] = streetlightHeights[0][2];

            streetlightHeights[2][0] = streetlightHeights[0][2];
            streetlightHeights[2][1] = streetlightHeights[0][3];
            streetlightHeights[2][2] = streetlightHeights[0][0];
            streetlightHeights[2][3] = streetlightHeights[0][1];

            streetlightHeights[3][0] = streetlightHeights[0][1];
            streetlightHeights[3][1] = streetlightHeights[0][2];
            streetlightHeights[3][2] = streetlightHeights[0][3];
            streetlightHeights[3][3] = streetlightHeights[0][0];
        }

    public:
        constexpr RoadPaintMergeablePiece(
            const std::array<uint32_t, 4>& _imageIndexOffsets,
            const std::array<int16_t, 4>& _tunnelHeights,
            const bool _isMultiTileMerge)
            : imageIndexOffsets(_imageIndexOffsets)
            , streetlightHeights()
            , isMultiTileMerge(_isMultiTileMerge)
        {
            streetlightHeights = {};
            streetlightHeights[0] = _tunnelHeights;
            rotateTunnelHeights();
        }

        std::array<uint32_t, 4> imageIndexOffsets;
        std::array<std::array<int16_t, 4>, 4> streetlightHeights;
        bool isMultiTileMerge;
    };

    constexpr int16_t kNoStreetlight = -1;
    constexpr std::array<int16_t, 4> kNoStreetlights = { kNoStreetlight, kNoStreetlight, kNoStreetlight, kNoStreetlight };

    consteval RoadPaintMergeablePiece rotateRoadPP(const RoadPaintMergeablePiece& reference, const std::array<uint8_t, 4>& rotationTable)
    {
        return RoadPaintMergeablePiece{
            std::array<uint32_t, 4>{
                reference.imageIndexOffsets[rotationTable[0]],
                reference.imageIndexOffsets[rotationTable[1]],
                reference.imageIndexOffsets[rotationTable[2]],
                reference.imageIndexOffsets[rotationTable[3]],
            },
            std::array<int16_t, 4>{
                reference.streetlightHeights[0][rotationTable[0]],
                reference.streetlightHeights[0][rotationTable[1]],
                reference.streetlightHeights[0][rotationTable[2]],
                reference.streetlightHeights[0][rotationTable[3]],
            },
            reference.isMultiTileMerge
        };
    }
}
