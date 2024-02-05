#include "Objects/TrackObject.h"
#include "Paint.h"
#include <array>
#include <span>

namespace OpenLoco::Paint
{
    constexpr uint8_t rotl4bit(uint8_t val, uint8_t rotation)
    {
        return ((val << rotation) | (val >> (4 - rotation))) & 0xF;
    }

    constexpr uint8_t rotr4bit(uint8_t val, uint8_t rotation)
    {
        return ((val >> rotation) | (val << (4 - rotation))) & 0xF;
    }

    constexpr std::array<std::array<uint8_t, 4>, 4> kSegMap1 = {
        std::array<uint8_t, 4>{ 0, 1, 2, 3 },
        std::array<uint8_t, 4>{ 2, 0, 3, 1 },
        std::array<uint8_t, 4>{ 3, 2, 1, 0 },
        std::array<uint8_t, 4>{ 1, 3, 0, 2 },
    };
    constexpr std::array<std::array<uint8_t, 4>, 4> kSegMap2 = {
        std::array<uint8_t, 4>{ 0, 1, 2, 3 },
        std::array<uint8_t, 4>{ 1, 3, 0, 2 },
        std::array<uint8_t, 4>{ 3, 2, 1, 0 },
        std::array<uint8_t, 4>{ 2, 0, 3, 1 },
    };

    constexpr SegmentFlags rotlSegmentFlags(SegmentFlags val, uint8_t rotation)
    {
        SegmentFlags ret = SegmentFlags::none;
        const auto _val = enumValue(val);
        for (auto i = 0U; i < 4; ++i)
        {
            if (_val & (1U << i))
            {
                ret |= static_cast<SegmentFlags>(1U << kSegMap1[rotation][i]);
            }
        }
        for (auto i = 5U; i < 9; ++i)
        {
            if (_val & (1U << i))
            {
                ret |= static_cast<SegmentFlags>(1U << (kSegMap2[rotation][i - 5] + 5));
            }
        }

        return ret | (val & SegmentFlags::x1y1);
    }

    static_assert(rotl4bit(0b1000, 1) == 0b0001);
    static_assert(rotlSegmentFlags(SegmentFlags::x0y1 | SegmentFlags::x1y1 | SegmentFlags::x2y1, 0) == (SegmentFlags::x0y1 | SegmentFlags::x1y1 | SegmentFlags::x2y1));
    static_assert(rotlSegmentFlags(SegmentFlags::x0y1 | SegmentFlags::x1y1 | SegmentFlags::x2y1, 1) == (SegmentFlags::x1y0 | SegmentFlags::x1y1 | SegmentFlags::x1y2));
    static_assert(rotlSegmentFlags(SegmentFlags::x0y1 | SegmentFlags::x1y1 | SegmentFlags::x2y1, 2) == (SegmentFlags::x0y1 | SegmentFlags::x1y1 | SegmentFlags::x2y1));
    static_assert(rotlSegmentFlags(SegmentFlags::x0y1 | SegmentFlags::x1y1 | SegmentFlags::x2y1, 3) == (SegmentFlags::x1y0 | SegmentFlags::x1y1 | SegmentFlags::x1y2));
    static_assert(rotlSegmentFlags(SegmentFlags::x0y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x1y2, 0) == (SegmentFlags::x0y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x1y2));
    static_assert(rotlSegmentFlags(SegmentFlags::x0y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x1y2, 1) == (SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2));
    static_assert(rotlSegmentFlags(SegmentFlags::x0y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x1y2, 2) == (SegmentFlags::x2y0 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x2y1));
    static_assert(rotlSegmentFlags(SegmentFlags::x0y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x1y2, 3) == (SegmentFlags::x0y0 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x0y1));

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
                bridgeEdges[i] = rotl4bit(bridgeEdges[0], i);
            }
            for (auto i = 1; i < 4; ++i)
            {
                bridgeQuarters[i] = rotl4bit(bridgeQuarters[0], i);
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
            const std::array<SegmentFlags, 4>& _segments,
            bool _isMergable)
            : imageIndexOffsets(_imageIndexOffsets)
            , boundingBoxOffsets(_boundingBoxOffsets)
            , boundingBoxSizes(_boundingBoxSizes)
            , bridgeEdges(_bridgeEdges)
            , bridgeQuarters(_bridgeQuarters)
            , bridgeType(_bridgeType)
            , segments(_segments)
            , isMergable(_isMergable)
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
            , isMergable(true)
        {
            tunnelHeights[0] = _tunnelHeights;
            bridgeEdges[0] = _bridgeEdges;
            bridgeQuarters[0] = _bridgeQuarters;
            segments[0] = _segments;
            rotateTunnelHeights();
            rotateBridgeEdgesQuarters();
            rotateSegements();
        }
        constexpr TrackPaintPiece(
            const std::array<uint32_t, 4>& _imageIndexOffsets,
            const std::array<World::Pos3, 4>& _boundingBoxOffsets,
            const std::array<World::Pos3, 4>& _boundingBoxSizes,
            uint8_t _bridgeEdges,
            uint8_t _bridgeQuarters,
            const std::array<uint8_t, 4>& _bridgeType,
            const std::array<int16_t, 4>& _tunnelHeights,
            SegmentFlags _segments)
            : imageIndexOffsets()
            , boundingBoxOffsets(_boundingBoxOffsets)
            , boundingBoxSizes(_boundingBoxSizes)
            , bridgeEdges()
            , bridgeQuarters()
            , bridgeType(_bridgeType)
            , tunnelHeights()
            , segments()
            , isMergable(false)
        {
            imageIndexOffsets[0][0] = _imageIndexOffsets[0];
            imageIndexOffsets[1][0] = _imageIndexOffsets[1];
            imageIndexOffsets[2][0] = _imageIndexOffsets[2];
            imageIndexOffsets[3][0] = _imageIndexOffsets[3];
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
        bool isMergable;
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
            },
            reference.isMergable
        };
    }

    // 0x004125DD, 0x0041270E
    constexpr TrackPaintPiece kStraight0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kStraight0BallastNE, TrackObj::ImageIds::Style0::kStraight0SleeperNE, TrackObj::ImageIds::Style0::kStraight0RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kStraight0BallastSE, TrackObj::ImageIds::Style0::kStraight0SleeperSE, TrackObj::ImageIds::Style0::kStraight0RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kStraight0BallastNE, TrackObj::ImageIds::Style0::kStraight0SleeperNE, TrackObj::ImageIds::Style0::kStraight0RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kStraight0BallastSE, TrackObj::ImageIds::Style0::kStraight0SleeperSE, TrackObj::ImageIds::Style0::kStraight0RailSE },
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
        /* Segments */ SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1,
    };

    constexpr std::array<TrackPaintPiece, 1> kStraightTPP = {
        kStraight0,
    };

    // 0x0041BDBD, 0x0041C121, 0x0041C047, 0x0041C3AB
    constexpr TrackPaintPiece kDiagonal0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kDiagonal0BallastE, TrackObj::ImageIds::Style0::kDiagonal0SleeperE, TrackObj::ImageIds::Style0::kDiagonal0RailE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kDiagonal0BallastS, TrackObj::ImageIds::Style0::kDiagonal0SleeperS, TrackObj::ImageIds::Style0::kDiagonal0RailS },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kDiagonal3BallastE, TrackObj::ImageIds::Style0::kDiagonal3SleeperE, TrackObj::ImageIds::Style0::kDiagonal3RailE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kDiagonal3BallastS, TrackObj::ImageIds::Style0::kDiagonal3SleeperS, TrackObj::ImageIds::Style0::kDiagonal3RailS },
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
        /* BridgeEdges */ 0b1111,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x0y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x1y2,
    };

    // 0x0041BE97, 0x0041C1FB, 0x0041BF6F, 0x0041C2D3
    constexpr TrackPaintPiece kDiagonal1 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kDiagonal1BallastE, TrackObj::ImageIds::Style0::kDiagonal1SleeperE, TrackObj::ImageIds::Style0::kDiagonal1RailE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kDiagonal1BallastS, TrackObj::ImageIds::Style0::kDiagonal1SleeperS, TrackObj::ImageIds::Style0::kDiagonal1RailS },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kDiagonal2BallastE, TrackObj::ImageIds::Style0::kDiagonal2SleeperE, TrackObj::ImageIds::Style0::kDiagonal2RailE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kDiagonal2BallastS, TrackObj::ImageIds::Style0::kDiagonal2SleeperS, TrackObj::ImageIds::Style0::kDiagonal2RailS },
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
        /* Segments */ SegmentFlags::x0y0,
    };

    constexpr TrackPaintPiece kDiagonal2 = rotateTrackPP(kDiagonal1, kRotationTable2301);

    constexpr TrackPaintPiece kDiagonal3 = rotateTrackPP(kDiagonal0, kRotationTable2301);

    constexpr std::array<TrackPaintPiece, 4> kDiagonalTPP = {
        kDiagonal0,
        kDiagonal1,
        kDiagonal2,
        kDiagonal3,
    };

    // 0x00413BDF, 0x00413D19, 0x00413E53, 0x00413F8B
    constexpr TrackPaintPiece kRightCurveVerySmall0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveVerySmall0BallastNE, TrackObj::ImageIds::Style0::kRightCurveVerySmall0SleeperNE, TrackObj::ImageIds::Style0::kRightCurveVerySmall0RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveVerySmall0BallastSE, TrackObj::ImageIds::Style0::kRightCurveVerySmall0SleeperSE, TrackObj::ImageIds::Style0::kRightCurveVerySmall0RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveVerySmall0BallastSW, TrackObj::ImageIds::Style0::kRightCurveVerySmall0SleeperSW, TrackObj::ImageIds::Style0::kRightCurveVerySmall0RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveVerySmall0BallastNW, TrackObj::ImageIds::Style0::kRightCurveVerySmall0SleeperNW, TrackObj::ImageIds::Style0::kRightCurveVerySmall0RailNW },
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
        /* Segments */ SegmentFlags::x1y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    constexpr std::array<TrackPaintPiece, 1> kRightCurveVerySmallTPP = {
        kRightCurveVerySmall0,
    };

    constexpr TrackPaintPiece kLeftCurveVerySmall0 = rotateTrackPP(kRightCurveVerySmall0, kRotationTable1230);

    constexpr std::array<TrackPaintPiece, 1> kLeftCurveVerySmallTPP = {
        kLeftCurveVerySmall0,
    };

    // 0x00414EED, 0x0041528F, 0x00415631, 0x004159D1
    constexpr TrackPaintPiece kRightCurveSmall0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall0BallastNE, TrackObj::ImageIds::Style0::kRightCurveSmall0SleeperNE, TrackObj::ImageIds::Style0::kRightCurveSmall0RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall0BallastSE, TrackObj::ImageIds::Style0::kRightCurveSmall0SleeperSE, TrackObj::ImageIds::Style0::kRightCurveSmall0RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall0BallastSW, TrackObj::ImageIds::Style0::kRightCurveSmall0SleeperSW, TrackObj::ImageIds::Style0::kRightCurveSmall0RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall0BallastNW, TrackObj::ImageIds::Style0::kRightCurveSmall0SleeperNW, TrackObj::ImageIds::Style0::kRightCurveSmall0RailNW },
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
        /* BridgeEdges */ 0b0111,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            kNoTunnel,
            0,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x00414FEE, 0x00415390, 0x00415732, 0x00415AD2
    constexpr TrackPaintPiece kRightCurveSmall1 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall1BallastNE, TrackObj::ImageIds::Style0::kRightCurveSmall1SleeperNE, TrackObj::ImageIds::Style0::kRightCurveSmall1RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall1BallastSE, TrackObj::ImageIds::Style0::kRightCurveSmall1SleeperSE, TrackObj::ImageIds::Style0::kRightCurveSmall1RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall1BallastSW, TrackObj::ImageIds::Style0::kRightCurveSmall1SleeperSW, TrackObj::ImageIds::Style0::kRightCurveSmall1RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall1BallastNW, TrackObj::ImageIds::Style0::kRightCurveSmall1SleeperNW, TrackObj::ImageIds::Style0::kRightCurveSmall1RailNW },
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
        /* Segments */ SegmentFlags::x0y0,
    };

    // 0x004150BD, 0x0041545F, 0x00415801, 0x00415BA1
    constexpr TrackPaintPiece kRightCurveSmall2 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall2BallastNE, TrackObj::ImageIds::Style0::kRightCurveSmall2SleeperNE, TrackObj::ImageIds::Style0::kRightCurveSmall2RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall2BallastSE, TrackObj::ImageIds::Style0::kRightCurveSmall2SleeperSE, TrackObj::ImageIds::Style0::kRightCurveSmall2RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall2BallastSW, TrackObj::ImageIds::Style0::kRightCurveSmall2SleeperSW, TrackObj::ImageIds::Style0::kRightCurveSmall2RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall2BallastNW, TrackObj::ImageIds::Style0::kRightCurveSmall2SleeperNW, TrackObj::ImageIds::Style0::kRightCurveSmall2RailNW },
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

    // 0x0041518E, 0x00415530, 0x004158D0, 0x00415C72
    constexpr TrackPaintPiece kRightCurveSmall3 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall3BallastNE, TrackObj::ImageIds::Style0::kRightCurveSmall3SleeperNE, TrackObj::ImageIds::Style0::kRightCurveSmall3RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall3BallastSE, TrackObj::ImageIds::Style0::kRightCurveSmall3SleeperSE, TrackObj::ImageIds::Style0::kRightCurveSmall3RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall3BallastSW, TrackObj::ImageIds::Style0::kRightCurveSmall3SleeperSW, TrackObj::ImageIds::Style0::kRightCurveSmall3RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveSmall3BallastNW, TrackObj::ImageIds::Style0::kRightCurveSmall3SleeperNW, TrackObj::ImageIds::Style0::kRightCurveSmall3RailNW },
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
        /* BridgeEdges */ 0b1110,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            0,
            kNoTunnel,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x2y0 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
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

    // 0x0041873B, 0x00418BDB, 0x0041907D, 0x0041951B
    constexpr TrackPaintPiece kRightCurve0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve0BallastNE, TrackObj::ImageIds::Style0::kRightCurve0SleeperNE, TrackObj::ImageIds::Style0::kRightCurve0RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve0BallastSE, TrackObj::ImageIds::Style0::kRightCurve0SleeperSE, TrackObj::ImageIds::Style0::kRightCurve0RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve0BallastSW, TrackObj::ImageIds::Style0::kRightCurve0SleeperSW, TrackObj::ImageIds::Style0::kRightCurve0RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve0BallastNW, TrackObj::ImageIds::Style0::kRightCurve0SleeperNW, TrackObj::ImageIds::Style0::kRightCurve0RailNW },
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
        /* BridgeEdges */ 0b0101,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            kNoTunnel,
            0,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x0y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x00418845, 0x00418CE5, 0x00419187, 0x00419625
    constexpr TrackPaintPiece kRightCurve1 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve1BallastNE, TrackObj::ImageIds::Style0::kRightCurve1SleeperNE, TrackObj::ImageIds::Style0::kRightCurve1RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve1BallastSE, TrackObj::ImageIds::Style0::kRightCurve1SleeperSE, TrackObj::ImageIds::Style0::kRightCurve1RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve1BallastSW, TrackObj::ImageIds::Style0::kRightCurve1SleeperSW, TrackObj::ImageIds::Style0::kRightCurve1RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve1BallastNW, TrackObj::ImageIds::Style0::kRightCurve1SleeperNW, TrackObj::ImageIds::Style0::kRightCurve1RailNW },
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
        /* BridgeEdges */ 0b0110,
        /* BridgeQuarters */ 0b0010,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x0041891F, 0x00418DBF, 0x0041925F, 0x004196FF
    constexpr TrackPaintPiece kRightCurve2 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve2BallastNE, TrackObj::ImageIds::Style0::kRightCurve2SleeperNE, TrackObj::ImageIds::Style0::kRightCurve2RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve2BallastSE, TrackObj::ImageIds::Style0::kRightCurve2SleeperSE, TrackObj::ImageIds::Style0::kRightCurve2RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve2BallastSW, TrackObj::ImageIds::Style0::kRightCurve2SleeperSW, TrackObj::ImageIds::Style0::kRightCurve2RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve2BallastNW, TrackObj::ImageIds::Style0::kRightCurve2SleeperNW, TrackObj::ImageIds::Style0::kRightCurve2RailNW },
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
        /* Segments */ SegmentFlags::x0y0 | SegmentFlags::x2y0 | SegmentFlags::x0y2 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x0y1,
    };

    // 0x004189F7, 0x00418E99, 0x00419339, 0x004197D9
    constexpr TrackPaintPiece kRightCurve3 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve3BallastNE, TrackObj::ImageIds::Style0::kRightCurve3SleeperNE, TrackObj::ImageIds::Style0::kRightCurve3RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve3BallastSE, TrackObj::ImageIds::Style0::kRightCurve3SleeperSE, TrackObj::ImageIds::Style0::kRightCurve3RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve3BallastSW, TrackObj::ImageIds::Style0::kRightCurve3SleeperSW, TrackObj::ImageIds::Style0::kRightCurve3RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve3BallastNW, TrackObj::ImageIds::Style0::kRightCurve3SleeperNW, TrackObj::ImageIds::Style0::kRightCurve3RailNW },
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
        /* BridgeEdges */ 0b0110,
        /* BridgeQuarters */ 0b0010,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x2y0 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x00418AD1, 0x00418F73, 0x00419411, 0x004198B3
    constexpr TrackPaintPiece kRightCurve4 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve4BallastNE, TrackObj::ImageIds::Style0::kRightCurve4SleeperNE, TrackObj::ImageIds::Style0::kRightCurve4RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve4BallastSE, TrackObj::ImageIds::Style0::kRightCurve4SleeperSE, TrackObj::ImageIds::Style0::kRightCurve4RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve4BallastSW, TrackObj::ImageIds::Style0::kRightCurve4SleeperSW, TrackObj::ImageIds::Style0::kRightCurve4RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurve4BallastNW, TrackObj::ImageIds::Style0::kRightCurve4SleeperNW, TrackObj::ImageIds::Style0::kRightCurve4RailNW },
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
        /* BridgeEdges */ 0b1010,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            0,
            kNoTunnel,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x2y0 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    constexpr std::array<TrackPaintPiece, 5> kRightCurveTPP = {
        kRightCurve0,
        kRightCurve1,
        kRightCurve2,
        kRightCurve3,
        kRightCurve4,
    };

    constexpr TrackPaintPiece kLeftCurve0 = rotateTrackPP(kRightCurve4, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurve1 = rotateTrackPP(kRightCurve3, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurve2 = rotateTrackPP(kRightCurve2, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurve3 = rotateTrackPP(kRightCurve1, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurve4 = rotateTrackPP(kRightCurve0, kRotationTable1230);

    constexpr std::array<TrackPaintPiece, 5> kLeftCurveTPP = {
        kLeftCurve0,
        kLeftCurve1,
        kLeftCurve2,
        kLeftCurve3,
        kLeftCurve4,
    };

    // 0x0041ABE7, 0x0041B055, 0x0041B4C3, 0x0041B931
    constexpr TrackPaintPiece kRightCurveLarge0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge0BallastNE, TrackObj::ImageIds::Style0::kRightCurveLarge0SleeperNE, TrackObj::ImageIds::Style0::kRightCurveLarge0RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge0BallastSE, TrackObj::ImageIds::Style0::kRightCurveLarge0SleeperSE, TrackObj::ImageIds::Style0::kRightCurveLarge0RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge0BallastSW, TrackObj::ImageIds::Style0::kRightCurveLarge0SleeperSW, TrackObj::ImageIds::Style0::kRightCurveLarge0RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge0BallastNW, TrackObj::ImageIds::Style0::kRightCurveLarge0SleeperNW, TrackObj::ImageIds::Style0::kRightCurveLarge0RailNW },
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
        /* BridgeEdges */ 0b0101,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            kNoTunnel,
            0,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x0y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x0041ACF1, 0x0041B15F, 0x0041B5CD, 0x0041BA3B
    constexpr TrackPaintPiece kRightCurveLarge1 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge1BallastNE, TrackObj::ImageIds::Style0::kRightCurveLarge1SleeperNE, TrackObj::ImageIds::Style0::kRightCurveLarge1RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge1BallastSE, TrackObj::ImageIds::Style0::kRightCurveLarge1SleeperSE, TrackObj::ImageIds::Style0::kRightCurveLarge1RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge1BallastSW, TrackObj::ImageIds::Style0::kRightCurveLarge1SleeperSW, TrackObj::ImageIds::Style0::kRightCurveLarge1RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge1BallastNW, TrackObj::ImageIds::Style0::kRightCurveLarge1SleeperNW, TrackObj::ImageIds::Style0::kRightCurveLarge1RailNW },
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
        /* BridgeEdges */ 0b0111,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x0041ADCB, 0x0041B239, 0x0041B6A5, 0x0041BB15
    constexpr TrackPaintPiece kRightCurveLarge2 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge2BallastNE, TrackObj::ImageIds::Style0::kRightCurveLarge2SleeperNE, TrackObj::ImageIds::Style0::kRightCurveLarge2RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge2BallastSE, TrackObj::ImageIds::Style0::kRightCurveLarge2SleeperSE, TrackObj::ImageIds::Style0::kRightCurveLarge2RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge2BallastSW, TrackObj::ImageIds::Style0::kRightCurveLarge2SleeperSW, TrackObj::ImageIds::Style0::kRightCurveLarge2RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge2BallastNW, TrackObj::ImageIds::Style0::kRightCurveLarge2SleeperNW, TrackObj::ImageIds::Style0::kRightCurveLarge2RailNW },
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
        /* Segments */ SegmentFlags::x0y0 | SegmentFlags::x2y0 | SegmentFlags::x1y0 | SegmentFlags::x0y1,
    };

    // 0x0041AEA3, 0x0041B313, 0x0041B77F, 0x0041BBEF
    constexpr TrackPaintPiece kRightCurveLarge3 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge3BallastNE, TrackObj::ImageIds::Style0::kRightCurveLarge3SleeperNE, TrackObj::ImageIds::Style0::kRightCurveLarge3RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge3BallastSE, TrackObj::ImageIds::Style0::kRightCurveLarge3SleeperSE, TrackObj::ImageIds::Style0::kRightCurveLarge3RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge3BallastSW, TrackObj::ImageIds::Style0::kRightCurveLarge3SleeperSW, TrackObj::ImageIds::Style0::kRightCurveLarge3RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge3BallastNW, TrackObj::ImageIds::Style0::kRightCurveLarge3SleeperNW, TrackObj::ImageIds::Style0::kRightCurveLarge3RailNW },
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
        /* Segments */ SegmentFlags::x2y2,
    };

    // 0x0041AF7B, 0x0041B3EB, 0x0041B857, 0x0041BCC7
    constexpr TrackPaintPiece kRightCurveLarge4 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge4BallastNE, TrackObj::ImageIds::Style0::kRightCurveLarge4SleeperNE, TrackObj::ImageIds::Style0::kRightCurveLarge4RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge4BallastSE, TrackObj::ImageIds::Style0::kRightCurveLarge4SleeperSE, TrackObj::ImageIds::Style0::kRightCurveLarge4RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge4BallastSW, TrackObj::ImageIds::Style0::kRightCurveLarge4SleeperSW, TrackObj::ImageIds::Style0::kRightCurveLarge4RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kRightCurveLarge4BallastNW, TrackObj::ImageIds::Style0::kRightCurveLarge4SleeperNW, TrackObj::ImageIds::Style0::kRightCurveLarge4RailNW },
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
        /* BridgeEdges */ 0b1111,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x2y0 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x2y1,
    };

    constexpr std::array<TrackPaintPiece, 5> kRightCurveLargeTPP = {
        kRightCurveLarge0,
        kRightCurveLarge1,
        kRightCurveLarge2,
        kRightCurveLarge3,
        kRightCurveLarge4,
    };

    // 0x00419A2D, 0x00419E9D, 0x0041A30B, 0x0041A779
    constexpr TrackPaintPiece kLeftCurveLarge0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge0BallastNE, TrackObj::ImageIds::Style0::kLeftCurveLarge0SleeperNE, TrackObj::ImageIds::Style0::kLeftCurveLarge0RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge0BallastSE, TrackObj::ImageIds::Style0::kLeftCurveLarge0SleeperSE, TrackObj::ImageIds::Style0::kLeftCurveLarge0RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge0BallastSW, TrackObj::ImageIds::Style0::kLeftCurveLarge0SleeperSW, TrackObj::ImageIds::Style0::kLeftCurveLarge0RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge0BallastNW, TrackObj::ImageIds::Style0::kLeftCurveLarge0SleeperNW, TrackObj::ImageIds::Style0::kLeftCurveLarge0RailNW },
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
        /* BridgeEdges */ 0b0101,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            kNoTunnel,
            0,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x0y0 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x0y1 | SegmentFlags::x2y1,
    };

    // 0x00419B37, 0x00419FA7, 0x0041A415, 0x0041A883
    constexpr TrackPaintPiece kLeftCurveLarge1 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge1BallastNE, TrackObj::ImageIds::Style0::kLeftCurveLarge1SleeperNE, TrackObj::ImageIds::Style0::kLeftCurveLarge1RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge1BallastSE, TrackObj::ImageIds::Style0::kLeftCurveLarge1SleeperSE, TrackObj::ImageIds::Style0::kLeftCurveLarge1RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge1BallastSW, TrackObj::ImageIds::Style0::kLeftCurveLarge1SleeperSW, TrackObj::ImageIds::Style0::kLeftCurveLarge1RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge1BallastNW, TrackObj::ImageIds::Style0::kLeftCurveLarge1SleeperNW, TrackObj::ImageIds::Style0::kLeftCurveLarge1RailNW },
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
        /* BridgeEdges */ 0b1101,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x0y0 | SegmentFlags::x2y0 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x2y1,
    };

    // 0x00419C11, 0x0041A07F, 0x0041A4EF, 0x0041A95D
    constexpr TrackPaintPiece kLeftCurveLarge2 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge2BallastNE, TrackObj::ImageIds::Style0::kLeftCurveLarge2SleeperNE, TrackObj::ImageIds::Style0::kLeftCurveLarge2RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge2BallastSE, TrackObj::ImageIds::Style0::kLeftCurveLarge2SleeperSE, TrackObj::ImageIds::Style0::kLeftCurveLarge2RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge2BallastSW, TrackObj::ImageIds::Style0::kLeftCurveLarge2SleeperSW, TrackObj::ImageIds::Style0::kLeftCurveLarge2RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge2BallastNW, TrackObj::ImageIds::Style0::kLeftCurveLarge2SleeperNW, TrackObj::ImageIds::Style0::kLeftCurveLarge2RailNW },
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
        /* BridgeEdges */ 0b0011,
        /* BridgeQuarters */ 0b0001,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x0y1 | SegmentFlags::x1y2,
    };

    // 0x00419CEB, 0x0041A159, 0x0041A5C9, 0x0041AA35
    constexpr TrackPaintPiece kLeftCurveLarge3 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge3BallastNE, TrackObj::ImageIds::Style0::kLeftCurveLarge3SleeperNE, TrackObj::ImageIds::Style0::kLeftCurveLarge3RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge3BallastSE, TrackObj::ImageIds::Style0::kLeftCurveLarge3SleeperSE, TrackObj::ImageIds::Style0::kLeftCurveLarge3RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge3BallastSW, TrackObj::ImageIds::Style0::kLeftCurveLarge3SleeperSW, TrackObj::ImageIds::Style0::kLeftCurveLarge3RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge3BallastNW, TrackObj::ImageIds::Style0::kLeftCurveLarge3SleeperNW, TrackObj::ImageIds::Style0::kLeftCurveLarge3RailNW },
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
        /* BridgeEdges */ 0b1100,
        /* BridgeQuarters */ 0b0100,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x2y0,
    };

    // 0x00419DC3, 0x0041A231, 0x0041A6A1, 0x0041AB0D
    constexpr TrackPaintPiece kLeftCurveLarge4 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge4BallastNE, TrackObj::ImageIds::Style0::kLeftCurveLarge4SleeperNE, TrackObj::ImageIds::Style0::kLeftCurveLarge4RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge4BallastSE, TrackObj::ImageIds::Style0::kLeftCurveLarge4SleeperSE, TrackObj::ImageIds::Style0::kLeftCurveLarge4RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge4BallastSW, TrackObj::ImageIds::Style0::kLeftCurveLarge4SleeperSW, TrackObj::ImageIds::Style0::kLeftCurveLarge4RailSW },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kLeftCurveLarge4BallastNW, TrackObj::ImageIds::Style0::kLeftCurveLarge4SleeperNW, TrackObj::ImageIds::Style0::kLeftCurveLarge4RailNW },
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
        /* BridgeEdges */ 0b1111,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    constexpr std::array<TrackPaintPiece, 5> kLeftCurveLargeTPP = {
        kLeftCurveLarge0,
        kLeftCurveLarge1,
        kLeftCurveLarge2,
        kLeftCurveLarge3,
        kLeftCurveLarge4,
    };

    constexpr TrackPaintPiece kDiagonalRightCurveLarge0 = rotateTrackPP(kLeftCurveLarge4, kRotationTable3012);

    constexpr TrackPaintPiece kDiagonalRightCurveLarge1 = rotateTrackPP(kLeftCurveLarge2, kRotationTable3012);

    constexpr TrackPaintPiece kDiagonalRightCurveLarge2 = rotateTrackPP(kLeftCurveLarge3, kRotationTable3012);

    constexpr TrackPaintPiece kDiagonalRightCurveLarge3 = rotateTrackPP(kLeftCurveLarge1, kRotationTable3012);

    constexpr TrackPaintPiece kDiagonalRightCurveLarge4 = rotateTrackPP(kLeftCurveLarge0, kRotationTable3012);

    constexpr std::array<TrackPaintPiece, 5> kDiagonalRightCurveLargeTPP = {
        kDiagonalRightCurveLarge0,
        kDiagonalRightCurveLarge1,
        kDiagonalRightCurveLarge2,
        kDiagonalRightCurveLarge3,
        kDiagonalRightCurveLarge4,
    };

    constexpr TrackPaintPiece kDiagonalLeftCurveLarge0 = rotateTrackPP(kRightCurveLarge4, kRotationTable2301);

    constexpr TrackPaintPiece kDiagonalLeftCurveLarge1 = rotateTrackPP(kRightCurveLarge2, kRotationTable2301);

    constexpr TrackPaintPiece kDiagonalLeftCurveLarge2 = rotateTrackPP(kRightCurveLarge3, kRotationTable2301);

    constexpr TrackPaintPiece kDiagonalLeftCurveLarge3 = rotateTrackPP(kRightCurveLarge1, kRotationTable2301);

    constexpr TrackPaintPiece kDiagonalLeftCurveLarge4 = rotateTrackPP(kRightCurveLarge0, kRotationTable2301);

    constexpr std::array<TrackPaintPiece, 5> kDiagonalLeftCurveLargeTPP = {
        kDiagonalLeftCurveLarge0,
        kDiagonalLeftCurveLarge1,
        kDiagonalLeftCurveLarge2,
        kDiagonalLeftCurveLarge3,
        kDiagonalLeftCurveLarge4,
    };

    // 0x0041CEAF, 0x0041D277, 0x0041D16D, 0x0041D533
    constexpr TrackPaintPiece kSBendLeft0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendLeft0BallastNE, TrackObj::ImageIds::Style0::kSBendLeft0SleeperNE, TrackObj::ImageIds::Style0::kSBendLeft0RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendLeft0BallastSE, TrackObj::ImageIds::Style0::kSBendLeft0SleeperSE, TrackObj::ImageIds::Style0::kSBendLeft0RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendLeft3BallastNE, TrackObj::ImageIds::Style0::kSBendLeft3SleeperNE, TrackObj::ImageIds::Style0::kSBendLeft3RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendLeft3BallastSE, TrackObj::ImageIds::Style0::kSBendLeft3SleeperSE, TrackObj::ImageIds::Style0::kSBendLeft3RailSE },
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
        /* BridgeEdges */ 0b0101,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            kNoTunnel,
            0,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x0y0 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x0y1 | SegmentFlags::x2y1,
    };

    // 0x0041CFB9, 0x0041D381, 0x0041D093, 0x0041D459
    constexpr TrackPaintPiece kSBendLeft1 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendLeft1BallastNE, TrackObj::ImageIds::Style0::kSBendLeft1SleeperNE, TrackObj::ImageIds::Style0::kSBendLeft1RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendLeft1BallastSE, TrackObj::ImageIds::Style0::kSBendLeft1SleeperSE, TrackObj::ImageIds::Style0::kSBendLeft1RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendLeft2BallastNE, TrackObj::ImageIds::Style0::kSBendLeft2SleeperNE, TrackObj::ImageIds::Style0::kSBendLeft2RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendLeft2BallastSE, TrackObj::ImageIds::Style0::kSBendLeft2SleeperSE, TrackObj::ImageIds::Style0::kSBendLeft2RailSE },
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
        /* BridgeEdges */ 0b1100,
        /* BridgeQuarters */ 0b0100,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x0y0 | SegmentFlags::x2y0 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x2y1,
    };

    constexpr TrackPaintPiece kSBendLeft2 = rotateTrackPP(kSBendLeft1, kRotationTable2301);

    constexpr TrackPaintPiece kSBendLeft3 = rotateTrackPP(kSBendLeft0, kRotationTable2301);

    constexpr std::array<TrackPaintPiece, 4> kSBendLeftTPP = {
        kSBendLeft0,
        kSBendLeft1,
        kSBendLeft2,
        kSBendLeft3,
    };

    // 0x0041D63D, 0x0041DA03, 0x0041D8F9, 0x0041DCC1
    constexpr TrackPaintPiece kSBendRight0 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendRight0BallastNE, TrackObj::ImageIds::Style0::kSBendRight0SleeperNE, TrackObj::ImageIds::Style0::kSBendRight0RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendRight0BallastSE, TrackObj::ImageIds::Style0::kSBendRight0SleeperSE, TrackObj::ImageIds::Style0::kSBendRight0RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendRight3BallastNE, TrackObj::ImageIds::Style0::kSBendRight3SleeperNE, TrackObj::ImageIds::Style0::kSBendRight3RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendRight3BallastSE, TrackObj::ImageIds::Style0::kSBendRight3SleeperSE, TrackObj::ImageIds::Style0::kSBendRight3RailSE },
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
        /* BridgeEdges */ 0b0101,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            kNoTunnel,
            0,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x0y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x0041D747, 0x0041DB0D, 0x0041D821, 0x0041DBE7
    constexpr TrackPaintPiece kSBendRight1 = {
        std::array<std::array<uint32_t, 3>, 4>{
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendRight1BallastNE, TrackObj::ImageIds::Style0::kSBendRight1SleeperNE, TrackObj::ImageIds::Style0::kSBendRight1RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendRight1BallastSE, TrackObj::ImageIds::Style0::kSBendRight1SleeperSE, TrackObj::ImageIds::Style0::kSBendRight1RailSE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendRight2BallastNE, TrackObj::ImageIds::Style0::kSBendRight2SleeperNE, TrackObj::ImageIds::Style0::kSBendRight2RailNE },
            std::array<uint32_t, 3>{ TrackObj::ImageIds::Style0::kSBendRight2BallastSE, TrackObj::ImageIds::Style0::kSBendRight2SleeperSE, TrackObj::ImageIds::Style0::kSBendRight2RailSE },
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
        /* BridgeEdges */ 0b0110,
        /* BridgeQuarters */ 0b0010,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    constexpr TrackPaintPiece kSBendRight2 = rotateTrackPP(kSBendRight1, kRotationTable2301);

    constexpr TrackPaintPiece kSBendRight3 = rotateTrackPP(kSBendRight0, kRotationTable2301);

    constexpr std::array<TrackPaintPiece, 4> kSBendRightTPP = {
        kSBendRight0,
        kSBendRight1,
        kSBendRight2,
        kSBendRight3,
    };

    // 0x0041C4BB, 0x0041C639, 0x0041C7B7, 0x0041C935
    constexpr TrackPaintPiece kStraightSlopeUp0 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kStraightSlopeUp0NE,
            TrackObj::ImageIds::Style0::kStraightSlopeUp0SE,
            TrackObj::ImageIds::Style0::kStraightSlopeUp0SW,
            TrackObj::ImageIds::Style0::kStraightSlopeUp0NW,
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
        /* Segments */ SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1,
    };

    // 0x0041C578, 0x0041C6F6, 0x0041C874, 0x0041C9F2
    constexpr TrackPaintPiece kStraightSlopeUp1 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kStraightSlopeUp1NE,
            TrackObj::ImageIds::Style0::kStraightSlopeUp1SE,
            TrackObj::ImageIds::Style0::kStraightSlopeUp1SW,
            TrackObj::ImageIds::Style0::kStraightSlopeUp1NW,
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
        /* Segments */ SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1,
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

    // 0x0041CAB3, 0x0041CBA4, 0x0041CC95, 0x0041CD86
    constexpr TrackPaintPiece kStraightSteepSlopeUp0 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kStraightSteepSlopeUp0NE,
            TrackObj::ImageIds::Style0::kStraightSteepSlopeUp0SE,
            TrackObj::ImageIds::Style0::kStraightSteepSlopeUp0SW,
            TrackObj::ImageIds::Style0::kStraightSteepSlopeUp0NW,
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
        /* Segments */ SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1,
    };

    constexpr std::array<TrackPaintPiece, 1> kStraightSteepSlopeUpTPP = {
        kStraightSteepSlopeUp0,
    };

    constexpr TrackPaintPiece kStraightSteepSlopeDown0 = rotateTrackPP(kStraightSteepSlopeUp0, kRotationTable2301);

    constexpr std::array<TrackPaintPiece, 1> kStraightSteepSlopeDownTPP = {
        kStraightSteepSlopeDown0,
    };

    // 0x00415DE3, 0x0041606D, 0x004162F7, 0x0041657F
    constexpr TrackPaintPiece kRightCurveSmallSlopeUp0 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp0NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp0SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp0SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp0NW,
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
        /* BridgeEdges */ 0b0111,
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
        /* Segments */ SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x00415E9D, 0x00416127, 0x004163B1, 0x00416639
    constexpr TrackPaintPiece kRightCurveSmallSlopeUp1 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp1NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp1SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp1SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp1NW,
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
        /* BridgeEdges */ 0b1001,
        /* BridgeQuarters */ 0b1000,
        /* BridgeType */ std::array<uint8_t, 4>{
            16,
            13,
            14,
            15,
        },
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x0y0,
    };

    // 0x00415F25, 0x004161AF, 0x00416439, 0x004166C1
    constexpr TrackPaintPiece kRightCurveSmallSlopeUp2 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp2NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp2SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp2SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp2NW,
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
        /* BridgeEdges */ 0b0110,
        /* BridgeQuarters */ 0b0010,
        /* BridgeType */ std::array<uint8_t, 4>{
            14,
            15,
            16,
            13,
        },
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x00415FAF, 0x00416239, 0x004164C1, 0x0041674B
    constexpr TrackPaintPiece kRightCurveSmallSlopeUp3 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp3NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp3SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp3SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeUp3NW,
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
        /* BridgeEdges */ 0b1110,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ std::array<uint8_t, 4>{
            4,
            6,
            8,
            2,
        },
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            16,
            kNoTunnel,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x2y0 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    constexpr std::array<TrackPaintPiece, 4> kRightCurveSmallSlopeUpTPP = {
        kRightCurveSmallSlopeUp0,
        kRightCurveSmallSlopeUp1,
        kRightCurveSmallSlopeUp2,
        kRightCurveSmallSlopeUp3,
    };

    // 0x00416809, 0x00416A93, 0x00416D1D, 0x00416FA5
    constexpr TrackPaintPiece kRightCurveSmallSlopeDown0 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown0NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown0SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown0SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown0NW,
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
        /* BridgeEdges */ 0b0111,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ std::array<uint8_t, 4>{
            6,
            8,
            2,
            4,
        },
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            kNoTunnel,
            16,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x004168C7, 0x00416B51, 0x00416DDB, 0x00417063
    constexpr TrackPaintPiece kRightCurveSmallSlopeDown1 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown1NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown1SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown1SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown1NW,
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
        /* BridgeEdges */ 0b1001,
        /* BridgeQuarters */ 0b1000,
        /* BridgeType */ std::array<uint8_t, 4>{
            16,
            13,
            14,
            15,
        },
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x0y0,
    };

    // 0x0041694F, 0x00416BD9, 0x00416E63, 0x004170EB
    constexpr TrackPaintPiece kRightCurveSmallSlopeDown2 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown2NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown2SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown2SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown2NW,
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
        /* BridgeEdges */ 0b0110,
        /* BridgeQuarters */ 0b0010,
        /* BridgeType */ std::array<uint8_t, 4>{
            14,
            15,
            16,
            13,
        },
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x004169D9, 0x00416C63, 0x00416EEB, 0x00417175
    constexpr TrackPaintPiece kRightCurveSmallSlopeDown3 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown3NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown3SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown3SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSlopeDown3NW,
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
        /* BridgeEdges */ 0b1110,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ std::array<uint8_t, 4>{
            7,
            1,
            3,
            5,
        },
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            0,
            kNoTunnel,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x2y0 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    constexpr std::array<TrackPaintPiece, 4> kRightCurveSmallSlopeDownTPP = {
        kRightCurveSmallSlopeDown0,
        kRightCurveSmallSlopeDown1,
        kRightCurveSmallSlopeDown2,
        kRightCurveSmallSlopeDown3,
    };

    constexpr TrackPaintPiece kLeftCurveSmallSlopeUp0 = rotateTrackPP(kRightCurveSmallSlopeDown3, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmallSlopeUp1 = rotateTrackPP(kRightCurveSmallSlopeDown1, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmallSlopeUp2 = rotateTrackPP(kRightCurveSmallSlopeDown2, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmallSlopeUp3 = rotateTrackPP(kRightCurveSmallSlopeDown0, kRotationTable1230);

    constexpr std::array<TrackPaintPiece, 4> kLeftCurveSmallSlopeUpTPP = {
        kLeftCurveSmallSlopeUp0,
        kLeftCurveSmallSlopeUp1,
        kLeftCurveSmallSlopeUp2,
        kLeftCurveSmallSlopeUp3,
    };

    constexpr TrackPaintPiece kLeftCurveSmallSlopeDown0 = rotateTrackPP(kRightCurveSmallSlopeUp3, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmallSlopeDown1 = rotateTrackPP(kRightCurveSmallSlopeUp1, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmallSlopeDown2 = rotateTrackPP(kRightCurveSmallSlopeUp2, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmallSlopeDown3 = rotateTrackPP(kRightCurveSmallSlopeUp0, kRotationTable1230);

    constexpr std::array<TrackPaintPiece, 4> kLeftCurveSmallSlopeDownTPP = {
        kLeftCurveSmallSlopeDown0,
        kLeftCurveSmallSlopeDown1,
        kLeftCurveSmallSlopeDown2,
        kLeftCurveSmallSlopeDown3,
    };

    // 0x0041729F, 0x00417529, 0x004177B3, 0x00417A3B
    constexpr TrackPaintPiece kRightCurveSmallSteepSlopeUp0 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp0NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp0SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp0SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp0NW,
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
        /* BridgeEdges */ 0b0111,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ std::array<uint8_t, 4>{
            9,
            10,
            11,
            12,
        },
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            kNoTunnel,
            0,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x00417359, 0x004175E3, 0x0041786D, 0x00417AF5
    constexpr TrackPaintPiece kRightCurveSmallSteepSlopeUp1 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp1NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp1SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp1SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp1NW,
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
        /* BridgeEdges */ 0b1001,
        /* BridgeQuarters */ 0b1000,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x0y0,
    };

    // 0x004173E1, 0x0041766B, 0x004178F5, 0x00417B7D
    constexpr TrackPaintPiece kRightCurveSmallSteepSlopeUp2 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp2NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp2SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp2SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp2NW,
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
        /* BridgeEdges */ 0b0110,
        /* BridgeQuarters */ 0b0010,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x0041746B, 0x004176F5, 0x0041797D, 0x00417C07
    constexpr TrackPaintPiece kRightCurveSmallSteepSlopeUp3 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp3NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp3SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp3SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeUp3NW,
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
        /* BridgeEdges */ 0b1110,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ std::array<uint8_t, 4>{
            10,
            11,
            12,
            9,
        },
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            16,
            kNoTunnel,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x2y0 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    constexpr std::array<TrackPaintPiece, 4> kRightCurveSmallSteepSlopeUpTPP = {
        kRightCurveSmallSteepSlopeUp0,
        kRightCurveSmallSteepSlopeUp1,
        kRightCurveSmallSteepSlopeUp2,
        kRightCurveSmallSteepSlopeUp3,
    };

    // 0x00417CC5, 0x00417F4F, 0x004181D9, 0x0041846D
    constexpr TrackPaintPiece kRightCurveSmallSteepSlopeDown0 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown0NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown0SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown0SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown0NW,
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
        /* BridgeEdges */ 0b0111,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ std::array<uint8_t, 4>{
            11,
            12,
            9,
            10,
        },
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            kNoTunnel,
            16,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x0y2 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x0y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x00417D83, 0x0041800D, 0x0041829A, 0x0041852E
    constexpr TrackPaintPiece kRightCurveSmallSteepSlopeDown1 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown1NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown1SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown1SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown1NW,
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
        /* BridgeEdges */ 0b1001,
        /* BridgeQuarters */ 0b1000,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x0y0,
    };

    // 0x00417E0B, 0x00418095, 0x00418325, 0x004185B9
    constexpr TrackPaintPiece kRightCurveSmallSteepSlopeDown2 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown2NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown2SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown2SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown2NW,
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
        /* BridgeEdges */ 0b0110,
        /* BridgeQuarters */ 0b0010,
        /* BridgeType */ kFlatBridge,
        /* TunnelHeights */ kNoTunnels,
        /* Segments */ SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    // 0x00417E95, 0x0041811F, 0x004183B0, 0x00418646
    constexpr TrackPaintPiece kRightCurveSmallSteepSlopeDown3 = {
        std::array<uint32_t, 4>{
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown3NE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown3SE,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown3SW,
            TrackObj::ImageIds::Style0::kRightCurveSmallSteepSlopeDown3NW,
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
        /* BridgeEdges */ 0b1110,
        /* BridgeQuarters */ 0b1111,
        /* BridgeType */ std::array<uint8_t, 4>{
            12,
            9,
            10,
            11,
        },
        /* TunnelHeights */ std::array<int16_t, 4>{
            kNoTunnel,
            0,
            kNoTunnel,
            kNoTunnel,
        },
        /* Segments */ SegmentFlags::x2y0 | SegmentFlags::x2y2 | SegmentFlags::x1y1 | SegmentFlags::x1y0 | SegmentFlags::x2y1 | SegmentFlags::x1y2,
    };

    constexpr std::array<TrackPaintPiece, 4> kRightCurveSmallSteepSlopeDownTPP = {
        kRightCurveSmallSteepSlopeDown0,
        kRightCurveSmallSteepSlopeDown1,
        kRightCurveSmallSteepSlopeDown2,
        kRightCurveSmallSteepSlopeDown3,
    };

    constexpr TrackPaintPiece kLeftCurveSmallSteepSlopeUp0 = rotateTrackPP(kRightCurveSmallSteepSlopeDown3, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmallSteepSlopeUp1 = rotateTrackPP(kRightCurveSmallSteepSlopeDown1, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmallSteepSlopeUp2 = rotateTrackPP(kRightCurveSmallSteepSlopeDown2, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmallSteepSlopeUp3 = rotateTrackPP(kRightCurveSmallSteepSlopeDown0, kRotationTable1230);

    constexpr std::array<TrackPaintPiece, 4> kLeftCurveSmallSteepSlopeUpTPP = {
        kLeftCurveSmallSteepSlopeUp0,
        kLeftCurveSmallSteepSlopeUp1,
        kLeftCurveSmallSteepSlopeUp2,
        kLeftCurveSmallSteepSlopeUp3,
    };

    constexpr TrackPaintPiece kLeftCurveSmallSteepSlopeDown0 = rotateTrackPP(kRightCurveSmallSteepSlopeUp3, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmallSteepSlopeDown1 = rotateTrackPP(kRightCurveSmallSteepSlopeUp1, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmallSteepSlopeDown2 = rotateTrackPP(kRightCurveSmallSteepSlopeUp2, kRotationTable1230);

    constexpr TrackPaintPiece kLeftCurveSmallSteepSlopeDown3 = rotateTrackPP(kRightCurveSmallSteepSlopeUp0, kRotationTable1230);

    constexpr std::array<TrackPaintPiece, 4> kLeftCurveSmallSteepSlopeDownTPP = {
        kLeftCurveSmallSteepSlopeDown0,
        kLeftCurveSmallSteepSlopeDown1,
        kLeftCurveSmallSteepSlopeDown2,
        kLeftCurveSmallSteepSlopeDown3,
    };

    constexpr std::array<std::span<const TrackPaintPiece>, 26> kTrackPaintParts = {
        kStraightTPP,
        kDiagonalTPP,
        kLeftCurveVerySmallTPP,
        kRightCurveVerySmallTPP,
        kLeftCurveSmallTPP,
        kRightCurveSmallTPP,
        kLeftCurveTPP,
        kRightCurveTPP,
        kLeftCurveLargeTPP,
        kRightCurveLargeTPP,
        kDiagonalLeftCurveLargeTPP,
        kDiagonalRightCurveLargeTPP,
        kSBendLeftTPP,
        kSBendRightTPP,
        kStraightSlopeUpTPP,
        kStraightSlopeDownTPP,
        kStraightSteepSlopeUpTPP,
        kStraightSteepSlopeDownTPP,
        kLeftCurveSmallSlopeUpTPP,
        kRightCurveSmallSlopeUpTPP,
        kLeftCurveSmallSlopeDownTPP,
        kRightCurveSmallSlopeDownTPP,
        kLeftCurveSmallSteepSlopeUpTPP,
        kRightCurveSmallSteepSlopeUpTPP,
        kLeftCurveSmallSteepSlopeDownTPP,
        kRightCurveSmallSteepSlopeDownTPP,
    };

}
