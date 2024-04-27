#include "PaintSurface.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Map/MapSelection.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Objects/IndustryObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/SnowObject.h"
#include "Paint.h"
#include "PaintTileDecorations.h"
#include "Ui/ViewportInteraction.h"
#include "World/IndustryManager.h"
#include <OpenLoco/Core/Numerics.hpp>

namespace OpenLoco::Paint
{
    struct CornerHeight
    {
        uint8_t top;
        uint8_t right;
        uint8_t bottom;
        uint8_t left;
    };

    struct TileDescriptor
    {
        World::Pos2 pos;
        const World::SurfaceElement* elSurface;
        const LandObject* landObject;
        uint8_t slope;
        CornerHeight cornerHeights;
    };

    // TODO: Check
    static constexpr std::array<CornerHeight, 32> cornerHeights = {
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

    std::array<std::array<World::Pos2, 4>, 4> _unk4FD99E = {
        std::array<World::Pos2, 4>{
            World::Pos2{ 32, 0 },
            World::Pos2{ -32, 0 },
            World::Pos2{ -64, -32 },
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

    // 0x00465F99
    static void paintSurfaceFullWaterTileSelection(PaintSession& session, const World::SurfaceElement& elSurface, uint8_t slope)
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
        const auto displaySlope = k4FD97E[slope];
        const auto imageId = ImageId(kCornerSelectionBoxFromSlope[displaySlope], ExtColour::unk26);

        // TODO: Push/pop last ps?
        auto* lastPs = session.getLastPS();

        session.addToPlotListAsParent(imageId, { 0, 0, height }, { 32, 32, 1 });

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

    // 0x004656BF
    void paintSurface(PaintSession& session, World::SurfaceElement& elSurface)
    {
        session.setItemType(Ui::ViewportInteraction::InteractionItem::surface);
        const auto zoomLevel = session.getRenderTarget()->zoomLevel;
        session.setDidPassSurface(true);

        // 0x00F252B0 / 0x00F252B4 but if 0x00F252B0 == -2 that means industrial
        [[maybe_unused]] uint8_t landObjId = elSurface.terrain();

        const auto rotation = session.getRotation();
        const auto baseHeight = elSurface.baseHeight();
        const auto rotatedSlope = getRotatedSlope(elSurface.slope(), rotation);

        const auto selfDescriptor = TileDescriptor{
            session.getSpritePosition(),
            &elSurface,
            ObjectManager::get<LandObject>(elSurface.terrain()),
            rotatedSlope,
            {
                static_cast<uint8_t>(elSurface.baseZ() + cornerHeights[rotatedSlope].top),
                static_cast<uint8_t>(elSurface.baseZ() + cornerHeights[rotatedSlope].right),
                static_cast<uint8_t>(elSurface.baseZ() + cornerHeights[rotatedSlope].bottom),
                static_cast<uint8_t>(elSurface.baseZ() + cornerHeights[rotatedSlope].left),
            },
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

            const uint8_t baseZ = descriptor.elSurface->baseZ();
            const CornerHeight& ch = cornerHeights[surfaceSlope];

            descriptor.pos = position;
            descriptor.landObject = ObjectManager::get<LandObject>(descriptor.elSurface->terrain());
            descriptor.slope = surfaceSlope;
            descriptor.cornerHeights.top = baseZ + ch.top;
            descriptor.cornerHeights.right = baseZ + ch.right;
            descriptor.cornerHeights.bottom = baseZ + ch.bottom;
            descriptor.cornerHeights.left = baseZ + ch.left;
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
        [[maybe_unused]] const auto cliffEdgeImageBase = selfDescriptor.landObject->cliffEdgeImage;
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

            if (zoomLevel == 0 && industryObj->hasFlags(IndustryObjectFlags::unk26)
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
            auto* landObj = ObjectManager::get<LandObject>(elSurface.terrain());
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
    }
}
