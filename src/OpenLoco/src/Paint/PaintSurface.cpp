#include "PaintSurface.h"
#include "Config.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Map/MapSelection.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/Wave.h"
#include "Map/WaveManager.h"
#include "Objects/IndustryObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/SnowObject.h"
#include "Objects/TunnelObject.h"
#include "Objects/WaterObject.h"
#include "Paint.h"
#include "PaintTileDecorations.h"
#include "Ui/ViewportInteraction.h"
#include "World/IndustryManager.h"
#include "World/Station.h"
#include <OpenLoco/Core/Numerics.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Paint
{
    static loco_global<LandObjectFlags[32], 0x00F003D3> _F003D3;

    struct CornerHeight
    {
        uint8_t top;    // al
        uint8_t right;  // ah
        uint8_t bottom; // cl
        uint8_t left;   // ch
    };

    struct TileDescriptor
    {
        World::Pos2 pos;
        const World::SurfaceElement* elSurface;
        uint8_t landObjectId;
        uint8_t slope;
        CornerHeight cornerHeights;
        uint8_t snowCoverage;
        uint8_t var6SLR5;
    };

    // TODO: Check
    static constexpr std::array<CornerHeight, 32> kCornerHeights = {
        // T  R  B  L
        CornerHeight{ 0, 0, 0, 0 },
        CornerHeight{ 0, 0, 1, 0 },
        CornerHeight{ 0, 0, 0, 1 },
        CornerHeight{ 0, 0, 1, 1 },
        CornerHeight{ 1, 0, 0, 0 },
        CornerHeight{ 1, 0, 1, 0 },
        CornerHeight{ 1, 0, 0, 1 },
        CornerHeight{ 1, 0, 1, 1 },
        CornerHeight{ 0, 1, 0, 0 },
        CornerHeight{ 0, 1, 1, 0 },
        CornerHeight{ 0, 1, 0, 1 },
        CornerHeight{ 0, 1, 1, 1 },
        CornerHeight{ 1, 1, 0, 0 },
        CornerHeight{ 1, 1, 1, 0 },
        CornerHeight{ 1, 1, 0, 1 },
        CornerHeight{ 1, 1, 1, 1 },
        CornerHeight{ 0, 0, 0, 0 },
        CornerHeight{ 0, 0, 1, 0 },
        CornerHeight{ 0, 0, 0, 1 },
        CornerHeight{ 0, 0, 1, 1 },
        CornerHeight{ 1, 0, 0, 0 },
        CornerHeight{ 1, 0, 1, 0 },
        CornerHeight{ 1, 0, 0, 1 },
        CornerHeight{ 1, 0, 1, 2 },
        CornerHeight{ 0, 1, 0, 0 },
        CornerHeight{ 0, 1, 1, 0 },
        CornerHeight{ 0, 1, 0, 1 },
        CornerHeight{ 0, 1, 2, 1 },
        CornerHeight{ 1, 1, 0, 0 },
        CornerHeight{ 1, 2, 1, 0 },
        CornerHeight{ 2, 1, 0, 1 },
        CornerHeight{ 1, 1, 1, 1 },
    };

    static constexpr std::array<uint8_t, 32> k4FD97E = {
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

    static constexpr std::array<uint8_t, 19> k4FD30C = {
        0,
        0,
        0,
        8,
        0,
        0,
        8,
        16,
        0,
        8,
        0,
        16,
        8,
        16,
        16,
        16,
        16,
        16,
        16,
    };

    // bottom left tint
    static constexpr std::array<uint8_t, 19> k4FDA5E = {
        2,
        5,
        1,
        4,
        2,
        5,
        1,
        2,
        2,
        4,
        1,
        2,
        1,
        3,
        0,
        3,
        1,
        5,
        0,
    };

    // top left tint
    static constexpr std::array<uint8_t, 19> k4FDA71 = {
        2,
        5,
        2,
        4,
        2,
        5,
        1,
        1,
        3,
        4,
        3,
        2,
        1,
        2,
        0,
        3,
        1,
        5,
        0,
    };

    // top right tint
    static constexpr std::array<uint8_t, 19> k4FDA84 = {
        2,
        2,
        2,
        4,
        0,
        0,
        1,
        1,
        3,
        4,
        3,
        5,
        1,
        2,
        2,
        3,
        1,
        5,
        0,
    };

    // bottom right tint
    static constexpr std::array<uint8_t, 19> k4FDA97 = {
        2,
        2,
        1,
        4,
        0,
        0,
        1,
        2,
        2,
        4,
        1,
        5,
        1,
        3,
        2,
        3,
        1,
        5,
        0,
    };

    std::array<std::array<World::Pos2, 4>, 4> _unk4FD99E = {
        std::array<World::Pos2, 4>{
            World::Pos2{ 32, 0 },
            World::Pos2{ -32, 32 },
            World::Pos2{ -64, 0 },
            World::Pos2{ 0, -64 },
        },
        std::array<World::Pos2, 4>{
            World::Pos2{ 0, 32 },
            World::Pos2{ -64, 0 },
            World::Pos2{ -32, -64 },
            World::Pos2{ 32, -32 },
        },
        std::array<World::Pos2, 4>{
            World::Pos2{ 0, -32 },
            World::Pos2{ 0, 0 },
            World::Pos2{ -32, 0 },
            World::Pos2{ -32, -32 },
        },
        std::array<World::Pos2, 4>{
            World::Pos2{ -32, 0 },
            World::Pos2{ -32, -32 },
            World::Pos2{ 0, -32 },
            World::Pos2{ 0, 0 },
        },
    };

    static constexpr std::array<std::array<uint32_t, 19>, 4> kSnowCoverageSlopeToMask = {
        std::array<uint32_t, 19>{
            ImageIds::snowMaskCoverage1Slope0,
            ImageIds::snowMaskCoverage1Slope1,
            ImageIds::snowMaskCoverage1Slope2,
            ImageIds::snowMaskCoverage1Slope3,
            ImageIds::snowMaskCoverage1Slope4,
            ImageIds::snowMaskCoverage1Slope5,
            ImageIds::snowMaskCoverage1Slope6,
            ImageIds::snowMaskCoverage1Slope7,
            ImageIds::snowMaskCoverage1Slope8,
            ImageIds::snowMaskCoverage1Slope9,
            ImageIds::snowMaskCoverage1Slope10,
            ImageIds::snowMaskCoverage1Slope11,
            ImageIds::snowMaskCoverage1Slope12,
            ImageIds::snowMaskCoverage1Slope13,
            ImageIds::snowMaskCoverage1Slope14,
            ImageIds::snowMaskCoverage1Slope15,
            ImageIds::snowMaskCoverage1Slope16,
            ImageIds::snowMaskCoverage1Slope17,
            ImageIds::snowMaskCoverage1Slope18,
        },
        std::array<uint32_t, 19>{
            ImageIds::snowMaskCoverage2Slope0,
            ImageIds::snowMaskCoverage2Slope1,
            ImageIds::snowMaskCoverage2Slope2,
            ImageIds::snowMaskCoverage2Slope3,
            ImageIds::snowMaskCoverage2Slope4,
            ImageIds::snowMaskCoverage2Slope5,
            ImageIds::snowMaskCoverage2Slope6,
            ImageIds::snowMaskCoverage2Slope7,
            ImageIds::snowMaskCoverage2Slope8,
            ImageIds::snowMaskCoverage2Slope9,
            ImageIds::snowMaskCoverage2Slope10,
            ImageIds::snowMaskCoverage2Slope11,
            ImageIds::snowMaskCoverage2Slope12,
            ImageIds::snowMaskCoverage2Slope13,
            ImageIds::snowMaskCoverage2Slope14,
            ImageIds::snowMaskCoverage2Slope15,
            ImageIds::snowMaskCoverage2Slope16,
            ImageIds::snowMaskCoverage2Slope17,
            ImageIds::snowMaskCoverage2Slope18,
        },
        std::array<uint32_t, 19>{
            ImageIds::snowMaskCoverage3Slope0,
            ImageIds::snowMaskCoverage3Slope1,
            ImageIds::snowMaskCoverage3Slope2,
            ImageIds::snowMaskCoverage3Slope3,
            ImageIds::snowMaskCoverage3Slope4,
            ImageIds::snowMaskCoverage3Slope5,
            ImageIds::snowMaskCoverage3Slope6,
            ImageIds::snowMaskCoverage3Slope7,
            ImageIds::snowMaskCoverage3Slope8,
            ImageIds::snowMaskCoverage3Slope9,
            ImageIds::snowMaskCoverage3Slope10,
            ImageIds::snowMaskCoverage3Slope11,
            ImageIds::snowMaskCoverage3Slope12,
            ImageIds::snowMaskCoverage3Slope13,
            ImageIds::snowMaskCoverage3Slope14,
            ImageIds::snowMaskCoverage3Slope15,
            ImageIds::snowMaskCoverage3Slope16,
            ImageIds::snowMaskCoverage3Slope17,
            ImageIds::snowMaskCoverage3Slope18,
        },
        std::array<uint32_t, 19>{
            ImageIds::snowMaskCoverage4Slope0,
            ImageIds::snowMaskCoverage4Slope1,
            ImageIds::snowMaskCoverage4Slope2,
            ImageIds::snowMaskCoverage4Slope3,
            ImageIds::snowMaskCoverage4Slope4,
            ImageIds::snowMaskCoverage4Slope5,
            ImageIds::snowMaskCoverage4Slope6,
            ImageIds::snowMaskCoverage4Slope7,
            ImageIds::snowMaskCoverage4Slope8,
            ImageIds::snowMaskCoverage4Slope9,
            ImageIds::snowMaskCoverage4Slope10,
            ImageIds::snowMaskCoverage4Slope11,
            ImageIds::snowMaskCoverage4Slope12,
            ImageIds::snowMaskCoverage4Slope13,
            ImageIds::snowMaskCoverage4Slope14,
            ImageIds::snowMaskCoverage4Slope15,
            ImageIds::snowMaskCoverage4Slope16,
            ImageIds::snowMaskCoverage4Slope17,
            ImageIds::snowMaskCoverage4Slope18,
        }
    };

    static constexpr std::array<uint32_t, 19> kCornerSelectionBoxFromSlope = {
        ImageIds::constructionSelectionCornersSlope0,
        ImageIds::constructionSelectionCornersSlope1,
        ImageIds::constructionSelectionCornersSlope2,
        ImageIds::constructionSelectionCornersSlope3,
        ImageIds::constructionSelectionCornersSlope4,
        ImageIds::constructionSelectionCornersSlope5,
        ImageIds::constructionSelectionCornersSlope6,
        ImageIds::constructionSelectionCornersSlope7,
        ImageIds::constructionSelectionCornersSlope8,
        ImageIds::constructionSelectionCornersSlope9,
        ImageIds::constructionSelectionCornersSlope10,
        ImageIds::constructionSelectionCornersSlope11,
        ImageIds::constructionSelectionCornersSlope12,
        ImageIds::constructionSelectionCornersSlope13,
        ImageIds::constructionSelectionCornersSlope14,
        ImageIds::constructionSelectionCornersSlope15,
        ImageIds::constructionSelectionCornersSlope16,
        ImageIds::constructionSelectionCornersSlope17,
        ImageIds::constructionSelectionCornersSlope18,
    };
    static constexpr std::array<ExtColour, 4> kCornerColours = {
        ExtColour::unk21,
        ExtColour::unk22,
        ExtColour::unk23,
        ExtColour::unk24,
    };

    static constexpr std::array<uint32_t, 19> kQuadSelectionBoxFromSlope = {
        ImageIds::constructionSelectionQuadsSlope0,
        ImageIds::constructionSelectionQuadsSlope1,
        ImageIds::constructionSelectionQuadsSlope2,
        ImageIds::constructionSelectionQuadsSlope3,
        ImageIds::constructionSelectionQuadsSlope4,
        ImageIds::constructionSelectionQuadsSlope5,
        ImageIds::constructionSelectionQuadsSlope6,
        ImageIds::constructionSelectionQuadsSlope7,
        ImageIds::constructionSelectionQuadsSlope8,
        ImageIds::constructionSelectionQuadsSlope9,
        ImageIds::constructionSelectionQuadsSlope10,
        ImageIds::constructionSelectionQuadsSlope11,
        ImageIds::constructionSelectionQuadsSlope12,
        ImageIds::constructionSelectionQuadsSlope13,
        ImageIds::constructionSelectionQuadsSlope14,
        ImageIds::constructionSelectionQuadsSlope15,
        ImageIds::constructionSelectionQuadsSlope16,
        ImageIds::constructionSelectionQuadsSlope17,
        ImageIds::constructionSelectionQuadsSlope18,
    };
    static constexpr std::array<ExtColour, 4> kQuarterColours = {
        ExtColour::unk27,
        ExtColour::unk28,
        ExtColour::unk29,
        ExtColour::unk2A,
    };

    static constexpr std::array<uint32_t, 19> kEdgeSelectionBoxFromSlope = {
        ImageIds::constructionSelectionEdgesSlope0,
        ImageIds::constructionSelectionEdgesSlope1,
        ImageIds::constructionSelectionEdgesSlope2,
        ImageIds::constructionSelectionEdgesSlope3,
        ImageIds::constructionSelectionEdgesSlope4,
        ImageIds::constructionSelectionEdgesSlope5,
        ImageIds::constructionSelectionEdgesSlope6,
        ImageIds::constructionSelectionEdgesSlope7,
        ImageIds::constructionSelectionEdgesSlope8,
        ImageIds::constructionSelectionEdgesSlope9,
        ImageIds::constructionSelectionEdgesSlope10,
        ImageIds::constructionSelectionEdgesSlope11,
        ImageIds::constructionSelectionEdgesSlope12,
        ImageIds::constructionSelectionEdgesSlope13,
        ImageIds::constructionSelectionEdgesSlope14,
        ImageIds::constructionSelectionEdgesSlope15,
        ImageIds::constructionSelectionEdgesSlope16,
        ImageIds::constructionSelectionEdgesSlope17,
        ImageIds::constructionSelectionEdgesSlope18,
    };
    static constexpr std::array<ExtColour, 4> kEdgeColours = {
        ExtColour::unk22,
        ExtColour::unk23,
        ExtColour::unk24,
        ExtColour::unk25,
    };
    static constexpr std::array<uint32_t, 19> kCatchmentFromSlope = {
        ImageIds::constructionCatchmentSlope0,
        ImageIds::constructionCatchmentSlope1,
        ImageIds::constructionCatchmentSlope2,
        ImageIds::constructionCatchmentSlope3,
        ImageIds::constructionCatchmentSlope4,
        ImageIds::constructionCatchmentSlope5,
        ImageIds::constructionCatchmentSlope6,
        ImageIds::constructionCatchmentSlope7,
        ImageIds::constructionCatchmentSlope8,
        ImageIds::constructionCatchmentSlope9,
        ImageIds::constructionCatchmentSlope10,
        ImageIds::constructionCatchmentSlope11,
        ImageIds::constructionCatchmentSlope12,
        ImageIds::constructionCatchmentSlope13,
        ImageIds::constructionCatchmentSlope14,
        ImageIds::constructionCatchmentSlope15,
        ImageIds::constructionCatchmentSlope16,
        ImageIds::constructionCatchmentSlope17,
        ImageIds::constructionCatchmentSlope18,
    };
    static constexpr std::array<uint32_t, 19> kGridlinesBoxFromSlope = {
        ImageIds::gridlinesSlope0,
        ImageIds::gridlinesSlope1,
        ImageIds::gridlinesSlope2,
        ImageIds::gridlinesSlope3,
        ImageIds::gridlinesSlope4,
        ImageIds::gridlinesSlope5,
        ImageIds::gridlinesSlope6,
        ImageIds::gridlinesSlope7,
        ImageIds::gridlinesSlope8,
        ImageIds::gridlinesSlope9,
        ImageIds::gridlinesSlope10,
        ImageIds::gridlinesSlope11,
        ImageIds::gridlinesSlope12,
        ImageIds::gridlinesSlope13,
        ImageIds::gridlinesSlope14,
        ImageIds::gridlinesSlope15,
        ImageIds::gridlinesSlope16,
        ImageIds::gridlinesSlope17,
        ImageIds::gridlinesSlope18,
    };
    static constexpr std::array<uint32_t, 19> kSurfaceSmoothFromSlope = {
        ImageIds::surfaceSmoothSlope0,
        ImageIds::surfaceSmoothSlope1,
        ImageIds::surfaceSmoothSlope2,
        ImageIds::surfaceSmoothSlope3,
        ImageIds::surfaceSmoothSlope4,
        ImageIds::surfaceSmoothSlope5,
        ImageIds::surfaceSmoothSlope6,
        ImageIds::surfaceSmoothSlope7,
        ImageIds::surfaceSmoothSlope8,
        ImageIds::surfaceSmoothSlope9,
        ImageIds::surfaceSmoothSlope10,
        ImageIds::surfaceSmoothSlope11,
        ImageIds::surfaceSmoothSlope12,
        ImageIds::surfaceSmoothSlope13,
        ImageIds::surfaceSmoothSlope14,
        ImageIds::surfaceSmoothSlope15,
        ImageIds::surfaceSmoothSlope16,
        ImageIds::surfaceSmoothSlope17,
        ImageIds::surfaceSmoothSlope18,
    };

    constexpr std::array<uint8_t, 8> kUndergroundViewSnowNoise = {
        0, // Don't display (No snow handled differently)
        0, // Don't display
        0, // Don't display
        3,
        6,
        0, // Don't display (full snow handled differently)
        0,
        0,
    };

    constexpr std::array<uint8_t, 16> kSlopeToWaterShape = {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        2,
        0,
        0,
        0,
        3,
        0,
        1,
        4,
        0
    };

    static constexpr uint8_t getRotatedSlope(uint8_t slope, uint8_t rotation)
    {
        return Numerics::rotl4bit(slope & 0xF, rotation) | (slope & 0x10);
    }

    static void paintMainSurface(PaintSession& session, uint32_t imageIndex, int16_t baseHeight)
    {
        const auto imageId = [imageIndex, &session]() {
            if ((session.getViewFlags() & (Ui::ViewportFlags::underground_view | Ui::ViewportFlags::flag_7)) != Ui::ViewportFlags::none)
            {
                return ImageId(imageIndex).withTranslucency(ExtColour::unk30);
            }
            return ImageId(imageIndex);
        }();

        const World::Pos3 offsetUnk(0, 0, baseHeight);
        const World::Pos3 bbSizeUnk(32, 32, -1);
        session.addToPlotListAsParent(imageId, offsetUnk, bbSizeUnk);
    }

    struct SnowImage
    {
        uint32_t baseImage;
        uint32_t imageMask;
    };

    // 0x00465F01
    static void paintSurfaceCornerSelection(PaintSession& session, const uint8_t displaySlope, const uint8_t corner)
    {
        const auto colour = kCornerColours[corner];
        const auto imageId = ImageId(kCornerSelectionBoxFromSlope[displaySlope], colour);
        session.attachToPrevious(imageId, { 0, 0 });
    }

    // 0x00465F08
    static void paintSurfaceFullTileSelection(PaintSession& session, const uint8_t displaySlope)
    {
        const auto imageId = ImageId(kCornerSelectionBoxFromSlope[displaySlope], ExtColour::unk25);
        session.attachToPrevious(imageId, { 0, 0 });
    }

    // 0x00465F36
    static void paintSurfaceQuadSelection(PaintSession& session, const uint8_t displaySlope, const uint8_t quad)
    {
        const auto colour = kQuarterColours[quad];
        const auto imageId = ImageId(kQuadSelectionBoxFromSlope[displaySlope], colour);
        session.attachToPrevious(imageId, { 0, 0 });
    }

    // 0x00465F69
    static void paintSurfaceEdgeSelection(PaintSession& session, const uint8_t displaySlope, const uint8_t edge)
    {
        const auto colour = kEdgeColours[edge];
        const auto imageId = ImageId(kEdgeSelectionBoxFromSlope[displaySlope], colour);
        session.attachToPrevious(imageId, { 0, 0 });
    }

    struct WaterSurface
    {
        int16_t height;
        uint8_t slope;
    };

    static WaterSurface getWaterSurface(const World::SurfaceElement& elSurface, uint8_t slope)
    {
        int16_t height = elSurface.baseHeight();
        if (elSurface.waterHeight() > height)
        {
            height += World::kMicroZStep;
            if (elSurface.waterHeight() != height
                || !elSurface.isSlopeDoubleHeight())
            {
                // This is the simple case where the water is flat as no
                // bits of surface breach the water
                slope = 0;
                height = elSurface.waterHeight();
            }
            else
            {
                // Adjust the slope to handle surface breaches
                slope = Numerics::rotl4bit((slope & 0xF) ^ 0xF, 2);
            }
        }
        return WaterSurface{ .height = height, .slope = slope };
    }

    // 0x00465F99
    static void paintSurfaceFullWaterTileSelection(PaintSession& session, const World::SurfaceElement& elSurface, uint8_t slope)
    {
        const auto waterSurface = getWaterSurface(elSurface, slope);
        const auto displaySlope = k4FD97E[waterSurface.slope];
        const auto imageId = ImageId(kCornerSelectionBoxFromSlope[displaySlope], ExtColour::unk26);

        // TODO: Push/pop last ps?
        auto* lastPs = session.getLastPS();

        session.addToPlotListAsParent(imageId, { 0, 0, waterSurface.height }, { 32, 32, 1 });

        session.setLastPS(lastPs);
    }

    // 0x00465EE9
    static void paintSurfaceSelection(PaintSession& session, const World::SurfaceElement& elSurface, const uint8_t slope)
    {
        const auto rotation = session.getRotation();
        const auto unkF252AC = k4FD97E[slope];
        switch (World::getMapSelectionCorner())
        {
            case World::MapSelectionType::corner0:
            case World::MapSelectionType::corner1:
            case World::MapSelectionType::corner2:
            case World::MapSelectionType::corner3:
            {
                uint32_t corner = enumValue(World::getMapSelectionCorner()) - enumValue(World::MapSelectionType::corner0);
                corner = (corner + rotation) & 3;
                paintSurfaceCornerSelection(session, unkF252AC, corner);
                break;
            }
            case World::MapSelectionType::full:
                paintSurfaceFullTileSelection(session, unkF252AC);
                break;

            case World::MapSelectionType::fullWater:
                paintSurfaceFullWaterTileSelection(session, elSurface, slope);
                break;

            case World::MapSelectionType::quarter0:
            case World::MapSelectionType::quarter1:
            case World::MapSelectionType::quarter2:
            case World::MapSelectionType::quarter3:
            {
                uint32_t quad = enumValue(World::getMapSelectionCorner()) - enumValue(World::MapSelectionType::quarter0);
                quad = (quad + rotation) & 3;
                paintSurfaceQuadSelection(session, unkF252AC, quad);
                break;
            }
            case World::MapSelectionType::edge0:
            case World::MapSelectionType::edge1:
            case World::MapSelectionType::edge2:
            case World::MapSelectionType::edge3:
            {
                uint32_t edge = enumValue(World::getMapSelectionCorner()) - enumValue(World::MapSelectionType::edge0);
                edge = (edge + rotation) & 3;
                paintSurfaceEdgeSelection(session, unkF252AC, edge);
                break;
            }
        }
    }

    static void paintSurfaceSmoothenEdge(
        PaintSession& session, uint8_t edge, const TileDescriptor& self, const TileDescriptor& neighbour)
    {
        if (neighbour.elSurface == nullptr)
            return;

        if (neighbour.cornerHeights.left != neighbour.cornerHeights.right
            || neighbour.cornerHeights.bottom != neighbour.cornerHeights.top)
            return;

        // If either is industrial
        if (neighbour.landObjectId == 0xFFU || self.landObjectId == 0xFFU)
        {
            return;
        }

        if (self.snowCoverage == 3)
        {
            return;
        }

        // 0x00F25304
        const auto displaySlope = k4FD97E[self.slope];
        const auto neighbourDislaySlope = k4FD97E[neighbour.slope];

        uint8_t dh = 0, cl = 0;
        switch (edge)
        {
            case 0:
                dh = k4FDA5E[displaySlope];
                cl = k4FDA84[neighbourDislaySlope];
                break;

            case 1:
                dh = k4FDA71[displaySlope];
                cl = k4FDA97[neighbourDislaySlope];
                break;

            case 2:
                dh = k4FDA97[displaySlope];
                cl = k4FDA71[neighbourDislaySlope];
                break;

            case 3:
                dh = k4FDA84[displaySlope];
                cl = k4FDA5E[neighbourDislaySlope];
                break;
        }

        auto selfObj = self.snowCoverage >= 4 ? 0xFFU : self.landObjectId;
        auto neighbourObj = neighbour.snowCoverage >= 4 ? 0xFFU : neighbour.landObjectId;

        if (self.var6SLR5 == neighbour.var6SLR5 && selfObj == neighbourObj)
        {
            // same tint
            if (cl == dh)
                return;

            if ((_F003D3[self.landObjectId] & LandObjectFlags::unk4) != LandObjectFlags::none)
                return;
        }
        else
        {
            if ((_F003D3[self.landObjectId] & LandObjectFlags::unk5) != LandObjectFlags::none)
                return;

            if ((_F003D3[neighbour.landObjectId] & LandObjectFlags::unk5) != LandObjectFlags::none)
                return;
        }

        const auto baseImageId = ImageId(kSurfaceSmoothFromSlope[displaySlope]);

        if (neighbourObj == 0xFFU)
        {
            auto* snowObj = ObjectManager::get<SnowObject>();
            const auto variation = 38 + cl;
            const auto maskImageId = ImageId(snowObj->image).withIndexOffset(variation);

            auto* attachedPs = session.attachToPrevious(baseImageId, { 0, 0 });
            if (attachedPs != nullptr)
            {
                attachedPs->maskedImageId = maskImageId;
                attachedPs->flags |= PaintStructFlags::hasMaskedImage;
            }
        }
        else
        {
            auto* landObj = ObjectManager::get<LandObject>(neighbour.landObjectId);
            const auto variation = landObj->var_0E * neighbour.var6SLR5 + 19 + cl;
            const auto maskImageId = ImageId(landObj->image).withIndexOffset(variation);

            auto* attachedPs = session.attachToPrevious(baseImageId, { 0, 0 });
            if (attachedPs != nullptr)
            {
                attachedPs->maskedImageId = maskImageId;
                attachedPs->flags |= PaintStructFlags::hasMaskedImage;
            }
        }
    }

    static void paintMainUndergroundSurface(PaintSession& session, uint32_t imageIndex, uint8_t displaySlope)
    {
        auto* attachedPs = session.attachToPrevious(ImageId(imageIndex), { 0, 0 });
        if (attachedPs != nullptr)
        {
            attachedPs->maskedImageId = ImageId(kGridlinesBoxFromSlope[displaySlope]);
            attachedPs->flags |= PaintStructFlags::hasMaskedImage;
        }
    }

    constexpr std::array<uint8_t, 4> kEdgeFactorOffset = { 0, 16, 16, 0 };
    constexpr std::array<uint8_t, 4> kEdgeUndergroundOffset = { 0, 0, 67, 64 };
    constexpr std::array<World::Pos3, 4> kEdgeImageOffset = {
        World::Pos3{ 30, 0, 0 },
        World::Pos3{ 0, 30, 0 },
        World::Pos3{ 0, -2, 1 },
        World::Pos3{ -2, 0, 1 },
    };
    constexpr std::array<World::Pos3, 4> kEdgeBoundingBoxSize = {
        World::Pos3{ 0, 30, 15 },
        World::Pos3{ 30, 0, 15 },
        World::Pos3{ 30, 0, 15 },
        World::Pos3{ 0, 30, 15 },
    };
    constexpr std::array<std::array<std::array<uint32_t, 5>, 4>, 2> kEdgeMaskImageFromSlope = {
        std::array<std::array<uint32_t, 5>, 4>{
            std::array<uint32_t, 5>{
                ImageIds::cliffEdge0MaskSlope0,
                ImageIds::cliffEdge0MaskSlope1,
                ImageIds::cliffEdge0MaskSlope2,
                ImageIds::cliffEdge0MaskSlope3,
                ImageIds::cliffEdge0MaskSlope4,
            },
            std::array<uint32_t, 5>{
                ImageIds::cliffEdge1MaskSlope0,
                ImageIds::cliffEdge1MaskSlope1,
                ImageIds::cliffEdge1MaskSlope2,
                ImageIds::cliffEdge1MaskSlope3,
                ImageIds::cliffEdge1MaskSlope4,
            },
            std::array<uint32_t, 5>{
                ImageIds::cliffEdge1MaskSlope0,
                ImageIds::cliffEdge1MaskSlope1,
                ImageIds::cliffEdge1MaskSlope2,
                ImageIds::cliffEdge1MaskSlope3,
                ImageIds::cliffEdge1MaskSlope4,
            },
            std::array<uint32_t, 5>{
                ImageIds::cliffEdge0MaskSlope0,
                ImageIds::cliffEdge0MaskSlope1,
                ImageIds::cliffEdge0MaskSlope2,
                ImageIds::cliffEdge0MaskSlope3,
                ImageIds::cliffEdge0MaskSlope4,
            },
        },
        std::array<std::array<uint32_t, 5>, 4>{
            std::array<uint32_t, 5>{
                ImageIds::cliffEdge0UndergroundMaskSlope0,
                ImageIds::cliffEdge0UndergroundMaskSlope1,
                ImageIds::cliffEdge0UndergroundMaskSlope2,
                ImageIds::cliffEdge0UndergroundMaskSlope3,
                ImageIds::cliffEdge0UndergroundMaskSlope4,
            },
            std::array<uint32_t, 5>{
                ImageIds::cliffEdge1UndergroundMaskSlope0,
                ImageIds::cliffEdge1UndergroundMaskSlope1,
                ImageIds::cliffEdge1UndergroundMaskSlope2,
                ImageIds::cliffEdge1UndergroundMaskSlope3,
                ImageIds::cliffEdge1UndergroundMaskSlope4,
            },
            std::array<uint32_t, 5>{
                ImageIds::cliffEdge1UndergroundMaskSlope0,
                ImageIds::cliffEdge1UndergroundMaskSlope1,
                ImageIds::cliffEdge1UndergroundMaskSlope2,
                ImageIds::cliffEdge1UndergroundMaskSlope3,
                ImageIds::cliffEdge1UndergroundMaskSlope4,
            },
            std::array<uint32_t, 5>{
                ImageIds::cliffEdge0UndergroundMaskSlope0,
                ImageIds::cliffEdge0UndergroundMaskSlope1,
                ImageIds::cliffEdge0UndergroundMaskSlope2,
                ImageIds::cliffEdge0UndergroundMaskSlope3,
                ImageIds::cliffEdge0UndergroundMaskSlope4,
            },
        },
    };

    // TODO: Unscramble this so they are the same
    constexpr std::array<uint8_t, 4> kCliffEdgeToTunnelEdge = { 2, 1, 3, 0 };

    static void paintSurfaceCliffEdge(PaintSession& session, uint8_t edge, const TileDescriptor& self, const TileDescriptor& neighbour, uint32_t cliffEdgeImageBase)
    {
        if (neighbour.elSurface == nullptr)
        {
            return;
        }
        if (neighbour.cornerHeights.top <= neighbour.cornerHeights.right
            && neighbour.cornerHeights.bottom <= neighbour.cornerHeights.left)
        {
            return;
        }

        const bool isUnderground = (session.getViewFlags() & Ui::ViewportFlags::underground_view) != Ui::ViewportFlags::none;
        const auto spritePos = session.getSpritePosition();
        const uint32_t factor = ((spritePos.x ^ spritePos.y) & 0b10'0000) + kEdgeFactorOffset[edge];
        const auto undergroundOffset = kEdgeUndergroundOffset[edge];
        if (undergroundOffset != 0 && isUnderground)
        {
            const auto offset = neighbour.cornerHeights.bottom - neighbour.cornerHeights.top + 1;
            const auto yOffset = ((neighbour.cornerHeights.top - self.elSurface->baseZ() / kMicroToSmallZStep) * kMicroZStep) + 1;
            const auto image = ImageId(cliffEdgeImageBase).withIndexOffset(offset + undergroundOffset);
            session.attachToPrevious(image, Ui::Point(0, -yOffset));
        }

        auto& maskArr = kEdgeMaskImageFromSlope[isUnderground][edge];
        uint8_t highest = neighbour.cornerHeights.left;
        if (highest != neighbour.cornerHeights.right)
        {
            uint8_t unk = 3;
            if (highest >= neighbour.cornerHeights.right)
            {
                unk = 4;
                highest = neighbour.cornerHeights.right;
            }
            if (highest != neighbour.cornerHeights.top && highest != neighbour.cornerHeights.bottom)
            {
                const auto image = ImageId(cliffEdgeImageBase).withIndexOffset(factor + (highest & 0xF));
                const World::Pos3 offset = kEdgeImageOffset[edge] + World::Pos3(0, 0, highest * kMicroZStep);
                const World::Pos3 boundBoxSize = kEdgeBoundingBoxSize[edge];
                auto* ps = session.addToPlotListAsParent(image, offset, boundBoxSize);
                if (ps != nullptr)
                {
                    ps->flags |= PaintStructFlags::hasMaskedImage;
                    ps->maskedImageId = ImageId(maskArr[unk]);
                }
                highest++;
            }
        }

        uint8_t tunnelNum = 0;
        while (highest < neighbour.cornerHeights.top && highest < neighbour.cornerHeights.bottom)
        {
            const auto tunnelEdge = kCliffEdgeToTunnelEdge[edge];
            while (highest > session.getTunnels(tunnelEdge)[tunnelNum].height)
            {
                tunnelNum++;
            }

            auto& tunnel = session.getTunnels(tunnelEdge)[tunnelNum];
            if (highest == tunnel.height)
            {
                if (edge == 0)
                {
                    auto* tunnelObj = ObjectManager::get<TunnelObject>(tunnel.type);
                    {
                        const auto image = ImageId(tunnelObj->image);
                        const World::Pos3 offset = World::Pos3(30, 0, 0) + World::Pos3(0, 0, highest * kMicroZStep);
                        const World::Pos3 boundBoxSize = World::Pos3(2, 2, 25);
                        const World::Pos3 boundBoxOffset = World::Pos3(29, 0, 6) + World::Pos3(0, 0, highest * kMicroZStep);

                        session.addToPlotListAsParent(image, offset, boundBoxOffset, boundBoxSize);
                    }
                    {
                        const auto image = ImageId(tunnelObj->image).withIndexOffset(1);
                        const World::Pos3 offset = World::Pos3(30, 0, 0) + World::Pos3(0, 0, highest * kMicroZStep);
                        const World::Pos3 boundBoxSize = World::Pos3(1, 1, 31);
                        const World::Pos3 boundBoxOffset = World::Pos3(30, 30, 0) + World::Pos3(0, 0, highest * kMicroZStep);

                        session.addToPlotListAsParent(image, offset, boundBoxOffset, boundBoxSize);
                    }
                }
                else if (edge == 1)
                {
                    auto* tunnelObj = ObjectManager::get<TunnelObject>(tunnel.type);
                    {
                        const auto image = ImageId(tunnelObj->image).withIndexOffset(2);
                        const World::Pos3 offset = World::Pos3(0, 30, 0) + World::Pos3(0, 0, highest * kMicroZStep);
                        const World::Pos3 boundBoxSize = World::Pos3(2, 2, 25);
                        const World::Pos3 boundBoxOffset = World::Pos3(0, 29, 6) + World::Pos3(0, 0, highest * kMicroZStep);

                        session.addToPlotListAsParent(image, offset, boundBoxOffset, boundBoxSize);
                    }
                    {
                        const auto image = ImageId(tunnelObj->image).withIndexOffset(3);
                        const World::Pos3 offset = World::Pos3(0, 30, 0) + World::Pos3(0, 0, highest * kMicroZStep);
                        const World::Pos3 boundBoxSize = World::Pos3(1, 1, 31);
                        const World::Pos3 boundBoxOffset = World::Pos3(30, 30, 0) + World::Pos3(0, 0, highest * kMicroZStep);

                        session.addToPlotListAsParent(image, offset, boundBoxOffset, boundBoxSize);
                    }
                }
                highest += 2;
                tunnelNum++;
                continue;
            }

            const auto image = ImageId(cliffEdgeImageBase).withIndexOffset(factor + (highest & 0xF));
            const World::Pos3 offset = kEdgeImageOffset[edge] + World::Pos3(0, 0, highest * kMicroZStep);
            const World::Pos3 boundBoxSize = kEdgeBoundingBoxSize[edge];
            auto* ps = session.addToPlotListAsParent(image, offset, boundBoxSize);
            if (ps != nullptr)
            {
                ps->flags |= PaintStructFlags::hasMaskedImage;
                ps->maskedImageId = ImageId(maskArr[0]);
            }
            highest++;
        }

        uint8_t unk = 1;
        if (highest >= neighbour.cornerHeights.top)
        {
            unk = 2;
            if (highest >= neighbour.cornerHeights.bottom)
            {
                return;
            }
        }
        const auto image = ImageId(cliffEdgeImageBase).withIndexOffset(factor + (highest & 0xF));
        const World::Pos3 offset = kEdgeImageOffset[edge] + World::Pos3(0, 0, highest * kMicroZStep);
        const World::Pos3 boundBoxSize = kEdgeBoundingBoxSize[edge];
        auto* ps = session.addToPlotListAsParent(image, offset, boundBoxSize);
        if (ps != nullptr)
        {
            ps->flags |= PaintStructFlags::hasMaskedImage;
            ps->maskedImageId = ImageId(maskArr[unk]);
        }
    }

    static void paintSurfaceWaterCliffEdge(PaintSession& session, uint8_t edge, const int16_t waterHeight, const TileDescriptor& neighbour, uint32_t cliffEdgeImageBase)
    {
        if (neighbour.elSurface == nullptr)
        {
            return;
        }
        if (neighbour.elSurface->waterHeight() == waterHeight)
        {
            return;
        }

        const auto cornerHeights = CornerHeight(waterHeight / kMicroZStep, neighbour.cornerHeights.right, waterHeight / kMicroZStep, neighbour.cornerHeights.left );

        if (cornerHeights.top <= cornerHeights.right
            && cornerHeights.bottom <= cornerHeights.left)
        {
            return;
        }

        const bool isUnderground = (session.getViewFlags() & Ui::ViewportFlags::underground_view) != Ui::ViewportFlags::none;
        const auto spritePos = session.getSpritePosition();
        const uint32_t factor = ((spritePos.x ^ spritePos.y) & 0b10'0000) + kEdgeFactorOffset[edge];
        const auto undergroundOffset = kEdgeUndergroundOffset[edge];
        if (undergroundOffset != 0 && isUnderground)
        {
            const auto offset = 1;
            const auto yOffset = 1;
            const auto image = ImageId(cliffEdgeImageBase).withIndexOffset(offset + undergroundOffset);
            session.attachToPrevious(image, Ui::Point(0, -yOffset));
        }

        auto& maskArr = kEdgeMaskImageFromSlope[isUnderground][edge];
        uint8_t highest = cornerHeights.left;
        if (highest != cornerHeights.right)
        {
            uint8_t unk = 3;
            if (highest >= cornerHeights.right)
            {
                unk = 4;
                highest = cornerHeights.right;
            }
            if (highest != cornerHeights.top && highest != cornerHeights.bottom)
            {
                const auto image = ImageId(cliffEdgeImageBase).withIndexOffset(factor + (highest & 0xF));
                const World::Pos3 offset = kEdgeImageOffset[edge] + World::Pos3(0, 0, highest * kMicroZStep);
                const World::Pos3 boundBoxSize = kEdgeBoundingBoxSize[edge];
                auto* ps = session.addToPlotListAsParent(image, offset, boundBoxSize);
                if (ps != nullptr)
                {
                    ps->flags |= PaintStructFlags::hasMaskedImage;
                    ps->maskedImageId = ImageId(maskArr[unk]);
                }
                highest++;
            }
        }

        uint8_t tunnelNum = 0;
        while (highest < cornerHeights.top && highest < cornerHeights.bottom)
        {
            const auto tunnelEdge = kCliffEdgeToTunnelEdge[edge];
            while (highest > session.getTunnels(tunnelEdge)[tunnelNum].height)
            {
                tunnelNum++;
            }

            auto& tunnel = session.getTunnels(tunnelEdge)[tunnelNum];
            if (highest == tunnel.height)
            {
                if (edge == 0)
                {
                    auto* tunnelObj = ObjectManager::get<TunnelObject>(tunnel.type);
                    {
                        const auto image = ImageId(tunnelObj->image);
                        const World::Pos3 offset = World::Pos3(30, 0, 0) + World::Pos3(0, 0, highest * kMicroZStep);
                        const World::Pos3 boundBoxSize = World::Pos3(2, 2, 25);
                        const World::Pos3 boundBoxOffset = World::Pos3(29, 0, 6) + World::Pos3(0, 0, highest * kMicroZStep);

                        session.addToPlotListAsParent(image, offset, boundBoxOffset, boundBoxSize);
                    }
                    {
                        const auto image = ImageId(tunnelObj->image).withIndexOffset(1);
                        const World::Pos3 offset = World::Pos3(30, 0, 0) + World::Pos3(0, 0, highest * kMicroZStep);
                        const World::Pos3 boundBoxSize = World::Pos3(1, 1, 31);
                        const World::Pos3 boundBoxOffset = World::Pos3(30, 30, 0) + World::Pos3(0, 0, highest * kMicroZStep);

                        session.addToPlotListAsParent(image, offset, boundBoxOffset, boundBoxSize);
                    }
                }
                else if (edge == 1)
                {
                    auto* tunnelObj = ObjectManager::get<TunnelObject>(tunnel.type);
                    {
                        const auto image = ImageId(tunnelObj->image).withIndexOffset(2);
                        const World::Pos3 offset = World::Pos3(0, 30, 0) + World::Pos3(0, 0, highest * kMicroZStep);
                        const World::Pos3 boundBoxSize = World::Pos3(2, 2, 25);
                        const World::Pos3 boundBoxOffset = World::Pos3(0, 29, 6) + World::Pos3(0, 0, highest * kMicroZStep);

                        session.addToPlotListAsParent(image, offset, boundBoxOffset, boundBoxSize);
                    }
                    {
                        const auto image = ImageId(tunnelObj->image).withIndexOffset(3);
                        const World::Pos3 offset = World::Pos3(0, 30, 0) + World::Pos3(0, 0, highest * kMicroZStep);
                        const World::Pos3 boundBoxSize = World::Pos3(1, 1, 31);
                        const World::Pos3 boundBoxOffset = World::Pos3(30, 30, 0) + World::Pos3(0, 0, highest * kMicroZStep);

                        session.addToPlotListAsParent(image, offset, boundBoxOffset, boundBoxSize);
                    }
                }
                highest += 2;
                tunnelNum++;
                continue;
            }

            const auto image = ImageId(cliffEdgeImageBase).withIndexOffset(factor + (highest & 0xF));
            const World::Pos3 offset = kEdgeImageOffset[edge] + World::Pos3(0, 0, highest * kMicroZStep);
            const World::Pos3 boundBoxSize = kEdgeBoundingBoxSize[edge];
            auto* ps = session.addToPlotListAsParent(image, offset, boundBoxSize);
            if (ps != nullptr)
            {
                ps->flags |= PaintStructFlags::hasMaskedImage;
                ps->maskedImageId = ImageId(maskArr[0]);
            }
            highest++;
        }

        uint8_t unk = 1;
        if (highest >= cornerHeights.top)
        {
            unk = 2;
            if (highest >= cornerHeights.bottom)
            {
                return;
            }
        }
        const auto image = ImageId(cliffEdgeImageBase).withIndexOffset(factor + (highest & 0xF));
        const World::Pos3 offset = kEdgeImageOffset[edge] + World::Pos3(0, 0, highest * kMicroZStep);
        const World::Pos3 boundBoxSize = kEdgeBoundingBoxSize[edge];
        auto* ps = session.addToPlotListAsParent(image, offset, boundBoxSize);
        if (ps != nullptr)
        {
            ps->flags |= PaintStructFlags::hasMaskedImage;
            ps->maskedImageId = ImageId(maskArr[unk]);
        }
    }

    // 0x004656BF
    void paintSurface(PaintSession& session, World::SurfaceElement& elSurface)
    {
        session.setItemType(Ui::ViewportInteraction::InteractionItem::surface);
        const auto zoomLevel = session.getRenderTarget()->zoomLevel;
        session.setDidPassSurface(true);

        // 0x00F252B0 / 0x00F252B4 but if 0x00F252B0 == -2 that means industrial
        [[maybe_unused]] uint8_t landObjId = elSurface.terrain();
        const auto* landObj = ObjectManager::get<LandObject>(landObjId);

        const auto rotation = session.getRotation();
        const auto baseHeight = elSurface.baseHeight();
        const auto rotatedSlope = getRotatedSlope(elSurface.slope(), rotation);

        const auto selfDescriptor = TileDescriptor{
            session.getSpritePosition(),
            &elSurface,
            elSurface.isIndustrial() ? static_cast<uint8_t>(0xFFU) : elSurface.terrain(),
            rotatedSlope,
            {
                static_cast<uint8_t>(elSurface.baseZ() / kMicroToSmallZStep + kCornerHeights[rotatedSlope].top),
                static_cast<uint8_t>(elSurface.baseZ() / kMicroToSmallZStep + kCornerHeights[rotatedSlope].right),
                static_cast<uint8_t>(elSurface.baseZ() / kMicroToSmallZStep + kCornerHeights[rotatedSlope].bottom),
                static_cast<uint8_t>(elSurface.baseZ() / kMicroToSmallZStep + kCornerHeights[rotatedSlope].left),
            },
            elSurface.snowCoverage(),
            elSurface.var_6_SLR5(),
        };
        std::array<TileDescriptor, 4> tileDescriptors;

        for (std::size_t i = 0; i < std::size(tileDescriptors); i++)
        {
            const auto& offset = _unk4FD99E[i][rotation];
            const auto position = session.getSpritePosition() + offset;

            TileDescriptor& descriptor = tileDescriptors[i];

            descriptor.elSurface = nullptr;
            if (!World::validCoords(position))
            {
                continue;
            }

            descriptor.elSurface = World::TileManager::get(position).surface();
            if (descriptor.elSurface == nullptr)
            {
                continue;
            }

            const uint32_t surfaceSlope = getRotatedSlope(descriptor.elSurface->slope(), rotation);

            const uint8_t microZ = descriptor.elSurface->baseZ() / kMicroToSmallZStep;
            const uint8_t selfMicroZ = selfDescriptor.elSurface->baseZ() / kMicroToSmallZStep;
            const CornerHeight& ch = kCornerHeights[surfaceSlope];

            descriptor.pos = position;
            descriptor.landObjectId = descriptor.elSurface->isIndustrial() ? static_cast<uint8_t>(0xFFU) : descriptor.elSurface->terrain();
            descriptor.slope = surfaceSlope;
            if (i == 0)
            {
                descriptor.cornerHeights.top = selfMicroZ + kCornerHeights[selfDescriptor.slope].left;
                descriptor.cornerHeights.right = microZ + ch.top;
                descriptor.cornerHeights.bottom = selfMicroZ + kCornerHeights[selfDescriptor.slope].bottom;
                descriptor.cornerHeights.left = microZ + ch.right;
            }
            else if (i == 1)
            {
                descriptor.cornerHeights.top = selfMicroZ + kCornerHeights[selfDescriptor.slope].right;
                descriptor.cornerHeights.right = microZ + ch.top;
                descriptor.cornerHeights.bottom = selfMicroZ + kCornerHeights[selfDescriptor.slope].bottom;
                descriptor.cornerHeights.left = microZ + ch.left;
            }
            else if (i == 2)
            {
                descriptor.cornerHeights.top = selfMicroZ + kCornerHeights[selfDescriptor.slope].top;
                descriptor.cornerHeights.right = microZ + ch.right;
                descriptor.cornerHeights.bottom = selfMicroZ + kCornerHeights[selfDescriptor.slope].left;
                descriptor.cornerHeights.left = microZ + ch.bottom;
            }
            else if (i == 3)
            {
                descriptor.cornerHeights.top = selfMicroZ + kCornerHeights[selfDescriptor.slope].top;
                descriptor.cornerHeights.right = microZ + ch.left;
                descriptor.cornerHeights.bottom = selfMicroZ + kCornerHeights[selfDescriptor.slope].right;
                descriptor.cornerHeights.left = microZ + ch.bottom;
            }
            descriptor.snowCoverage = descriptor.elSurface->snowCoverage();
            descriptor.var6SLR5 = descriptor.elSurface->var_6_SLR5();
        }

        if (((session.getViewFlags() & Ui::ViewportFlags::height_marks_on_land) != Ui::ViewportFlags::none)
            && zoomLevel == 0)
        {
            const auto markerPos = session.getUnkPosition() + World::Pos2(16, 16);
            const auto markerHeight = World::TileManager::getHeight(markerPos).landHeight + 3;
            const auto markerImageIndex = getHeightMarkerImage(markerHeight);

            const auto imageId = ImageId{ getHeightMarkerImage(markerImageIndex), Colour::mutedAvocadoGreen };
            const World::Pos3 offset(16, 16, markerHeight);
            const World::Pos3 bbSize(1, 1, 0);
            session.addToPlotListAsParent(imageId, offset, bbSize);
        }

        // 0x00F25314
        const auto cliffEdgeImageBase = landObj->cliffEdgeImage;
        // 0x00F252AC
        const auto unkF252AC = k4FD97E[selfDescriptor.slope];
        // 0x00F25344
        std::optional<SnowImage> snowImage = std::nullopt;

        if (elSurface.isIndustrial())
        {
            // 0x00465C96
            auto* industry = IndustryManager::get(elSurface.industryId());
            auto* industryObj = ObjectManager::get<IndustryObject>(industry->objectId);

            session.setItemType(Ui::ViewportInteraction::InteractionItem::industryTree);

            const auto variation = industryObj->var_1A * elSurface.var_6_SLR5() + ((industryObj->var_E9 - 1) & rotation) * 21;

            // Draw trees if they exist
            {
                const auto height = baseHeight + k4FD30C[unkF252AC];
                const auto imageIndex = industryObj->var_16 + variation + (elSurface.snowCoverage() ? 20 : 19);

                const World::Pos3 offset(0, 0, height);
                const World::Pos3 bbOffset(14, 14, height + 4);
                const World::Pos3 bbSize(4, 4, 14);
                session.addToPlotList4FD150(ImageId(imageIndex), offset, bbOffset, bbSize);
            }

            session.setItemType(Ui::ViewportInteraction::InteractionItem::surface);

            if ((zoomLevel == 0 && industryObj->hasFlags(IndustryObjectFlags::unk26))
                || elSurface.snowCoverage() == 0)
            {
                // Draw main surface image
                const auto imageIndex = industryObj->var_16 + variation + unkF252AC;
                paintMainSurface(session, imageIndex, baseHeight);
            }
            else
            {
                // Draw snow surface image
                auto* snowObj = ObjectManager::get<SnowObject>();
                if (elSurface.snowCoverage() == 5)
                {
                    paintMainSurface(session, unkF252AC + snowObj->image, baseHeight);
                }
                else
                {
                    const auto imageIndex = industryObj->var_16 + variation + unkF252AC;
                    paintMainSurface(session, imageIndex, baseHeight);

                    if ((session.getViewFlags() & (Ui::ViewportFlags::underground_view | Ui::ViewportFlags::flag_7)) == Ui::ViewportFlags::none)
                    {
                        snowImage = SnowImage{ .baseImage = unkF252AC + snowObj->image, .imageMask = kSnowCoverageSlopeToMask[elSurface.snowCoverage() - 1][unkF252AC] };
                    }
                }
            }
        }
        else
        {
            const auto variation = landObj->var_0E * elSurface.var_6_SLR5() + ((landObj->var_04 - 1) & rotation) * 25;
            if (elSurface.snowCoverage())
            {
                // Draw snow surface image
                auto* snowObj = ObjectManager::get<SnowObject>();
                if (elSurface.snowCoverage() == 5)
                {
                    paintMainSurface(session, unkF252AC + snowObj->image, baseHeight);
                }
                else
                {
                    const auto imageIndex = landObj->image + variation + unkF252AC;
                    paintMainSurface(session, imageIndex, baseHeight);

                    if ((session.getViewFlags() & (Ui::ViewportFlags::underground_view | Ui::ViewportFlags::flag_7)) == Ui::ViewportFlags::none)
                    {
                        snowImage = SnowImage{ .baseImage = unkF252AC + snowObj->image, .imageMask = kSnowCoverageSlopeToMask[elSurface.snowCoverage() - 1][unkF252AC] };
                    }
                }
            }
            else
            {
                const auto imageIndex = [&elSurface, zoomLevel, unkF252AC, &landObj, variation]() {
                    if (!elSurface.water()
                        && elSurface.variation() != 0
                        && zoomLevel == 0
                        && unkF252AC == 0
                        && (landObj->var_03 - 1) == elSurface.var_6_SLR5())
                    {
                        return landObj->mapPixelImage + 3 + elSurface.variation();
                    }

                    return landObj->image + variation + unkF252AC;
                }();

                paintMainSurface(session, imageIndex, baseHeight);
            }
        }
        // 0x00465E92

        if (World::hasMapSelectionFlag(World::MapSelectionFlags::enable))
        {
            const auto pos = session.getUnkPosition();
            auto [pointA, pointB] = getMapSelectionArea();
            if (pos.x >= pointA.x && pos.x <= pointB.x && pos.y >= pointA.y
                && pos.y <= pointB.y)
            {
                paintSurfaceSelection(session, elSurface, selfDescriptor.slope);
            }
        }

        if (World::hasMapSelectionFlag(World::MapSelectionFlags::enableConstruct))
        {
            if (isWithinMapSelectionTiles(session.getUnkPosition()))
            {
                const auto colour = World::hasMapSelectionFlag(World::MapSelectionFlags::unk_03) ? ExtColour::unk2B : ExtColour::unk25;

                if (elSurface.water() && elSurface.waterHeight() > elSurface.baseHeight())
                {
                    const auto waterSurface = getWaterSurface(elSurface, selfDescriptor.slope);
                    const auto waterDisplaySlope = k4FD97E[waterSurface.slope];
                    const auto imageId = ImageId(kCornerSelectionBoxFromSlope[waterDisplaySlope], colour);

                    // TODO: Push/pop last ps?
                    auto* lastPs = session.getLastPS();

                    session.addToPlotListAsParent(imageId, { 0, 0, waterSurface.height }, { 32, 32, 1 });

                    session.setLastPS(lastPs);
                }
                else
                {
                    const auto imageId = ImageId(kCornerSelectionBoxFromSlope[unkF252AC], colour);
                    session.attachToPrevious(imageId, { 0, 0 });
                }
            }
        }

        if (World::hasMapSelectionFlag(World::MapSelectionFlags::catchmentArea))
        {
            if (isWithinCatchmentDisplay(session.getUnkPosition()))
            {
                const auto waterSurface = getWaterSurface(elSurface, selfDescriptor.slope);
                const auto waterDisplaySlope = k4FD97E[waterSurface.slope];
                const auto imageId = ImageId(kCatchmentFromSlope[waterDisplaySlope], Colour::darkBlue);

                // TODO: Push/pop last ps?
                auto* lastPs = session.getLastPS();

                session.addToPlotListAsParent(imageId, { 0, 0, waterSurface.height }, { 32, 32, 1 });

                session.setLastPS(lastPs);
            }
        }

        if ((session.getViewFlags() & Ui::ViewportFlags::gridlines_on_landscape) != Ui::ViewportFlags::none
            && zoomLevel <= 2
            && (session.getViewFlags() & Ui::ViewportFlags::underground_view) == Ui::ViewportFlags::none)
        {
            const auto imageId = ImageId(kGridlinesBoxFromSlope[unkF252AC], ExtColour::unk30);
            session.attachToPrevious(imageId, { 0, 0 });
        }

        if (snowImage.has_value() && zoomLevel <= 2)
        {
            const auto imageId = ImageId(snowImage->baseImage);
            auto* attachedPs = session.attachToPrevious(imageId, { 0, 0 });
            if (attachedPs != nullptr)
            {
                attachedPs->flags |= PaintStructFlags::hasMaskedImage;
                attachedPs->maskedImageId = ImageId(snowImage->imageMask);
            }
        }

        if (zoomLevel == 0
            && (session.getViewFlags() & (Ui::ViewportFlags::underground_view | Ui::ViewportFlags::flag_7)) == Ui::ViewportFlags::none
            && ((Config::get().old.flags & Config::Flags::landscapeSmoothing) != Config::Flags::none))
        {
            paintSurfaceSmoothenEdge(session, 2, selfDescriptor, tileDescriptors[2]);
            paintSurfaceSmoothenEdge(session, 3, selfDescriptor, tileDescriptors[3]);
            paintSurfaceSmoothenEdge(session, 0, selfDescriptor, tileDescriptors[0]);
            paintSurfaceSmoothenEdge(session, 1, selfDescriptor, tileDescriptors[1]);
        }

        if (((session.getViewFlags() & Ui::ViewportFlags::underground_view) != Ui::ViewportFlags::none)
            && ((session.getViewFlags() & Ui::ViewportFlags::flag_7) == Ui::ViewportFlags::none))
        {
            if (elSurface.isIndustrial())
            {
                auto* industry = IndustryManager::get(elSurface.industryId());
                auto* industryObj = ObjectManager::get<IndustryObject>(industry->objectId);

                const auto variation = industryObj->var_1A * elSurface.var_6_SLR5() + ((industryObj->var_E9 - 1) & rotation) * 21;
                const auto imageIndex = industryObj->var_16 + variation + unkF252AC;

                if ((zoomLevel == 0 && industryObj->hasFlags(IndustryObjectFlags::unk26))
                    || selfDescriptor.snowCoverage == 0)
                {
                    paintMainUndergroundSurface(session, imageIndex, unkF252AC);
                }
                else if (selfDescriptor.snowCoverage == 5)
                {
                    auto* snowObj = ObjectManager::get<SnowObject>();

                    const auto imageId = ImageId(snowObj->image).withIndexOffset(19 + unkF252AC);
                    session.attachToPrevious(imageId, { 0, 0 });
                }
                else
                {
                    auto* snowObj = ObjectManager::get<SnowObject>();
                    const auto snowNoise = kUndergroundViewSnowNoise[selfDescriptor.snowCoverage];

                    if (snowNoise != 0)
                    {
                        const auto imageId = ImageId(snowObj->image).withIndexOffset(19 + unkF252AC).withNoiseMask(snowNoise);
                        session.attachToPrevious(imageId, { 0, 0 });
                    }

                    paintMainUndergroundSurface(session, imageIndex, unkF252AC);
                }
            }
            else
            {
                const auto variation = landObj->var_0E * elSurface.var_6_SLR5() + ((landObj->var_04 - 1) & rotation) * 25;
                const auto imageIndex = landObj->image + variation + unkF252AC;

                if (selfDescriptor.snowCoverage == 0)
                {
                    paintMainUndergroundSurface(session, imageIndex, unkF252AC);
                }
                else if (selfDescriptor.snowCoverage == 5)
                {
                    // 0x00466389
                    auto* snowObj = ObjectManager::get<SnowObject>();

                    const auto imageId = ImageId(snowObj->image).withIndexOffset(19 + unkF252AC);
                    session.attachToPrevious(imageId, { 0, 0 });
                }
                else
                {
                    auto* snowObj = ObjectManager::get<SnowObject>();
                    const auto snowNoise = kUndergroundViewSnowNoise[selfDescriptor.snowCoverage];

                    if (snowNoise != 0)
                    {
                        const auto imageId = ImageId(snowObj->image).withIndexOffset(19 + unkF252AC).withNoiseMask(snowNoise);
                        session.attachToPrevious(imageId, { 0, 0 });
                    }

                    paintMainUndergroundSurface(session, imageIndex, unkF252AC);
                }
            }
        }

        if (((session.getViewFlags() & Ui::ViewportFlags::flag_8) == Ui::ViewportFlags::none))
        {
            paintSurfaceCliffEdge(session, 2, selfDescriptor, tileDescriptors[2], cliffEdgeImageBase);
            paintSurfaceCliffEdge(session, 3, selfDescriptor, tileDescriptors[3], cliffEdgeImageBase);
            paintSurfaceCliffEdge(session, 0, selfDescriptor, tileDescriptors[0], cliffEdgeImageBase);
            paintSurfaceCliffEdge(session, 1, selfDescriptor, tileDescriptors[1], cliffEdgeImageBase);
        }

        session.setWaterHeight2(0);
        if (elSurface.water() != 0)
        {
            session.setItemType(Ui::ViewportInteraction::InteractionItem::water);

            const auto waterHeight = elSurface.waterHeight();
            // Why are there 2 ???
            session.setWaterHeight2(waterHeight);
            session.setWaterHeight(waterHeight);

            uint8_t shape = 0;
            if (waterHeight <= baseHeight + World::kMicroZStep)
            {
                shape = kSlopeToWaterShape[rotatedSlope & 0xF];
            }

            auto* waterObj = ObjectManager::get<WaterObject>();
            const auto imageId = ImageId(waterObj->image).withIndexOffset(shape + 35).withTranslucency(ExtColour::null);

            session.addToPlotListAsParent(imageId, { 0, 0, waterHeight }, { 32, 32, -1 });

            const auto attachedImage = ImageId(waterObj->image).withIndexOffset(shape + 30);
            session.attachToPrevious(attachedImage, { 0, 0 });

            // Draw waves
            if (elSurface.isFlag6() && zoomLevel == 0)
            {
                const auto waveIndex = WaveManager::getWaveIndex(toTileSpace(session.getUnkPosition()));
                const auto& wave = WaveManager::getWave(waveIndex);

                const auto waveImage = ImageId(waterObj->image).withIndexOffset(wave.frame + 60);
                session.attachToPrevious(waveImage, { 0, 0 });
            }

            // Do water edges
            paintSurfaceWaterCliffEdge(session, 2, waterHeight, tileDescriptors[2], cliffEdgeImageBase);
            paintSurfaceWaterCliffEdge(session, 3, waterHeight, tileDescriptors[3], cliffEdgeImageBase);
            paintSurfaceWaterCliffEdge(session, 0, waterHeight, tileDescriptors[0], cliffEdgeImageBase);
            paintSurfaceWaterCliffEdge(session, 1, waterHeight, tileDescriptors[1], cliffEdgeImageBase);
        }
    }
}
