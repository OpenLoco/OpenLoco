#include "PaintTile.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
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
#include "PaintBridge.h"
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
        auto& arrow = getConstructionArrow();
        if (!World::hasMapSelectionFlag(World::MapSelectionFlags::enableConstructionArrow))
        {
            return;
        }
        if (session.getUnkPosition() != arrow.pos)
        {
            return;
        }
        session.setEntityPosition(loc);
        const auto dirIndex = (arrow.direction & 0xFC) | (((arrow.direction & 0x3) + session.getRotation()) & 0x3);
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
        session.addToPlotListAsParent(imageId, { 0, 0, arrow.pos.z }, World::Pos3(0, 0, arrow.pos.z + 10), { 32, 32, -1 });
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
            if ((bridgeObj->flags & BridgeObjectFlags::hasRoof) != BridgeObjectFlags::none)
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
                if (paintBridge(session))
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
