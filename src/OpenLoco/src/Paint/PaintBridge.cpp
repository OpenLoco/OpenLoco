#include "PaintBridge.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Objects/BridgeObject.h"
#include "Objects/ObjectManager.h"
#include "Paint.h"
#include "Ui/ViewportInteraction.h"
#include <OpenLoco/Core/Numerics.hpp>

namespace OpenLoco::Paint
{
    struct DeckImages
    {
        uint32_t deck;
        uint32_t wallLhs;
        uint32_t wallRhs;
    };

    // Based on 0x004F91FE
    constexpr std::array<DeckImages, 16> k4F91FE = {
        DeckImages{
            Bridge::ImageIds::deckBaseGentleSlopeNE0,
            Bridge::ImageIds::deckWallLhsGentleSlopeNE0,
            Bridge::ImageIds::deckWallRhsGentleSlopeNE0,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseGentleSlopeNE1,
            Bridge::ImageIds::deckWallLhsGentleSlopeNE1,
            Bridge::ImageIds::deckWallRhsGentleSlopeNE1,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseGentleSlopeSE0,
            Bridge::ImageIds::deckWallLhsGentleSlopeSE0,
            Bridge::ImageIds::deckWallRhsGentleSlopeSE0,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseGentleSlopeSE1,
            Bridge::ImageIds::deckWallLhsGentleSlopeSE1,
            Bridge::ImageIds::deckWallRhsGentleSlopeSE1,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseGentleSlopeSW0,
            Bridge::ImageIds::deckWallLhsGentleSlopeSW0,
            Bridge::ImageIds::deckWallRhsGentleSlopeSW0,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseGentleSlopeSW1,
            Bridge::ImageIds::deckWallLhsGentleSlopeSW1,
            Bridge::ImageIds::deckWallRhsGentleSlopeSW1,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseGentleSlopeNW0,
            Bridge::ImageIds::deckWallLhsGentleSlopeNW0,
            Bridge::ImageIds::deckWallRhsGentleSlopeNW0,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseGentleSlopeNW1,
            Bridge::ImageIds::deckWallLhsGentleSlopeNW1,
            Bridge::ImageIds::deckWallRhsGentleSlopeNW1,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseSteepSlopeNE,
            Bridge::ImageIds::deckWallLhsSteepSlopeNE,
            Bridge::ImageIds::deckWallRhsSteepSlopeNE,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseSteepSlopeSE,
            Bridge::ImageIds::deckWallLhsSteepSlopeSE,
            Bridge::ImageIds::deckWallRhsSteepSlopeSE,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseSteepSlopeSW,
            Bridge::ImageIds::deckWallLhsSteepSlopeSW,
            Bridge::ImageIds::deckWallRhsSteepSlopeSW,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseSteepSlopeNW,
            Bridge::ImageIds::deckWallLhsSteepSlopeNW,
            Bridge::ImageIds::deckWallRhsSteepSlopeNW,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseSlopedCurveEdge0,
            0,
            0,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseSlopedCurveEdge1,
            0,
            0,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseSlopedCurveEdge2,
            0,
            0,
        },
        DeckImages{
            Bridge::ImageIds::deckBaseSlopedCurveEdge3,
            0,
            0,
        },
    };

    struct SpanImages
    {
        uint32_t deck;
        uint32_t supportHeaderLhs;
        uint32_t supportHeaderRhs;
        uint32_t wallLhs;
        uint32_t wallRhs;
        uint32_t roof;
    };

    std::array<std::array<SpanImages, 4>, 2> kSpanImages = {
        std::array<SpanImages, 4>{
            SpanImages{
                Bridge::ImageIds::deckBaseNEspan0,
                Bridge::ImageIds::supportHeaderLhsNEspan0,
                Bridge::ImageIds::supportHeaderRhsNEspan0,
                Bridge::ImageIds::deckWallLhsNEspan0,
                Bridge::ImageIds::deckWallRhsNEspan0,
                Bridge::ImageIds::roofNEspan0 },
            SpanImages{
                Bridge::ImageIds::deckBaseNEspan1,
                Bridge::ImageIds::supportHeaderLhsNEspan1,
                Bridge::ImageIds::supportHeaderRhsNEspan1,
                Bridge::ImageIds::deckWallLhsNEspan1,
                Bridge::ImageIds::deckWallRhsNEspan1,
                Bridge::ImageIds::roofNEspan1 },
            SpanImages{
                Bridge::ImageIds::deckBaseNEspan2,
                Bridge::ImageIds::supportHeaderLhsNEspan2,
                Bridge::ImageIds::supportHeaderRhsNEspan2,
                Bridge::ImageIds::deckWallLhsNEspan2,
                Bridge::ImageIds::deckWallRhsNEspan2,
                Bridge::ImageIds::roofNEspan2 },
            SpanImages{
                Bridge::ImageIds::deckBaseNEspan3,
                Bridge::ImageIds::supportHeaderLhsNEspan3,
                Bridge::ImageIds::supportHeaderRhsNEspan3,
                Bridge::ImageIds::deckWallLhsNEspan3,
                Bridge::ImageIds::deckWallRhsNEspan3,
                Bridge::ImageIds::roofNEspan3 },
        },
        std::array<SpanImages, 4>{
            SpanImages{
                Bridge::ImageIds::deckBaseSWspan0,
                Bridge::ImageIds::supportHeaderLhsSWspan0,
                Bridge::ImageIds::supportHeaderRhsSWspan0,
                Bridge::ImageIds::deckWallLhsSWspan0,
                Bridge::ImageIds::deckWallRhsSWspan0,
                Bridge::ImageIds::roofSWspan0 },
            SpanImages{
                Bridge::ImageIds::deckBaseSWspan1,
                Bridge::ImageIds::supportHeaderLhsSWspan1,
                Bridge::ImageIds::supportHeaderRhsSWspan1,
                Bridge::ImageIds::deckWallLhsSWspan1,
                Bridge::ImageIds::deckWallRhsSWspan1,
                Bridge::ImageIds::roofSWspan1 },
            SpanImages{
                Bridge::ImageIds::deckBaseSWspan2,
                Bridge::ImageIds::supportHeaderLhsSWspan2,
                Bridge::ImageIds::supportHeaderRhsSWspan2,
                Bridge::ImageIds::deckWallLhsSWspan2,
                Bridge::ImageIds::deckWallRhsSWspan2,
                Bridge::ImageIds::roofSWspan2 },
            SpanImages{
                Bridge::ImageIds::deckBaseSWspan3,
                Bridge::ImageIds::supportHeaderLhsSWspan3,
                Bridge::ImageIds::supportHeaderRhsSWspan3,
                Bridge::ImageIds::deckWallLhsSWspan3,
                Bridge::ImageIds::deckWallRhsSWspan3,
                Bridge::ImageIds::roofSWspan3 },
        },
    };

    constexpr std::array<uint16_t, 17> k4F91DC = {
        0,
        5,
        5,
        9,
        9,
        5,
        5,
        9,
        9,
        5,
        9,
        5,
        9,
        17,
        33,
        65,
        129,
    };
    // Based on 0x004F905C
    constexpr std::array<uint32_t, 32> kSlopeToSupportEdgeImageRhsNE = {
        0,
        Bridge::ImageIds::supportSegmentRhsSlope2NE,
        Bridge::ImageIds::supportSegmentRhsSlope1NE,
        0,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope2NE,
        Bridge::ImageIds::supportSegmentRhsSlope1NE,
        0,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope2NE,
        Bridge::ImageIds::supportSegmentRhsSlope1NE,
        0,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope2NE,
        Bridge::ImageIds::supportSegmentRhsSlope1NE,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope1NE,
        0,
        0,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope2NE,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope2NE,
        Bridge::ImageIds::supportSegmentRhsSlope1NE,
        0,
    };
    // Based on 0x004F909C
    constexpr std::array<uint32_t, 32> kSlopeToSupportEdgeImageLhsNE = {
        0,
        0,
        0,
        0,
        Bridge::ImageIds::supportSegmentLhsSlope1NE,
        Bridge::ImageIds::supportSegmentLhsSlope1NE,
        Bridge::ImageIds::supportSegmentLhsSlope1NE,
        Bridge::ImageIds::supportSegmentLhsSlope1NE,
        Bridge::ImageIds::supportSegmentLhsSlope2NE,
        Bridge::ImageIds::supportSegmentLhsSlope2NE,
        Bridge::ImageIds::supportSegmentLhsSlope2NE,
        Bridge::ImageIds::supportSegmentLhsSlope2NE,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        Bridge::ImageIds::supportSegmentLhsSlope1NE,
        0,
        0,
        0,
        Bridge::ImageIds::supportSegmentLhsSlope2NE,
        0,
        Bridge::ImageIds::supportSegmentLhsSlope2NE,
        Bridge::ImageIds::supportSegmentLhsSlope1NE,
        0,
    };
    constexpr std::array<int16_t, 32> k4F8F5C = {
        0,
        16,
        16,
        16,
        0,
        16,
        16,
        16,
        0,
        16,
        16,
        16,
        0,
        16,
        16,
        16,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        32,
        0,
        0,
        0,
        32,
        0,
        16,
        16,
        0
    };
    constexpr std::array<int16_t, 32> k4F8F9C = {
        0,
        0,
        0,
        0,
        16,
        16,
        16,
        16,
        16,
        16,
        16,
        16,
        16,
        16,
        16,
        16,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        16,
        0,
        0,
        0,
        16,
        0,
        32,
        32,
        0,
    };

    constexpr std::array<int16_t, 32> k4F8FDC = {
        0,
        0,
        16,
        16,
        16,
        16,
        16,
        16,
        0,
        0,
        16,
        16,
        16,
        16,
        16,
        16,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        32,
        0,
        0,
        0,
        16,
        0,
        16,
        32,
        0,
    };

    constexpr std::array<int16_t, 32> k4F901C = {
        0,
        16,
        0,
        16,
        0,
        16,
        16,
        16,
        16,
        16,
        16,
        16,
        16,
        16,
        16,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        16,
        0,
        0,
        0,
        32,
        0,
        32,
        16,
        0,
    };
    // Based on 0x004F90DC
    constexpr std::array<uint32_t, 32> kSlopeToSupportEdgeImageLhsSW = {
        0,
        0,
        Bridge::ImageIds::supportSegmentLhsSlope2SW,
        Bridge::ImageIds::supportSegmentLhsSlope2SW,
        Bridge::ImageIds::supportSegmentLhsSlope1SW,
        Bridge::ImageIds::supportSegmentLhsSlope1SW,
        0,
        0,
        0,
        0,
        Bridge::ImageIds::supportSegmentLhsSlope2SW,
        Bridge::ImageIds::supportSegmentLhsSlope2SW,
        Bridge::ImageIds::supportSegmentLhsSlope1SW,
        Bridge::ImageIds::supportSegmentLhsSlope1SW,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        Bridge::ImageIds::supportSegmentLhsSlope2SW,
        0,
        0,
        0,
        Bridge::ImageIds::supportSegmentLhsSlope2SW,
        0,
        Bridge::ImageIds::supportSegmentLhsSlope1SW,
        Bridge::ImageIds::supportSegmentLhsSlope1SW,
        0,
    };
    // Based on 0x004F911C
    constexpr std::array<uint32_t, 32> kSlopeToSupportEdgeImageRhsSW = {
        0,
        Bridge::ImageIds::supportSegmentRhsSlope2SW,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope2SW,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope2SW,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope2SW,
        Bridge::ImageIds::supportSegmentRhsSlope1SW,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope1SW,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope1SW,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope1SW,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope2SW,
        0,
        0,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope2SW,
        0,
        Bridge::ImageIds::supportSegmentRhsSlope1SW,
        Bridge::ImageIds::supportSegmentRhsSlope1SW,
        0,
    };
    constexpr std::array<int16_t, 32> k4F915C = {
        0,
        0,
        16,
        16,
        0,
        0,
        16,
        16,
        0,
        0,
        16,
        16,
        0,
        0,
        16,
        16,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        32,
        0,
        0,
        0,
        16,
        0,
        0,
        16,
        0
    };

    constexpr std::array<int16_t, 32> k4F919C = {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        16,
        16,
        16,
        16,
        16,
        16,
        16,
        16,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        16,
        0,
        32,
        16,
        0,
    };

    // DUPLICTED FROM PAINTSURFACE
    // 0x004FD97E
    // Truncates a SurfaceSlope slope into only the representable values
    // input is SurfaceSlope 0 <-> 31 with some unrepresentable
    // output is 0 <-> 18
    // unrepresentable will be displayed at 0 (flat)
    static constexpr std::array<uint8_t, 32> kSlopeToDisplaySlope = {
        0,
        2,
        1,
        3,
        8,
        10,
        9,
        11,
        4,
        6,
        5,
        7,
        12,
        14,
        13,
        15,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        17,
        0,
        0,
        0,
        16,
        0,
        18,
        15,
        0,
    };

    static constexpr std::array<std::array<uint32_t, 19>, 5> kSlopeToBridgeShadow = {
        std::array<uint32_t, 19>{
            ImageIds::bridgeShadowFullTileSlope0,
            ImageIds::bridgeShadowFullTileSlope1,
            ImageIds::bridgeShadowFullTileSlope2,
            ImageIds::bridgeShadowFullTileSlope3,
            ImageIds::bridgeShadowFullTileSlope4,
            ImageIds::bridgeShadowFullTileSlope5,
            ImageIds::bridgeShadowFullTileSlope6,
            ImageIds::bridgeShadowFullTileSlope7,
            ImageIds::bridgeShadowFullTileSlope8,
            ImageIds::bridgeShadowFullTileSlope9,
            ImageIds::bridgeShadowFullTileSlope10,
            ImageIds::bridgeShadowFullTileSlope11,
            ImageIds::bridgeShadowFullTileSlope12,
            ImageIds::bridgeShadowFullTileSlope13,
            ImageIds::bridgeShadowFullTileSlope14,
            ImageIds::bridgeShadowFullTileSlope15,
            ImageIds::bridgeShadowFullTileSlope16,
            ImageIds::bridgeShadowFullTileSlope17,
            ImageIds::bridgeShadowFullTileSlope18,
        },
        std::array<uint32_t, 19>{
            ImageIds::bridgeShadowQuarter0Slope0,
            ImageIds::bridgeShadowQuarter0Slope1,
            ImageIds::bridgeShadowQuarter0Slope2,
            ImageIds::bridgeShadowQuarter0Slope3,
            ImageIds::bridgeShadowQuarter0Slope4,
            ImageIds::bridgeShadowQuarter0Slope5,
            ImageIds::bridgeShadowQuarter0Slope6,
            ImageIds::bridgeShadowQuarter0Slope7,
            ImageIds::bridgeShadowQuarter0Slope8,
            ImageIds::bridgeShadowQuarter0Slope9,
            ImageIds::bridgeShadowQuarter0Slope10,
            ImageIds::bridgeShadowQuarter0Slope11,
            ImageIds::bridgeShadowQuarter0Slope12,
            ImageIds::bridgeShadowQuarter0Slope13,
            ImageIds::bridgeShadowQuarter0Slope14,
            ImageIds::bridgeShadowQuarter0Slope15,
            ImageIds::bridgeShadowQuarter0Slope16,
            ImageIds::bridgeShadowQuarter0Slope17,
            ImageIds::bridgeShadowQuarter0Slope18,
        },
        std::array<uint32_t, 19>{
            ImageIds::bridgeShadowQuarter1Slope0,
            ImageIds::bridgeShadowQuarter1Slope1,
            ImageIds::bridgeShadowQuarter1Slope2,
            ImageIds::bridgeShadowQuarter1Slope3,
            ImageIds::bridgeShadowQuarter1Slope4,
            ImageIds::bridgeShadowQuarter1Slope5,
            ImageIds::bridgeShadowQuarter1Slope6,
            ImageIds::bridgeShadowQuarter1Slope7,
            ImageIds::bridgeShadowQuarter1Slope8,
            ImageIds::bridgeShadowQuarter1Slope9,
            ImageIds::bridgeShadowQuarter1Slope10,
            ImageIds::bridgeShadowQuarter1Slope11,
            ImageIds::bridgeShadowQuarter1Slope12,
            ImageIds::bridgeShadowQuarter1Slope13,
            ImageIds::bridgeShadowQuarter1Slope14,
            ImageIds::bridgeShadowQuarter1Slope15,
            ImageIds::bridgeShadowQuarter1Slope16,
            ImageIds::bridgeShadowQuarter1Slope17,
            ImageIds::bridgeShadowQuarter1Slope18,
        },
        std::array<uint32_t, 19>{
            ImageIds::bridgeShadowQuarter2Slope0,
            ImageIds::bridgeShadowQuarter2Slope1,
            ImageIds::bridgeShadowQuarter2Slope2,
            ImageIds::bridgeShadowQuarter2Slope3,
            ImageIds::bridgeShadowQuarter2Slope4,
            ImageIds::bridgeShadowQuarter2Slope5,
            ImageIds::bridgeShadowQuarter2Slope6,
            ImageIds::bridgeShadowQuarter2Slope7,
            ImageIds::bridgeShadowQuarter2Slope8,
            ImageIds::bridgeShadowQuarter2Slope9,
            ImageIds::bridgeShadowQuarter2Slope10,
            ImageIds::bridgeShadowQuarter2Slope11,
            ImageIds::bridgeShadowQuarter2Slope12,
            ImageIds::bridgeShadowQuarter2Slope13,
            ImageIds::bridgeShadowQuarter2Slope14,
            ImageIds::bridgeShadowQuarter2Slope15,
            ImageIds::bridgeShadowQuarter2Slope16,
            ImageIds::bridgeShadowQuarter2Slope17,
            ImageIds::bridgeShadowQuarter2Slope18,
        },
        std::array<uint32_t, 19>{
            ImageIds::bridgeShadowQuarter3Slope0,
            ImageIds::bridgeShadowQuarter3Slope1,
            ImageIds::bridgeShadowQuarter3Slope2,
            ImageIds::bridgeShadowQuarter3Slope3,
            ImageIds::bridgeShadowQuarter3Slope4,
            ImageIds::bridgeShadowQuarter3Slope5,
            ImageIds::bridgeShadowQuarter3Slope6,
            ImageIds::bridgeShadowQuarter3Slope7,
            ImageIds::bridgeShadowQuarter3Slope8,
            ImageIds::bridgeShadowQuarter3Slope9,
            ImageIds::bridgeShadowQuarter3Slope10,
            ImageIds::bridgeShadowQuarter3Slope11,
            ImageIds::bridgeShadowQuarter3Slope12,
            ImageIds::bridgeShadowQuarter3Slope13,
            ImageIds::bridgeShadowQuarter3Slope14,
            ImageIds::bridgeShadowQuarter3Slope15,
            ImageIds::bridgeShadowQuarter3Slope16,
            ImageIds::bridgeShadowQuarter3Slope17,
            ImageIds::bridgeShadowQuarter3Slope18,
        },
    };

    struct SupportLengths
    {
        int16_t lhsSupportLength; // dx
        int16_t rhsSupportLength; // 0x00525CFC
    };

    static void paintFlatSingleQuarterNoSupport(PaintSession& session, const BridgeObject& bridgeObj, const BridgeEntry& bridgeEntry, const std::array<uint32_t, 3>& imageIndexs, const World::Pos3& wallBoundingBoxOffset)
    {
        const auto baseHeightOffset = World::Pos3{ 0, 0, bridgeEntry.height };

        if ((bridgeObj.flags & BridgeObjectFlags::hasRoof) != BridgeObjectFlags::none)
        {
            const auto roofImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(imageIndexs[0]);
            constexpr World::Pos3 bbOffset = { 0, 0, 30 };
            constexpr World::Pos3 bbLength = { 32, 32, 0 };
            session.addToPlotList4FD150(roofImage, baseHeightOffset, bbOffset + baseHeightOffset, bbLength);
        }
        const auto offset = baseHeightOffset - World::Pos3{ 0, 0, 16 };
        const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(imageIndexs[1]);
        constexpr World::Pos3 bbOffset2 = { 0, 0, 14 };
        constexpr World::Pos3 bbLength2 = { 32, 32, 1 };
        session.addToPlotList4FD150(image, offset, bbOffset2 + offset, bbLength2);

        if (bridgeEntry.subType == 0)
        {
            const auto wallImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(imageIndexs[2]);
            constexpr World::Pos3 bbLength3 = { 2, 2, 26 };
            session.addToPlotList4FD150(wallImage, baseHeightOffset, wallBoundingBoxOffset + baseHeightOffset, bbLength3);
        }
    }

    // 0x0042C36D
    static void paintSupportDiagonal(PaintSession& session, const BridgeObject& bridgeObj, const BridgeEntry& bridgeEntry, int16_t lhsSupportLength, int16_t rhsSupportLength, const int16_t supportHeight)
    {
        {
            auto rhsSupportHeight = supportHeight;
            while (rhsSupportLength >= 16)
            {
                const bool is16section = rhsSupportLength == 16 || ((rhsSupportHeight - 16) == session.getWaterHeight());
                const int16_t sectionHeight = is16section ? 16 : 32;
                const auto bbLength = is16section ? World::Pos3{ 2, 2, 15 } : World::Pos3{ 2, 2, 31 };
                const auto imageIndex = is16section ? Bridge::ImageIds::supportSegmentEdge3Lhs16SW : Bridge::ImageIds::supportSegmentEdge3Lhs32SW;

                rhsSupportHeight -= sectionHeight;
                rhsSupportLength -= sectionHeight;
                const auto supportSectionImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(imageIndex);
                const auto heightOffset = World::Pos3{ 0, 0, rhsSupportHeight };
                constexpr World::Pos3 bbOffset = { 30, 0, 0 };
                session.addToPlotList4FD150(supportSectionImage, heightOffset, bbOffset + heightOffset, bbLength);
            }
        }

        {
            auto lhsSupportHeight = supportHeight;
            while (lhsSupportLength >= 16)
            {
                const bool is16section = lhsSupportLength == 16 || ((lhsSupportHeight - 16) == session.getWaterHeight());
                const int16_t sectionHeight = is16section ? 16 : 32;
                const auto bbLength = is16section ? World::Pos3{ 2, 2, 15 } : World::Pos3{ 2, 2, 31 };
                const auto imageIndex = is16section ? Bridge::ImageIds::supportSegmentEdge3Rhs16SW : Bridge::ImageIds::supportSegmentEdge3Rhs32SW;

                lhsSupportLength -= sectionHeight;
                lhsSupportHeight -= sectionHeight;
                const auto supportSectionImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(imageIndex);
                const auto heightOffset = World::Pos3{ 0, 0, lhsSupportHeight };
                constexpr World::Pos3 bbOffset = { 0, 30, 0 };
                session.addToPlotList4FD150(supportSectionImage, heightOffset, bbOffset + heightOffset, bbLength);
            }
        }
    }

    // 0x0042BF7F
    static void paintSupportNE(PaintSession& session, const BridgeObject& bridgeObj, const BridgeEntry& bridgeEntry, const uint8_t pillarSpacing, int16_t lhsSupportLength, int16_t rhsSupportLength, const int16_t supportHeight, const uint32_t supportEdgeLhsImage, const uint32_t supportEdgeRhsImage)
    {
        if (pillarSpacing & (1U << 0))
        {
            auto lhsSupportHeight = supportHeight;
            while (lhsSupportLength >= 16)
            {
                const bool is16section = lhsSupportLength == 16 || ((lhsSupportHeight - 16) == session.getWaterHeight());
                const int16_t sectionHeight = is16section ? 16 : 32;
                const auto bbLength = is16section ? World::Pos3{ 2, 32, 15 } : World::Pos3{ 2, 32, 31 };
                const auto imageIndex = is16section ? Bridge::ImageIds::supportSegmentLhs16NE : Bridge::ImageIds::supportSegmentLhs32NE;

                lhsSupportLength -= sectionHeight;
                lhsSupportHeight -= sectionHeight;
                const auto supportSectionImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(imageIndex);
                const auto heightOffset = World::Pos3{ 0, 0, lhsSupportHeight };
                constexpr World::Pos3 bbOffset = { 0, 0, 0 };
                session.addToPlotList4FD150(supportSectionImage, heightOffset, bbOffset + heightOffset, bbLength);
            }
            if (supportEdgeLhsImage != 0)
            {
                lhsSupportHeight -= 16;
                const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(supportEdgeLhsImage);
                const auto heightOffset = World::Pos3{ 0, 0, lhsSupportHeight };
                constexpr World::Pos3 bbOffset = { 0, 0, 0 };
                constexpr World::Pos3 bbLength = { 2, 32, 14 };
                session.addToPlotList4FD150(image, heightOffset, bbOffset + heightOffset, bbLength);
            }
        }

        if (pillarSpacing & (1U << 1))
        {
            auto rhsSupportHeight = supportHeight;
            while (rhsSupportLength >= 16)
            {
                const bool is16section = rhsSupportLength == 16 || ((rhsSupportHeight - 16) == session.getWaterHeight());
                const int16_t sectionHeight = is16section ? 16 : 32;
                const auto bbLength = is16section ? World::Pos3{ 2, 32, 15 } : World::Pos3{ 2, 32, 31 };
                const auto imageIndex = is16section ? Bridge::ImageIds::supportSegmentRhs16NE : Bridge::ImageIds::supportSegmentRhs32NE;

                rhsSupportLength -= sectionHeight;
                rhsSupportHeight -= sectionHeight;
                const auto supportSectionImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(imageIndex);
                const auto heightOffset = World::Pos3{ 0, 0, rhsSupportHeight };
                constexpr World::Pos3 bbOffset = { 30, 0, 0 };
                session.addToPlotList4FD150(supportSectionImage, heightOffset, bbOffset + heightOffset, bbLength);
            }
            if (supportEdgeRhsImage != 0)
            {
                rhsSupportHeight -= 16;
                const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(supportEdgeRhsImage);
                const auto heightOffset = World::Pos3{ 0, 0, rhsSupportHeight };
                constexpr World::Pos3 bbOffset = { 30, 0, 0 };
                constexpr World::Pos3 bbLength = { 2, 32, 14 };
                session.addToPlotList4FD150(image, heightOffset, bbOffset + heightOffset, bbLength);
            }
        }
    }

    // 0x0042C176
    static void paintSupportSW(PaintSession& session, const BridgeObject& bridgeObj, const BridgeEntry& bridgeEntry, const uint8_t pillarSpacing, int16_t lhsSupportLength, int16_t rhsSupportLength, const int16_t supportHeight, const uint32_t supportEdgeLhsImage, const uint32_t supportEdgeRhsImage)
    {
        if (pillarSpacing & (1U << 0))
        {
            auto lhsSupportHeight = supportHeight;
            while (lhsSupportLength >= 16)
            {
                const bool is16section = lhsSupportLength == 16 || ((lhsSupportHeight - 16) == session.getWaterHeight());
                const int16_t sectionHeight = is16section ? 16 : 32;
                const auto bbLength = is16section ? World::Pos3{ 32, 2, 15 } : World::Pos3{ 32, 2, 31 };
                const auto imageIndex = is16section ? Bridge::ImageIds::supportSegmentLhs16SW : Bridge::ImageIds::supportSegmentLhs32SW;

                lhsSupportLength -= sectionHeight;
                lhsSupportHeight -= sectionHeight;
                const auto supportSectionImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(imageIndex);
                const auto heightOffset = World::Pos3{ 0, 0, lhsSupportHeight };
                constexpr World::Pos3 bbOffset = { 0, 0, 0 };
                session.addToPlotList4FD150(supportSectionImage, heightOffset, bbOffset + heightOffset, bbLength);
            }
            if (supportEdgeLhsImage != 0)
            {
                lhsSupportHeight -= 16;
                const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(supportEdgeLhsImage);
                const auto heightOffset = World::Pos3{ 0, 0, lhsSupportHeight };
                constexpr World::Pos3 bbOffset = { 0, 0, 0 };
                constexpr World::Pos3 bbLength = { 32, 2, 14 };
                session.addToPlotList4FD150(image, heightOffset, bbOffset + heightOffset, bbLength);
            }
        }

        if (pillarSpacing & (1U << 1))
        {
            auto rhsSupportHeight = supportHeight;
            while (rhsSupportLength >= 16)
            {
                const bool is16section = rhsSupportLength == 16 || ((rhsSupportHeight - 16) == session.getWaterHeight());
                const int16_t sectionHeight = is16section ? 16 : 32;
                const auto bbLength = is16section ? World::Pos3{ 32, 2, 15 } : World::Pos3{ 32, 2, 31 };
                const auto imageIndex = is16section ? Bridge::ImageIds::supportSegmentRhs16SW : Bridge::ImageIds::supportSegmentRhs32SW;

                rhsSupportLength -= sectionHeight;
                rhsSupportHeight -= sectionHeight;
                const auto supportSectionImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(imageIndex);
                const auto heightOffset = World::Pos3{ 0, 0, rhsSupportHeight };
                constexpr World::Pos3 bbOffset = { 0, 30, 0 };
                session.addToPlotList4FD150(supportSectionImage, heightOffset, bbOffset + heightOffset, bbLength);
            }
            if (supportEdgeRhsImage != 0)
            {
                rhsSupportHeight -= 16;
                const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(supportEdgeRhsImage);
                const auto heightOffset = World::Pos3{ 0, 0, rhsSupportHeight };
                constexpr World::Pos3 bbOffset = { 0, 30, 0 };
                constexpr World::Pos3 bbLength = { 32, 2, 14 };
                session.addToPlotList4FD150(image, heightOffset, bbOffset + heightOffset, bbLength);
            }
        }
    }

    // SPECIAL needs to do the front supports to ground as well
    // 0x0042BCD5
    static void paintFlatSingleQuarterSupportFront(PaintSession& session, const BridgeObject& bridgeObj, const BridgeEntry& bridgeEntry, const int16_t supportLength, const uint8_t slope)
    {
        const auto baseHeightOffset = World::Pos3{ 0, 0, bridgeEntry.height };

        if ((bridgeObj.flags & BridgeObjectFlags::hasRoof) != BridgeObjectFlags::none)
        {
            const auto roofImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(Bridge::ImageIds::roofEdge3);
            constexpr World::Pos3 bbOffset = { 0, 0, 30 };
            constexpr World::Pos3 bbLength = { 32, 32, 0 };
            session.addToPlotList4FD150(roofImage, baseHeightOffset, bbOffset + baseHeightOffset, bbLength);
        }

        const auto supportLengths = [&session, &bridgeObj, &bridgeEntry, supportLength, slope]() -> std::optional<SupportLengths> {
            if (session.getSupportHeight(1).height == 0xFFFFU)
            {
                return std::nullopt;
            }
            if (session.getSupportHeight(2).height == 0xFFFFU)
            {
                return std::nullopt;
            }
            int16_t unkHeight = supportLength - bridgeObj.deckDepth;
            if (unkHeight < 0)
            {
                return std::nullopt;
            }
            if (bridgeObj.deckDepth == 32)
            {
                unkHeight = bridgeEntry.height - 16;
                if (unkHeight == session.getWaterHeight())
                {
                    return std::nullopt;
                }
            }

            const int16_t rhsSupportLength = unkHeight - k4F915C[slope];
            if (rhsSupportLength < 0)
            {
                return std::nullopt;
            }

            const int16_t lhsSupportLenght = unkHeight - k4F919C[slope];
            if (unkHeight < 0)
            {
                return std::nullopt;
            }
            return SupportLengths{ lhsSupportLenght, rhsSupportLength };
        }();
        if (supportLengths.has_value())
        {
            // 0x0042BD9C

            const auto& [lhsSupportLenght, rhsSupportLength] = supportLengths.value();
            const auto supportHeight = bridgeEntry.height - bridgeObj.deckDepth;

            const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(Bridge::ImageIds::deckBaseWithSupportHeaderEdge3);
            const auto offset = baseHeightOffset - World::Pos3{ 0, 0, bridgeObj.deckDepth };
            constexpr World::Pos3 bbOffset = { 0, 0, 14 };
            constexpr World::Pos3 bbLength = { 32, 32, 1 };
            session.addToPlotList4FD150(image, offset, bbOffset + offset, bbLength);

            paintSupportDiagonal(session, bridgeObj, bridgeEntry, lhsSupportLenght, rhsSupportLength, supportHeight);
        }
        else
        {
            // 0x0042BE22
            const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(Bridge::ImageIds::deckBaseNoSupportEdge3);
            const auto offset = baseHeightOffset - World::Pos3{ 0, 0, 16 };
            constexpr World::Pos3 bbOffset = { 0, 0, 14 };
            constexpr World::Pos3 bbLength = { 32, 32, 1 };
            session.addToPlotList4FD150(image, offset, bbOffset + offset, bbLength);
        }
        // 0x0042BE77

        if (bridgeEntry.subType == 0)
        {
            const auto wallImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(Bridge::ImageIds::deckWallEdge3);
            constexpr World::Pos3 bbOffset = { 17, 17, 2 };
            constexpr World::Pos3 bbLength3 = { 2, 2, 24 };
            session.addToPlotList4FD150(wallImage, baseHeightOffset, bbOffset + baseHeightOffset, bbLength3);
        }
    }

    // 0x0042B4DC
    static void paintBridgeNE(PaintSession& session, const BridgeObject& bridgeObj, const BridgeEntry& bridgeEntry, const int16_t supportLength, const uint8_t slope, const uint8_t pillarSpacing, const SpanImages& spanImages)
    {
        const auto baseHeightOffset = World::Pos3{ 0, 0, bridgeEntry.height };
        if ((bridgeObj.flags & BridgeObjectFlags::hasRoof) != BridgeObjectFlags::none)
        {
            const auto roofImageIdx = (bridgeEntry.edgesQuarters & 0xF0) == 0xF0 ? Bridge::ImageIds::roofFullTile : spanImages.roof;
            const auto roofImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(roofImageIdx);
            constexpr World::Pos3 bbOffset = { 0, 0, 30 };
            constexpr World::Pos3 bbLength = { 32, 32, 0 };
            session.addToPlotList4FD150(roofImage, baseHeightOffset, bbOffset + baseHeightOffset, bbLength);
        }

        const auto supportLengths = [&session, &bridgeEntry, &bridgeObj, pillarSpacing, supportLength, slope]() -> std::optional<SupportLengths> {
            if ((bridgeEntry.edgesQuarters & ((1U << 5) | (1U << 4))) == ((1U << 5) | (1U << 4)))
            {
                return std::nullopt;
            }
            if (pillarSpacing & (1U << 1))
            {
                if (session.getSupportHeight(1).height == 0xFFFFU
                    || session.getSupportHeight(3).height == 0xFFFFU
                    || session.getSupportHeight(7).height == 0xFFFFU)
                {
                    return std::nullopt;
                }
            }
            if (pillarSpacing & (1U << 0))
            {
                if (session.getSupportHeight(0).height == 0xFFFFU
                    || session.getSupportHeight(2).height == 0xFFFFU
                    || session.getSupportHeight(6).height == 0xFFFFU)
                {
                    return std::nullopt;
                }
            }

            int16_t unkHeight = supportLength - bridgeObj.deckDepth;
            if (unkHeight < 0)
            {
                return std::nullopt;
            }
            if (bridgeObj.deckDepth == 32)
            {
                unkHeight = bridgeEntry.height - 16;
                if (unkHeight == session.getWaterHeight())
                {
                    return std::nullopt;
                }
            }

            const int16_t rhsSupportLength = unkHeight - k4F8F5C[slope];
            if (rhsSupportLength < 0)
            {
                return std::nullopt;
            }

            const int16_t lhsSupportLength = unkHeight - k4F8F9C[slope];
            if (lhsSupportLength < 0)
            {
                return std::nullopt;
            }
            return SupportLengths{ lhsSupportLength, rhsSupportLength };
        }();

        if (supportLengths.has_value())
        {
            const auto& [lhsSupportLength, rhsSupportLength] = supportLengths.value();
            const auto supportEdgeRhsImage = kSlopeToSupportEdgeImageRhsNE[slope];
            const auto supportEdgeLhsImage = kSlopeToSupportEdgeImageLhsNE[slope];
            const auto supportHeight = bridgeEntry.height - bridgeObj.deckDepth;

            {
                const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(spanImages.deck);
                const auto offset = baseHeightOffset - World::Pos3{ 0, 0, bridgeObj.deckDepth };
                const World::Pos3 bbOffset = { 0, 0, static_cast<int16_t>(bridgeObj.deckDepth - 2) };
                constexpr World::Pos3 bbLength = { 32, 32, 1 };
                session.addToPlotList4FD150(image, offset, bbOffset + offset, bbLength);
            }
            {
                const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(spanImages.supportHeaderLhs);
                const auto offset = baseHeightOffset - World::Pos3{ 0, 0, bridgeObj.deckDepth };
                constexpr World::Pos3 bbOffset = { 0, 0, 0 };
                const World::Pos3 bbLength = { 2, 32, static_cast<int16_t>(bridgeObj.deckDepth - 3) };
                session.addToPlotList4FD150(image, offset, bbOffset + offset, bbLength);
            }
            {
                const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(spanImages.supportHeaderRhs);
                const auto offset = baseHeightOffset - World::Pos3{ 0, 0, bridgeObj.deckDepth };
                constexpr World::Pos3 bbOffset = { 30, 0, 0 };
                const World::Pos3 bbLength = { 2, 32, static_cast<int16_t>(bridgeObj.deckDepth - 3) };
                session.addToPlotList4FD150(image, offset, bbOffset + offset, bbLength);
            }

            paintSupportNE(session, bridgeObj, bridgeEntry, pillarSpacing, lhsSupportLength, rhsSupportLength, supportHeight, supportEdgeLhsImage, supportEdgeRhsImage);
        }
        else
        {
            const auto baseImageIdx = (bridgeEntry.edgesQuarters & 0xF0) == 0xF0 ? Bridge::ImageIds::deckBaseNoSupport : spanImages.deck;
            const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(baseImageIdx);
            const auto offset = baseHeightOffset - World::Pos3{ 0, 0, 16 };
            constexpr World::Pos3 bbOffset = { 0, 0, 14 };
            constexpr World::Pos3 bbLength = { 32, 32, 1 };
            session.addToPlotList4FD150(image, offset, bbOffset + offset, bbLength);
        }
    }

    // 0x0042B25D
    static void paintBridgeSW(PaintSession& session, const BridgeObject& bridgeObj, const BridgeEntry& bridgeEntry, const int16_t supportLength, const uint8_t slope, const uint8_t pillarSpacing, const SpanImages& spanImages)
    {
        const auto baseHeightOffset = World::Pos3{ 0, 0, bridgeEntry.height };
        if ((bridgeObj.flags & BridgeObjectFlags::hasRoof) != BridgeObjectFlags::none)
        {
            const auto roofImageIdx = (bridgeEntry.edgesQuarters & 0xF0) == 0xF0 ? Bridge::ImageIds::roofFullTile : spanImages.roof;
            const auto roofImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(roofImageIdx);
            constexpr World::Pos3 bbOffset = { 0, 0, 30 };
            constexpr World::Pos3 bbLength = { 32, 32, 0 };
            session.addToPlotList4FD150(roofImage, baseHeightOffset, bbOffset + baseHeightOffset, bbLength);
        }

        const auto supportLengths = [&session, &bridgeEntry, &bridgeObj, pillarSpacing, supportLength, slope]() -> std::optional<SupportLengths> {
            if ((bridgeEntry.edgesQuarters & ((1U << 6) | (1U << 5))) == ((1U << 6) | (1U << 5)))
            {
                return std::nullopt;
            }
            if (pillarSpacing & (1U << 0))
            {
                if (session.getSupportHeight(0).height == 0xFFFFU
                    || session.getSupportHeight(1).height == 0xFFFFU
                    || session.getSupportHeight(5).height == 0xFFFFU)
                {
                    return std::nullopt;
                }
            }
            if (pillarSpacing & (1U << 1))
            {
                if (session.getSupportHeight(2).height == 0xFFFFU
                    || session.getSupportHeight(3).height == 0xFFFFU
                    || session.getSupportHeight(8).height == 0xFFFFU)
                {
                    return std::nullopt;
                }
            }

            int16_t unkHeight = supportLength - bridgeObj.deckDepth;
            if (unkHeight < 0)
            {
                return std::nullopt;
            }
            if (bridgeObj.deckDepth == 32)
            {
                unkHeight = bridgeEntry.height - 16;
                if (unkHeight == session.getWaterHeight())
                {
                    return std::nullopt;
                }
            }

            const int16_t rhsSupportLength = unkHeight - k4F901C[slope];
            if (rhsSupportLength < 0)
            {
                return std::nullopt;
            }

            const int16_t lhsSupportLength = unkHeight - k4F8FDC[slope];
            if (lhsSupportLength < 0)
            {
                return std::nullopt;
            }
            return SupportLengths{ lhsSupportLength, rhsSupportLength };
        }();
        if (supportLengths.has_value())
        {
            const auto& [lhsSupportLength, rhsSupportLength] = supportLengths.value();
            const auto supportEdgeRhsImage = kSlopeToSupportEdgeImageRhsSW[slope];
            const auto supportEdgeLhsImage = kSlopeToSupportEdgeImageLhsSW[slope];
            const auto supportHeight = bridgeEntry.height - bridgeObj.deckDepth;

            {
                const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(spanImages.deck);
                const auto offset = baseHeightOffset - World::Pos3{ 0, 0, bridgeObj.deckDepth };
                const World::Pos3 bbOffset = { 0, 0, static_cast<int16_t>(bridgeObj.deckDepth - 2) };
                constexpr World::Pos3 bbLength = { 32, 32, 1 };
                session.addToPlotList4FD150(image, offset, bbOffset + offset, bbLength);
            }
            {
                const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(spanImages.supportHeaderLhs);
                const auto offset = baseHeightOffset - World::Pos3{ 0, 0, bridgeObj.deckDepth };
                constexpr World::Pos3 bbOffset = { 0, 0, 0 };
                const World::Pos3 bbLength = { 32, 2, static_cast<int16_t>(bridgeObj.deckDepth - 3) };
                session.addToPlotList4FD150(image, offset, bbOffset + offset, bbLength);
            }
            {
                const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(spanImages.supportHeaderRhs);
                const auto offset = baseHeightOffset - World::Pos3{ 0, 0, bridgeObj.deckDepth };
                constexpr World::Pos3 bbOffset = { 0, 30, 0 };
                const World::Pos3 bbLength = { 32, 2, static_cast<int16_t>(bridgeObj.deckDepth - 3) };
                session.addToPlotList4FD150(image, offset, bbOffset + offset, bbLength);
            }

            paintSupportSW(session, bridgeObj, bridgeEntry, pillarSpacing, lhsSupportLength, rhsSupportLength, supportHeight, supportEdgeLhsImage, supportEdgeRhsImage);
        }
        else
        {
            const auto baseImageIdx = (bridgeEntry.edgesQuarters & 0xF0) == 0xF0 ? Bridge::ImageIds::deckBaseNoSupport : spanImages.deck;
            const auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(baseImageIdx);
            const auto offset = baseHeightOffset - World::Pos3{ 0, 0, 16 };
            constexpr World::Pos3 bbOffset = { 0, 0, 14 };
            constexpr World::Pos3 bbLength = { 32, 32, 1 };
            session.addToPlotList4FD150(image, offset, bbOffset + offset, bbLength);
        }
    }

    // 0x0042AC9C
    bool paintBridge(PaintSession& session)
    {
        session.setItemType(Ui::ViewportInteraction::InteractionItem::bridge);

        auto& bridgeEntry = session.getBridgeEntry();
        if (bridgeEntry.isEmpty())
        {
            return false;
        }

        const auto genHeight = Numerics::ceil2(session.getGeneralSupportHeight().height, 16);
        const auto supportLength = bridgeEntry.height - genHeight;
        if (supportLength < 0)
        {
            return false;
        }

        auto* bridgeObj = ObjectManager::get<BridgeObject>(bridgeEntry.objectId);

        // Height to the base of the bridge platform
        const auto baseHeightOffset = World::Pos3{ 0, 0, bridgeEntry.height };

        if (k4F91DC[bridgeEntry.subType] & (1U << 0))
        {
            const auto& deckImages = k4F91FE[bridgeEntry.subType - 1];
            const auto deckImage = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(deckImages.deck);
            constexpr World::Pos3 bbOffset = { 2, 2, 0 };
            constexpr World::Pos3 bbLength = { 28, 28, 1 };
            session.addToPlotList4FD150(deckImage, baseHeightOffset, bbOffset + baseHeightOffset, bbLength);

            if (k4F91DC[bridgeEntry.subType] & (1U << 2))
            {
                if (!(bridgeEntry.edgesQuarters & (1U << 7)))
                {
                    const auto wallImage = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(deckImages.wallLhs);
                    constexpr World::Pos3 bbOffset2 = { 2, 0, 8 };
                    constexpr World::Pos3 bbLength2 = { 28, 1, 30 };
                    session.addToPlotList4FD150(wallImage, baseHeightOffset, bbOffset2 + baseHeightOffset, bbLength2);
                }
                if (!(bridgeEntry.edgesQuarters & (1U << 5)))
                {
                    const auto wallImage = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(deckImages.wallRhs);
                    constexpr World::Pos3 bbOffset2 = { 1, 30, 8 };
                    constexpr World::Pos3 bbLength2 = { 29, 1, 30 };
                    session.addToPlotList4FD150(wallImage, baseHeightOffset, bbOffset2 + baseHeightOffset, bbLength2);
                }
            }
            if (k4F91DC[bridgeEntry.subType] & (1U << 3))
            {
                if (!(bridgeEntry.edgesQuarters & (1U << 4)))
                {
                    const auto wallImage = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(deckImages.wallLhs);
                    constexpr World::Pos3 bbOffset2 = { 0, 2, 8 };
                    constexpr World::Pos3 bbLength2 = { 1, 28, 30 };
                    session.addToPlotList4FD150(wallImage, baseHeightOffset, bbOffset2 + baseHeightOffset, bbLength2);
                }
                if (!(bridgeEntry.edgesQuarters & (1U << 6)))
                {
                    const auto wallImage = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(deckImages.wallRhs);
                    constexpr World::Pos3 bbOffset2 = { 30, 1, 8 };
                    constexpr World::Pos3 bbLength2 = { 1, 29, 30 };
                    session.addToPlotList4FD150(wallImage, baseHeightOffset, bbOffset2 + baseHeightOffset, bbLength2);
                }
            }

            auto offset2 = baseHeightOffset + World::Pos3{ 0, 0, 8 };
            if (k4F91DC[bridgeEntry.subType] & (1U << 4))
            {
                const auto image2 = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(Bridge::ImageIds::deckWallEdge0);
                constexpr World::Pos3 bbOffset2 = { 22, 24, 0 };
                constexpr World::Pos3 bbLength2 = { 2, 2, 26 };
                session.addToPlotList4FD150(image2, offset2, bbOffset2 + offset2, bbLength2);
            }
            if (k4F91DC[bridgeEntry.subType] & (1U << 5))
            {
                const auto image2 = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(Bridge::ImageIds::deckWallEdge1);
                constexpr World::Pos3 bbOffset2 = { 7, 7, 0 };
                constexpr World::Pos3 bbLength2 = { 2, 2, 26 };
                session.addToPlotList4FD150(image2, offset2, bbOffset2 + offset2, bbLength2);
            }
            if (k4F91DC[bridgeEntry.subType] & (1U << 6))
            {
                const auto image2 = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(Bridge::ImageIds::deckWallEdge2);
                constexpr World::Pos3 bbOffset2 = { 24, 22, 0 };
                constexpr World::Pos3 bbLength2 = { 2, 2, 26 };
                session.addToPlotList4FD150(image2, offset2, bbOffset2 + offset2, bbLength2);
            }
            if (k4F91DC[bridgeEntry.subType] & (1U << 7))
            {
                const auto image2 = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(Bridge::ImageIds::deckWallEdge3);
                constexpr World::Pos3 bbOffset2 = { 17, 17, 2 };
                constexpr World::Pos3 bbLength2 = { 2, 2, 24 };
                session.addToPlotList4FD150(image2, offset2, bbOffset2 + offset2, bbLength2);
            }
        }

        if (!(session.getGeneralSupportHeight().slope & (1U << 5)) && supportLength == 0)
        {
            // No shadows generated for this
            return true;
        }

        // Shadow image changes depending on the deck quarters occupied
        auto shadowType = 0U;

        const uint8_t slope = session.getGeneralSupportHeight().slope & 0x1FU;
        const uint8_t quarters = bridgeEntry.edgesQuarters & 0xFU;

        switch (quarters)
        {
            case 1:
            {
                // 0x0042B9BA
                shadowType = 4;
                paintFlatSingleQuarterNoSupport(session, *bridgeObj, bridgeEntry, { Bridge::ImageIds::roofEdge0, Bridge::ImageIds::deckBaseNoSupportEdge0, Bridge::ImageIds::deckWallEdge0 }, { 22, 24, 0 });
            }
            break;
            case 2:
            {
                // 0x0042BAC3
                shadowType = 2;
                paintFlatSingleQuarterNoSupport(session, *bridgeObj, bridgeEntry, { Bridge::ImageIds::roofEdge1, Bridge::ImageIds::deckBaseNoSupportEdge1, Bridge::ImageIds::deckWallEdge1 }, { 7, 7, 0 });
            }
            break;
            case 4:
                // 0x0042BBCC
                shadowType = 3;
                paintFlatSingleQuarterNoSupport(session, *bridgeObj, bridgeEntry, { Bridge::ImageIds::roofEdge2, Bridge::ImageIds::deckBaseNoSupportEdge2, Bridge::ImageIds::deckWallEdge2 }, { 24, 22, 0 });
                break;
            case 8:
                // 0x0042BCD5
                // SPECIAL needs to do the front supports to ground as well
                shadowType = 1;
                paintFlatSingleQuarterSupportFront(session, *bridgeObj, bridgeEntry, supportLength, slope);
                break;
            default:
            {
                // 0x0042B08A
                auto unkTile = World::toTileSpace(Math::Vector::rotate(session.getUnkPosition(), session.getRotation()));
                unkTile.x &= bridgeObj->spanLength - 1;
                unkTile.y &= bridgeObj->spanLength - 1;

                const auto xPillarSpacing = bridgeObj->pillarSpacing / (1U << (unkTile.x * 2));
                const auto yPillarSpacing = bridgeObj->pillarSpacing / (1U << (unkTile.y * 2));
                const auto& xSpanImages = kSpanImages[0][unkTile.x];
                const auto& ySpanImages = kSpanImages[1][unkTile.y];

                bool bridgeRotation = [&session, &bridgeEntry]() {
                    switch (session.getRotation())
                    {
                        case 0:
                        case 2:
                            return (
                                !(bridgeEntry.edgesQuarters & (1U << 5))
                                || !(bridgeEntry.edgesQuarters & (1U << 7)));
                        case 1:
                        case 3:
                            return !(
                                !(bridgeEntry.edgesQuarters & (1U << 6))
                                || !(bridgeEntry.edgesQuarters & (1U << 4)));
                    }
                    return false;
                }();

                if (bridgeRotation)
                {
                    paintBridgeNE(session, *bridgeObj, bridgeEntry, supportLength, slope, xPillarSpacing, xSpanImages);
                }
                else
                {
                    paintBridgeSW(session, *bridgeObj, bridgeEntry, supportLength, slope, yPillarSpacing, ySpanImages);
                }
                // 0x0042B837
                if (bridgeEntry.subType == 0)
                {
                    if (!(bridgeEntry.edgesQuarters & (1U << 7)))
                    {
                        const auto wallImage = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(xSpanImages.wallLhs);
                        constexpr World::Pos3 bbOffset = { 2, 0, 0 };
                        constexpr World::Pos3 bbLength = { 28, 1, 26 };
                        session.addToPlotList4FD150(wallImage, baseHeightOffset, bbOffset + baseHeightOffset, bbLength);
                    }
                    if (!(bridgeEntry.edgesQuarters & (1U << 4)))
                    {
                        const auto wallImage = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(ySpanImages.wallLhs);
                        constexpr World::Pos3 bbOffset = { 0, 2, 0 };
                        constexpr World::Pos3 bbLength = { 1, 28, 26 };
                        session.addToPlotList4FD150(wallImage, baseHeightOffset, bbOffset + baseHeightOffset, bbLength);
                    }
                    if (!(bridgeEntry.edgesQuarters & (1U << 5)))
                    {
                        const auto wallImage = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(xSpanImages.wallRhs);
                        constexpr World::Pos3 bbOffset = { 0, 30, 2 };
                        constexpr World::Pos3 bbLength = { 28, 1, 24 };
                        session.addToPlotList4FD150(wallImage, baseHeightOffset, bbOffset + baseHeightOffset, bbLength);
                    }
                    if (!(bridgeEntry.edgesQuarters & (1U << 6)))
                    {
                        const auto wallImage = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(ySpanImages.wallRhs);
                        constexpr World::Pos3 bbOffset = { 30, 2, 2 };
                        constexpr World::Pos3 bbLength = { 1, 28, 24 };
                        session.addToPlotList4FD150(wallImage, baseHeightOffset, bbOffset + baseHeightOffset, bbLength);
                    }
                }
                break;
            }
        }

        // 0x0042BED2
        // Paint shadow
        if (session.getRenderTarget()->zoomLevel <= 1)
        {
            auto displaySlope = 0;
            auto height = session.getWaterHeight2();
            if (height == 0)
            {
                height = session.getSurfaceHeight();
                displaySlope = kSlopeToDisplaySlope[session.getSurfaceSlope()];
            }

            const auto shadowImage = ImageId(kSlopeToBridgeShadow[shadowType][displaySlope]).withTranslucency(ExtColour::unk32);

            const World::Pos3 heightOffset = { 0, 0, height };
            constexpr World::Pos3 bbOffset2 = { 15, 15, 1 };
            constexpr World::Pos3 bbLength2 = { 2, 2, 1 };
            session.addToPlotList4FD150(shadowImage, heightOffset, bbOffset2 + heightOffset, bbLength2);
        }
        return true;
    }
}
