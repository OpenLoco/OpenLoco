#include "PaintTile.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Map/BuildingElement.h"
#include "Map/IndustryElement.h"
#include "Map/MapSelection.h"
#include "Map/RoadElement.h"
#include "Map/SignalElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Map/TreeElement.h"
#include "Map/WallElement.h"
#include "Objects/BridgeObject.h"
#include "Objects/ObjectManager.h"
#include "Paint.h"
#include "PaintBuilding.h"
#include "PaintIndustry.h"
#include "PaintRoad.h"
#include "PaintSignal.h"
#include "PaintStation.h"
#include "PaintSurface.h"
#include "PaintTrack.h"
#include "PaintTree.h"
#include "PaintWall.h"
#include "Ui.h"
#include "Ui/ViewportInteraction.h"
#include "World/Station.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;
using namespace OpenLoco::World;

namespace OpenLoco::Paint
{
    // 0x004621FF
    static void paintVoid(PaintSession& session, const World::Pos2& loc)
    {
        constexpr World::Pos2 kUnkOffsets[4] = {
            { 0, 0 },
            { 32, 0 },
            { 32, 32 },
            { 0, 32 },
        };

        const auto loc2 = loc + kUnkOffsets[session.getRotation()];
        const auto vpPos = World::gameToScreen(World::Pos3(loc2.x, loc2.y, 16), session.getRotation());
        if (vpPos.y + 32 <= session.getRenderTarget()->y)
        {
            return;
        }
        if (vpPos.y - 20 >= session.getRenderTarget()->height + session.getRenderTarget()->y)
        {
            return;
        }

        session.setEntityPosition(loc2);
        session.setItemType(InteractionItem::noInteraction);
        session.addToPlotListAsParent(ImageId{ ImageIds::blank_tile }, { 0, 0, 16 }, { 32, 32, -1 });
    }

    // 0x00461EA7
    static void paintConstructionArrow(PaintSession& session, const World::Pos2& loc)
    {
        static loco_global<World::Pos3, 0x00F24942> _constructionArrowLocation;
        static loco_global<uint8_t, 0x00F24948> _constructionArrowDirection;
        if (!World::hasMapSelectionFlag(World::MapSelectionFlags::enableConstructionArrow))
        {
            return;
        }
        if (session.getUnkPosition() != _constructionArrowLocation)
        {
            return;
        }
        session.setEntityPosition(loc);
        const auto dirIndex = (_constructionArrowDirection & 0xFC) | (((_constructionArrowDirection & 0x3) + session.getRotation()) & 0x3);
        constexpr std::array<uint32_t, 16> kConstructionArrowImages = {
            ImageIds::construction_arrow_north,
            ImageIds::construction_arrow_east,
            ImageIds::construction_arrow_south,
            ImageIds::construction_arrow_west,
            ImageIds::construction_arrow_north2,
            ImageIds::construction_arrow_east2,
            ImageIds::construction_arrow_south2,
            ImageIds::construction_arrow_west2,
            ImageIds::construction_arrow_north3,
            ImageIds::construction_arrow_east3,
            ImageIds::construction_arrow_south3,
            ImageIds::construction_arrow_west3,
            ImageIds::construction_arrow_north_east,
            ImageIds::construction_arrow_south_east,
            ImageIds::construction_arrow_south_west,
            ImageIds::construction_arrow_north_west,
        };
        const auto imageId = ImageId{ kConstructionArrowImages[dirIndex], Colour::yellow };
        session.setItemType(InteractionItem::noInteraction);
        session.addToPlotListAsParent(imageId, { 0, 0, _constructionArrowLocation->z }, World::Pos3(0, 0, _constructionArrowLocation->z + 10), { 32, 32, -1 });
    }

    constexpr std::array<std::array<World::Pos3, 9>, 2> kSupportBoundingBoxOffsets = {
        std::array<World::Pos3, 9>{
            World::Pos3{ 2, 2, 6 },
            World::Pos3{ 28, 2, 6 },
            World::Pos3{ 2, 28, 6 },
            World::Pos3{ 28, 28, 6 },
            World::Pos3{ 15, 15, 6 },
            World::Pos3{ 15, 2, 6 },
            World::Pos3{ 2, 15, 6 },
            World::Pos3{ 28, 15, 6 },
            World::Pos3{ 15, 28, 6 },
        },
        std::array<World::Pos3, 9>{
            World::Pos3{ 2, 2, 28 },
            World::Pos3{ 28, 2, 28 },
            World::Pos3{ 2, 28, 28 },
            World::Pos3{ 28, 28, 28 },
            World::Pos3{ 15, 15, 28 },
            World::Pos3{ 15, 2, 28 },
            World::Pos3{ 2, 15, 28 },
            World::Pos3{ 28, 15, 28 },
            World::Pos3{ 15, 28, 28 },
        },
    };

    constexpr std::array<World::Pos3, 2> kSupportBoundingBoxLengths = {
        World::Pos3{ 1, 1, 17 },
        World::Pos3{ 1, 1, 1 },
    };

    constexpr std::array<std::array<uint8_t, 4>, 4> kFrequencyRotationMap = {
        std::array<uint8_t, 4>{ 1U << 0, 1U << 2, 1U << 1, 1U << 3 },
        std::array<uint8_t, 4>{ 1U << 3, 1U << 0, 1U << 2, 1U << 1 },
        std::array<uint8_t, 4>{ 1U << 1, 1U << 3, 1U << 0, 1U << 2 },
        std::array<uint8_t, 4>{ 1U << 2, 1U << 1, 1U << 3, 1U << 0 },
    };

    // 0x0046748F
    static void paintSupports(PaintSession& session)
    {
        // Copy the supports
        const TrackRoadAdditionSupports supports = session.getAdditionSupport();
        // Clear the supports as this function will have taken care of their render
        session.setAdditionSupport(TrackRoadAdditionSupports{});

        auto& bridge = session.getBridgeEntry();
        if (!bridge.isEmpty())
        {
            auto* bridgeObj = ObjectManager::get<BridgeObject>(bridge.objectId);
            // Bridge blocks the supports due to the roof
            if (bridgeObj->noRoof & (1U << 0))
            {
                return;
            }
        }

        const auto pos = session.getSpritePosition();
        for (auto i = 0U; i < std::size(kSegmentOffsets); ++i)
        {
            const auto seg = kSegmentOffsets[i];

            // No support at this location
            if (supports.segmentImages[i] == 0)
            {
                continue;
            }
            // Support blocked by something at this location
            if ((supports.occupiedSegments & seg) != SegmentFlags::none)
            {
                continue;
            }

            const auto frequency = supports.segmentFrequency[i];

            bool frequenceSkip = [&]() {
                auto& line = kFrequencyRotationMap[session.getRotation()];

                if ((frequency & line[0]) && !(pos.x & 0b0010'0000))
                {
                    return true;
                }
                if ((frequency & line[1]) && !(pos.y & 0b0010'0000))
                {
                    return true;
                }
                if ((frequency & line[2]) && (pos.x & 0b0010'0000))
                {
                    return true;
                }
                if ((frequency & line[3]) && (pos.y & 0b0010'0000))
                {
                    return true;
                }
                return false;
            }();

            if (frequenceSkip)
            {
                continue;
            }

            session.setCurrentItem(supports.segmentInteractionItem[i]);
            session.setItemType(supports.segmentInteractionType[i]);

            const auto heightOffset = World::Pos3{ 0,
                                                   0,
                                                   supports.height };

            for (auto j = 0; j < 2; ++j)
            {
                const auto bbOffset = kSupportBoundingBoxOffsets[j][i] + heightOffset;
                const auto& bbLength = kSupportBoundingBoxLengths[j];
                const auto imageId = ImageId::fromUInt32(supports.segmentImages[i]).withIndexOffset(j);
                session.addToPlotList4FD150(imageId, heightOffset, bbOffset, bbLength);
            }
        }
    }

    constexpr std::array<uint8_t, 18> k4F9242 = {
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        5,
        3,
        4,
        2,
        0,
    };
    constexpr std::array<uint32_t, 17> k4F91FE = {
        0,
        84,
        87,
        90,
        93,
        96,
        99,
        102,
        105,
        108,
        111,
        114,
        117,
        120,
        121,
        122,
        123,
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

    struct UnkHeights
    {
        int16_t height1; // dx
        int16_t height2; // 0x00525CFC
    };

    static void paintFlatSingleQuarterNoSupport(PaintSession& session, const BridgeObject& bridgeObj, const BridgeEntry& bridgeEntry, const std::array<uint32_t, 3>& imageIndexs, const World::Pos3& wallBoundingBoxOffset)
    {
        const auto baseHeightOffset = World::Pos3{ 0, 0, bridgeEntry.height };

        if (bridgeObj.noRoof & (1U << 0))
        {
            auto roofImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(imageIndexs[0]);
            World::Pos3 bbOffset = { 0, 0, 30 };
            World::Pos3 bbLength = { 32, 32, 0 };
            session.addToPlotList4FD150(roofImage, baseHeightOffset, bbOffset + baseHeightOffset, bbLength);
        }
        const auto offset = baseHeightOffset - World::Pos3{ 0, 0, 16 };
        auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(imageIndexs[1]);
        World::Pos3 bbOffset2 = { 0, 0, 14 };
        World::Pos3 bbLength2 = { 32, 32, 1 };
        session.addToPlotList4FD150(image, offset, bbOffset2 + offset, bbLength2);

        if (bridgeEntry.subType == 0)
        {
            auto wallImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(imageIndexs[2]);
            World::Pos3 bbLength3 = { 2, 2, 26 };
            session.addToPlotList4FD150(wallImage, baseHeightOffset, wallBoundingBoxOffset + baseHeightOffset, bbLength3);
        }
    }

    // 0x0042C36D
    static void paintSupport1(PaintSession& session, const BridgeObject& bridgeObj, const BridgeEntry& bridgeEntry, int16_t height525CFE, int16_t height525CFC, int16_t height525D04)
    {
        auto height525D06 = height525D04;
        while (height525CFC >= 16)
        {
            bool is16section = height525CFC == 16 || ((height525D04 - 16) == session.getWaterHeight());
            int16_t sectionHeight = is16section ? 16 : 32;
            const auto bbLength = is16section ? World::Pos3{ 2, 2, 15 } : World::Pos3{ 2, 2, 31 };
            const auto imageIndex = is16section ? 32 : 34;

            height525D04 -= sectionHeight;
            height525CFC -= sectionHeight;
            auto supportSectionImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(imageIndex);
            const auto heightOffset = World::Pos3{ 0, 0, height525D04 };
            World::Pos3 bbOffset = { 30, 0, 0 };
            session.addToPlotList4FD150(supportSectionImage, heightOffset, bbOffset + heightOffset, bbLength);
        }

        while (height525CFE >= 16)
        {
            bool is16section = height525CFE == 16 || ((height525D06 - 16) == session.getWaterHeight());
            int16_t sectionHeight = is16section ? 16 : 32;
            const auto bbLength = is16section ? World::Pos3{ 2, 2, 15 } : World::Pos3{ 2, 2, 31 };
            const auto imageIndex = is16section ? 33 : 35;

            height525CFE -= sectionHeight;
            height525D06 -= sectionHeight;
            auto supportSectionImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(imageIndex);
            const auto heightOffset = World::Pos3{ 0, 0, height525D06 };
            World::Pos3 bbOffset = { 0, 30, 0 };
            session.addToPlotList4FD150(supportSectionImage, heightOffset, bbOffset + heightOffset, bbLength);
        }
    }

    // SPECIAL needs to do the front supports to ground as well
    // 0x0042BCD5
    static void paintFlatSingleQuarterSupportFront(PaintSession& session, const BridgeObject& bridgeObj, const BridgeEntry& bridgeEntry, const int16_t supportLength, const uint8_t slope)
    {
        const auto baseHeightOffset = World::Pos3{ 0, 0, bridgeEntry.height };

        if (bridgeObj.noRoof & (1U << 0))
        {
            auto roofImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(10);
            World::Pos3 bbOffset = { 0, 0, 30 };
            World::Pos3 bbLength = { 32, 32, 0 };
            session.addToPlotList4FD150(roofImage, baseHeightOffset, bbOffset + baseHeightOffset, bbLength);
        }

        auto unks = [&session, &bridgeObj, &bridgeEntry, supportLength, slope]() -> std::optional<UnkHeights> {
            if (session.getSupportHeight(1).height == 0xFFFFU)
            {
                return std::nullopt;
            }
            if (session.getSupportHeight(2).height == 0xFFFFU)
            {
                return std::nullopt;
            }
            int16_t unkHeight = supportLength - bridgeObj.var_06;
            if (unkHeight < 0)
            {
                return std::nullopt;
            }
            if (bridgeObj.var_06 == 32)
            {
                unkHeight = bridgeEntry.height - 16;
                if (unkHeight == session.getWaterHeight())
                {
                    return std::nullopt;
                }
            }

            const int16_t unkHeight2 = unkHeight - k4F915C[slope];
            if (unkHeight2 < 0)
            {
                return std::nullopt;
            }

            unkHeight -= k4F919C[slope];
            if (unkHeight < 0)
            {
                return std::nullopt;
            }
            return UnkHeights{ unkHeight, unkHeight2 };
        }();
        if (unks.has_value())
        {
            // 0x0042BD9C

            const auto& [unkHeight, unkHeight2] = unks.value();
            const auto unk525D04 = bridgeEntry.height - bridgeObj.var_06;

            auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(11);
            auto offset = baseHeightOffset - World::Pos3{ 0, 0, bridgeObj.var_06 };
            World::Pos3 bbOffset = { 0, 0, 14 };
            World::Pos3 bbLength = { 32, 32, 1 };
            session.addToPlotList4FD150(image, offset, bbOffset + offset, bbLength);

            paintSupport1(session, bridgeObj, bridgeEntry, unkHeight, unkHeight2, unk525D04);
        }
        else
        {
            // 0x0042BE22
            auto image = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(5);
            auto offset = baseHeightOffset - World::Pos3{ 0, 0, 16 };
            World::Pos3 bbOffset = { 0, 0, 14 };
            World::Pos3 bbLength = { 32, 32, 1 };
            session.addToPlotList4FD150(image, offset, bbOffset + offset, bbLength);
        }
        // 0x0042BE77

        if (bridgeEntry.subType == 0)
        {
            auto wallImage = bridgeEntry.imageBase.withIndex(bridgeObj.image).withIndexOffset(12);
            World::Pos3 bbOffset = { 17, 17, 2 };
            World::Pos3 bbLength3 = { 2, 2, 24 };
            session.addToPlotList4FD150(wallImage, baseHeightOffset, bbOffset + baseHeightOffset, bbLength3);
        }
    }

    // 0x0042AC9C
    static bool sub_42AC9C(PaintSession& session)
    {
        uint8_t unkF25340 = 0;
        bool unk525D0C = false;

        session.setItemType(Ui::ViewportInteraction::InteractionItem::bridge);

        auto& bridgeEntry = session.getBridgeEntry();
        if (bridgeEntry.isEmpty())
        {
            return false;
        }
        // ceil to 16
        auto genHeight = session.getGeneralSupportHeight().height + 15;
        genHeight &= ~(0xF);
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
            auto image = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(k4F91FE[bridgeEntry.subType]);
            unkF25340 = k4F9242[bridgeEntry.subType];
            World::Pos3 bbOffset = { 2, 2, 0 };
            World::Pos3 bbLength = { 28, 28, 1 };
            session.addToPlotList4FD150(image, baseHeightOffset, bbOffset + baseHeightOffset, bbLength);

            if (k4F91DC[bridgeEntry.subType] & (1U << 2))
            {
                if (!(bridgeEntry.edgesQuarters & (1U << 7)))
                {
                    auto wallImage = image.withIndexOffset(1);
                    World::Pos3 bbOffset2 = { 2, 0, 8 };
                    World::Pos3 bbLength2 = { 28, 1, 30 };
                    session.addToPlotList4FD150(wallImage, baseHeightOffset, bbOffset2 + baseHeightOffset, bbLength2);
                }
                if (!(bridgeEntry.edgesQuarters & (1U << 5)))
                {
                    auto wallImage = image.withIndexOffset(2);
                    World::Pos3 bbOffset2 = { 1, 30, 8 };
                    World::Pos3 bbLength2 = { 29, 1, 30 };
                    session.addToPlotList4FD150(wallImage, baseHeightOffset, bbOffset2 + baseHeightOffset, bbLength2);
                }
            }
            if (k4F91DC[bridgeEntry.subType] & (1U << 3))
            {
                if (!(bridgeEntry.edgesQuarters & (1U << 4)))
                {
                    auto wallImage = image.withIndexOffset(1);
                    World::Pos3 bbOffset2 = { 0, 2, 8 };
                    World::Pos3 bbLength2 = { 1, 28, 30 };
                    session.addToPlotList4FD150(wallImage, baseHeightOffset, bbOffset2 + baseHeightOffset, bbLength2);
                }
                if (!(bridgeEntry.edgesQuarters & (1U << 6)))
                {
                    auto wallImage = image.withIndexOffset(2);
                    World::Pos3 bbOffset2 = { 30, 1, 8 };
                    World::Pos3 bbLength2 = { 1, 29, 30 };
                    session.addToPlotList4FD150(wallImage, baseHeightOffset, bbOffset2 + baseHeightOffset, bbLength2);
                }
            }

            auto offset2 = baseHeightOffset + World::Pos3{ 0, 0, 8 };
            if (k4F91DC[bridgeEntry.subType] & (1U << 4))
            {
                auto image2 = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(14);
                World::Pos3 bbOffset2 = { 22, 24, 0 };
                World::Pos3 bbLength2 = { 2, 2, 26 };
                session.addToPlotList4FD150(image2, offset2, bbOffset2 + offset2, bbLength2);
            }
            if (k4F91DC[bridgeEntry.subType] & (1U << 5))
            {
                auto image2 = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(13);
                World::Pos3 bbOffset2 = { 7, 7, 0 };
                World::Pos3 bbLength2 = { 2, 2, 26 };
                session.addToPlotList4FD150(image2, offset2, bbOffset2 + offset2, bbLength2);
            }
            if (k4F91DC[bridgeEntry.subType] & (1U << 6))
            {
                auto image2 = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(15);
                World::Pos3 bbOffset2 = { 24, 22, 0 };
                World::Pos3 bbLength2 = { 2, 2, 26 };
                session.addToPlotList4FD150(image2, offset2, bbOffset2 + offset2, bbLength2);
            }
            if (k4F91DC[bridgeEntry.subType] & (1U << 7))
            {
                auto image2 = bridgeEntry.imageBase.withIndex(bridgeObj->image).withIndexOffset(12);
                World::Pos3 bbOffset2 = { 17, 17, 2 };
                World::Pos3 bbLength2 = { 2, 2, 24 };
                session.addToPlotList4FD150(image2, offset2, bbOffset2 + offset2, bbLength2);
            }
        }

        if (!(session.getGeneralSupportHeight().slope & (1U << 5)) && supportLength == 0)
        {
            // No need to do 0x0042BED2 as unk525D0C never true
            return true;
        }

        unkF25340 = 1;
        unk525D0C = true;

        [[maybe_unused]] const uint8_t slope = session.getGeneralSupportHeight().slope & 0x1FU;
        const uint8_t quarters = bridgeEntry.edgesQuarters & 0xFU;

        switch (quarters)
        {
            case 1:
            {
                // 0x0042B9BA
                unkF25340 = 5;
                paintFlatSingleQuarterNoSupport(session, *bridgeObj, bridgeEntry, { 7, 2, 14 }, { 22, 24, 0 });
            }
            break;
            case 2:
            {
                // 0x0042BAC3
                unkF25340 = 3;
                paintFlatSingleQuarterNoSupport(session, *bridgeObj, bridgeEntry, { 8, 3, 13 }, { 7, 7, 0 });
            }
            break;
            case 4:
                // 0x0042BBCC
                unkF25340 = 4;
                paintFlatSingleQuarterNoSupport(session, *bridgeObj, bridgeEntry, { 9, 4, 15 }, { 24, 22, 0 });
                break;
            case 8:
                // 0x0042BCD5
                // SPECIAL needs to do the front supports to ground as well
                unkF25340 = 2;
                paintFlatSingleQuarterSupportFront(session, *bridgeObj, bridgeEntry, supportLength, slope);
                break;
            default:
                // 0x0042B08A
                break;
        }
        // 0x0042B063

        // 0x0042BED2
        // Paint shadow
        if (unkF25340 != 0 && session.getRenderTarget()->zoomLevel <= 1)
        {
            auto displaySlope = 0;
            auto height = session.getWaterHeight2();
            if (height == 0)
            {
                height = session.getSurfaceHeight();
                displaySlope = kSlopeToDisplaySlope[session.getSurfaceSlope()];
            }

            const auto shadowImage = ImageId(3612 + displaySlope + unkF25340 * 19).withTranslucency(ExtColour::unk32);
            if (unk525D0C)
            {
                World::Pos3 heightOffset = { 0, 0, height };
                World::Pos3 bbOffset2 = { 15, 15, 1 };
                World::Pos3 bbLength2 = { 2, 2, 1 };
                session.addToPlotList4FD150(shadowImage, heightOffset, bbOffset2 + heightOffset, bbLength2);
            }
        }
        return true;
        // registers regs;
        // call(0x0042AC9C, regs);
        // return regs.al != 0;
    }

    // Returns std::nullopt on no need to paint
    static std::optional<Ui::viewport_pos> paintTileElementsSetup(PaintSession& session, const World::Pos2& loc)
    {
        session.setSegmentsSupportHeight(SegmentFlags::all, std::numeric_limits<uint16_t>::max(), 0);
        session.setGeneralSupportHeight(std::numeric_limits<uint16_t>::max(), 0);
        session.resetTunnels();
        session.setUnkPosition(loc);
        session.setMapPosition(loc);

        session.setMaxHeight(loc);

        constexpr World::Pos2 kUnkOffsets[4] = {
            { 0, 0 },
            { 32, 0 },
            { 32, 32 },
            { 0, 32 },
        };

        const auto loc2 = loc + kUnkOffsets[session.getRotation()];
        const auto vpPos = World::gameToScreen(World::Pos3(loc2.x, loc2.y, 0), session.getRotation());
        paintConstructionArrow(session, loc2);

        if (vpPos.y + 52 <= session.getRenderTarget()->y)
        {
            return std::nullopt;
        }
        if (vpPos.y - session.getMaxHeight() > session.getRenderTarget()->y + session.getRenderTarget()->height)
        {
            return std::nullopt;
        }

        session.setEntityPosition(loc2);
        session.resetTileColumn({ vpPos.x, vpPos.y });
        return { vpPos };
    }

    static void paintTileElementsEndLoop(PaintSession& session, const World::TileElement& el)
    {
        if (el.isLast() || el.baseZ() != ((&el) + 1)->baseZ())
        {
            if (session.getRoadExits() != 0)
            {
                finalisePaintRoad(session);
            }
            if (session.getAdditionSupportHeight() != 0)
            {
                paintSupports(session);
            }

            session.finaliseTrackRoadOrdering();
            session.finaliseTrackRoadAdditionsOrdering();
            session.setOccupiedAdditionSupportSegments(SegmentFlags::none);
            auto& bridgeEntry = session.getBridgeEntry();
            if (!bridgeEntry.isEmpty())
            {
                if (sub_42AC9C(session))
                {
                    session.setSegmentsSupportHeight(SegmentFlags::all, 0xFFFF, 0);
                }
                if (session.getGeneralSupportHeight().height >= bridgeEntry.height)
                {
                    session.setGeneralSupportHeight(bridgeEntry.height, 0x20);
                }
                session.setBridgeEntry(kNullBridgeEntry);
            }

            if (session.get525CF8() != SegmentFlags::none)
            {
                session.setSegmentsSupportHeight(session.get525CF8(), 0xFFFF, 0);
                session.set525CF8(SegmentFlags::none);
            }
        }
    }

    // 0x00461CF8
    void paintTileElements(PaintSession& session, const World::Pos2& loc)
    {
        if (!World::drawableCoords(loc))
        {
            paintVoid(session, loc);
            return;
        }

        const auto vpPos = paintTileElementsSetup(session, loc);
        if (!vpPos)
        {
            return;
        }

        auto tile = TileManager::get(loc);
        for (auto& el : tile)
        {
            session.setUnkVpY(vpPos->y - el.baseHeight());
            session.setCurrentItem(&el);
            switch (el.type())
            {
                case World::ElementType::surface:
                {
                    auto& elSurface = el.get<World::SurfaceElement>();
                    paintSurface(session, elSurface);
                    break;
                }
                case World::ElementType::track:
                {
                    auto& elTrack = el.get<World::TrackElement>();
                    paintTrack(session, elTrack);
                    break;
                }
                case World::ElementType::station:
                {
                    auto& elStation = el.get<World::StationElement>();
                    paintStation(session, elStation);
                    break;
                }
                case World::ElementType::signal:
                {
                    auto& elSignal = el.get<World::SignalElement>();
                    paintSignal(session, elSignal);
                    break;
                }
                case World::ElementType::building:
                {
                    auto& elBuilding = el.get<World::BuildingElement>();
                    paintBuilding(session, elBuilding);

                    break;
                }
                case World::ElementType::tree:
                {
                    auto& elTree = el.get<World::TreeElement>();
                    paintTree(session, elTree);
                    break;
                }
                case World::ElementType::wall:
                {
                    auto& elWall = el.get<World::WallElement>();
                    paintWall(session, elWall);
                    break;
                }
                case World::ElementType::road:
                {
                    auto& elRoad = el.get<World::RoadElement>();
                    paintRoad(session, elRoad);
                    break;
                }
                case World::ElementType::industry:
                {
                    auto& elIndustry = el.get<World::IndustryElement>();
                    paintIndustry(session, elIndustry);
                    break;
                }
            }
            paintTileElementsEndLoop(session, el);
        }
    }

    // 0x004617C6
    void paintTileElements2(PaintSession& session, const World::Pos2& loc)
    {
        if (!World::drawableCoords(loc))
        {
            return;
        }

        const auto vpPos = paintTileElementsSetup(session, loc);
        if (!vpPos)
        {
            return;
        }

        auto tile = TileManager::get(loc);
        for (auto& el : tile)
        {
            session.setUnkVpY(vpPos->y - el.baseHeight());
            session.setCurrentItem(&el);
            switch (el.type())
            {
                case World::ElementType::surface:
                case World::ElementType::track:
                case World::ElementType::signal:
                case World::ElementType::wall:
                case World::ElementType::road:
                    continue;

                case World::ElementType::station:
                {
                    auto& elStation = el.get<StationElement>();
                    switch (elStation.stationType())
                    {
                        case StationType::airport:
                        case StationType::docks:
                            paintStation(session, elStation);
                            break;
                        default:
                        case StationType::roadStation:
                        case StationType::trainStation:
                            continue;
                    }
                    break;
                }
                case World::ElementType::building:
                {
                    auto& elBuilding = el.get<BuildingElement>();
                    paintBuilding(session, elBuilding);
                    break;
                }
                case World::ElementType::tree:
                {
                    auto& elTree = el.get<TreeElement>();
                    paintTree(session, elTree);
                    break;
                }
                case World::ElementType::industry:
                {
                    auto& elIndustry = el.get<IndustryElement>();
                    paintIndustry(session, elIndustry);
                    break;
                }
            }
            paintTileElementsEndLoop(session, el);
        }
    }
}
