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
#include "Paint.h"
#include "PaintIndustry.h"
#include "PaintSignal.h"
#include "PaintStation.h"
#include "PaintTrack.h"
#include "PaintTree.h"
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

    // 0x004792E7 streetlights?
    static void sub_4792E7([[maybe_unused]] PaintSession& session)
    {
        call(0x004792E7);
    }

    // 0x0046748F
    static void sub_46748F([[maybe_unused]] PaintSession& session)
    {
        call(0x0046748F);
    }

    // 0x0042AC9C
    static bool sub_42AC9C([[maybe_unused]] PaintSession& session)
    {
        registers regs;
        call(0x0042AC9C, regs);
        return regs.al != 0;
    }

    // 0x004656BF
    static void paintSurface([[maybe_unused]] PaintSession& session, World::SurfaceElement& elSurface)
    {
        registers regs;
        regs.esi = X86Pointer(&elSurface);
        regs.dx = elSurface.baseHeight();
        call(0x004656BF, regs);
    }

    // 0x0042C6C4
    static void paintBuilding([[maybe_unused]] PaintSession& session, World::BuildingElement& elBuilding)
    {
        registers regs;
        regs.esi = X86Pointer(&elBuilding);
        regs.ecx = (session.getRotation() + (elBuilding.data()[0] & 0x3)) & 0x3;
        regs.dx = elBuilding.baseHeight();
        call(0x0042C6C4, regs);
    }

    // 0x004C3D7C
    static void paintWall([[maybe_unused]] PaintSession& session, World::WallElement& elWall)
    {
        registers regs;
        regs.esi = X86Pointer(&elWall);
        regs.ecx = (session.getRotation() + (elWall.data()[0] & 0x3)) & 0x3;
        regs.dx = elWall.baseHeight();
        call(0x004C3D7C, regs);
    }

    // 0x004759A6
    static void paintRoad([[maybe_unused]] PaintSession& session, World::RoadElement& elRoad)
    {
        registers regs;
        regs.esi = X86Pointer(&elRoad);
        regs.ecx = (session.getRotation() + elRoad.unkDirection()) & 0x3;
        regs.dx = elRoad.baseHeight();
        call(0x004759A6, regs);
    }

    // Returns std::nullopt on no need to paint
    static std::optional<Ui::viewport_pos> paintTileElementsSetup(PaintSession& session, const World::Pos2& loc)
    {
        session.setSegmentSupportHeight(SegmentFlags::all, std::numeric_limits<uint16_t>::max(), 0);
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
            if (session.get112C300() != 0)
            {
                sub_4792E7(session);
            }
            if (session.getAdditionSupportHeight() != 0)
            {
                sub_46748F(session);
            }

            session.finaliseTrackRoadOrdering();
            session.finaliseTrackRoadAdditionsOrdering();
            session.setOccupiedAdditionSupportSegments(SegmentFlags::none);
            auto& bridgeEntry = session.getBridgeEntry();
            if (!bridgeEntry.isEmpty())
            {
                if (sub_42AC9C(session))
                {
                    session.setSegmentSupportHeight(SegmentFlags::all, 0xFFFF, 0);
                }
                if (session.getGeneralSupportHeight().height >= bridgeEntry.height)
                {
                    session.setGeneralSupportHeight(bridgeEntry.height, 0x20);
                }
                session.setBridgeEntry(kNullBridgeEntry);
            }

            if (session.get525CF8() != SegmentFlags::none)
            {
                session.setSegmentSupportHeight(session.get525CF8(), 0xFFFF, 0);
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
