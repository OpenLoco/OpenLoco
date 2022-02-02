#include "PaintTile.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Map/TileManager.h"
#include "../Station.h"
#include "../Ui.h"
#include "Paint.h"
#include "PaintSignal.h"
#include "PaintTree.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::ViewportInteraction;
using namespace OpenLoco::Map;

namespace OpenLoco::Paint
{
    // 0x004621FF
    static void paintVoid(PaintSession& session, const Map::Pos2& loc)
    {
        constexpr Map::Pos2 kUnkOffsets[4] = {
            { 0, 0 },
            { 32, 0 },
            { 32, 32 },
            { 0, 32 },
        };

        const auto loc2 = loc + kUnkOffsets[session.getRotation()];
        const auto vpPos = Map::gameToScreen(Map::Pos3(loc2.x, loc2.y, 16), session.getRotation());
        if (vpPos.y + 32 <= session.getContext()->y)
        {
            return;
        }
        if (vpPos.y - 20 >= session.getContext()->height + session.getContext()->y)
        {
            return;
        }

        session.setEntityPosition(loc2);
        session.setItemType(InteractionItem::noInteraction);
        session.addToPlotListAsParent(ImageIds::blank_tile, { 0, 0, 16 }, { 32, 32, 255 });
    }

    // 0x00461EA7
    static void paintConstructionArrow(PaintSession& session, const Map::Pos2& loc)
    {
        static loco_global<Map::Pos3, 0x00F24942> _constructionArrowLocation;
        static loco_global<uint8_t, 0x00F24948> _constructionArrowDirection;
        if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::unk_02))
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
        const auto imageId = Gfx::recolour(kConstructionArrowImages[dirIndex], Colour::yellow);
        session.setItemType(InteractionItem::noInteraction);
        session.addToPlotListAsParent(imageId, { 0, 0, _constructionArrowLocation->z }, Map::Pos3(0, 0, _constructionArrowLocation->z + 10), { 32, 32, 255 });
    }

    // 0x004792E7 streetlights?
    static void sub_4792E7(PaintSession& session)
    {
        call(0x004792E7);
    }

    // 0x0046748F
    static void sub_46748F(PaintSession& session)
    {
        call(0x0046748F);
    }

    // 0x0045CA67
    static void sub_45CA67(PaintSession& session)
    {
        call(0x0045CA67);
    }

    // 0x0045CC1B
    static void sub_45CC1B(PaintSession& session)
    {
        call(0x0045CC1B);
    }

    // 0x0042AC9C
    static bool sub_42AC9C(PaintSession& session)
    {
        registers regs;
        call(0x0042AC9C, regs);
        return regs.al != 0;
    }

    // 0x004656BF
    static void paintSurface(PaintSession& session, Map::SurfaceElement& elSurface)
    {
        registers regs;
        regs.esi = X86Pointer(&elSurface);
        regs.dx = elSurface.baseZ() * 4;
        call(0x004656BF, regs);
    }

    // 0x0049B6BF
    static void paintTrack(PaintSession& session, Map::TrackElement& elTrack)
    {
        registers regs;
        regs.esi = X86Pointer(&elTrack);
        regs.ecx = (session.getRotation() + elTrack.unkDirection()) & 0x3;
        regs.dx = elTrack.baseZ() * 4;
        call(0x0049B6BF, regs);
    }

    // 0x0048B313
    static void paintStation(PaintSession& session, Map::StationElement& elStation)
    {
        registers regs;
        regs.esi = X86Pointer(&elStation);
        regs.ecx = (session.getRotation() + elStation.rotation()) & 0x3;
        regs.dx = elStation.baseZ() * 4;
        call(0x0048B313, regs);
    }

    // 0x0042C6C4
    static void paintBuilding(PaintSession& session, Map::BuildingElement& elBuilding)
    {
        registers regs;
        regs.esi = X86Pointer(&elBuilding);
        regs.ecx = (session.getRotation() + (elBuilding.data()[0] & 0x3)) & 0x3;
        regs.dx = elBuilding.baseZ() * 4;
        call(0x0042C6C4, regs);
    }

    // 0x004C3D7C
    static void paintWall(PaintSession& session, Map::WallElement& elWall)
    {
        registers regs;
        regs.esi = X86Pointer(&elWall);
        regs.ecx = (session.getRotation() + (elWall.data()[0] & 0x3)) & 0x3;
        regs.dx = elWall.baseZ() * 4;
        call(0x004C3D7C, regs);
    }

    // 0x004759A6
    static void paintRoad(PaintSession& session, Map::RoadElement& elRoad)
    {
        registers regs;
        regs.esi = X86Pointer(&elRoad);
        regs.ecx = (session.getRotation() + elRoad.unkDirection()) & 0x3;
        regs.dx = elRoad.baseZ() * 4;
        call(0x004759A6, regs);
    }

    // 0x00453C52
    static void paintIndustry(PaintSession& session, Map::IndustryElement& elIndustry)
    {
        registers regs;
        regs.esi = X86Pointer(&elIndustry);
        regs.ecx = (session.getRotation() + (elIndustry.data()[0] & 0x3)) & 0x3;
        regs.dx = elIndustry.baseZ() * 4;
        call(0x00453C52, regs);
    }

    // Returns std::nullopt on no need to paint
    static std::optional<Ui::viewport_pos> paintTileElementsSetup(PaintSession& session, const Map::Pos2& loc)
    {
        session.setSegmentSupportHeight(Segment::all, std::numeric_limits<uint16_t>::max(), 0);
        session.setGeneralSupportHeight(std::numeric_limits<uint16_t>::max(), 0);
        session.resetTunnels();
        session.setUnkPosition(loc);
        session.setMapPosition(loc);

        session.setMaxHeight(loc);

        constexpr Map::Pos2 kUnkOffsets[4] = {
            { 0, 0 },
            { 32, 0 },
            { 32, 32 },
            { 0, 32 },
        };

        const auto loc2 = loc + kUnkOffsets[session.getRotation()];
        const auto vpPos = Map::gameToScreen(Map::Pos3(loc2.x, loc2.y, 0), session.getRotation());
        paintConstructionArrow(session, loc2);

        if (vpPos.y + 52 <= session.getContext()->y)
        {
            return std::nullopt;
        }
        if (vpPos.y - session.getMaxHeight() > session.getContext()->y + session.getContext()->height)
        {
            return std::nullopt;
        }

        session.setEntityPosition(loc2);
        session.resetTileColumn({ vpPos.x, vpPos.y });
        return { vpPos };
    }

    static void paintTileElementsEndLoop(PaintSession& session, const Map::TileElement& el)
    {
        if (el.isLast() || el.baseZ() != ((&el) + 1)->baseZ())
        {
            if (session.get112C300() != 0)
            {
                sub_4792E7(session);
            }
            if (session.getF003F4() != 0)
            {
                sub_46748F(session);
            }

            sub_45CA67(session);
            sub_45CC1B(session);
            session.setF003F6(0);
            if (session.get525CE4(0) != 0xFFFF)
            {
                if (sub_42AC9C(session))
                {
                    session.setSegmentSupportHeight(Segment::all, 0xFFFF, 0);
                }
                if (session.getGeneralSupportHeight().height >= session.get525CE4(0))
                {
                    session.setGeneralSupportHeight(session.get525CE4(0), 0x20);
                }
                session.set525CE4(0, 0xFFFF);
                session.set525CF0(0);
            }

            if (session.get525CF8() != 0)
            {
                for (auto bit = Utility::bitScanForward(session.get525CF8()); bit != -1; bit = Utility::bitScanForward(session.get525CF8()))
                {
                    session.set525CF8(session.get525CF8() & ~(1 << bit));
                    session.setSegmentSupportHeight(1 << bit, 0xFFFF, 0);
                }
            }
        }
    }

    // 0x00461CF8
    void paintTileElements(PaintSession& session, const Map::Pos2& loc)
    {
        if (!Map::drawableCoords(loc))
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
            session.setUnkVpY(vpPos->y - el.baseZ() * 4);
            session.setCurrentItem(&el);
            switch (el.type())
            {
                case Map::ElementType::surface:
                {
                    auto& elSurface = el.get<Map::SurfaceElement>();
                    paintSurface(session, elSurface);
                    break;
                }
                case Map::ElementType::track:
                {
                    auto& elTrack = el.get<Map::TrackElement>();
                    paintTrack(session, elTrack);
                    break;
                }
                case Map::ElementType::station:
                {
                    auto& elStation = el.get<Map::StationElement>();
                    paintStation(session, elStation);
                    break;
                }
                case Map::ElementType::signal:
                {
                    auto& elSignal = el.get<Map::SignalElement>();
                    paintSignal(session, elSignal);
                    break;
                }
                case Map::ElementType::building:
                {
                    auto& elBuilding = el.get<Map::BuildingElement>();
                    paintBuilding(session, elBuilding);

                    break;
                }
                case Map::ElementType::tree:
                {
                    auto& elTree = el.get<Map::TreeElement>();
                    paintTree(session, elTree);
                    break;
                }
                case Map::ElementType::wall:
                {
                    auto& elWall = el.get<Map::WallElement>();
                    paintWall(session, elWall);
                    break;
                }
                case Map::ElementType::road:
                {
                    auto& elRoad = el.get<Map::RoadElement>();
                    paintRoad(session, elRoad);
                    break;
                }
                case Map::ElementType::industry:
                {
                    auto& elIndustry = el.get<Map::IndustryElement>();
                    paintIndustry(session, elIndustry);
                    break;
                }
            }
            paintTileElementsEndLoop(session, el);
        }
    }

    // 0x004617C6
    void paintTileElements2(PaintSession& session, const Map::Pos2& loc)
    {
        if (!Map::drawableCoords(loc))
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
            session.setUnkVpY(vpPos->y - el.baseZ() * 4);
            session.setCurrentItem(&el);
            switch (el.type())
            {
                case Map::ElementType::surface:
                case Map::ElementType::track:
                case Map::ElementType::signal:
                case Map::ElementType::wall:
                case Map::ElementType::road:
                    continue;

                case Map::ElementType::station:
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
                case Map::ElementType::building:
                {
                    auto& elBuilding = el.get<BuildingElement>();
                    paintBuilding(session, elBuilding);
                    break;
                }
                case Map::ElementType::tree:
                {
                    auto& elTree = el.get<TreeElement>();
                    paintTree(session, elTree);
                    break;
                }
                case Map::ElementType::industry:
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
